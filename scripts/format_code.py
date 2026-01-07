#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import subprocess
import argparse
import re
from pathlib import Path

def find_source_files(directory, extensions=None):
    """
    Recursively find all source code files in the specified directory
    
    Args:
        directory: Directory path to search
        extensions: List of file extensions, defaults to common C/C++ file extensions
    
    Returns:
        List of source code file paths
    """
    if extensions is None:
        extensions = ['.c', '.cpp', '.cxx', '.cc', '.h', '.hpp', '.hxx', '.hh']
    
    source_files = []
    directory = Path(directory)
    
    for ext in extensions:
        # Recursively find all matching files
        pattern = f"**/*{ext}"
        files = directory.glob(pattern)
        source_files.extend(files)
    
    return [str(f) for f in source_files]

def move_trailing_comments(file_path):
    """
    Move trailing comments from variable definitions to the line above
    
    Args:
        file_path: File path to process
    
    Returns:
        success: bool - Whether processing was successful
    """
    try:
        # Try multiple encoding methods
        encodings = ['utf-8', 'gbk', 'gb2312', 'latin1']
        lines = None
        
        for encoding in encodings:
            try:
                with open(file_path, 'r', encoding=encoding) as f:
                    lines = f.readlines()
                break
            except UnicodeDecodeError:
                continue
            except Exception:
                continue
        
        if lines is None:
            print(f"  Unable to read file {file_path}: failed with multiple encodings")
            return False
        
        modified_lines = []
        changes_made = False
        
        modified_lines = []
        changes_made = False
        
        for line in lines:
            stripped_line = line.rstrip()
            
            # Find trailing // comments
            comment_pos = stripped_line.find('//')
            if comment_pos > 0:  # Ensure it's not a leading comment
                before_comment = stripped_line[:comment_pos].rstrip()
                after_comment = stripped_line[comment_pos + 2:].strip()
                
                # Check if it's #define, variable definition, or control statement line
                is_define = re.match(r'^\s*#\s*define\s+\w+', before_comment)
                # Match any line with assignment (including pointer assignments)
                is_assignment = re.match(r'^\s*[\w\*\s\[\].*&]+.*[=]', before_comment)
                # Match control statements: if, while, for, switch, else, do, return, goto, break, continue
                is_control = re.match(r'^\s*(if|while|for|switch|else|do|return|goto|break|continue|case|default)\b', before_comment)
                # Match function calls or other statements with parentheses
                is_function_call = re.match(r'^\s*\w+.*\(', before_comment)
                # Match variable declarations (very flexible pattern)
                is_declaration = re.match(r'^\s*(?:[a-zA-Z_][a-zA-Z0-9_]*\s+)+[\w\*\s\[\].*&\s,]+', before_comment)
                
                if is_define or is_assignment or is_control or is_function_call or is_declaration:
                    # Extract indentation (keep indentation for #define)
                    if is_define:
                        # For #define, keep original indentation
                        indent_match = re.match(r'^(\s*)', line)
                        indent = len(indent_match.group(1)) if indent_match else 0
                    else:
                        # For variable definition, use variable definition indentation
                        indent_match = re.match(r'^(\s*)', line)
                        indent = len(indent_match.group(1)) if indent_match else 0
                    
                    # Add comment line before the code line
                    comment_line = ' ' * indent + f'// {after_comment}\n'
                    modified_lines.append(comment_line)
                    
                    # Add code line (without comment part)
                    code_line = before_comment + '\n'
                    modified_lines.append(code_line)
                    
                    changes_made = True
                    continue
            
            # If no modification needed, keep original line
            modified_lines.append(line)
        
        # If there are modifications, write back to file
        if changes_made:
            # Try to maintain original file encoding
            try:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.writelines(modified_lines)
            except UnicodeEncodeError:
                # If UTF-8 encoding fails, try original encoding
                for encoding in encodings[1:]:  # Skip utf-8, try other encodings
                    try:
                        with open(file_path, 'w', encoding=encoding) as f:
                            f.writelines(modified_lines)
                        break
                    except:
                        continue
            print(f"  Moved trailing comments: {file_path}")
        
        return True
        
    except Exception as e:
        print(f"  Error processing trailing comments for {file_path}: {str(e)}")
        return False

