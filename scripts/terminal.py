import subprocess
import sys
import time
import os
import threading
import queue
import re
from datetime import datetime
from enum import Enum


class TestStatus(Enum):
    """测试状态枚举"""
    NOT_FOUND = "未检测到状态"
    PASS = "PASS"
    FAIL = "FAIL"
    NOT_EXIST = "NOT_EXIST"


def remove_c_comments(text):
    """
    移除 C 语言风格的注释：// 和 /* */
    返回移除注释后的文本行列表
    """
    lines = text.split('\n')
    result_lines = []

    in_multiline_comment = False
    comment_buffer = ""  # 存储多行注释内容（不使用，但需要累加以移动指针）

    for line in lines:
        i = 0
        clean_line = ""
        line_length = len(line)

        while i < line_length:
            if not in_multiline_comment:
                if i + 1 < line_length and line[i] == '/' and line[i+1] == '/':
                    break
                elif i + 1 < line_length and line[i] == '/' and line[i+1] == '*':
                    in_multiline_comment = True
                    i += 2
                    comment_buffer = ""  # 重置注释缓冲区
                else:
                    clean_line += line[i]
                    i += 1
            else:
                if i + 1 < line_length and line[i] == '*' and line[i+1] == '/':
                    in_multiline_comment = False
                    i += 2
                    comment_buffer = ""  # 清空注释缓冲区
                else:
                    comment_buffer += line[i]
                    i += 1

        if not in_multiline_comment and clean_line.strip():
            result_lines.append(clean_line.rstrip())

    if in_multiline_comment:
        print("警告：文件包含未结束的多行注释")

    return result_lines


def read_commands_from_file(file_path):
    """从文件读取命令列表，跳过 C 语言风格注释"""
    commands = []
    try:
        # 尝试多种编码读取文件
        content = ""
        for enc in ['utf-8', 'gbk', 'utf-8-sig']:
            try:
                with open(file_path, 'r', encoding=enc) as f:
                    content = f.read()
                print(f"文件编码识别为：{enc}")
                break
            except UnicodeDecodeError:
                continue

        if not content:
            print("错误：无法解码命令文件")
            return []

        commands = remove_c_comments(content)
        commands = [cmd for cmd in commands if cmd.strip()]

        print(f"读取到 {len(commands)} 条有效命令（已跳过注释）")
        return commands
    except FileNotFoundError:
        print(f"错误：文件 '{file_path}' 未找到")
        return []
    except Exception as e:
        print(f"读取文件时出错：{e}")
        return []


def check_test_status(line):
    """
    检查行中的测试状态
    返回 TestStatus 枚举值
    """
    line_upper = line.upper()

    if "TEST PASS" in line_upper:
        return TestStatus.PASS
    elif "TEST FAIL" in line_upper:
        return TestStatus.FAIL
    elif "TEST NOT EXIST" in line_upper or "TEST NOT_EXIST" in line_upper:
        return TestStatus.NOT_EXIST

    return TestStatus.NOT_FOUND


def safe_print(text):
    """
    安全打印函数，防止 UnicodeEncodeError
    这是修复脚本崩溃的关键
    """
    try:
        print(text, end='', flush=True)
    except UnicodeEncodeError:
        # 如果控制台无法显示某些字符，编码后忽略错误再打印
        try:
            safe_text = text.encode(sys.stdout.encoding, errors='replace').decode(
                sys.stdout.encoding, errors='ignore')
            print(safe_text, end='', flush=True)
        except:
            # 最后一道防线：跳过打印
            pass
    except Exception:
        pass


def get_test_prefix(cmd):
    """
    从命令中提取测试前缀
    例如: "crc.test" -> "crc"
    """
    parts = cmd.split()
    if not parts:
        return None

    cmd_name = parts[0]
    match = re.match(r'^([^.]+)', cmd_name)

    if match:
        return match.group(1)

    return None


