#!/usr/bin/env python3
"""
文件内容删除脚本 - 内置规则版本
可以删除当前文件夹及其子文件夹中所有文件的特定内容

使用示例:
  python linux2kinetis.py                          # 两者都执行
  python linux2kinetis.py --mode content           # 只删除文件内容
  python linux2kinetis.py --mode files             # 只删除指定文件
  python linux2kinetis.py --mode both --backup     # 两者都执行并备份
  python linux2kinetis.py --list-rules             # 查看可用规则
"""

import os
import re
import argparse
import sys
import shutil
from pathlib import Path

# ================================
# 内置删除规则配置
# 在这里添加或修改要删除的内容
# ================================

# 规则类型说明：
# - "text": 普通文本替换
# - "regex": 正则表达式替换
# - "delete_file": 删除文件

# ========================================
# 简化删除规则配置 - 只需要提供要删除的内容
# ========================================
#
# 使用方法：
# 1. 在下面 DELETE_PATTERNS 列表中添加要删除的内容模式
# 2. 支持正则表达式，例如：
#    - r'^\s*#include.*'  # 删除所有 #include 开头的行
#    - r'TODO.*'          # 删除包含 TODO 的行
#    - 'DEBUG'            # 删除包含 DEBUG 的行（普通文本匹配）
# 3. 所有模式都是正则表达式格式，建议使用 r'' 原始字符串
#
# ========================================

DELETE_PATTERNS = [
    r'^\s*#include\s*[<"]generated/deconfig\.h[>"]\s*$',  # 删除 generated/deconfig.h 包含
    r'^\s*MODULE_AUTHOR\s*\(',                                    # 删除 MODULE_AUTHOR 声明
    r'^\s*MODULE_DESCRIPTION\s*\(',                                 # 删除 MODULE_DESCRIPTION 声明
    r'^\s*MODULE_ALIAS\s*\(',                                       # 删除 MODULE_ALIAS 声明
    r'^\s*MODULE_LICENSE\s*\(',                                     # 删除 MODULE_LICENSE 声明
    r'^\s*module_init\s*\(',                                       # 删除 module_init 函数
    r'^\s*module_exit\s*\(',                                       # 删除 module_exit 函数
    r'^\s*MODULE_INFO\s*\(',                                       # 删除 MODULE_INFO 声明
    r'^\s*EXPORT_SYMBOL\s*\(',                                     # 删除 EXPORT_SYMBOL 声明
    r'^\s*raw_spin_lock_irqsave\s*\(',                             # 删除 raw_spin_lock_irqsave 调用
    r'^\s*raw_spin_unlock_irqrestore\s*\(',                         # 删除 raw_spin_unlock_irqrestore 调用
    
    # 在这里添加更多删除模式，例如：
    # r'^\s*#if\s+0\s*\*',        # 删除 #if 0 条件编译块
    # r'^\s*#endif',               # 删除 #endif
    # r'TODO:',                    # 删除包含 TODO: 的行
    # r'debug_printf',              # 删除 debug_printf 调用
]

# ========================================
# 要复制的文件配置
# ========================================

# 要复制的文件列表，支持通配符
FILES_TO_COPY = [
    "*.c",
    "*.h",
    # "Makefile",      # 如果需要复制Makefile，取消注释
    # "Kconfig",       # 如果需要复制Kconfig，取消注释
    "*.txt",
    "README*",
    "LICENSE*",
    # 添加更多文件模式
]

# 要跳过的文件夹（复制时）
COPY_EXCLUDE_DIRS = [
    ".git",
    ".svn",
    ".hg",
    "__pycache__",
    "node_modules",
    "build",
    "dist",
    "bin",
    "obj"
]

# ========================================
# 文件删除配置
# ========================================

# 要删除的文件名列表
FILES_TO_DELETE = ["Kconfig", "Makefile", "TODO", "*.o", "*.S", "*.cmd", "*.release"]

# 要跳过的文件夹路径
EXCLUDED_PATHS = [
    r"E:\Code\Kinetis\scripts",
    # r"E:\Code\Kinetis\include"
]

# ================================
# 新增文件复制功能
# ================================