def find_identifiers_in_file(file_path):
    """
    找出文件中的所有函数名和变量名
    
    Args:
        file_path: 文件路径
    
    Returns:
        set: 包含所有函数名和变量名的集合
    """
    try:
        encodings = ['utf-8', 'gbk', 'gb2312', 'latin1']
        content = None
        
        for encoding in encodings:
            try:
                with open(file_path, 'r', encoding=encoding) as f:
                    content = f.read()
                break
            except UnicodeDecodeError:
                continue
            except Exception:
                continue
        
        if content is None:
            return set()
        
        identifiers = set()
        macros = set()
        
        # 匹配函数定义和提取参数（排除for、while、if等控制语句）
        # 先找到所有可能的匹配，然后过滤掉控制语句
        potential_pattern = r'\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)\s*\{'
        potential_matches = re.findall(potential_pattern, content)
        
        control_keywords = {'if', 'for', 'while', 'switch', 'catch', 'try'}
        function_matches = []
        
        for func_name, func_params in potential_matches:
            if func_name not in control_keywords:
                function_matches.append((func_name, func_params))
        
        # 添加函数名和提取参数
        for func_name, func_params in function_matches:
            identifiers.add(func_name)  # 函数名
            
            # 提取函数参数中的标识符
            # 匹配参数中的变量名（包括类型和变量名）
            param_pattern = r'(?:const\s+)?(?:unsigned\s+|signed\s+)?(?:[a-zA-Z_][a-zA-Z0-9_]*\s+)*\*?\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*(?:\[[^\]]*\])?\s*(?:,|$)'
            param_matches = re.findall(param_pattern, func_params)
            identifiers.update(param_matches)
        
        # 匹配宏定义 #define
        macro_pattern = r'^\s*#\s*define\s+([a-zA-Z_][a-zA-Z0-9_]*)\b'
        macro_matches = re.findall(macro_pattern, content, re.MULTILINE)
        macros.update(macro_matches)
        
        # 匹配条件编译宏 #ifdef/#ifndef/#if defined()
        ifdef_pattern = r'^\s*#\s*(?:ifdef|ifndef|if\s+defined)\s*\(\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\)'
        ifdef_matches = re.findall(ifdef_pattern, content, re.MULTILINE)
        macros.update(ifdef_matches)
        
        # 匹配#undef
        undef_pattern = r'^\s*#\s*undef\s+([a-zA-Z_][a-zA-Z0-9_]*)\b'
        undef_matches = re.findall(undef_pattern, content, re.MULTILINE)
        macros.update(undef_matches)
        
        # 匹配结构体、联合体、枚举类型名称
        struct_type_pattern = r'(?:typedef\s+)?(?:struct|union|enum)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(?:\{|[;\n])'
        struct_type_matches = re.findall(struct_type_pattern, content)
        identifiers.update(struct_type_matches)
        
        # 匹配typedef结构体定义结束后的类型名
        typedef_struct_pattern = r'typedef\s+struct\s*(?:[a-zA-Z_][a-zA-Z0-9_]*)?\s*\{[^}]*\}\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*[;,]'
        typedef_struct_matches = re.findall(typedef_struct_pattern, content, re.MULTILINE | re.DOTALL)
        identifiers.update(typedef_struct_matches)
        
        # 匹配其他typedef定义的新类型名
        typedef_pattern = r'typedef\s+(?:[\w\s\*\[\]]+)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*[;,](?!\s*\{)'
        typedef_matches = re.findall(typedef_pattern, content)
        identifiers.update(typedef_matches)
        
        # 匹配结构体成员变量（在结构体内部）
        # 先提取所有结构体内容（包括无名的typedef结构体）
        struct_content_pattern = r'typedef\s+struct\s*(?:[a-zA-Z_][a-zA-Z0-9_]*)?\s*\{([^}]*)\}\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*[;,]'
        typedef_struct_contents = re.findall(struct_content_pattern, content, re.MULTILINE | re.DOTALL)
        
        # 处理typedef结构体
        for struct_content, type_name in typedef_struct_contents:
            identifiers.add(type_name)  # 添加类型名
            
            # 在结构体内部匹配成员变量
            member_pattern = r'\b(?:int|char|float|double|void|long|short|unsigned|signed|static|extern|const|volatile|struct|union|enum|u8|u16|u32|s8|s16|s32|_dt_frame_st)\s+(?:\*\s*)*(?:\w+\s+)*([a-zA-Z_][a-zA-Z0-9_]*)\s*[;=\[\]]'
            member_matches = re.findall(member_pattern, struct_content)
            identifiers.update(member_matches)
        
        # 同时匹配非typedef的结构体
        struct_content_pattern2 = r'(?:typedef\s+)?struct\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\{([^}]*)\}'
        struct_contents2 = re.findall(struct_content_pattern2, content, re.MULTILINE | re.DOTALL)
        for struct_type_name, struct_content in struct_contents2:
            identifiers.add(struct_type_name)  # 添加结构体类型名
            # 在结构体内部匹配成员变量
            member_pattern = r'\b(?:int|char|float|double|void|long|short|unsigned|signed|static|extern|const|volatile|struct|union|enum|u8|u16|u32|s8|s16|s32|_dt_frame_st)\s+(?:\*\s*)*(?:\w+\s+)*([a-zA-Z_][a-zA-Z0-9_]*)\s*[;=\[\]]'
            member_matches = re.findall(member_pattern, struct_content)
            identifiers.update(member_matches)
            
        # 为了处理更复杂的情况，再尝试一种模式
        struct_content_pattern3 = r'\{([^}]*)\}'  # 找所有大括号内的内容
        all_brace_contents = re.findall(struct_content_pattern3, content, re.MULTILINE | re.DOTALL)
        for struct_content in all_brace_contents:
            # 如果这个内容看起来像是结构体内容（包含分号和变量声明）
            if ';' in struct_content and any(type_name in struct_content for type_name in ['u8', 'u16', 'u32', 'int', 'char', '_dt_frame_st']):
                # 在结构体内部匹配成员变量
                member_pattern = r'\b(?:int|char|float|double|void|long|short|unsigned|signed|static|extern|const|volatile|struct|union|enum|u8|u16|u32|s8|s16|s32|_dt_frame_st)\s+(?:\*\s*)*(?:\w+\s+)*([a-zA-Z_][a-zA-Z0-9_]*)\s*[;=\[\]]'
                member_matches = re.findall(member_pattern, struct_content)
                identifiers.update(member_matches)
                
                # 匹配结构体中的数组
                array_in_struct_pattern = r'\b(?:int|char|float|double|void|long|short|unsigned|signed|static|extern|const|volatile|struct|union|enum|u8|u16|u32|s8|s16|s32|_dt_frame_st)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\['
                array_in_struct_matches = re.findall(array_in_struct_pattern, struct_content)
                identifiers.update(array_in_struct_matches)
            # 在结构体内部匹配成员变量
            member_pattern = r'\b(?:int|char|float|double|void|long|short|unsigned|signed|static|extern|const|volatile|struct|union|enum|u8|u16|u32|s8|s16|s32)\s+(?:\*\s*)*(?:\w+\s+)*([a-zA-Z_][a-zA-Z0-9_]*)\s*[;=\[\]]'
            member_matches = re.findall(member_pattern, struct_content)
            identifiers.update(member_matches)
            
            # 匹配结构体中的数组
            array_in_struct_pattern = r'\b(?:int|char|float|double|void|long|short|unsigned|signed|static|extern|const|volatile|struct|union|enum|u8|u16|u32|s8|s16|s32)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\['
            array_in_struct_matches = re.findall(array_in_struct_pattern, struct_content)
            identifiers.update(array_in_struct_matches)
        
        # 匹配变量声明（增强模式）
        # 匹配更多类型和复杂变量声明
        types_pattern = r'\b(?:int|char|float|double|void|long|short|unsigned|signed|static|extern|const|volatile|struct|union|enum|u8|u16|u32|s8|s16|s32|f32|f64|vec3_f|u8|u16|u32|s8|s16|s32)'
        # 匹配类型声明后的变量名（包括复杂的声明）
        var_pattern = types_pattern + r'\s+(?:\*\s*|const\s+|volatile\s+)*(?:\w+\s+)*([a-zA-Z_][a-zA-Z0-9_]*)\s*(?:\[.*?\]|\[[^\]]*\])?\s*[;=,\[\]]'
        var_matches = re.findall(var_pattern, content)
        identifiers.update(var_matches)
        
        # 额外匹配：结构体成员访问模式（如 st_imu_data.gyrSensitivity）
        member_access_pattern = r'\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\.\s*([a-zA-Z_][a-zA-Z0-9_]*)'
        member_matches = re.findall(member_access_pattern, content)
        for struct_var, member in member_matches:
            identifiers.add(member)  # 添加成员变量名
            identifiers.add(struct_var)  # 添加结构体变量名
        
        # 额外匹配：指针结构体成员访问模式（如 st_imu_data->gyrSensitivity）
        ptr_member_access_pattern = r'\b([a-zA-Z_][a-zA-Z0-9_]*)\s*->\s*([a-zA-Z_][a-zA-Z0-9_]*)'
        ptr_member_matches = re.findall(ptr_member_access_pattern, content)
        for ptr_struct, member in ptr_member_matches:
            identifiers.add(member)  # 添加成员变量名
            identifiers.add(ptr_struct)  # 添加指针变量名
        
        # 匹配指针变量
        ptr_pattern = r'\b(?:int|char|float|double|void|long|short|unsigned|signed|static|extern|const|volatile|struct|union|enum|u8|u16|u32|s8|s16|s32|f32|f64)\s*\*\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*[;=,\[]'
        ptr_matches = re.findall(ptr_pattern, content)
        identifiers.update(ptr_matches)
        
        # 匹配数组变量
        array_pattern = r'\b(?:int|char|float|double|void|long|short|unsigned|signed|static|extern|const|volatile|struct|union|enum|u8|u16|u32|s8|s16|s32|f32|f64)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\['
        array_matches = re.findall(array_pattern, content)
        identifiers.update(array_matches)
        
        # 匹配结构体变量声明
        struct_var_pattern = r'\b(?:struct|union|enum)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s+(?:\w+\s+)*([a-zA-Z_][a-zA-Z0-9_]*)\s*[;=\[\]]'
        struct_var_matches = re.findall(struct_var_pattern, content)
        for match in struct_var_matches:
            # match是一个元组，包含两个元素：结构体类型名和变量名
            if len(match) == 2:
                identifiers.add(match[0])  # 结构体类型名
                identifiers.add(match[1])  # 变量名
            else:
                identifiers.update(match)  # 兼容其他情况
        
        # 过滤掉常见的关键字和已符合Linux风格的标识符
        keywords = {
            'if', 'else', 'for', 'while', 'do', 'switch', 'case', 'default', 'break', 'continue',
            'return', 'goto', 'sizeof', 'typeof', 'alignof', 'offsetof', 'main', 'printf', 'scanf',
            'malloc', 'free', 'memcpy', 'memset', 'strlen', 'strcpy', 'strcmp', 'strcat',
            'NULL', 'true', 'false', 'bool', 'int', 'char', 'float', 'double', 'void',
            'long', 'short', 'unsigned', 'signed', 'static', 'extern', 'const', 'volatile',
            'struct', 'union', 'enum', 'typedef', 'auto', 'register', 'inline', 'restrict',
            # 常见的嵌入式系统类型缩写
            'u8', 'u16', 'u32', 'u64', 's8', 's16', 's32', 's64', 'f32', 'f64',
            # 常见的标准类型
            'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t', 'int8_t', 'int16_t', 'int32_t', 'int64_t',
            'size_t', 'time_t', 'pid_t', 'ssize_t', 'off_t', 'FILE'
        }
        
        # 过滤掉常见宏关键字
        macro_keywords = {
            'NULL', 'TRUE', 'FALSE', 'EOF', 'BUFSIZ', 'FILENAME_MAX', 'FOPEN_MAX', 'L_tmpnam',
            'TMP_MAX', 'EXIT_SUCCESS', 'EXIT_FAILURE', 'RAND_MAX', 'CLOCKS_PER_SEC'
        }
        
        # 过滤掉单个字符的标识符（通常是临时变量）
        # 过滤掉已经符合Linux小写+下划线风格且没有改进空间的标识符
        filtered_identifiers = set()
        for identifier in identifiers:
            # 检查是否真的是Linux风格（纯小写+下划线）
            is_linux_style = re.match(r'^[a-z]+(_[a-z0-9]+)*$', identifier)
            # 检查是否包含大写字母（驼峰命名的特征）
            has_upper = any(c.isupper() for c in identifier)
            
            # 特殊处理：如果已经是Linux风格但可能仍有改进空间（如太长、包含技术缩写等），也保留
            # 这里我们放宽限制，让更多标识符被处理
            needs_improvement = (
                not is_linux_style or  # 不是Linux风格
                has_upper or  # 包含大写字母
                '_' not in identifier or  # 没有下划线但可能太长
                len(identifier) > 15  # 太长，可能包含未分割的复合词
            )
            
            if (identifier not in keywords and 
                len(identifier) > 1 and 
                (needs_improvement or not is_linux_style) and  # 需要改进或不是Linux风格
                not identifier.isupper()):  # 排除全大写的常量
                filtered_identifiers.add(identifier)
        
        # 过滤宏名，排除已符合大写+下划线风格的宏和宏关键字
        filtered_macros = set()
        for macro in macros:
            if (macro not in macro_keywords and 
                len(macro) > 1 and 
                not re.match(r'^[A-Z]+(_[A-Z0-9]+)*$', macro)):
                filtered_macros.add(macro)
        
        return filtered_identifiers, filtered_macros
        
    except Exception as e:
        print(f"  Error finding identifiers in {file_path}: {str(e)}")
        return set()