def analyze_log_file(log_path):
    """
    分析日志文件，检查是否包含错误关键词
    返回分析结果字典
    """
    error_patterns = [
        (r'(?i)fail', 'FAIL'),
        (r'(?i)error', 'ERROR'),
        (r'(?i)crash', 'CRASH'),
        (r'(?i)fatal', 'FATAL'),
        (r'(?i)exception', 'EXCEPTION'),
        (r'(?i)assert', 'ASSERT'),
        (r'(?i)panic', 'PANIC'),
        (r'(?i)abort', 'ABORT'),
    ]

    result = {
        'has_error': False,
        'errors': {},
        'line_count': 0,
        'pass_count': 0
    }

    first_error = None  # 记录第一个错误

    try:
        with open(log_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()

        result['line_count'] = len(lines)

        for i, line in enumerate(lines, 1):
            # 统计 PASS
            if 'TEST PASS' in line.upper():
                result['pass_count'] += 1

            # 如果已经发现错误，停止分析
            if first_error:
                break

            # 检查错误关键词
            for pattern, keyword in error_patterns:
                matches = list(re.finditer(pattern, line))
                if matches:
                    # 记录第一个错误
                    first_error = {
                        'line_num': i,
                        'line_content': line.strip(),
                        'matches': [m.group() for m in matches],
                        'is_first': True
                    }
                    result['errors'][keyword] = [first_error]
                    result['has_error'] = True
                    break  # 找到第一个错误后停止

    except Exception as e:
        result['errors']['FILE_ERROR'] = [{
            'line_num': 0,
            'line_content': f'无法读取日志文件: {e}',
            'matches': []
        }]
        result['has_error'] = True

    return result


def generate_report(output_dir, prefix_log_map, main_log_file):
    """
    生成测试报告
    """
    report_file = os.path.join(output_dir, 'all_test_case_result.log')
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    print(f"\n生成测试报告...")
    total_tests = len(prefix_log_map)
    passed_tests = 0
    failed_tests = 0
    error_details = []

    with open(report_file, 'w', encoding='utf-8') as f:
        f.write("-" * 80 + "\n")
        f.write(f"测试报告\n")
        f.write(f"生成时间: {timestamp}\n")
        f.write("-" * 80 + "\n\n")

        f.write(f"总计测试项: {total_tests}\n\n")

        for prefix, log_filename in sorted(prefix_log_map.items(), key=lambda x: x[1]):
            log_path = os.path.join(output_dir, log_filename)
            analysis = analyze_log_file(log_path)

            f.write("-" * 80 + "\n")
            f.write(f"测试项: {prefix}\n")
            f.write(f"日志文件: {log_filename}\n")
            f.write(f"日志行数: {analysis['line_count']}\n")
            f.write(f"PASS 数量: {analysis['pass_count']}\n")

            if analysis['has_error']:
                failed_tests += 1
                f.write(f"状态: ❌ FAILED\n")

                for keyword, errors in analysis['errors'].items():
                    f.write(f"\n  [{keyword}] 发现 {len(errors)} 处:\n")
                    for err in errors[:5]:  # 最多显示 5 条
                        prefix = "【首次】" if err.get(
                            'is_first', False) else "       "
                        f.write(
                            f"    {prefix}行 {err['line_num']}: {err['line_content'][:100]}\n")
                    if len(errors) > 5:
                        f.write(f"    ... 还有 {len(errors) - 5} 处\n")

                error_details.append({
                    'prefix': prefix,
                    'log_file': log_filename,
                    'errors': analysis['errors']
                })
            else:
                passed_tests += 1
                f.write(f"状态: ✅ PASSED\n")

            f.write("\n")

        f.write("-" * 80 + "\n")
        f.write("测试总结\n")
        f.write("-" * 80 + "\n")
        f.write(f"总计: {total_tests} 项测试\n")
        f.write(f"通过: {passed_tests} 项\n")
        f.write(f"失败: {failed_tests} 项\n")
        f.write(
            f"通过率: {passed_tests/total_tests*100:.1f}%\n" if total_tests > 0 else "通过率: N/A\n")

        if failed_tests > 0:
            f.write("\n失败测试列表:\n")
            for detail in error_details:
                f.write(f"  - {detail['prefix']} ({detail['log_file']})\n")

        f.write(f"\n主日志文件: {main_log_file}\n")
        f.write("-" * 80 + "\n")

    print(f"✅ 测试报告已生成: {report_file}")
    print(
        f"\n测试结果: 通过 {passed_tests}/{total_tests}, 失败 {failed_tests}/{total_tests}")

    return {
        'total': total_tests,
        'passed': passed_tests,
        'failed': failed_tests,
        'report_file': report_file
    }


def run_interactive_mode(commands, cmd_file, exe_path, output_dir='log'):
    """
    交互式模式：启动程序一次，然后连续发送所有命令
    """
    if not commands:
        print("没有命令可执行")
        return False

    exe_path = os.path.abspath(exe_path)

    if not os.path.exists(exe_path):
        print(f"错误：可执行文件 '{exe_path}' 不存在")
        return False

    print(f"从文件 '{cmd_file}' 读取到 {len(commands)} 条有效命令")
    print(f"将使用可执行文件：{exe_path}")
    print("开始执行命令（交互式模式）...")

    exe_dir = os.path.dirname(exe_path)

    # 确保输出目录存在
    if not os.path.isabs(output_dir):
        output_dir = os.path.join(exe_dir, output_dir)
    if not os.path.exists(output_dir):
        os.makedirs(output_dir, exist_ok=True)

    # 使用队列存储输出，线程安全
    output_queue = queue.Queue()
    stop_reading = threading.Event()
    prompt_received = threading.Event()

    # 主日志文件（总日志）
    main_log_file = os.path.join(output_dir, "all_test_case_output.log")
    print(f"主日志文件：{main_log_file}")

    # 打开主日志文件（使用行缓冲）
    try:
        main_log_obj = open(main_log_file, 'w', encoding='utf-8', buffering=1)
    except Exception as e:
        print(f"无法创建主日志文件：{e}")
        return False

    # 当前测试的 log 文件对象
    current_test_log_obj = None
    current_test_log_path = None

    # 记录已分配的日志文件名（避免重复编号）
    prefix_log_map = {}

    process = None

    def read_output():
        """后台线程：持续读取程序输出并放入队列"""
        nonlocal current_test_log_obj
        line_buffer = ""
        byte_count = 0
        line_count = 0

        # 尝试设置非阻塞模式（仅 Unix/Linux）
        if sys.platform != 'win32':
            try:
                import fcntl
                fd = process.stdout.fileno()
                fl = fcntl.fcntl(fd, fcntl.F_GETFL)
                fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)
            except (ImportError, AttributeError, OSError):
                pass  # 非阻塞设置失败，继续使用阻塞模式

        while not stop_reading.is_set():
            try:
                # 检查进程是否还在运行
                if process.poll() is not None and not stop_reading.is_set():
                    print(f"DEBUG: 进程已退出，读取剩余数据...")
                    # 进程已退出，读取剩余输出
                    try:
                        remaining_data = process.stdout.read()
                        if remaining_data:
                            try:
                                line_str = remaining_data.decode(
                                    'utf-8', errors='replace')
                            except:
                                line_str = remaining_data.decode(
                                    'gbk', errors='replace')
                            safe_print(line_str)
                            # 写入主日志
                            main_log_obj.write(line_str)
                            main_log_obj.flush()
                    except Exception as e:
                        print(f"DEBUG: 读取剩余数据时出错：{e}")
                    print(f"DEBUG: 共读取 {line_count} 行，{byte_count} 字节")
                    break

                # 按字节读取（每次最多 4096 字节）
                try:
                    raw_data = process.stdout.read(4096)
                except (IOError, BlockingIOError):
                    # 非阻塞模式下无数据
                    time.sleep(0.001)
                    continue
                except Exception as e:
                    # 其他错误
                    print(f"DEBUG: 读取数据时出错：{e}")
                    time.sleep(0.001)
                    continue

                if not raw_data:
                    # 没有数据，检查进程是否还在运行
                    if process.poll() is not None:
                        print(f"DEBUG: 没有数据但进程已退出")
                        # 进程已退出，尝试继续读取剩余数据
                        try:
                            remaining_data = process.stdout.read()
                            if remaining_data:
                                try:
                                    line_str = remaining_data.decode(
                                        'utf-8', errors='replace')
                                except:
                                    line_str = remaining_data.decode(
                                        'gbk', errors='replace')
                                safe_print(line_str)
                                # 写入主日志
                                main_log_obj.write(line_str)
                                main_log_obj.flush()
                        except Exception as e:
                            print(f"DEBUG: 读取剩余数据时出错：{e}")
                        print(f"DEBUG: 共读取 {line_count} 行，{byte_count} 字节")
                        break

                    # 进程还在运行但没有数据，这可能意味着程序在等待输入
                    # 检查是否已经检测到提示符
                    if prompt_received.is_set():
                        # 已经检测到提示符，继续读取后续输出
                        time.sleep(0.001)
                        continue
                    else:
                        # 还没有检测到提示符，短暂休眠
                        time.sleep(0.001)
                        continue

                # 解码数据（修复：使用 errors='replace' 防止解码失败）
                try:
                    decoded_data = raw_data.decode('utf-8', errors='replace')
                except:
                    try:
                        decoded_data = raw_data.decode('gbk', errors='replace')
                    except:
                        decoded_data = raw_data.decode(
                            'utf-8', errors='ignore')

                byte_count += len(raw_data)

                # 按行分割
                lines = decoded_data.split('\n')

                # 第一行是前一行 buffer 的延续
                if lines:
                    lines[0] = line_buffer + lines[0]
                    line_buffer = lines[-1]  # 最后一行可能不完整
                    lines = lines[:-1]  # 去掉最后一行（不完整的）

                    for line in lines:
                        if line:
                            # 去除行尾的 \r（Windows 换行符的一部分）
                            line = line.rstrip('\r')
                            line_count += 1
                            output_queue.put(line + '\n')

                            # 输出到终端（调试用）- 使用安全打印
                            safe_print(line + '\n')

                            # 写入主日志
                            main_log_obj.write(line)
                            main_log_obj.write('\n')
                            main_log_obj.flush()

                            # 如果当前有打开的测试日志文件，也写入
                            if current_test_log_obj:
                                current_test_log_obj.write(line)
                                current_test_log_obj.write('\n')
                                current_test_log_obj.flush()

                            # 检测提示符
                            if '/ #' in line:
                                prompt_received.set()

            except Exception as e:
                print(f"读取输出时出错：{e}")
                import traceback
                traceback.print_exc()
                time.sleep(0.1)

    try:
        print(f"启动程序：{exe_path}")
        print(f"工作目录：{exe_dir}")

        # 设置环境变量
        env = os.environ.copy()
        env['PYTHONUNBUFFERED'] = '1'

        # 启动进程 - 不使用任何 creationflags
        process = subprocess.Popen(
            [exe_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            cwd=exe_dir,
            bufsize=0,  # 无缓冲
            env=env,
            creationflags=0,
            text=False  # 二进制模式
        )

        print(f"程序已启动，PID: {process.pid}")

        # 启动读取线程
        reader_thread = threading.Thread(target=read_output, daemon=True)
        reader_thread.start()

        # 等待一小段时间确保读取线程已经启动
        time.sleep(0.1)
        print("读取线程已启动，开始读取程序输出...")

        # 等待提示符（最多 30 秒）
        if not prompt_received.wait(timeout=30):
            print("警告：未收到提示符，但继续执行...")
            # 输出已收到的内容帮助调试
            print("已收到的输出预览：")
            preview_lines = []
            try:
                while len(preview_lines) < 10:
                    line = output_queue.get_nowait()
                    preview_lines.append(line.strip())
            except queue.Empty:
                pass
            for line in preview_lines:
                print(f"  {line}")
        else:
            print("程序已就绪！")

    except Exception as e:
        print(f"启动程序失败：{e}")
        import traceback
        traceback.print_exc()
        if main_log_obj:
            main_log_obj.close()
        return False

    all_passed = True
    fail_reason = None
    failed_at_command = None

    try:
        for i, cmd in enumerate(commands, 1):
            print(f"\n[{i}/{len(commands)}] 发送命令：{cmd}")

            # 先检查是否是 dsim_delay 命令
            is_delay_cmd = False
            if cmd.startswith('dsim_delay'):
                parts = cmd.split()
                if len(parts) == 3:
                    try:
                        delay_value = int(parts[2])
                        is_delay_cmd = True
                        # 延迟命令保持当前 log 文件打开，记录延迟期间的输出
                        print(f"等待 {delay_value} 秒...（期间输出将写入当前 log 文件）")
                        time.sleep(delay_value)
                        print(f"等待结束，继续执行下一条命令")
                        # 不 continue，继续执行后续流程（但不发送命令）
                    except ValueError:
                        print(f"警告：dsim_delay 参数错误 '{cmd}'，跳过此命令")
                        continue
                else:
                    print(f"警告：dsim_delay 格式错误，应为 'dsim_delay second <秒数>'，跳过此命令")
                    continue

            # 根据命令前缀打开/切换对应的 log 文件
            prefix = get_test_prefix(cmd)
            if prefix:
                # dsim_delay 命令不分配新的 log 文件编号
                if not is_delay_cmd:
                    if prefix not in prefix_log_map:
                        # 为新前缀分配编号
                        next_index = len(prefix_log_map) + 1
                        prefix_log_map[prefix] = f"{next_index:03d}_{prefix}.log"

                    log_filename = prefix_log_map[prefix]
                    current_test_log_path = os.path.join(output_dir, log_filename)

                    try:
                        # 如果前缀相同，继续使用同一个文件对象（追加模式）
                        if current_test_log_obj and os.path.abspath(current_test_log_obj.name) == os.path.abspath(current_test_log_path):
                            # 同一个文件，继续写入
                            pass
                        else:
                            # 不同文件或首次打开，检查文件是否存在决定使用 'w' 还是 'a'
                            if current_test_log_obj:
                                current_test_log_obj.close()
                            mode = 'w' if not os.path.exists(current_test_log_path) else 'a'
                            current_test_log_obj = open(
                                current_test_log_path, mode, encoding='utf-8', buffering=1)
                    except Exception as e:
                        print(f"警告：无法打开测试日志文件：{e}")
                        current_test_log_obj = None
            else:
                current_test_log_obj = None

            # 如果是 dsim_delay 命令，跳过命令发送和测试状态检查
            if is_delay_cmd:
                continue

            # 清空队列中旧的数据
            while not output_queue.empty():
                try:
                    output_queue.get_nowait()
                except queue.Empty:
                    break

            # 检查进程状态
            if process.poll() is not None:
                print(f"程序已意外退出，退出码：{process.returncode}")
                all_passed = False
                failed_at_command = i
                fail_reason = "程序异常退出"
                break

            # 发送命令
            try:
                cmd_bytes = (cmd + '\n').encode('utf-8')
                process.stdin.write(cmd_bytes)
                process.stdin.flush()

            except BrokenPipeError:
                print("发送命令失败：管道已断开")
                all_passed = False
                failed_at_command = i
                fail_reason = "管道断开"
                break
            except Exception as e:
                print(f"发送命令失败：{e}")
                all_passed = False
                failed_at_command = i
                fail_reason = "发送命令错误"
                break

            # ============================================================
            # ✅ 关键修改：滑动超时窗口（有输出就重置计时器）
            # ============================================================
            test_status = TestStatus.NOT_FOUND
            no_output_timeout = 60    # 无输出超时时间（秒）
            max_total_time = 300      # 最大总执行时间（秒），防止死循环
            start_time = time.time()
            last_output_time = time.time()  # 最后一次收到输出的时间
            check_interval = 0.1
            last_progress_time = 0  # 上次打印进度提示的时间

            while time.time() - start_time < max_total_time:
                # 检查进程状态
                if process.poll() is not None:
                    print("程序已退出")
                    break

                # 检查队列中的新输出
                try:
                    while True:
                        line = output_queue.get_nowait()
                        last_output_time = time.time()  # ✅ 收到输出，重置超时计时器

                        # 检查状态
                        status = check_test_status(line)
                        if status != TestStatus.NOT_FOUND:
                            test_status = status
                            # ✅ 检测到状态后，继续读取后续输出（可能包含提示符）
                            # 不立即 break，继续等待一小段时间让所有输出完成
                            time.sleep(0.1)  # 等待可能的后续输出
                            break
                except queue.Empty:
                    pass

                if test_status != TestStatus.NOT_FOUND:
                    break

                # ✅ 检查无输出时间（而不是总执行时间）
                no_output_duration = time.time() - last_output_time
                if no_output_duration > no_output_timeout:
                    print(f"⚠️ 已 {no_output_timeout} 秒无新输出，判定超时")
                    break

                # 进度提示（每 5 秒无输出时显示）
                current_no_output_seconds = int(no_output_duration)
                if current_no_output_seconds >= 5 and current_no_output_seconds % 5 == 0:
                    current_time = time.time()
                    if current_time - last_progress_time >= 1:  # 确保间隔足够才打印
                        print(
                            f"等待输出中... (无输出 {current_no_output_seconds}s / {no_output_timeout}s)")
                        last_progress_time = current_time

                time.sleep(check_interval)
            # ============================================================

            # 处理测试结果
            if test_status == TestStatus.PASS:
                pass  # 不等待提示符，直接进入下一次循环

            elif test_status == TestStatus.FAIL:
                print(f"❌ 检测到 TEST FAIL，停止执行")
                all_passed = False
                failed_at_command = i
                fail_reason = "TEST FAIL"
                break

            elif test_status == TestStatus.NOT_EXIST:
                print(f"❌ 检测到 TEST NOT EXIST，停止执行")
                all_passed = False
                failed_at_command = i
                fail_reason = "TEST NOT_EXIST"
                break

            else:
                print(f"⚠️ 未检测到测试状态标记")

                # 显示最近收到的输出
                print("最近 5 行输出：")
                recent_lines = []
                try:
                    while len(recent_lines) < 5:
                        line = output_queue.get_nowait()
                        recent_lines.append(line.strip())
                except queue.Empty:
                    pass
                for line in recent_lines:
                    print(f"  {line}")

                if process.poll() is not None:
                    print("程序已异常退出")
                    all_passed = False
                    failed_at_command = i
                    fail_reason = "程序异常退出"
                    break

                print(f"\n命令 '{cmd}' 执行超时")
                print("停止执行")
                fail_reason = "执行超时"
                failed_at_command = i
                break

        # 输出最终结果
        separator = "-" * 80
        summary = f"\n\n{separator}\n"
        if all_passed:
            summary += "✅ 所有命令执行完成且检测到 TEST PASS\n"
        else:
            if fail_reason:
                summary += f"❌ 执行失败：在命令 {failed_at_command} 处检测到 {fail_reason}\n"
                if failed_at_command and failed_at_command <= len(commands):
                    summary += f"失败命令：{commands[failed_at_command-1]}\n"
        summary += f"测试结束时间：{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n"
        summary += f"{'-' * 80}\n"

        print(summary)

    except KeyboardInterrupt:
        print("\n\n用户中断执行")
        all_passed = False
    except Exception as e:
        print(f"\n执行过程中出错：{e}")
        import traceback
        traceback.print_exc()
        all_passed = False
    finally:
        # 清理
        stop_reading.set()

        if process and process.poll() is None:
            try:
                print("\n正在关闭程序...")
                # 尝试优雅退出
                try:
                    process.stdin.write(b'exit\n')
                    process.stdin.flush()
                    time.sleep(1)
                except:
                    pass

                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=2)
            except:
                pass

        # 等待读取线程结束
        time.sleep(1)

        # 关闭日志文件
        if main_log_obj:
            main_log_obj.close()
            print(f"主日志已保存到：{main_log_file}")

        if current_test_log_obj:
            current_test_log_obj.close()
            if current_test_log_path:
                print(f"测试日志已保存到：{current_test_log_path}")

    # 生成测试报告
    if prefix_log_map:
        report_result = generate_report(
            output_dir, prefix_log_map, main_log_file)
        # 根据报告结果判断整体是否通过
        all_passed = (report_result['failed'] == 0)

    return all_passed


