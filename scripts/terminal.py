import subprocess
import sys
import time
import os
from enum import Enum

class TestStatus(Enum):
    """测试状态枚举"""
    NOT_FOUND = "未检测到状态"
    PASS = "PASS"
    FAIL = "FAIL"
    NOT_EXIST = "NOT_EXIST"

def remove_c_comments(text):
    """
    移除C语言风格的注释：// 和 /* */
    返回移除注释后的文本行列表
    """
    lines = text.split('\n')
    result_lines = []
    
    # 状态标志
    in_multiline_comment = False
    comment_buffer = ""
    
    for line in lines:
        i = 0
        clean_line = ""
        line_length = len(line)
        
        while i < line_length:
            if not in_multiline_comment:
                # 查找单行注释
                if i + 1 < line_length and line[i] == '/' and line[i+1] == '/':
                    # 找到单行注释，跳过本行剩余部分
                    break
                # 查找多行注释开始
                elif i + 1 < line_length and line[i] == '/' and line[i+1] == '*':
                    in_multiline_comment = True
                    i += 2  # 跳过/*
                    comment_buffer = ""
                else:
                    clean_line += line[i]
                    i += 1
            else:
                # 在多行注释中，查找结束标记
                if i + 1 < line_length and line[i] == '*' and line[i+1] == '/':
                    in_multiline_comment = False
                    i += 2  # 跳过*/
                    # 清空注释缓冲区
                    comment_buffer = ""
                else:
                    comment_buffer += line[i]
                    i += 1
        
        # 如果不在多行注释中，并且有非空内容，则添加到结果
        if not in_multiline_comment and clean_line.strip():
            result_lines.append(clean_line.rstrip())
        elif clean_line.strip():
            # 如果有多行注释开始但未结束，可以决定是否保留剩余部分
            # 这里我们保留非注释部分，但跳过注释
            pass
    
    # 如果文件以未结束的多行注释结束，发出警告
    if in_multiline_comment:
        print("警告：文件包含未结束的多行注释")
    
    return result_lines

def read_commands_from_file(file_path):
    """从文件读取命令列表，跳过C语言风格注释"""
    commands = []
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 移除所有C语言风格注释
        commands = remove_c_comments(content)
        
        # 进一步过滤：移除空行和只有空格的行
        commands = [cmd for cmd in commands if cmd.strip()]
        
        print(f"读取到 {len(commands)} 条有效命令（已跳过注释）")
        
        return commands
    except FileNotFoundError:
        print(f"错误：文件 '{file_path}' 未找到")
        return []
    except Exception as e:
        print(f"读取文件时出错: {e}")
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