def copy_files_before_delete(source_dir, dest_dir, verbose=False):
    """
    在删除操作前复制指定文件
    
    Args:
        source_dir: 源目录
        dest_dir: 目标目录
        verbose: 是否显示详细信息
    
    Returns:
        tuple: (复制的文件数, 跳过的文件数, 错误列表)
    """
    from fnmatch import fnmatch
    
    if not os.path.exists(source_dir):
        print(f"错误: 源目录不存在: {source_dir}")
        return 0, 0, [(source_dir, "源目录不存在")]
    
    if not os.path.exists(dest_dir):
        try:
            os.makedirs(dest_dir, exist_ok=True)
            print(f"已创建目标目录: {dest_dir}")
        except Exception as e:
            print(f"错误: 无法创建目标目录 {dest_dir}: {e}")
            return 0, 0, [(dest_dir, f"无法创建目录: {e}")]
    
    copied_files = 0
    skipped_files = 0
    errors = []
    
    print(f"\n开始从 {source_dir} 复制文件到 {dest_dir}")
    print(f"要复制的文件模式: {FILES_TO_COPY}")
    print(f"排除的目录: {COPY_EXCLUDE_DIRS}")
    print("-" * 60)
    
    for root, dirs, files in os.walk(source_dir):
        # 排除不需要的目录
        dirs[:] = [d for d in dirs if d not in COPY_EXCLUDE_DIRS]
        
        for file in files:
            # 检查文件是否匹配要复制的模式
            should_copy = False
            for pattern in FILES_TO_COPY:
                if fnmatch(file, pattern):
                    should_copy = True
                    break
            
            if not should_copy:
                if verbose:
                    print(f"跳过 (不匹配模式): {os.path.join(root, file)}")
                skipped_files += 1
                continue
            
            # 获取完整路径
            source_path = os.path.join(root, file)
            
            # 计算相对路径
            rel_path = os.path.relpath(source_path, source_dir)
            dest_path = os.path.join(dest_dir, rel_path)
            
            # 创建目标目录（如果不存在）
            dest_dir_path = os.path.dirname(dest_path)
            os.makedirs(dest_dir_path, exist_ok=True)
            
            try:
                # 复制文件
                shutil.copy2(source_path, dest_path)
                print(f"复制: {rel_path}")
                copied_files += 1
            except Exception as e:
                error_msg = f"复制失败: {e}"
                errors.append((source_path, error_msg))
                print(f"复制失败: {rel_path}")
                print(f"  错误: {error_msg}")
    
    return copied_files, skipped_files, errors

def auto_generate_rules():
    """
    自动将简单模式转换为完整规则
    
    Returns:
        list: 完整的规则列表
    """
    rules = []
    for i, pattern in enumerate(DELETE_PATTERNS):
        rule = {
            "name": f"删除规则{i+1}",
            "type": "regex",  # 默认都使用正则表达式
            "description": f"删除匹配模式 '{pattern}' 的行",
            "content": pattern,
            "multiline": False
        }
        rules.append(rule)
    return rules

def apply_rule_to_content(content, rule):
    """
    应用单个规则到内容
    
    Args:
        content: 原始内容
        rule: 规则字典
    
    Returns:
        str: 处理后的内容
    """
    try:
        if rule["type"] == "text":
            # 普通文本替换
            replacement = rule.get("replacement", "")
            return content.replace(rule["content"], replacement)
        elif rule["type"] == "regex":
            # 正则表达式替换
            replacement = rule.get("replacement", "")
            flags = 0
            if rule.get("multiline", False):
                flags |= re.MULTILINE
            if rule.get("ignorecase", False):
                flags |= re.IGNORECASE
                
            # 使用re.sub进行替换
            result = re.sub(rule["content"], replacement, content, flags=flags)
            return result
        else:
            print(f"警告: 未知规则类型: {rule['type']}")
            return content
    except Exception as e:
        print(f"应用规则 '{rule['name']}' 时出错: {e}")
        return content

