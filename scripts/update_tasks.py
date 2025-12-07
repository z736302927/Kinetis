#!/usr/bin/env python3
"""
根据Makefile更新VSCode tasks.json的脚本
解析Makefile中的编译配置，生成相应的VSCode任务配置
"""

import os
import re
import json
import sys
import argparse
from pathlib import Path

class MakefileParser:
    """解析Makefile的类"""
    
    def __init__(self, makefile_path):
        self.makefile_path = Path(makefile_path)
        self.content = ""
        self.variables = {}
        self.source_files = []
        self.include_dirs = []
        self.defines = []
        self.cflags = []
        
    def parse(self):
        """解析Makefile"""
        if not self.makefile_path.exists():
            raise FileNotFoundError(f"Makefile不存在: {self.makefile_path}")
        
        with open(self.makefile_path, 'r', encoding='utf-8') as f:
            self.content = f.read()
        
        self._parse_variables()
        self._parse_source_files()
        self._parse_includes()
        self._parse_defines()
        self._parse_cflags()
        
    def _parse_variables(self):
        """解析Makefile变量"""
        # 匹配变量定义格式: VAR = value 或 VAR += value
        var_pattern = r'^([A-Z_][A-Z0-9_]*)\s*[\+:]?=\s*(.*)$'
        
        # 首先展开所有的wildcard函数
        expanded_content = self._expand_wildcards(self.content)
        
        # 处理续行
        content_lines = []
        continuation = ""
        for line in expanded_content.split('\n'):
            line = line.strip()
            if line.endswith('\\'):
                continuation += line[:-1] + " "
            else:
                line = continuation + line
                content_lines.append(line)
                continuation = ""
        
        if continuation:
            content_lines.append(continuation)
        
        # 解析变量
        for line in content_lines:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
                
            match = re.match(var_pattern, line)
            if match:
                var_name, var_value = match.groups()
                if var_name in self.variables:
                    if '+=' in line:
                        self.variables[var_name] += ' ' + var_value
                else:
                    self.variables[var_name] = var_value
    
    def _expand_wildcards(self, content):
        """展开Makefile中的wildcard函数"""
        # 匹配$(wildcard pattern)格式
        wildcard_pattern = r'\$\$\(wildcard\s+([^)]+)\)'
        
        def replace_wildcard(match):
            pattern = match.group(1).strip()
            base_dir = self.makefile_path.parent.parent  # 从scripts目录回到项目根目录
            
            # 转换路径格式
            if pattern.startswith('../'):
                abs_pattern = base_dir / pattern[3:]
            else:
                abs_pattern = base_dir / pattern
            
            # 展开通配符
            try:
                if '*' in pattern:
                    matches = list(abs_pattern.parent.glob(abs_pattern.name))
                    return ' '.join(str(m.relative_to(base_dir)) for m in matches)
                else:
                    return str(abs_pattern.relative_to(base_dir)) if abs_pattern.exists() else ''
            except:
                return ''
        
        return re.sub(wildcard_pattern, replace_wildcard, content)
    
    def _parse_source_files(self):
        """解析源文件列表"""
        # 优先使用make print-sources命令获取准确的源文件列表
        try:
            import subprocess
            
            # 切换到scripts目录并执行make print-sources
            result = subprocess.run(
                ['make', 'print-sources'],
                cwd=self.makefile_path.parent,
                capture_output=True,
                text=True,
                timeout=30
            )
            
            if result.returncode == 0:
                base_dir = self.makefile_path.parent.parent
                lines = result.stdout.strip().split('\n')
                
                for line in lines:
                    line = line.strip()
                    # 跳过标题行
                    if 'C Sources:' in line or not line:
                        continue
                    # 处理文件路径
                    if line.endswith('.c'):
                        if line.startswith('../'):
                            # 相对于项目根目录的路径
                            rel_path = line[3:]  # 移除../
                            abs_path = (base_dir / rel_path).resolve()
                        elif line.startswith('E:/'):
                            # 已经是绝对路径
                            abs_path = Path(line)
                        else:
                            # 其他相对路径
                            abs_path = (base_dir / line).resolve()
                        
                        if abs_path.exists():
                            self.source_files.append(str(abs_path))
                return
        except Exception as e:
            print(f"警告: 使用make print-sources失败: {e}")
        
        # 备用方案：从现有的tasks.json中提取源文件列表
        tasks_json_path = self.makefile_path.parent.parent / '.vscode' / 'tasks.json'
        
        if tasks_json_path.exists():
            try:
                with open(tasks_json_path, 'r', encoding='utf-8') as f:
                    tasks_data = json.load(f)
                
                # 从现有任务中提取源文件
                tasks = tasks_data.get("tasks", [])
                if tasks:
                    task = tasks[0]  # 取第一个任务
                    args = task.get("args", [])
                    
                    # 提取以-g开头的参数中的源文件路径
                    base_dir = self.makefile_path.parent.parent
                    for i, arg in enumerate(args):
                        if arg == '-g' and i + 1 < len(args):
                            next_arg = args[i + 1]
                            if next_arg.endswith('.c'):
                                # 转换为绝对路径
                                if next_arg.startswith('E:/'):
                                    self.source_files.append(next_arg)
                                elif next_arg.startswith('"'):
                                    # 处理带引号的路径
                                    clean_path = next_arg.strip('"')
                                    abs_path = (base_dir / clean_path).resolve()
                                    self.source_files.append(str(abs_path))
                                else:
                                    abs_path = (base_dir / next_arg).resolve()
                                    self.source_files.append(str(abs_path))
                else:
                    print("警告: 未找到现有任务配置")
            except Exception as e:
                print(f"警告: 解析现有tasks.json失败: {e}")
        
        # 如果还是没有找到源文件，使用变量解析作为最后备用方案
        if not self.source_files and 'C_SOURCES' in self.variables:
            sources_text = self.variables['C_SOURCES']
            base_dir = self.makefile_path.parent.parent
            
            for src_pattern in sources_text.split():
                src_pattern = src_pattern.strip()
                if src_pattern and src_pattern.endswith('.c') and '*' not in src_pattern:
                    abs_pattern = (base_dir / src_pattern.lstrip('../')).resolve()
                    if abs_pattern.exists():
                        self.source_files.append(str(abs_pattern))
    
    def _parse_includes(self):
        """解析包含目录"""
        if 'C_INCLUDES' in self.variables:
            includes_text = self.variables['C_INCLUDES']
            base_dir = self.makefile_path.parent  # scripts目录
            project_root = base_dir.parent  # 项目根目录
            
            # 按空格分割，但需要重新组合 -include 参数
            tokens = includes_text.split()
            i = 0
            while i < len(tokens):
                token = tokens[i].strip()
                
                if token == '-I':
                    # 处理 -I 参数（下一项是路径）
                    if i + 1 < len(tokens):
                        include_path = tokens[i + 1]
                        # 处理相对路径（相对于项目根目录）
                        if include_path.startswith('../'):
                            abs_path = (project_root / include_path[3:]).resolve()
                            self.include_dirs.append('-I')
                            self.include_dirs.append(str(abs_path))
                        else:
                            abs_path = (project_root / include_path).resolve()
                            self.include_dirs.append('-I')
                            self.include_dirs.append(str(abs_path))
                        i += 2  # 跳过路径
                    else:
                        i += 1
                elif token == '-include':
                    # 处理 -include 参数（下一项是头文件路径）
                    if i + 1 < len(tokens):
                        header_file = tokens[i + 1]
                        # 处理相对路径（相对于项目根目录）
                        if header_file.startswith('../'):
                            abs_header = (project_root / header_file[3:]).resolve()
                        elif not header_file.startswith('/') and not header_file[1] == ':':
                            # 相对路径（不带../）
                            abs_header = (project_root / header_file).resolve()
                        else:
                            # 绝对路径
                            abs_header = Path(header_file).resolve()
                        # 使用 -include 作为单独参数，后跟完整路径
                        self.include_dirs.append("-include")
                        self.include_dirs.append(str(abs_header))
                        i += 2  # 跳过头文件路径
                    else:
                        i += 1
                elif token.startswith('-I'):
                    # 处理 -Ipath 格式
                    include_path = token[2:]
                    if include_path.startswith('../'):
                        abs_path = (base_dir.parent / include_path[3:]).resolve()
                        self.include_dirs.append(str(abs_path))
                    else:
                        abs_path = (base_dir.parent / include_path).resolve()
                        self.include_dirs.append(str(abs_path))
                    i += 1
                elif token.startswith('-include'):
                    # 处理 -includepath 格式
                    header_file = token[8:]
                    if header_file.startswith('../'):
                        abs_header = (base_dir.parent / header_file[3:]).resolve()
                    else:
                        abs_header = (base_dir.parent / header_file).resolve()
                    self.include_dirs.append("-include")
                    self.include_dirs.append(str(abs_header))
                    i += 1
                else:
                    # 其他参数，直接添加
                    self.include_dirs.append(token)
                    i += 1
    
    def _parse_defines(self):
        """解析预定义宏"""
        if 'C_DEFS' in self.variables:
            defs_text = self.variables['C_DEFS']
            for define in defs_text.split():
                define = define.strip()
                if define.startswith('-D'):
                    self.defines.append(define)
    
    def _parse_cflags(self):
        """解析编译标志"""
        # 获取基础编译标志
        base_flags = []
        if 'COMMON_FLAGS' in self.variables:
            flags_text = self.variables['COMMON_FLAGS']
            for flag in flags_text.split():
                flag = flag.strip()
                if flag and not flag.startswith('$'):
                    base_flags.append(flag)
        
        if 'WARN_FLAGS' in self.variables:
            flags_text = self.variables['WARN_FLAGS']
            for flag in flags_text.split():
                flag = flag.strip()
                if flag and not flag.startswith('$'):
                    base_flags.append(flag)
        
        if 'OPT' in self.variables:
            flags_text = self.variables['OPT']
            for flag in flags_text.split():
                flag = flag.strip()
                if flag and not flag.startswith('$'):
                    base_flags.append(flag)
        
        self.cflags.extend(base_flags)