def print_usage():
    """打印使用说明"""
    print("""
Kinetis 自动化测试脚本
=====================

功能说明：
1. 执行 make compile 编译程序
2. 从文本文件读取命令，每行一条
3. 支持 C 语言风格注释：// 和 /* */
4. 启动程序一次，然后连续发送所有命令
5. 实时监控程序输出，按命令前缀分类保存日志
6. 自动分析日志中的错误关键词并生成测试报告

检测状态：
  - TEST PASS：继续执行下一条命令
  - TEST FAIL：立即停止执行
  - TEST NOT EXIST：立即停止执行
  - 超时 (60秒)：停止执行

日志分析关键词：
  - FAIL, ERROR, CRASH, FATAL, EXCEPTION, ASSERT, PANIC, ABORT

输出文件：
  - output/all_test_case_output.log      主日志（所有输出）
  - output/001_xxx.log          各测试的独立日志
  - output/all_test_case_result.log       测试报告

使用方式：
python terminal.py [命令文件路径] [可执行程序路径]

或：
python terminal.py

示例：
python terminal.py Kinetis_Plan.txt .\\output\\Four-Axis-Flight.exe
    """)


def compile_program(makefile_path='Makefile', target='compile'):
    """
    执行 make compile 编译程序
    返回是否成功
    """
    print(f"开始编译程序...")
    print(f"Makefile: {makefile_path}")
    print(f"目标: {target}")

    try:
        result = subprocess.run(
            ['make', target],
            cwd=os.path.dirname(makefile_path) if os.path.dirname(
                makefile_path) else '.',
            capture_output=True,
            text=True,
            timeout=600  # 编译超时 10 分钟
        )

        if result.stdout:
            print(result.stdout)

        if result.stderr:
            print("编译错误输出：")
            print(result.stderr)

        if result.returncode == 0:
            print("✅ 编译成功")
            return True
        else:
            print(f"❌ 编译失败，退出码: {result.returncode}")
            return False

    except subprocess.TimeoutExpired:
        print("❌ 编译超时")
        return False
    except Exception as e:
        print(f"❌ 编译出错: {e}")
        return False


