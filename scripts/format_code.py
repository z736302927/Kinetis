#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import subprocess
import argparse
from pathlib import Path

def find_source_files(directory, extensions=None):
    """
    递归查找指定目录下的所有源代码文件
    
    Args:
        directory: 要搜索的目录路径
        extensions: 文件扩展名列表，默认包含常见的C/C++文件扩展名
    
    Returns:
        源代码文件路径列表
    """
    if extensions is None:
        extensions = ['.c', '.cpp', '.cxx', '.cc', '.h', '.hpp', '.hxx', '.hh']
    
    source_files = []
    directory = Path(directory)
    
    for ext in extensions:
        # 递归查找所有匹配的文件
        pattern = f"**/*{ext}"
        files = directory.glob(pattern)
        source_files.extend(files)
    
    return [str(f) for f in source_files]

def format_with_astyle(file_path, astyle_path='astyle', astyle_options=None):
    """
    使用astyle格式化单个文件
    
    Args:
        file_path: 要格式化的文件路径
        astyle_path: astyle可执行文件路径，默认使用系统PATH中的astyle
        astyle_options: astyle命令行选项列表
    
    Returns:
        (success: bool, output: str, error: str)
    """
    if astyle_options is None:
        astyle_options = [
            '--style=linux',              # Linux内核风格
            '--indent=force-tab=4',       # 强制使用tab缩进4个字符
            '--attach-closing-while',      # while循环的闭合括号附加
            '--indent-col1-comments',      # 缩进第一列注释
            '--max-continuation-indent=100', # 最大续行缩进100
            '--pad-comma',              # 逗号后加空格
            '--pad-include',             # include后加空格
            '--squeeze-lines=1',         # 删除多余空行
            '--break-after-logical',      # 逻辑运算符后换行
            '--indent-preproc-define',    # 缩进预处理定义
            '--pad-oper',               # 操作符加空格
            '--break-one-line-headers',  # 单行头换行
            '--pad-header',              # 头文件加空格
            '--indent-after-parens',     # 括号后缩进
            '--unpad-paren',            # 删除括号多余空格
            '--add-braces',             # 添加大括号
            # '---remove-braces',        # 移除大括号（已注释）
            '--align-pointer=name',       # 指针对齐方式
            '--align-reference=name',     # 引用对齐方式
            '--suffix=none',            # 不创建备份文件
        ]
    
    cmd = [astyle_path] + astyle_options + [file_path]
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30  # 30秒超时
        )
        
        return result.returncode == 0, result.stdout, result.stderr
        
    except subprocess.TimeoutExpired:
        return False, "", "astyle处理超时"
    except FileNotFoundError:
        return False, "", f"找不到astyle可执行文件: {astyle_path}"
    except Exception as e:
        return False, "", f"执行astyle时出错: {str(e)}"

def format_directory(directory, astyle_path='astyle', astyle_options=None, dry_run=False):
    """
    格式化整个目录下的所有C/C++源文件
    
    Args:
        directory: 要格式化的目录路径
        astyle_path: astyle可执行文件路径
        astyle_options: astyle命令行选项
        dry_run: 是否只显示将要格式化的文件，不实际执行
    
    Returns:
        (total_files, success_count, failed_files)
    """
    print(f"搜索目录: {directory}")
    
    # 查找所有源代码文件
    source_files = find_source_files(directory)
    
    if not source_files:
        print("未找到任何源代码文件")
        return 0, 0, []
    
    print(f"找到 {len(source_files)} 个源代码文件")
    
    total_files = len(source_files)
    success_count = 0
    failed_files = []
    
    for i, file_path in enumerate(source_files, 1):
        print(f"[{i}/{total_files}] 处理: {file_path}")
        
        if dry_run:
            print(f"  将格式化: {file_path}")
            success_count += 1
            continue
        
        success, output, error = format_with_astyle(file_path, astyle_path, astyle_options)
        
        if success:
            success_count += 1
            if output:
                print(f"  输出: {output.strip()}")
        else:
            failed_files.append(file_path)
            print(f"  错误: {error}")
    
    return total_files, success_count, failed_files

def main():
    parser = argparse.ArgumentParser(
        description='使用astyle格式化整个文件夹的C/C++代码',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例用法:
  python format_code.py ./src                    # 格式化src目录
  python format_code.py . --dry-run            # 预览模式，不实际格式化
  python format_code.py . --astyle /usr/bin/astyle  # 指定astyle路径
  python format_code.py . --style=allman       # 使用Allman风格
        """
    )
    
    parser.add_argument(
        'directory',
        nargs='?',
        default='.',
        help='要格式化的目录路径（默认为当前目录）'
    )
    
    parser.add_argument(
        '--astyle',
        default='astyle',
        help='astyle可执行文件路径（默认使用系统PATH中的astyle）'
    )
    
    parser.add_argument(
        '--style',
        choices=['attach', 'allman', 'gnu', 'google', 'linux'],
        default='attach',
        help='代码风格（默认为attach）'
    )
    
    parser.add_argument(
        '--indent',
        type=int,
        default=4,
        help='缩进空格数（默认为4）'
    )
    
    parser.add_argument(
        '--max-length',
        type=int,
        default=120,
        help='最大代码行长度（默认为120）'
    )
    
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='预览模式，只显示将要格式化的文件，不实际执行'
    )
    
    parser.add_argument(
        '--extensions',
        nargs='+',
        default=['.c', '.cpp', '.cxx', '.cc', '.h', '.hpp', '.hxx', '.hh'],
        help='要处理的文件扩展名（默认为所有C/C++文件）'
    )
    
    args = parser.parse_args()
    
    # 使用函数内部的默认选项（用户的自定义设置）
    astyle_options = None
    
    # 检查目录是否存在
    if not os.path.exists(args.directory):
        print(f"错误: 目录 '{args.directory}' 不存在")
        sys.exit(1)
    
    if not os.path.isdir(args.directory):
        print(f"错误: '{args.directory}' 不是一个目录")
        sys.exit(1)
    
    print("=" * 60)
    print("C/C++ 代码格式化工具")
    print("=" * 60)
    print(f"目标目录: {os.path.abspath(args.directory)}")
    print(f"代码风格: {args.style}")
    print(f"缩进: {args.indent} 个空格")
    print(f"最大行长度: {args.max_length}")
    print(f"文件扩展名: {', '.join(args.extensions)}")
    print(f"预览模式: {'是' if args.dry_run else '否'}")
    print(f"astyle路径: {args.astyle}")
    print("=" * 60)
    
    # 执行格式化
    total, success, failed = format_directory(
        args.directory,
        args.astyle,
        astyle_options,
        args.dry_run
    )
    
    print("\n" + "=" * 60)
    print("格式化完成!")
    print(f"总文件数: {total}")
    print(f"成功: {success}")
    print(f"失败: {len(failed)}")
    
    if failed:
        print("\n失败的文件:")
        for file_path in failed:
            print(f"  - {file_path}")
    
    if len(failed) > 0:
        sys.exit(1)
    
    print("=" * 60)

if __name__ == '__main__':
    main()