def to_linux_style(name):
    """
    将标识符转换为Linux底线风格
    
    Args:
        name: 原始标识符
    
    Returns:
        str: Linux底线风格的标识符
    """
    if name.isupper():
        # 全大写的常量保持不变
        return name
    
    # 第一步：智能分割驼峰命名，保留缩写词的完整性
    name_with_underscores = split_camel_case(name)
    
    # 第二步：处理复合词模式（如 addata -> add_data）
    name_with_underscores = split_compound_words(name_with_underscores)
    
    # 第三步：最后才做拼写和缩写纠正，只在完全匹配的单词上进行
    name_with_underscores = correct_spelling_and_abbreviations(name_with_underscores)
    
    # 第四步：统一转换为小写
    linux_name = name_with_underscores.lower()
    
    # 清理多余的下划线（连续多个下划线合并为一个）
    linux_name = re.sub(r'_+', '_', linux_name)
    
    # 如果第一个字符是下划线，去掉它
    if linux_name.startswith('_'):
        linux_name = linux_name[1:]
    
    return linux_name

def split_camel_case(name):
    """
    智能分割驼峰命名，识别常见的缩写和词汇边界
    
    Args:
        name: 原始标识符
    
    Returns:
        str: 插入下划线后的标识符
    """
    # 如果已经有下划线，需要处理每个部分，但不能简单地合并
    if '_' in name:
        parts = name.split('_')
        processed_parts = []
        for part in parts:
            if part:  # 跳过空字符串
                processed_part = _split_single_camel_word(part)
                processed_parts.append(processed_part)
        # 用下划线连接所有处理过的部分
        return '_'.join(processed_parts)
    else:
        # 没有下划线，直接处理
        return _split_single_camel_word(name)