def process_file_with_rules(file_path, rules, encoding='utf-8', backup=False):
    """
    使用规则列表处理单个文件
    
    Args:
        file_path: 文件路径
        rules: 规则列表
        encoding: 文件编码
        backup: 是否创建备份
    
    Returns:
        tuple: (是否修改, 错误信息, 应用的规则数)
    """
    try:
        # 检查文件大小，避免处理过大文件
        file_size = os.path.getsize(file_path)
        if file_size > 10 * 1024 * 1024:  # 10MB
            return False, f"文件过大 ({file_size} bytes)，已跳过", 0
        
        # 读取文件内容
        with open(file_path, 'r', encoding=encoding) as f:
            lines = f.readlines()
        
        # 备份文件（如果需要）
        if backup and lines:
            backup_path = file_path + '.bak'
            with open(backup_path, 'w', encoding=encoding) as f:
                f.writelines(lines)
        
        # 应用所有规则
        original_line_count = len(lines)
        new_lines = []
        applied_rules_count = 0
        
        for line in lines:
            line_modified = False
            for rule in rules:
                # 检查当前行是否匹配规则
                if rule["type"] == "text":
                    if rule["content"] in line:
                        line_modified = True
                        applied_rules_count += 1
                        break
                elif rule["type"] == "regex":
                    if re.search(rule["content"], line, flags=re.MULTILINE if rule.get("multiline", False) else 0):
                        line_modified = True
                        applied_rules_count += 1
                        break
            
            # 如果行没有被任何规则修改，保留它
            if not line_modified:
                new_lines.append(line)
        
        # 如果内容有变化，则写入文件
        if len(new_lines) != original_line_count:
            with open(file_path, 'w', encoding=encoding) as f:
                f.writelines(new_lines)
            return True, None, applied_rules_count
        else:
            return False, None, applied_rules_count
            
    except UnicodeDecodeError:
        # 如果UTF-8解码失败，尝试其他编码
        try:
            with open(file_path, 'r', encoding='latin-1') as f:
                lines = f.readlines()
            
            if backup and lines:
                backup_path = file_path + '.bak'
                with open(backup_path, 'w', encoding='latin-1') as f:
                    f.writelines(lines)
            
            # 应用所有规则
            original_line_count = len(lines)
            new_lines = []
            applied_rules_count = 0
            
            for line in lines:
                line_modified = False
                for rule in rules:
                    # 检查当前行是否匹配规则
                    if rule["type"] == "text":
                        if rule["content"] in line:
                            line_modified = True
                            applied_rules_count += 1
                            break
                    elif rule["type"] == "regex":
                        if re.search(rule["content"], line, flags=re.MULTILINE if rule.get("multiline", False) else 0):
                            line_modified = True
                            applied_rules_count += 1
                            break
                
                # 如果行没有被任何规则修改，保留它
                if not line_modified:
                    new_lines.append(line)
            
            # 如果内容有变化，则写入文件
            if len(new_lines) != original_line_count:
                with open(file_path, 'w', encoding='latin-1') as f:
                    f.writelines(new_lines)
                return True, None, applied_rules_count
            else:
                return False, None, applied_rules_count
                
        except Exception as e:
            return False, f"编码错误: {e}", 0
            
    except Exception as e:
        return False, f"处理错误: {e}", 0

def should_delete_file(filename):
    """
    检查文件是否应该被删除（支持通配符）
    
    Args:
        filename: 文件名
    
    Returns:
        bool: 是否应该删除
    """
    from fnmatch import fnmatch
    
    for pattern in FILES_TO_DELETE:
        if fnmatch(filename, pattern):
            return True
    return False

def should_skip_path(file_path):
    """
    检查是否应该跳过指定路径
    
    Args:
        file_path: 文件路径
    
    Returns:
        bool: 是否应该跳过
    """
    for excluded_path in EXCLUDED_PATHS:
        # 标准化路径以处理不同的路径分隔符
        normalized_file_path = os.path.normpath(file_path)
        normalized_excluded_path = os.path.normpath(excluded_path)
        
        # 检查文件路径是否以排除路径开头
        if normalized_file_path.startswith(normalized_excluded_path):
            return True
    
    return False

def delete_specific_files(root_dir, verbose=False):
    """
    删除特定的文件
    
    Args:
        root_dir: 根目录
        verbose: 是否显示详细信息
    
    Returns:
        tuple: (删除的文件数, 跳过的文件数, 错误列表)
    """
    deleted_files = 0
    skipped_files = 0
    errors = []
    
    print("开始删除特定文件...")
    print(f"要删除的文件: {FILES_TO_DELETE}")
    print(f"排除的路径: {EXCLUDED_PATHS}")
    print()
    
    for root, dirs, files in os.walk(root_dir):
        # 排除不需要的目录
        dirs[:] = [d for d in dirs if d not in ['.git', '.svn', '.hg', '__pycache__', 'node_modules']]

        for file in files:
            if should_delete_file(file):
                file_path = os.path.join(root, file)
                
                # 检查是否应该跳过此路径
                if should_skip_path(file_path):
                    if verbose:
                        print(f"跳过文件 (在排除路径中): {file_path}")
                    skipped_files += 1
                    continue
                
                try:
                    # 删除文件
                    os.remove(file_path)
                    print(f"删除文件: {file_path}")
                    deleted_files += 1
                except Exception as e:
                    error_msg = f"删除失败: {e}"
                    errors.append((file_path, error_msg))
                    print(f"删除文件: {file_path}")
                    print(f"  错误: {error_msg}")
    
    return deleted_files, skipped_files, errors