class TasksJsonGenerator:
    """生成tasks.json的类"""
    
    def __init__(self, workspace_root):
        self.workspace_root = Path(workspace_root)
        self.tasks_json_path = self.workspace_root / '.vscode' / 'tasks.json'
        
    def generate_task_config(self, parser, gcc_path="E:/Plugin/mingw64/bin/gcc.exe"):
        """生成VSCode任务配置"""
        
        # 构建参数数组
        args = []
        
        # 添加基础编译标志
        args.extend(parser.cflags)
        
        # 添加包含目录
        i = 0
        while i < len(parser.include_dirs):
            include = parser.include_dirs[i]
            if include == '-include':
                # -include参数，后跟头文件路径
                args.append(include)
                if i + 1 < len(parser.include_dirs):
                    args.append(parser.include_dirs[i + 1])
                    i += 2  # 跳过头文件路径
                else:
                    i += 1
            else:
                # 普通的-I参数，确保是绝对路径
                args.append('-I')
                if not (include.startswith('E:/') or include.startswith('C:')):
                    # 如果不是绝对路径，转换为绝对路径
                    abs_path = (self.workspace_root / include).resolve()
                    args.append(str(abs_path))
                else:
                    args.append(include)
                i += 1
        
        # 添加预定义宏
        args.extend(parser.defines)
        
        # 添加调试标志
        args.extend(['-g', '-fdiagnostics-color=always'])
        
        # 添加源文件
        for src_file in parser.source_files:
            args.append('-g')
            args.append(src_file)
        
        # 添加输出文件
        output_dir = self.workspace_root / '.vscode' / 'output'
        output_file = output_dir / 'kinetis.exe'
        args.extend(['-o', str(output_file)])
        
        # 构建任务配置
        task_config = {
            "type": "cppbuild",
            "label": "Build with Makefile Configuration",
            "command": gcc_path,
            "args": args,
            "options": {
                "cwd": "E:/Plugin/mingw64/bin"
            },
            "problemMatcher": [
                "$gcc"
            ],
            "group": {
                "kind": "build",
                "isDefault": True
            },
            "detail": "Auto-generated from Makefile"
        }
        
        return task_config, output_dir
    
    def update_tasks_json(self, task_config, output_dir):
        """更新tasks.json文件"""
        
        # 确保输出目录存在
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # 读取现有的tasks.json或创建新的
        if self.tasks_json_path.exists():
            with open(self.tasks_json_path, 'r', encoding='utf-8') as f:
                tasks_data = json.load(f)
        else:
            tasks_data = {
                "version": "2.0.0",
                "tasks": []
            }
        
        # 更新或添加任务
        tasks = tasks_data.get("tasks", [])
        
        # 查找是否已存在相同标签的任务
        existing_task_index = -1
        for i, task in enumerate(tasks):
            if task.get("label") == task_config["label"]:
                existing_task_index = i
                break
        
        if existing_task_index >= 0:
            # 更新现有任务
            tasks[existing_task_index] = task_config
            print(f"更新现有任务: {task_config['label']}")
        else:
            # 添加新任务
            tasks.append(task_config)
            print(f"添加新任务: {task_config['label']}")
        
        tasks_data["tasks"] = tasks
        
        # 确保目录存在
        self.tasks_json_path.parent.mkdir(parents=True, exist_ok=True)
        
        # 写入文件
        with open(self.tasks_json_path, 'w', encoding='utf-8') as f:
            json.dump(tasks_data, f, indent=4, ensure_ascii=False)
        
        print(f"tasks.json已更新: {self.tasks_json_path}")
        
        # 输出统计信息
        print(f"\n任务配置统计:")
        print(f"  源文件数量: {len([arg for arg in task_config['args'] if arg.endswith('.c') and not arg.startswith('-')])}")
        print(f"  包含目录数量: {len([arg for arg in task_config['args'] if arg.startswith('-I') or arg.startswith('-include')])}")
        print(f"  预定义宏数量: {len([arg for arg in task_config['args'] if arg.startswith('-D')])}")
        print(f"  输出文件: {task_config['args'][-1]}")