def run_interactive_mode(commands, cmd_file, exe_path):
    """
    交互式模式：启动程序一次，然后连续发送所有命令
    这个模式适用于程序启动后保持运行并等待用户输入的情况
    """
    if not commands:
        print("没有命令可执行")
        return False
    
    # 转换为绝对路径
    exe_path = os.path.abspath(exe_path)
    
    if not os.path.exists(exe_path):
        print(f"错误：可执行文件 '{exe_path}' 不存在")
        return False
    
    print(f"从文件 '{cmd_file}' 读取到 {len(commands)} 条有效命令")
    print(f"将使用可执行文件: {exe_path}")
    print("开始执行命令（交互式模式）...")
    print("=" * 50)
    
    # 设置工作目录为可执行文件所在目录
    exe_dir = os.path.dirname(exe_path)
    
    try:
        print(f"启动程序: {exe_path}")
        print(f"工作目录: {exe_dir}")
        print("-" * 50)
        
        # 启动程序，使用管道进行输入输出
        process = subprocess.Popen(
            [exe_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            cwd=exe_dir,
            bufsize=1,
            universal_newlines=True,
            encoding='gbk',
            errors='ignore'
        )
        
        # 给程序一点时间启动
        print("等待程序启动...")
        time.sleep(2)
        
        # 读取并打印初始输出（如果有的话）
        initial_output = []
        for _ in range(10):  # 最多读取10行初始输出
            if process.poll() is not None:
                break
                
            try:
                # 非阻塞读取
                import select
                import sys as _sys
                
                if select.select([process.stdout], [], [], 0.1)[0]:
                    line = process.stdout.readline()
                    if line:
                        print(line, end='')
                        initial_output.append(line)
                else:
                    break
            except:
                break
        
        print("-" * 50)
        
    except Exception as e:
        print(f"启动程序失败: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    all_passed = True
    fail_reason = None
    failed_at_command = None
    
    try:
        for i, cmd in enumerate(commands, 1):
            print(f"\n[{i}/{len(commands)}] 发送命令: {cmd}")
            print("-" * 30)
            
            # 发送命令
            try:
                process.stdin.write(cmd + '\n')
                process.stdin.flush()
                print(f"已发送命令: {cmd}")
            except Exception as e:
                print(f"发送命令失败: {e}")
                all_passed = False
                failed_at_command = i
                fail_reason = "发送命令失败"
                break
            
            # 读取输出并检查状态
            test_status = TestStatus.NOT_FOUND
            timeout = 60  # 每条命令最多等待60秒
            start_time = time.time()
            output_lines = []
            
            # 持续读取输出直到找到状态标记或超时
            while time.time() - start_time < timeout:
                try:
                    # 检查进程是否已结束
                    if process.poll() is not None:
                        print("程序已退出")
                        # 读取剩余输出
                        remaining = process.stdout.read()
                        if remaining:
                            print(remaining, end='')
                            output_lines.append(remaining)
                        break
                    
                    # 尝试读取一行输出
                    import select
                    import sys as _sys
                    
                    # 使用select进行非阻塞读取
                    ready_to_read, _, _ = select.select([process.stdout], [], [], 0.5)
                    
                    if ready_to_read:
                        line = process.stdout.readline()
                        if line:
                            print(line, end='')
                            output_lines.append(line)
                            
                            # 检查状态标记
                            status = check_test_status(line)
                            if status != TestStatus.NOT_FOUND:
                                test_status = status
                                print(f"检测到状态: {status.value}")
                                break
                    
                except Exception as e:
                    print(f"读取输出时出错: {e}")
                    break
            
            # 检查测试状态
            if test_status == TestStatus.PASS:
                print(f"✅ 检测到 TEST PASS，准备执行下一条命令")
                # 给程序一点时间处理
                time.sleep(1)
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
                fail_reason = "TEST NOT EXIST"
                break
            else:
                print(f"⚠️  未检测到测试状态标记")
                
                # 检查程序是否仍在运行
                if process.poll() is not None:
                    print("程序已异常退出")
                    all_passed = False
                    failed_at_command = i
                    fail_reason = "程序异常退出"
                    
                    # 尝试读取剩余输出
                    try:
                        remaining = process.stdout.read()
                        if remaining:
                            print("程序退出前的输出:")
                            print(remaining, end='')
                    except:
                        pass
                    
                    break
                
                # 询问用户是否继续
                print(f"\n命令 '{cmd}' 执行完成但未检测到状态标记")
                print("可能原因:")
                print("  1. 程序仍在运行中")
                print("  2. 输出中没有包含TEST PASS/FAIL标记")
                print("  3. 程序需要更多时间")
                
                choice = input("\n是否继续执行下一条命令？(y/n): ").strip().lower()
                if choice != 'y':
                    print("用户选择停止执行")
                    break
        
        print("\n" + "=" * 50)
        
        # 输出最终结果
        if all_passed:
            print("✅ 所有命令执行完成且检测到TEST PASS")
        else:
            if fail_reason:
                print(f"❌ 执行失败：在命令 {failed_at_command} 处检测到 {fail_reason}")
                if failed_at_command <= len(commands):
                    print(f"失败命令：{commands[failed_at_command-1]}")
            else:
                print("⚠️  部分命令未检测到TEST PASS")
            
        print("=" * 50)
        
    except KeyboardInterrupt:
        print("\n\n用户中断执行")
        all_passed = False
    except Exception as e:
        print(f"\n执行过程中出错: {e}")
        import traceback
        traceback.print_exc()
        all_passed = False
    finally:
        # 清理进程
        print("\n清理进程...")
        if process.poll() is None:
            try:
                # 尝试优雅地退出
                print("尝试发送退出命令...")
                process.stdin.write('exit\n')
                process.stdin.flush()
                time.sleep(2)
            except:
                pass
            
            try:
                print("终止进程...")
                process.terminate()
                process.wait(timeout=5)
            except:
                try:
                    print("强制结束进程...")
                    process.kill()
                    process.wait(timeout=2)
                except:
                    pass
        
        print("执行结束")
    
    return all_passed

def print_usage():
    """打印使用说明"""
    print("""
交互式命令监控脚本
==================

功能说明：
1. 从文本文件读取命令，每行一条
2. 支持C语言风格注释：// 和 /* */
3. 启动程序一次，然后连续发送所有命令
4. 实时监控程序输出
5. 检测以下状态标记并采取相应操作：
   - TEST PASS：继续执行下一条命令
   - TEST FAIL：立即停止执行
   - TEST NOT EXIST：立即停止执行

使用方式：
python interactive_terminal.py [命令文件路径] [可执行程序路径]

或：
python interactive_terminal.py
然后输入命令文件路径和可执行程序路径

示例：
python interactive_terminal.py Kinetis_Plan.txt .\\output\\Four-Axis-Flight.exe

适用场景：
• 程序启动后保持运行并等待用户输入
• 程序通过标准输入(stdin)接收命令
• 程序通过标准输出(stdout)显示结果
    """)

def test_program_interaction(exe_path):
    """测试程序交互性"""
    print("测试程序交互性...")
    print("=" * 50)
    
    exe_path = os.path.abspath(exe_path)
    exe_dir = os.path.dirname(exe_path)
    
    if not os.path.exists(exe_path):
        print(f"错误：可执行文件 '{exe_path}' 不存在")
        return False
    
    try:
        # 启动程序
        process = subprocess.Popen(
            [exe_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            cwd=exe_dir,
            bufsize=1,
            universal_newlines=True,
            encoding='gbk',
            errors='ignore'
        )
        
        print("程序已启动，等待初始输出...")
        time.sleep(2)
        
        # 读取初始输出
        initial_output = []
        for _ in range(5):
            try:
                import select
                if select.select([process.stdout], [], [], 0.5)[0]:
                    line = process.stdout.readline()
                    if line:
                        print(f"初始输出: {line}", end='')
                        initial_output.append(line)
            except:
                break
        
        # 发送测试命令
        test_command = "help\n"
        print(f"\n发送测试命令: {test_command.strip()}")
        process.stdin.write(test_command)
        process.stdin.flush()
        
        # 读取响应
        print("等待响应...")
        time.sleep(2)
        
        response = []
        for _ in range(10):
            try:
                import select
                if select.select([process.stdout], [], [], 0.5)[0]:
                    line = process.stdout.readline()
                    if line:
                        print(f"响应: {line}", end='')
                        response.append(line)
            except:
                break
        
        # 清理
        if process.poll() is None:
            process.terminate()
            process.wait(timeout=2)
        
        print("=" * 50)
        
        if response:
            print("✅ 程序响应正常，适合交互式模式")
            return True
        else:
            print("⚠️  程序未响应，可能不适合交互式模式")
            return False
            
    except Exception as e:
        print(f"测试失败: {e}")
        return False

def main():
    """主函数"""
    print("=" * 60)
    print("交互式命令监控脚本")
    print("启动程序一次，连续发送所有命令")
    print("=" * 60)
    
    # 检查是否请求帮助
    if len(sys.argv) > 1 and sys.argv[1] in ['-h', '--help', '/?']:
        print_usage()
        return
    
    # 获取参数
    if len(sys.argv) >= 3:
        cmd_file = sys.argv[1]
        exe_path = sys.argv[2]
    elif len(sys.argv) == 2:
        cmd_file = sys.argv[1]
        exe_path = input("\n请输入可执行程序路径: ").strip()
    else:
        cmd_file = input("\n请输入命令文件路径: ").strip()
        exe_path = input("请输入可执行程序路径: ").strip()
    
    # 检查文件是否存在
    if not os.path.exists(cmd_file):
        print(f"错误：命令文件 '{cmd_file}' 不存在")
        return
    
    # 转换为绝对路径
    exe_path = os.path.abspath(exe_path)
    
    if not os.path.exists(exe_path):
        print(f"错误：可执行程序 '{exe_path}' 不存在")
        return
    
    # 可选：测试程序交互性
    test_choice = input("\n是否先测试程序交互性？(y/n，推荐y): ").strip().lower()
    if test_choice == 'y':
        if not test_program_interaction(exe_path):
            print("\n⚠️  程序可能不适合交互式模式")
            print("您可以尝试：")
            print("  1. 检查程序是否支持标准输入/输出")
            print("  2. 检查程序启动后是否等待用户输入")
            print("  3. 手动运行程序测试交互性")
            
            choice = input("\n是否继续执行？(y/n): ").strip().lower()
            if choice != 'y':
                print("执行取消")
                return
    
    # 读取命令（会自动跳过注释）
    commands = read_commands_from_file(cmd_file)
    
    if not commands:
        print("没有可执行的命令")
        return
    
    # 显示将要执行的命令
    print(f"\n将要执行的命令列表 (共 {len(commands)} 条):")
    for i, cmd in enumerate(commands[:20], 1):  # 显示前20条
        print(f"{i:3d}. {cmd}")
    
    if len(commands) > 20:
        print(f"... 还有 {len(commands) - 20} 条命令")
    
    # 显示执行策略
    print("\n执行策略:")
    print("  ✅ TEST PASS  -> 继续执行下一条命令")
    print("  ❌ TEST FAIL  -> 立即停止执行")
    print("  ❌ TEST NOT EXIST -> 立即停止执行")
    print("  ⏱️  超时(60秒) -> 询问是否继续")
    print("\n注意：")
    print(f"  • 将启动程序: {exe_path}")
    print("  • 程序将一直运行，直到所有命令执行完成")
    print("  • 需要程序输出中包含TEST PASS/FAIL等标记")
    
    # 确认执行
    choice = input("\n是否开始执行？(y/n): ").strip().lower()
    if choice == 'y':
        success = run_interactive_mode(commands, cmd_file, exe_path)
        sys.exit(0 if success else 1)
    else:
        print("执行取消")

if __name__ == "__main__":
    main()