def _split_single_camel_word(word):
    """
    分割单个驼峰单词
    
    Args:
        word: 单个单词
    
    Returns:
        str: 分割后的单词
    """
    if not any(c.isupper() for c in word):
        # 全是小写，不需要处理
        return word
    
    # 常见的完整缩写词汇（按长度排序，长的优先匹配）
    complete_abbreviations = [
        'USBHID', 'UART', 'GPIO', 'SPI', 'I2C', 'ADC', 'DAC', 'DMA', 'PWM', 'RTC', 'LED', 'LCD',
        'CPU', 'GPU', 'RAM', 'ROM', 'USB', 'HID', 'IRQ', 'ISR', 'CRC', 'MD5', 'SHA', 'AES', 'DES',
        'RSA', 'TLS', 'SSL', 'HTTP', 'HTTPS', 'FTP', 'SSH', 'DNS', 'DHCP', 'TCP', 'UDP',
        'MAC', 'VLAN', 'VPN', 'NAT', 'SNMP', 'SMTP', 'POP3', 'IMAP', 'MIME', 'URL', 'URI',
        'JSON', 'XML', 'HTML', 'CSS', 'SQL', 'API', 'SDK', 'GUI', 'CLI', 'IDE', 'PID', 'FFT',
        'AV', 'RGB', 'HSV', 'YUV', 'GPS', 'WIFI', 'JS', 'PHP', 'ASP', 'JSP', 'CGI', 'OOP',
        'OO', 'RDBMS', 'DBMS', 'ALU', 'FPU', 'EEPROM',
        # 飞控和光学相关技术缩写
        'ANO', 'OF', 'UP', 'Fusion', 'Deco', 'Flow', 'IMU', 'ACC', 'GYR', 'MAG',
        'Drift', 'Calib', 'Bias', 'Scale', 'Sens', 'Offset', 'Data', 'Ctrl', 'Task',
        # 常见完整单词
        'CALIBRATE', 'CALIBRATION', 'CONFIGURATION', 'INITIALIZATION', 'TRANSMISSION',
        'RECEIVER', 'TRANSMITTER', 'PERIPHERAL', 'INTERRUPT', 'CONTROLLER', 'PROCESSING'
    ]
    
    # 常见的技术词汇缩写（注意：这些不是完整单词，需要进一步处理）
    tech_abbreviations = [
        'Gyr', 'Av', 'Acc', 'Mag', 'Temp', 'Hum', 'Pres', 'Alt', 'Dist', 
        'Vel', 'Pos', 'Rot', 'Ang', 'Freq', 'Phase', 'Gain', 'Offset',
        'Bias', 'Noise', 'Sig', 'Data', 'Info', 'Msg', 'Cmd', 'Req',
        'Resp', 'Err', 'Stat', 'Val', 'Idx', 'Cnt', 'Num', 'Len', 'Sz',
        'Cfg', 'Param', 'Ctrl', 'Reg', 'Buf', 'Ptr', 'Ref', 'Src', 'Dst',
        'Init', 'Deinit', 'Reset', 'Start', 'Stop', 'Pause', 'Resume',
        'Enable', 'Disable', 'Read', 'Write', 'Send', 'Recv', 'Get', 'Set',
        'Calc', 'Comp', 'Conv', 'Filt', 'Proc', 'Handle', 'Update', 'Sync',
        'Async', 'Lock', 'Unlock', 'Wait', 'Signal', 'Notify', 'Trigger',
        'Flight'
    ]
    
    result = []
    i = 0
    n = len(word)
    
    while i < n:
        # 首先尝试匹配完整缩写
        found_match = False
        
        # 检查完整缩写
        for abbrev in complete_abbreviations:
            if word.startswith(abbrev, i):
                # 匹配到完整缩写
                if i > 0 and result and result[-1] != '_':
                    result.append('_')
                result.append(abbrev)
                i += len(abbrev)
                found_match = True
                break
        
        if found_match:
            continue
        
        # 检查技术缩写
        for abbrev in tech_abbreviations:
            if word.startswith(abbrev, i):
                # 匹配到技术缩写
                if i > 0 and result and result[-1] != '_':
                    result.append('_')
                result.append(abbrev)
                i += len(abbrev)
                found_match = True
                break
        
        if found_match:
            continue
        
        # 没有匹配到缩写，逐字符处理
        c = word[i]
        
        if c.isupper():
            if i > 0:
                # 添加下划线分隔
                if result and result[-1] != '_':
                    result.append('_')
            result.append(c.lower())
        else:
            result.append(c)
        
        i += 1
    
    # 组合结果
    name_with_underscores = ''.join(result)
    
    # 清理多余的下划线
    name_with_underscores = re.sub(r'_+', '_', name_with_underscores)
    
    # 去掉开头和结尾的下划线
    name_with_underscores = name_with_underscores.strip('_')
    
    return name_with_underscores