def list_rules():
    """列出所有可用的规则"""
    rules = auto_generate_rules()
    print("可用的删除规则:")
    print("-" * 60)
    for i, rule in enumerate(rules, 1):
        print(f"{i}. {rule['name']}")
        print(f"   类型: {rule['type']}")
        print(f"   描述: {rule['description']}")
        print(f"   模式: {rule['content']}")
        print()

def get_selected_rules(rule_names):
    """
    根据规则名称获取选中的规则
    
    Args:
        rule_names: 规则名称列表
    
    Returns:
        list: 选中的规则列表
    """
    all_rules = auto_generate_rules()
    
    if not rule_names:
        # 返回所有规则
        return all_rules
    
    selected_rules = []
    for name in rule_names:
        found = False
        for rule in all_rules:
            if rule['name'] == name:
                selected_rules.append(rule)
                found = True
                break
        if not found:
            print(f"警告: 未找到规则 '{name}'")
    
    return selected_rules

def process_directory(root_dir, rules, file_extensions=None, 
                     exclude_dirs=None, encoding='utf-8', 
                     backup=False, verbose=False):
    """
    处理目录中的所有文件
    
    Args:
        root_dir: 根目录
        rules: 要应用的规则列表
        file_extensions: 要处理的文件扩展名列表
        exclude_dirs: 要排除的目录列表
        encoding: 文件编码
        backup: 是否创建备份
        verbose: 是否显示详细信息
    """
    if exclude_dirs is None:
        exclude_dirs = ['.git', '.svn', '.hg', '__pycache__', 'node_modules']
    
    processed_files = 0
    modified_files = 0
    skipped_files = 0
    total_rules_applied = 0
    errors = []
    
    print(f"开始在目录中搜索: {root_dir}")
    print(f"应用 {len(rules)} 个规则:")
    for rule in rules:
        print(f"  - {rule['name']}: {rule['description']}")
    print()
    
    for root, dirs, files in os.walk(root_dir):
        # 排除不需要的目录
        dirs[:] = [d for d in dirs if d not in exclude_dirs]
        
        for file in files:
            file_path = os.path.join(root, file)
            
            # 检查是否应该跳过此路径
            if should_skip_path(file_path):
                if verbose:
                    print(f"跳过文件 (在排除路径中): {file_path}")
                skipped_files += 1
                continue
            
            # 检查文件扩展名
            if file_extensions:
                file_ext = Path(file).suffix.lower()
                if file_ext not in file_extensions:
                    if verbose:
                        print(f"跳过文件 (扩展名不匹配): {file_path}")
                    continue
            
            # 处理文件
            processed_files += 1
            
            modified, error, rules_applied = process_file_with_rules(
                file_path, rules, encoding, backup
            )
            
            if error:
                errors.append((file_path, error))
                print(f"处理文件: {file_path}")
                print(f"  错误: {error}")
            elif modified:
                modified_files += 1
                total_rules_applied += rules_applied
                print(f"处理文件: {file_path}")
                print(f"  已修改 (应用了 {rules_applied} 个规则)")
    
    # 输出统计信息
    print("\n" + "="*60)
    print("内容处理完成!")
    print(f"总处理文件数: {processed_files}")
    print(f"成功修改文件数: {modified_files}")
    print(f"跳过文件数: {skipped_files}")
    print(f"总规则应用次数: {total_rules_applied}")
    
    if errors:
        print(f"出错文件数: {len(errors)}")
        print("\n错误详情:")
        for file_path, error in errors:
            print(f"  {file_path}: {error}")
    else:
        print("所有文件处理成功!")
    
    return processed_files, modified_files, total_rules_applied, errors

def validate_rules():
    """验证规则格式是否正确"""
    if not DELETE_PATTERNS:
        print("错误: 没有定义任何删除模式")
        return False
    
    # 检查每个模式是否有效
    for i, pattern in enumerate(DELETE_PATTERNS):
        if not pattern.strip():
            print(f"警告: 规则 {i+1} 为空")
            continue
        try:
            # 测试正则表达式是否有效
            re.compile(pattern)
        except re.error as e:
            print(f"错误: 规则 {i+1} 的正则表达式无效: {e}")
            return False
    
    return True