def main():
    parser = argparse.ArgumentParser(description='根据Makefile更新VSCode tasks.json')
    parser.add_argument(
        '--makefile',
        help='Makefile文件路径 (默认: scripts/Makefile)',
        default='scripts/Makefile'
    )
    parser.add_argument(
        '--workspace',
        help='工作空间根目录路径 (默认: 自动检测)',
        default=None
    )
    parser.add_argument(
        '--gcc-path',
        help='GCC编译器路径',
        default='E:/Plugin/mingw64/bin/gcc.exe'
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='仅显示将要生成的配置，不实际写入文件'
    )
    
    args = parser.parse_args()
    
    # 自动检测工作空间根目录
    if args.workspace is None:
        current_dir = Path.cwd()
        if current_dir.name == 'scripts':
            # 如果在scripts目录下，使用父目录作为工作空间
            workspace_root = current_dir.parent
            # 调整Makefile路径
            if args.makefile == 'scripts/Makefile':
                args.makefile = 'Makefile'
        elif current_dir.name == '.vscode':
            # 如果在.vscode目录下，使用父目录作为工作空间
            workspace_root = current_dir.parent
        else:
            # 其他情况，尝试查找项目根目录
            workspace_root = current_dir
            # 查找scripts目录
            scripts_dir = workspace_root / 'scripts'
            if scripts_dir.exists():
                pass  # 使用当前目录作为工作空间
            else:
                # 向上查找项目根目录
                parent_dir = workspace_root.parent
                if (parent_dir / 'scripts').exists():
                    workspace_root = parent_dir
    else:
        workspace_root = Path(args.workspace)
    
    try:
        print("开始解析Makefile...")
        print(f"Makefile路径: {Path(args.makefile).resolve()}")
        print(f"工作空间路径: {workspace_root.resolve()}")
        
        # 解析Makefile
        makefile_parser = MakefileParser(args.makefile)
        makefile_parser.parse()
        
        # 生成任务配置
        generator = TasksJsonGenerator(workspace_root)
        
        print(f"解析完成:")
        print(f"  源文件: {len(makefile_parser.source_files)} 个")
        print(f"  包含目录: {len(makefile_parser.include_dirs)} 个")
        print(f"  预定义宏: {len(makefile_parser.defines)} 个")
        print(f"  编译标志: {len(makefile_parser.cflags)} 个")
        
        task_config, output_dir = generator.generate_task_config(makefile_parser, args.gcc_path)
        
        if args.dry_run:
            print(f"\n=== 模拟运行 - 任务配置 ===")
            print(json.dumps(task_config, indent=2, ensure_ascii=False))
            print(f"\n输出目录: {output_dir}")
        else:
            # 更新tasks.json
            generator.update_tasks_json(task_config, output_dir)
            
        print("\n操作完成!")
        
    except Exception as e:
        print(f"错误: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()