def split_compound_words(name):
    """
    分割复合词，识别常见的词汇边界
    
    Args:
        name: 原始标识符
    
    Returns:
        str: 插入下划线后的标识符
    """
    # 先转换为小写以便匹配
    lower_name = name.lower()
    result = lower_name
    
    lower_name = name.lower()
    
    # 常见的复合词替换规则（直接匹配和替换）
    replacements = [
        # 数据相关
        ('adddata', 'add_data'),
        ('datainfo', 'data_info'),
        ('datavalid', 'data_valid'),
        ('datasize', 'data_size'),
        ('datacount', 'data_count'),
        ('databuffer', 'data_buffer'),
        ('datanext', 'data_next'),
        ('dataprev', 'data_prev'),
        ('datahead', 'data_head'),
        ('datatail', 'data_tail'),
        
        # 配置相关
        ('configparam', 'config_param'),
        ('configvalue', 'config_value'),
        ('configsize', 'config_size'),
        ('configmode', 'config_mode'),
        ('configstate', 'config_state'),
        ('configflag', 'config_flag'),
        
        # 计数器相关
        ('counterreset', 'counter_reset'),
        ('countervalue', 'counter_value'),
        ('countermax', 'counter_max'),
        ('countermin', 'counter_min'),
        ('counterstart', 'counter_start'),
        ('counterstop', 'counter_stop'),
        
        # 缓冲区相关
        ('buffersize', 'buffer_size'),
        ('buffernext', 'buffer_next'),
        ('bufferhead', 'buffer_head'),
        ('buffertail', 'buffer_tail'),
        ('bufferfull', 'buffer_full'),
        ('bufferempty', 'buffer_empty'),
        
        # 硬件相关
        ('gpioctrl', 'gpio_ctrl'),
        ('gpiostate', 'gpio_state'),
        ('gpiomode', 'gpio_mode'),
        ('gpiovalue', 'gpio_value'),
        ('uartctrl', 'uart_ctrl'),
        ('uartconfig', 'uart_config'),
        ('uartstatus', 'uart_status'),
        ('timerctrl', 'timer_ctrl'),
        ('timervalue', 'timer_value'),
        ('timermode', 'timer_mode'),
        ('timerconfig', 'timer_config'),
        
        # 操作相关
        ('processinput', 'process_input'),
        ('processoutput', 'process_output'),
        ('handleread', 'handle_read'),
        ('handlewrite', 'handle_write'),
        ('handlevalue', 'handle_value'),
        ('updatestatus', 'update_status'),
        ('updatevalue', 'update_value'),
        ('updateconfig', 'update_config'),
        
        # 状态相关
        ('statusflag', 'status_flag'),
        ('statusvalue', 'status_value'),
        ('statusmode', 'status_mode'),
        ('statusready', 'status_ready'),
        ('statusbusy', 'status_busy'),
        ('statusidle', 'status_idle'),
        ('statusvalid', 'status_valid'),
        
        # 设备相关
        ('deviceinit', 'device_init'),
        ('deviceconfig', 'device_config'),
        ('devicestatus', 'device_status'),
        ('devicemode', 'device_mode'),
        ('devicevalue', 'device_value'),
        ('devicectrl', 'device_ctrl'),
        
        # 协议相关
        ('usbhid', 'usb_hid'),  # 特别处理usb_hid的情况
        ('usb_hid_adddata', 'usb_hid_add_data'),  # 特别处理这个例子
        ('usbconfig', 'usb_config'),
        ('usbstatus', 'usb_status'),
        ('usbctrl', 'usb_ctrl'),
        ('usbdata', 'usb_data'),
        ('hidconfig', 'hid_config'),
        ('hidstatus', 'hid_status'),
        ('hiddata', 'hid_data'),
        
        # 接收发送相关
        ('recvdata', 'recv_data'),
        ('recvcount', 'recv_count'),
        ('recvsize', 'recv_size'),
        ('recvbuffer', 'recv_buffer'),
        ('senddata', 'send_data'),
        ('sendcount', 'send_count'),
        ('sendsize', 'send_size'),
        ('sendbuffer', 'send_buffer'),
        
        # 其他常见模式
        ('tempvalue', 'temp_value'),
        ('tempdata', 'temp_data'),
        ('temprange', 'temp_range'),
        ('lengthmax', 'length_max'),
        ('lengthmin', 'length_min'),
        ('indexnext', 'index_next'),
        ('indexprev', 'index_prev'),
        ('valuemax', 'value_max'),
        ('valuemin', 'value_min'),
        ('sizenext', 'size_next'),
        ('sizeprev', 'size_prev'),
    ]
    
    result = lower_name
    
    # 应用替换规则
    for pattern, replacement in replacements:
        if pattern in result:
            result = result.replace(pattern, replacement)
    
    # 特殊处理：如果还是很长且没有下划线，尝试拆分
    if '_' not in result and len(result) > 6:
        # 尝试在常见的前缀后插入下划线
        prefixes = ['usb', 'hid', 'uart', 'gpio', 'spi', 'i2c', 'adc', 'dac', 'dma', 'timer', 'counter', 'buffer', 'config', 'status', 'process', 'handle', 'update', 'device', 'send', 'recv', 'data', 'value', 'size', 'count', 'length', 'index', 'mode', 'state', 'ctrl', 'reg', 'addr', 'port']
        
        for prefix in sorted(prefixes, key=len, reverse=True):
            if result.startswith(prefix) and len(result) > len(prefix):
                result = prefix + '_' + result[len(prefix):]
                break
    
    return result

def correct_spelling_and_abbreviations(name):
    """
    纠正常见的拼写错误和不规范的缩写
    
    Args:
        name: 原始标识符
    
    Returns:
        str: 纠正后的标识符
    """
    # 常见拼写错误纠正（按优先级排序，避免冲突）
    spelling_corrections = {
        # 基础错误词 - 只纠正真正的错误
        'lenght': 'length',
        'lengh': 'length',
        'widht': 'width',
        'heigth': 'height',
        'recieve': 'receive',
        'recive': 'receive',
        'reciever': 'receiver',
        'reciver': 'receiver',
        'sucess': 'success',
        'succes': 'success',
        'seperate': 'separate',
        'seprate': 'separate',
        'occured': 'occurred',
        'occurence': 'occurrence',
        'begining': 'beginning',
        'beggining': 'beginning',
        'comming': 'coming',
        'processsing': 'processing',
        'processig': 'processing',
        'initialise': 'initialize',
        'connexion': 'connection',
        'conection': 'connection',
        'temperary': 'temporary',
        'temperory': 'temporary',
        'intial': 'initial',
        'initail': 'initial',
        'disconect': 'disconnect',
        'conected': 'connected',
        
        # 硬件相关错误
        'adress': 'address',
        'adresss': 'address',
        'regist': 'register',
        'registor': 'register',
        'regester': 'register',
        'interrup': 'interrupt',
        'interupt': 'interrupt',
        'periphial': 'peripheral',
        'periperal': 'peripheral',
        'periferal': 'peripheral',
        'controll': 'control',
        'contoller': 'controller',
        'contorller': 'controller',
        'transmition': 'transmission',
        'transmision': 'transmission',
        'transmiter': 'transmitter',
        'recevier': 'receiver',
        'recever': 'receiver',
        
        # 算法相关错误
        'caclulate': 'calculate',
        'calulate': 'calculate',
        'calculte': 'calculate',
        'calculat': 'calculate',
        'comparsion': 'comparison',
        'comparision': 'comparison',
        'destory': 'destroy',
        'distroy': 'destroy',
        'simular': 'similar',
        'simillar': 'similar',
        'diffrent': 'different',
        'diferent': 'different',
        'defintion': 'definition',
        
        # 其他常见错误
        'curent': 'current',
        'currnet': 'current',
        'curren': 'current',
        'previus': 'previous',
        'previos': 'previous',
        'prevoius': 'previous',
        'follwing': 'following',
        'folowing': 'following',
        'allways': 'always',
        'alway': 'always',
        'alwasy': 'always',
        'usualy': 'usually',
        'ususally': 'usually',
        'szie': 'size',
        'sze': 'size',
        'lenth': 'length',
        'maxium': 'maximum',
        'maximun': 'maximum',
        'minium': 'minimum',
        'minimun': 'minimum',
        'amout': 'amount',
        'ammount': 'amount',
        'amunt': 'amount',
        'nummber': 'number',
        'numer': 'number',
        'numbr': 'number',
        'indx': 'index',
        'inddex': 'index',
        'inde': 'index',
        'configuraton': 'configuration',
        'configration': 'configuration',
        'paramater': 'parameter',
        'paramter': 'parameter',
        'parametre': 'parameter',
        'arguement': 'argument',
        'argumment': 'argument',
        'initilize': 'initialize',
        'initalize': 'initialize',
        'initiaze': 'initialize',
        'initilisation': 'initialization',
        'comunication': 'communication',
        'comunicaton': 'communication',
        'comuniction': 'communication',
        'protocal': 'protocol',
        'protocall': 'protocol',
        'protoccol': 'protocol',
        'paket': 'packet',
        'packt': 'packet',
        'pakect': 'packet',
        'mesage': 'message',
        'messge': 'message',
        'messsage': 'message',
        'menory': 'memory',
        'memmory': 'memory',
        'memoy': 'memory',
        'memery': 'memory',
        'allocat': 'allocate',
        'alocate': 'allocate',
        'delocate': 'deallocate',
        'dealocate': 'deallocate',
        'dealocat': 'deallocate',
        'fre': 'free',
        'fee': 'free',
    }
    
    # 常见不规范缩写纠正（保留合理的常见缩写）
    abbreviation_corrections = {
        # 仅纠正真正有问题的缩写
        'conf': 'configuration',  # config更常见，保留config
        'cfg': 'configuration',   # config更常见，保留config
        'inf': 'information',     # info更常见，保留info
        'mn': 'minimum',          # min更常见，保留min
        'ave': 'average',         # avg更常见，保留avg
        
        # 其他容易引起误解的缩写
        'rqst': 'request',        # req更常见
        'rsp': 'response',        # resp更常见
        'er': 'error',            # err更常见
        'ex': 'exception',        # exc更常见
        'ini': 'initialize',      # init更常见
    }
    
    # 分割为单词进行处理，避免过度纠正
    words = name.split('_') if '_' in name else [name]
    corrected_words = []
    
    for word in words:
        # 保持原始的大小写，只在确实有错误时才纠正
        corrected_word = word
        lower_word = word.lower()
        
        # 先纠正拼写错误（只替换完整的错误词，避免部分替换）
        for wrong_spelling, correct_spelling in spelling_corrections.items():
            if lower_word == wrong_spelling:  # 只在完全匹配时才纠正
                corrected_word = correct_spelling
                break
        
        # 再处理缩写（只处理完全匹配的缩写）
        if lower_word in abbreviation_corrections:
            corrected_word = abbreviation_corrections[lower_word]
        
        corrected_words.append(corrected_word)
    
    # 重新组合
    if len(corrected_words) > 1:
        return '_'.join(corrected_words)
    elif len(corrected_words) == 1:
        return corrected_words[0]
    else:
        return name