def main():
    parser = argparse.ArgumentParser(description='删除文件夹中所有文件的特定内容（内置规则）')
    parser.add_argument('-d', '--directory', default='.', 
                       help='要处理的目录 (默认: 当前目录)')
    parser.add_argument('-s', '--source', 
                       help='源目录（从中复制文件）')
    parser.add_argument('-t', '--target', 
                       help='目标目录（复制文件到此处）')
    parser.add_argument('-e', '--extensions', nargs='+', 
                       help='要处理的文件扩展名 (例如: .txt .py .js)')
    parser.add_argument('-r', '--rules', nargs='+', 
                       help='指定要应用的规则名称 (默认: 所有规则)')
    parser.add_argument('-x', '--exclude-dirs', nargs='+', 
                       default=['.git', '.svn', '.hg', '__pycache__', 'node_modules'],
                       help='要排除的目录')
    parser.add_argument('--encoding', default='utf-8', 
                       help='文件编码 (默认: utf-8)')
    parser.add_argument('--backup', action='store_true', 
                       help='在处理前创建备份文件 (.bak)')
    parser.add_argument('-v', '--verbose', action='store_true', 
                       help='显示详细信息')
    parser.add_argument('--list-rules', action='store_true', 
                       help='列出所有可用的规则并退出')
    parser.add_argument('--mode', choices=['content', 'files', 'both', 'copy'], default='both',
                       help='执行模式: content=只删除内容, files=只删除文件, both=两者都执行, copy=只复制文件 (默认: both)')
    parser.add_argument('--no-copy', action='store_true', 
                       help='不执行文件复制，即使指定了源目录和目标目录')
    
    args = parser.parse_args()
    
    # 验证规则
    if not validate_rules():
        print("脚本初始化失败，请检查规则定义")
        sys.exit(1)
    
    # 列出规则并退出
    if args.list_rules:
        list_rules()
        return
    
    # 验证目录是否存在
    if not os.path.exists(args.directory):
        print(f"错误: 目录 '{args.directory}' 不存在")
        sys.exit(1)
    
    # 检查是否需要执行复制操作
    if args.mode == 'copy' or (args.source and args.target and not args.no_copy):
        if not args.source or not args.target:
            print("错误: 复制模式需要指定源目录(--source)和目标目录(--target)")
            sys.exit(1)
        
        # 执行文件复制
        copied_files, skipped_files, copy_errors = copy_files_before_delete(
            args.source, args.target, args.verbose
        )
        
        print("\n" + "="*60)
        print("文件复制完成!")
        print(f"复制文件数: {copied_files}")
        print(f"跳过文件数: {skipped_files}")
        
        if copy_errors:
            print(f"复制失败文件数: {len(copy_errors)}")
            print("\n复制失败详情:")
            for file_path, error in copy_errors:
                print(f"  {file_path}: {error}")
        
        # 如果是只复制模式，则直接退出
        if args.mode == 'copy':
            print("复制模式完成，退出程序")
            return
    
    # 获取选中的规则
    selected_rules = get_selected_rules(args.rules)
    
    # 处理文件扩展名
    file_extensions = None
    if args.extensions:
        file_extensions = [ext.lower() if ext.startswith('.') else f'.{ext.lower()}' 
                          for ext in args.extensions]
    
    try:
        # 根据模式决定执行内容
        if args.mode in ['files', 'both']:
            # 执行文件删除
            deleted_files, skipped_files, delete_errors = delete_specific_files(
                args.directory, args.verbose
            )
            print("\n" + "="*60)
            print("文件删除完成!")
            print(f"删除文件数: {deleted_files}")
            print(f"跳过文件数: {skipped_files}")
            
            if delete_errors:
                print(f"删除失败文件数: {len(delete_errors)}")
                print("\n删除失败详情:")
                for file_path, error in delete_errors:
                    print(f"  {file_path}: {error}")
        else:
            print("跳过文件删除 (mode: content)")
        
        # 处理文件内容
        if args.mode in ['content', 'both']:
            if selected_rules:
                process_directory(
                    root_dir=args.directory,
                    rules=selected_rules,
                    file_extensions=file_extensions,
                    exclude_dirs=args.exclude_dirs,
                    encoding=args.encoding,
                    backup=args.backup,
                    verbose=args.verbose
                )
            else:
                print("没有找到可用的内容删除规则")
        else:
            print("跳过内容删除 (mode: files)")
            
    except KeyboardInterrupt:
        print("\n用户中断操作")
    except Exception as e:
        print(f"发生错误: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()