#!/usr/bin/env python3
"""
Unix转DOS格式脚本
递归转换指定目录下的所有文本文件
"""

import os
import sys
import argparse
import logging
from datetime import datetime
from pathlib import Path

def setup_logging(verbose=False):
    """设置日志配置"""
    log_format = '%(asctime)s - %(levelname)s - %(message)s'
    level = logging.DEBUG if verbose else logging.INFO
    
    handlers = [logging.StreamHandler(sys.stdout)]
    
    logging.basicConfig(
        level=level,
        format=log_format,
        handlers=handlers
    )

def is_unix_line_endings(file_path):
    """
    检查文件是否包含Unix换行符(\n)
    返回: True(有Unix换行符), False(纯DOS格式), None(无法确定/二进制文件)
    """
    try:
        with open(file_path, 'rb') as f:
            content = f.read()
            
        # 检查文件是否为空
        if len(content) == 0:
            return False
            
        # 检查是否有Unix换行符
        has_unix_nl = b'\n' in content
        
        # 检查是否有DOS换行符
        has_dos_nl = b'\r\n' in content
        
        # 如果没有换行符，可能是二进制文件或单行文件
        if not has_unix_nl and not has_dos_nl:
            return False
            
        # 如果有Unix换行符但没有DOS换行符，需要转换
        if has_unix_nl and not has_dos_nl:
            return True
            
        # 如果两者都有，检查是否有单独的Unix换行符
        if has_unix_nl and has_dos_nl:
            # 检查是否有不是\r\n的\n
            # 将\r\n替换为特殊标记，然后检查是否有\n
            temp_content = content.replace(b'\r\n', b'##TEMP##')
            if b'\n' in temp_content:
                return True  # 有单独的\n，需要转换
        
        return False  # 已经是纯DOS格式
    except Exception as e:
        logging.warning(f"无法检查文件 {file_path}: {e}")
        return None

def convert_unix_to_dos(file_path):
    """将Unix格式文件转换为DOS格式"""
    try:
        # 读取文件内容（二进制模式）
        with open(file_path, 'rb') as f:
            content = f.read()
        
        # 转换逻辑
        # 先将所有\r\n转换为\n，确保格式统一
        content = content.replace(b'\r\n', b'\n')
        # 然后将\n转换为\r\n
        content = content.replace(b'\n', b'\r\n')
        
        # 写回文件
        with open(file_path, 'wb') as f:
            f.write(content)
            
        return True
    except Exception as e:
        logging.error(f"转换失败 {file_path}: {e}")
        return False

def find_text_files(root_dir, extensions=None):
    """递归查找指定扩展名的文本文件"""
    if extensions is None:
        extensions = {'.txt', '.py', '.java', '.cpp', '.c', '.h', '.html', 
                     '.css', '.js', '.json', '.xml', '.md', '.yml', '.yaml', 
                     '.properties', '.ini', '.cfg', '.conf', '.csv', '.sql',
                     '.php', '.rb', '.go', '.rs', '.ts'}
    
    root_path = Path(root_dir)
    if not root_path.exists():
        raise FileNotFoundError(f"目录不存在: {root_dir}")
    
    text_files = []
    for ext in extensions:
        for file_path in root_path.rglob(f'*{ext}'):
            if file_path.is_file():
                text_files.append(file_path)
    
    # 也查找没有扩展名的文件
    for file_path in root_path.rglob('*'):
        if file_path.is_file() and file_path.suffix == '':
            text_files.append(file_path)
    
    return text_files

def main():
    parser = argparse.ArgumentParser(
        description='递归将Unix格式文件转换为DOS格式'
    )
    parser.add_argument(
        'directory',
        help='要处理的根目录'
    )
    parser.add_argument(
        '-e', '--extensions',
        nargs='+',
        help='要处理的文件扩展名列表（空格分隔）',
        default=None
    )
    
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='显示详细日志'
    )
    parser.add_argument(
        '-d', '--dry-run',
        action='store_true',
        help='模拟运行，不实际修改文件'
    )
    
    args = parser.parse_args()
    
    # 设置日志
    setup_logging(args.verbose)
    
    logging.info("=" * 60)
    logging.info("Unix转DOS格式脚本开始运行")
    logging.info(f"处理目录: {args.directory}")
    logging.info(f"模拟模式: {args.dry_run}")
    logging.info("=" * 60)
    
    try:
        # 查找文件
        logging.info("正在查找文件...")
        extensions = set(args.extensions) if args.extensions else None
        all_files = find_text_files(args.directory, extensions)
        
        if not all_files:
            logging.warning("未找到任何文本文件")
            return
        
        logging.info(f"找到 {len(all_files)} 个文件")
        
        # 处理文件
        converted_count = 0
        skipped_count = 0
        failed_count = 0
        already_dos_count = 0
        total_processed = 0
        
        for file_path in all_files:
            total_processed += 1
            
            try:
                # 检查文件格式
                logging.debug(f"检查文件: {file_path}")
                needs_conversion = is_unix_line_endings(file_path)
                
                if needs_conversion is None:
                    # 无法确定格式，跳过
                    logging.info(f"跳过（无法确定格式）: {file_path}")
                    skipped_count += 1
                    continue
                
                if not needs_conversion:
                    # 已经是DOS格式
                    logging.debug(f"已跳过（已是DOS格式）: {file_path}")
                    already_dos_count += 1
                    continue
                
                # 需要转换
                if args.dry_run:
                    logging.info(f"[模拟] 将转换: {file_path}")
                    converted_count += 1
                else:
                    logging.info(f"转换中: {file_path}")
                    if convert_unix_to_dos(file_path):
                        converted_count += 1
                    else:
                        failed_count += 1
                        
            except Exception as e:
                logging.error(f"处理失败 {file_path}: {e}")
                failed_count += 1
            
            # 进度显示
            if total_processed % 100 == 0:
                logging.info(f"处理进度: {total_processed}/{len(all_files)}")
        
        # 输出统计
        logging.info("=" * 60)
        logging.info("处理完成！")
        logging.info("=" * 60)
        logging.info(f"总共查找文件数: {len(all_files)}")
        logging.info(f"已处理文件数: {total_processed}")
        logging.info(f"成功转换文件数: {converted_count}")
        logging.info(f"已是DOS格式文件数: {already_dos_count}")
        logging.info(f"跳过文件数: {skipped_count}")
        logging.info(f"失败文件数: {failed_count}")
        
        if args.dry_run:
            logging.info("注意：本次为模拟运行，未实际修改文件")
        
    except Exception as e:
        logging.error(f"脚本执行出错: {e}", exc_info=True)
        sys.exit(1)

if __name__ == '__main__':
    main()