def to_macro_style(name):
    """
    将宏名转换为大写+下划线风格
    
    Args:
        name: 原始宏名
    
    Returns:
        str: 大写+下划线风格的宏名
    """
    # 如果已经是大写，保持不变
    if name.isupper():
        return name
    
    # 驼峰转大写+下划线
    result = []
    for i, c in enumerate(name):
        if i == 0:
            result.append(c.upper())
        elif c.isupper():
            # 只在以下情况下添加下划线：
            # 1. 前一个字符是小写（驼峰分界）
            # 2. 前一个字符是数字和当前是大写（数字到字母的分界）
            # 3. 后一个字符是小写（连续大写单词的结束，如 "XMLHttp" 中的 "H"）
            # 注意：不要在下划线后添加下划线
            prev_char = name[i-1]
            if ((prev_char.islower() or prev_char.isdigit()) or \
               (i+1 < len(name) and name[i+1].islower())) and \
               prev_char != '_':
                result.append('_' + c.upper())
            else:
                result.append(c.upper())
        elif c == '_':
            # 保留原有的下划线，但不要重复添加
            if i == 0 or result[-1] != '_':
                result.append('_')
        else:
            result.append(c.upper())
    
    macro_name = ''.join(result)
    
    # 清理多余的下划线（连续多个下划线合并为一个）
    macro_name = re.sub(r'_+', '_', macro_name)
    
    return macro_name

def format_identifiers_in_project(project_root, file_path):
    """
    对整个项目中的标识符进行格式化
    
    Args:
        project_root: 项目根目录
        file_path: 当前处理的文件路径
    
    Returns:
        bool: 是否成功
    """
    try:
        print(f"  开始格式化标识符...")
        
        # 找出当前文件中的所有需要格式化的标识符和宏
        identifiers, macros = find_identifiers_in_file(file_path)
        
        if not identifiers and not macros:
            print(f"  没有找到需要格式化的标识符")
            return True
        
        total_count = len(identifiers) + len(macros)
        print(f"  找到 {total_count} 个需要格式化的标识符（{len(identifiers)} 个变量/函数，{len(macros)} 个宏）")
        
        # 格式化变量和函数名
        if identifiers:
            print(f"  格式化变量和函数名...")
            # 按长度排序，先处理长的标识符避免冲突
            sorted_identifiers = sorted(identifiers, key=len, reverse=True)
            
            for original_name in sorted_identifiers:
                linux_name = to_linux_style(original_name)
                
                # 如果转换后名称相同，跳过
                if original_name == linux_name:
                    continue
                
                print(f"    变量/函数: {original_name} -> {linux_name}")
                
                # 在整个项目中搜索并替换这个标识符
                success = replace_identifier_in_project(project_root, original_name, linux_name)
                if not success:
                    print(f"      警告: 替换 {original_name} 时出现错误")
                    return False
        
        # 格式化宏名
        if macros:
            print(f"  格式化宏名...")
            # 按长度排序，先处理长的宏名避免冲突
            sorted_macros = sorted(macros, key=len, reverse=True)
            
            for original_macro in sorted_macros:
                macro_name = to_macro_style(original_macro)
                
                # 如果转换后名称相同，跳过
                if original_macro == macro_name:
                    continue
                
                print(f"    宏: {original_macro} -> {macro_name}")
                
                # 在整个项目中搜索并替换这个宏名
                success = replace_macro_in_project(project_root, original_macro, macro_name)
                if not success:
                    print(f"      警告: 替换宏 {original_macro} 时出现错误")
                    return False
        
        print(f"  标识符格式化完成")
        return True
        
    except Exception as e:
        print(f"  格式化标识符时出错: {str(e)}")
        return False

def replace_identifier_in_project(project_root, old_name, new_name):
    """
    在整个项目中替换标识符
    
    Args:
        project_root: 项目根目录
        old_name: 原始标识符
        new_name: 新标识符
    
    Returns:
        bool: 是否成功
    """
    try:
        # 找到项目中所有的源文件
        source_files = find_source_files(project_root)
        
        for source_file in source_files:
            try:
                encodings = ['utf-8', 'gbk', 'gb2312', 'latin1']
                content = None
                encoding_used = None
                
                for encoding in encodings:
                    try:
                        with open(source_file, 'r', encoding=encoding) as f:
                            content = f.read()
                        encoding_used = encoding
                        break
                    except UnicodeDecodeError:
                        continue
                    except Exception:
                        continue
                
                if content is None:
                    continue
                
                # 构建匹配模式，确保匹配完整的标识符
                pattern = r'\b' + re.escape(old_name) + r'\b'
                
                # 检查是否包含该标识符
                if re.search(pattern, content):
                    # 替换标识符
                    new_content = re.sub(pattern, new_name, content)
                    
                    # 如果内容发生了变化，写回文件
                    if new_content != content:
                        try:
                            with open(source_file, 'w', encoding=encoding_used) as f:
                                f.write(new_content)
                        except UnicodeEncodeError:
                            # 如果写回失败，尝试用UTF-8
                            with open(source_file, 'w', encoding='utf-8') as f:
                                f.write(new_content)
                        
                        print(f"      更新: {source_file}")
                
            except Exception as e:
                print(f"      处理文件 {source_file} 时出错: {str(e)}")
                return False
        
        return True
        
    except Exception as e:
        print(f"      在项目中替换标识符时出错: {str(e)}")
        return False