def main():
    """主函数"""
    print("-" * 80)
    print("交互式命令监控脚本")
    print("启动程序一次，连续发送所有命令")

    if len(sys.argv) > 1 and sys.argv[1] in ['-h', '--help', '/?']:
        print_usage()
        return

    default_cmd_file = r".\Kinetis_Plan.testlist"
    default_exe_path = r".\output\Four-Axis-Flight.exe"

    if len(sys.argv) >= 3:
        cmd_file = sys.argv[1]
        exe_path = sys.argv[2]
    elif len(sys.argv) == 2:
        cmd_file = sys.argv[1]
        exe_path = default_exe_path
    else:
        cmd_file = default_cmd_file
        exe_path = default_exe_path
        print(f"使用默认命令文件：{cmd_file}")
        print(f"使用默认可执行文件：{exe_path}")
        print()

    if not os.path.exists(cmd_file):
        print(f"错误：命令文件 '{cmd_file}' 不存在")
        return

    exe_path = os.path.abspath(exe_path)

    # 清理旧的 log 目录
    log_dir = os.path.join(os.getcwd(), 'output', 'log')
    if os.path.exists(log_dir):
        print(f"清理旧的日志目录: {log_dir}")
        try:
            import shutil
            shutil.rmtree(log_dir)
            print("✅ 旧日志已清理")
        except Exception as e:
            print(f"⚠️ 清理旧日志失败: {e}")

    # 先编译程序
    makefile_path = os.path.join(os.path.dirname(
        cmd_file) if os.path.dirname(cmd_file) else '.', 'Makefile')
    if os.path.exists(makefile_path):
        compile_success = compile_program(makefile_path)
        if not compile_success:
            print("编译失败，终止执行")
            return
    else:
        print(f"警告：未找到 Makefile，跳过编译步骤")

    if not os.path.exists(exe_path):
        print(f"错误：可执行程序 '{exe_path}' 不存在")
        return

    commands = read_commands_from_file(cmd_file)

    if not commands:
        print("没有可执行的命令")
        return

    print(f"\n将要执行的命令列表 (共 {len(commands)} 条):")
    for i, cmd in enumerate(commands[:20], 1):
        print(f"{i:3d}. {cmd}")

    if len(commands) > 20:
        print(f"... 还有 {len(commands) - 20} 条命令")

    print("\n执行策略:")
    print("  ✅ TEST PASS  -> 继续执行下一条命令")
    print("  ❌ TEST FAIL  -> 立即停止执行")
    print("  ❌ TEST NOT EXIST -> 立即停止执行")
    print("  ⏱️  超时 (60 秒) -> 停止执行")

    print("\n开始执行...\n")
    success = run_interactive_mode(
        commands, cmd_file, exe_path, output_dir='log')
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