def replace_macro_in_project(project_root, old_macro, new_macro):
    """
    在整个项目中替换宏名
    
    Args:
        project_root: 项目根目录
        old_macro: 原始宏名
        new_macro: 新宏名
    
    Returns:
        bool: 是否成功
    """
    try:
        # 找到项目中所有的源文件
        source_files = find_source_files(project_root)
        
        for source_file in source_files:
            try:
                encodings = ['utf-8', 'gbk', 'gb2312', 'latin1']
                content = None
                encoding_used = None
                
                for encoding in encodings:
                    try:
                        with open(source_file, 'r', encoding=encoding) as f:
                            content = f.read()
                        encoding_used = encoding
                        break
                    except UnicodeDecodeError:
                        continue
                    except Exception:
                        continue
                
                if content is None:
                    continue
                
                # 需要替换宏的几种模式：
                # 1. #define 宏名
                # 2. #ifdef/#ifndef/#if defined(宏名)
                # 3. #undef 宏名
                # 4. 代码中使用的宏名（独立标识符）
                
                changes_made = False
                new_content = content
                
                # 替换 #define
                define_pattern = r'^(#\s*define\s+)' + re.escape(old_macro) + r'\b'
                new_content = re.sub(define_pattern, r'\1' + new_macro, new_content, flags=re.MULTILINE)
                if new_content != content:
                    changes_made = True
                
                # 替换 #ifdef/#ifndef/#if defined()
                ifdef_pattern = r'^(#\s*(?:ifdef|ifndef|if\s+defined)\s*\(\s*)' + re.escape(old_macro) + r'\s*\)'
                new_content = re.sub(ifdef_pattern, r'\1' + new_macro + r')', new_content, flags=re.MULTILINE)
                if new_content != content:
                    changes_made = True
                
                # 替换 #undef
                undef_pattern = r'^(#\s*undef\s+)' + re.escape(old_macro) + r'\b'
                new_content = re.sub(undef_pattern, r'\1' + new_macro, new_content, flags=re.MULTILINE)
                if new_content != content:
                    changes_made = True
                
                # 替换代码中使用的宏（独立标识符）
                # 构建匹配模式，确保匹配完整的标识符，但避免匹配函数调用
                macro_pattern = r'\b' + re.escape(old_macro) + r'(?!\s*\()'  # 负向 lookahead，后面不能跟 (
                new_content = re.sub(macro_pattern, new_macro, new_content)
                if new_content != content:
                    changes_made = True
                
                # 如果内容发生了变化，写回文件
                if changes_made:
                    try:
                        with open(source_file, 'w', encoding=encoding_used) as f:
                            f.write(new_content)
                    except UnicodeEncodeError:
                        # 如果写回失败，尝试用UTF-8
                        with open(source_file, 'w', encoding='utf-8') as f:
                            f.write(new_content)
                    
                    print(f"      更新宏: {source_file}")
                
            except Exception as e:
                print(f"      处理文件 {source_file} 时出错: {str(e)}")
                return False
        
        return True
        
    except Exception as e:
        print(f"      在项目中替换宏时出错: {str(e)}")
        return False

def format_with_astyle(file_path, astyle_path='astyle', astyle_options=None):
    """
    Format a single file using astyle
    
    Args:
        file_path: File path to format
        astyle_path: astyle executable path, defaults to astyle in system PATH
        astyle_options: List of astyle command line options
    
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
            timeout=30  # 30 second timeout
        )
        
        return result.returncode == 0, result.stdout, result.stderr
        
    except subprocess.TimeoutExpired:
        return False, "", "astyle processing timeout"
    except FileNotFoundError:
        return False, "", f"astyle executable not found: {astyle_path}"
    except Exception as e:
        return False, "", f"Error executing astyle: {str(e)}"

def format_directory(directory, astyle_path='astyle', astyle_options=None, dry_run=False):
    """
    Format all C/C++ source files in the specified directory
    
    Args:
        directory: Directory path to format
        astyle_path: astyle executable path
        astyle_options: astyle command line options
        dry_run: Whether to only show files to be formatted without executing
    
    Returns:
        (total_files, success_count, failed_files)
    """
    print(f"Searching directory: {directory}")
    
    # Find all source code files
    source_files = find_source_files(directory)
    
    if not source_files:
        print("No source code files found")
        return 0, 0, []
    
    print(f"Found {len(source_files)} source code files")
    
    total_files = len(source_files)
    success_count = 0
    failed_files = []
    
    # 统计信息
    total_identifiers = 0
    total_functions = 0
    total_variables = 0
    total_macros = 0
    files_with_identifiers = 0
    comments_moved_files = 0
    
    for i, file_path in enumerate(source_files, 1):
        print(f"[{i}/{total_files}] Processing: {file_path}")
        
        if dry_run:
            print(f"  Would format: {file_path}")
            success_count += 1
            continue
        
        success, output, error = format_with_astyle(file_path, astyle_path, astyle_options)
        
        if success:
            # Step 2: Process trailing comments (automatic execution)
            if not dry_run:
                comment_success = move_trailing_comments(file_path)
                if comment_success:
                    comments_moved_files += 1
                
                if not comment_success:
                    failed_files.append(file_path)
                    continue
                
                # Step 3: Format identifiers to Linux style (automatic execution)
                identifiers, macros = find_identifiers_in_file(file_path)
                if identifiers or macros:
                    files_with_identifiers += 1
                    total_identifiers += len(identifiers) + len(macros)
                    total_macros += len(macros)
                    
                    # 更准确地统计函数和变量
                    # 函数名是那些在函数定义中匹配到的标识符
                    try:
                        # 重新读取文件内容来统计函数
                        with open(file_path, 'r', encoding='utf-8') as f:
                            content = f.read()
                        
                        # 匹配函数定义中的函数名
                        function_pattern = r'\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)\s*\{'
                        function_matches = re.findall(function_pattern, content)
                        matched_functions = set(function_matches)
                        
                        # 找出既在identifiers中又在matched_functions中的函数名
                        functions_in_identifiers = matched_functions & identifiers
                        
                        # 函数数量 = 既在identifiers中又在matched_functions中的数量
                        total_functions += len(functions_in_identifiers)
                        
                        # 变量数量 = identifiers中除了函数之外的其他标识符
                        variables_in_identifiers = identifiers - functions_in_identifiers
                        total_variables += len(variables_in_identifiers)
                        
                    except:
                        # 如果出错，使用简单统计
                        estimated_functions = min(len(identifiers), max(1, len(identifiers) // 3))  # 估计至少有一个函数，但不超过总数
                        total_functions += estimated_functions
                        total_variables += len(identifiers) - estimated_functions
                
                identifier_success = format_identifiers_in_project(directory, file_path)
                if not identifier_success:
                    print(f"  Warning: Identifier formatting failed for {file_path}")
                    # Continue even if identifier formatting fails
                
                # Step 4: Second astyle formatting (automatic execution)
                success2, output2, error2 = format_with_astyle(file_path, astyle_path, astyle_options)
                if not success2:
                    print(f"  Warning: Second astyle formatting failed for {file_path}: {error2}")
                    # Continue even if second formatting fails
            
            success_count += 1
            if output:
                print(f"  First astyle output: {output.strip()}")
            if output2:
                print(f"  Second astyle output: {output2.strip()}")
        else:
            failed_files.append(file_path)
            print(f"  Error: {error}")
    
    return total_files, success_count, failed_files, {
        'total_identifiers': total_identifiers,
        'total_functions': total_functions, 
        'total_variables': total_variables,
        'total_macros': total_macros,
        'files_with_identifiers': files_with_identifiers,
        'comments_moved_files': comments_moved_files
    }

def format_multiple_directories(directories, astyle_path='astyle', astyle_options=None, dry_run=False):
    """
    Format C/C++ source files in multiple directories
    
    Args:
        directories: List of directory paths to format
        astyle_path: astyle executable path
        astyle_options: astyle command line options
        dry_run: Whether to only show files to be formatted without executing
    
    Returns:
        dict: Combined results from all directories
    """
    total_files = 0
    total_success = 0
    all_failed_files = []
    
    # Combined statistics
    combined_stats = {
        'total_identifiers': 0,
        'total_functions': 0,
        'total_variables': 0,
        'total_macros': 0,
        'files_with_identifiers': 0,
        'comments_moved_files': 0
    }
    
    for directory in directories:
        print(f"\n{'='*60}")
        print(f"Processing directory: {os.path.abspath(directory)}")
        print(f"{'='*60}")
        
        # Check if directory exists
        if not os.path.exists(directory):
            print(f"Error: Directory '{directory}' does not exist, skipping...")
            continue
        
        if not os.path.isdir(directory):
            print(f"Error: '{directory}' is not a directory, skipping...")
            continue
        
        # Format this directory
        total, success, failed, stats = format_directory(
            directory,
            astyle_path,
            astyle_options,
            dry_run
        )
        
        # Update combined results
        total_files += total
        total_success += success
        all_failed_files.extend(failed)
        
        # Update combined statistics
        combined_stats['total_identifiers'] += stats['total_identifiers']
        combined_stats['total_functions'] += stats['total_functions']
        combined_stats['total_variables'] += stats['total_variables']
        combined_stats['total_macros'] += stats['total_macros']
        combined_stats['files_with_identifiers'] += stats['files_with_identifiers']
        combined_stats['comments_moved_files'] += stats['comments_moved_files']
    
    return total_files, total_success, all_failed_files, combined_stats

def main():
    parser = argparse.ArgumentParser(
        description='Format C/C++ code in entire folders using astyle',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Example usage:
  python format_code.py ./src                    # Format src directory (auto-move trailing comments)
  python format_code.py ./src ./lib ./include    # Format multiple directories
  python format_code.py . --dry-run            # Preview mode, do not actually format
  python format_code.py . --astyle /usr/bin/astyle  # Specify astyle path
  python format_code.py . --style=allman       # Use Allman style
  
Note: Script automatically moves trailing comments from variable definitions to the line above after formatting
      Script automatically formats identifiers to Linux style (lowercase with underscores)
      Script automatically formats macro names to uppercase with underscores
        """
    )
    
    parser.add_argument(
        'directories',
        nargs='*',
        default=['.'],
        help='Directory paths to format (default is current directory)'
    )
    
    parser.add_argument(
        '--astyle',
        default='astyle',
        help='astyle executable path (default uses astyle from system PATH)'
    )
    
    parser.add_argument(
        '--style',
        choices=['attach', 'allman', 'gnu', 'google', 'linux'],
        default='attach',
        help='Code style (default is attach)'
    )
    
    parser.add_argument(
        '--indent',
        type=int,
        default=4,
        help='Number of indentation spaces (default is 4)'
    )
    
    parser.add_argument(
        '--max-length',
        type=int,
        default=120,
        help='Maximum code line length (default is 120)'
    )
    
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Preview mode, only show files to be formatted without actual execution'
    )
    
    parser.add_argument(
        '--extensions',
        nargs='+',
        default=['.c', '.cpp', '.cxx', '.cc', '.h', '.hpp', '.hxx', '.hh'],
        help='File extensions to process (default is all C/C++ files)'
    )
    
    
    
    args = parser.parse_args()
    
    # Use default options from function (user's custom settings)
    astyle_options = None
    
    print("=" * 60)
    print("C/C++ Code Formatting Tool")
    print("=" * 60)
    print(f"Target directories: {len(args.directories)} path(s)")
    for directory in args.directories:
        print(f"  - {os.path.abspath(directory)}")
    print(f"Code style: {args.style}")
    print(f"Indentation: {args.indent} spaces")
    print(f"Maximum line length: {args.max_length}")
    print(f"File extensions: {', '.join(args.extensions)}")
    print(f"Preview mode: {'Yes' if args.dry_run else 'No'}")
    print(f"Auto move trailing comments: Yes")
    print(f"Auto format identifiers to Linux style: Yes")
    print(f"Auto format macros to uppercase style: Yes")
    print(f"Second astyle pass: Yes")
    print(f"astyle path: {args.astyle}")
    print("=" * 60)
    
    # Execute formatting for multiple directories
    total, success, failed, stats = format_multiple_directories(
        args.directories,
        args.astyle,
        astyle_options,
        args.dry_run
    )
    
    print("\n" + "=" * 60)
    print("All directories formatting completed!")
    print(f"Total files: {total}")
    print(f"Success: {success}")
    print(f"Failed: {len(failed)}")
    
    if not args.dry_run:
        print("\n" + "=" * 60)
        print("Combined Statistics:")
        print(f"Files with trailing comments moved: {stats['comments_moved_files']}")
        print(f"Files with identifiers formatted: {stats['files_with_identifiers']}")
        print(f"Total identifiers processed: {stats['total_identifiers']}")
        print(f"  - Functions: {stats['total_functions']}")
        print(f"  - Variables: {stats['total_variables']}")
        print(f"  - Macros: {stats['total_macros']}")
        files_without_changes = max(0, total - stats['files_with_identifiers'] - stats['comments_moved_files'])
        print(f"Files without changes needed: {files_without_changes}")
    
    if failed:
        print("\nFailed files:")
        for file_path in failed:
            print(f"  - {file_path}")
    
    if len(failed) > 0:
        sys.exit(1)
    
    print("=" * 60)

if __name__ == '__main__':
    main()