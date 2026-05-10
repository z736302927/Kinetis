#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import subprocess
import argparse
import re
from pathlib import Path


# =============================================================================
# Encoding Helpers — 消除多个函数中重复的编码探测逻辑
# =============================================================================

ENCODINGS = ['utf-8', 'gbk', 'gb2312', 'latin1']


def read_file_content(file_path):
    """Read file content with automatic encoding detection.

    Returns:
        (str content, str encoding_used) on success, (None, None) on failure.
    """
    for encoding in ENCODINGS:
        try:
            with open(file_path, 'r', encoding=encoding) as f:
                return f.read(), encoding
        except (UnicodeDecodeError, OSError):
            continue
    return None, None


def read_file_lines(file_path):
    """Read file lines with automatic encoding detection.

    Returns:
        (list[str] lines, str encoding_used) or (None, None).
    """
    for encoding in ENCODINGS:
        try:
            with open(file_path, 'r', encoding=encoding) as f:
                return f.readlines(), encoding
        except (UnicodeDecodeError, OSError):
            continue
    return None, None


def write_file(file_path, content, encoding='utf-8'):
    """Write content to file with encoding fallback.

    Uses a temporary file and atomic rename to avoid data loss on failure.
    Returns True on success.
    """
    import tempfile
    tried = {encoding}
    for enc in [encoding] + [e for e in ENCODINGS if e != encoding]:
        if enc in tried:
            continue
        tried.add(enc)
        tmp_path = None
        try:
            # Encode first to avoid truncating the original file on failure
            encoded = content.encode(enc)
            dir_path = Path(file_path).parent
            dir_path.mkdir(parents=True, exist_ok=True)
            with tempfile.NamedTemporaryFile(
                dir=dir_path, delete=False, suffix='.tmp', mode='wb'
            ) as f:
                tmp_path = f.name
                f.write(encoded)
            os.replace(tmp_path, file_path)
            return True
        except (UnicodeEncodeError, OSError):
            if tmp_path:
                try:
                    os.unlink(tmp_path)
                except Exception:
                    pass
            continue
    return False


def write_lines(file_path, lines, encoding='utf-8'):
    """Write a list of (newline-terminated) lines to file."""
    return write_file(file_path, ''.join(lines), encoding)


# =============================================================================
# File Discovery
# =============================================================================

C_EXTENSIONS = ['.c', '.cpp', '.cxx', '.cc', '.h', '.hpp', '.hxx', '.hh']


def find_source_files(directory, extensions=None):
    """Recursively find all source code files matching given extensions."""
    if extensions is None:
        extensions = C_EXTENSIONS
    ext_set = set(extensions)
    try:
        return sorted(
            str(p) for p in Path(directory).rglob('*')
            if p.suffix in ext_set and p.is_file()
        )
    except OSError:
        return []


# =============================================================================
# #define Alignment — 新增功能
# =============================================================================

_DEFINE_RE = re.compile(
    r'^(\s*#\s*define\s+)([a-zA-Z_][a-zA-Z0-9_]*)((?:\([^)]*\))?\s*)(.*)$'
)


def _is_define_line(line):
    """Check if a line is a simple #define statement (not function-like)."""
    m = _DEFINE_RE.match(line.rstrip('\n\r'))
    if not m:
        return False
    params = m.group(3).strip()
    # 跳过函数式宏定义（带参数的 #define FOO(x) ...）
    return not params.startswith('(')


def align_defines_in_file(file_path):
    """Align #define values across the entire file to a single column.

    Finds ALL #define lines (skipping function-like macros), determines the
    globally longest macro name, and aligns every value to that column.
    Blank lines, comments, and non-#define lines are preserved as-is.
    """
    lines, encoding = read_file_lines(file_path)
    if lines is None:
        print(f"  Unable to read file {file_path}")
        return False

    # Collect all #define line indices and find global longest name
    define_indices = []
    max_prefix_len = 0

    for idx, line in enumerate(lines):
        m = _DEFINE_RE.match(line.rstrip('\n\r'))
        if not m:
            continue
        params = m.group(3).strip()
        if params.startswith('('):
            continue  # skip function-like macros

        prefix = m.group(1)   # "#define "
        name = m.group(2)     # macro name
        prefix_name_len = len(prefix + name)
        if prefix_name_len > max_prefix_len:
            max_prefix_len = prefix_name_len
        define_indices.append(idx)

    if not define_indices:
        return False

    target_col = max_prefix_len + 4
    new_lines = list(lines)
    changed = False

    for idx in define_indices:
        raw = new_lines[idx].rstrip('\n\r')
        m = _DEFINE_RE.match(raw)
        # Guaranteed to match since we already validated above
        prefix = m.group(1)
        name = m.group(2)
        val = m.group(4)
        line_end = new_lines[idx][len(raw):]

        val_stripped = val.strip()
        if val_stripped:
            spaces = target_col - len(prefix + name)
            aligned = f"{prefix}{name}{' ' * spaces}{val_stripped}{line_end}"
            if aligned != new_lines[idx]:
                new_lines[idx] = aligned
                changed = True

    if changed:
        if not write_lines(file_path, new_lines, encoding):
            print(f"  Failed to write: {file_path}")
            return False
        print(f"  Aligned #defines: {file_path}")

    return changed


# =============================================================================
# Trailing Comment Processing
# =============================================================================

def move_trailing_comments(file_path):
    """Move trailing // comments from code lines to the line above."""
    lines, encoding = read_file_lines(file_path)
    if lines is None:
        print(f"  Unable to read file {file_path}")
        return False

    new_lines = []
    changed = False

    for line in lines:
        stripped = line.rstrip()
        pos = stripped.find('//')
        if pos > 0:
            before = stripped[:pos].rstrip()
            after = stripped[pos + 2:].strip()
            if _is_comment_candidate(before):
                indent = re.match(r'^(\s*)', line).group(1)
                new_lines.append(f"{indent}// {after}\n")
                new_lines.append(before + '\n')
                changed = True
                continue
        new_lines.append(line)

    if changed:
        write_lines(file_path, new_lines, encoding)
        print(f"  Moved trailing comments: {file_path}")

    return changed


def _is_comment_candidate(before):
    """Check if a line-before-comment is a candidate for comment extraction."""
    if re.match(r'^\s*#\s*define\s+\w+', before):
        return True
    if re.match(r'^\s*[\w\*\s\[\].*&]+.*[=]', before):
        return True
    if re.match(r'^\s*(if|while|for|switch|else|do|return|goto|break|continue|case|default)\b', before):
        return True
    if re.match(r'^\s*\w+.*\(', before):
        return True
    if re.match(r'^\s*(?:[a-zA-Z_][a-zA-Z0-9_]*\s+)+[\w\*\s\[\].*&\s,]+', before):
        return True
    return False


# =============================================================================
# Filename Formatting (保持已有逻辑基本不变，仅使用编码辅助函数)
# =============================================================================

def find_identifiers_in_file(file_path):
    """Find all function/variable names and macro names that need formatting."""
    content, _ = read_file_content(file_path)
    if content is None:
        return set(), set()

    identifiers = set()
    macros = set()

    # 函数定义
    control = {'if', 'for', 'while', 'switch', 'catch', 'try'}
    for func_name, _ in re.findall(r'\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)\s*\{', content):
        if func_name not in control:
            identifiers.add(func_name)

    # #define
    for m in re.finditer(r'^\s*#\s*define\s+([a-zA-Z_][a-zA-Z0-9_]*)\b', content, re.MULTILINE):
        macros.add(m.group(1))

    # #ifdef / #ifndef / #if defined(...)
    for m in re.finditer(r'^\s*#\s*(?:ifdef|ifndef|if\s+defined)\s*\(\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\)',
                         content, re.MULTILINE):
        macros.add(m.group(1))

    # #undef
    for m in re.finditer(r'^\s*#\s*undef\s+([a-zA-Z_][a-zA-Z0-9_]*)\b', content, re.MULTILINE):
        macros.add(m.group(1))

    # struct / union / enum 标签名
    for m in re.finditer(r'(?:typedef\s+)?(?:struct|union|enum)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(?:\{|[;\n])', content):
        identifiers.add(m.group(1))

    # typedef struct { ... } TypeName
    for m in re.finditer(r'typedef\s+struct\s*(?:[a-zA-Z_][a-zA-Z0-9_]*)?\s*\{[^}]*\}\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*[;,]',
                         content, re.DOTALL):
        identifiers.add(m.group(1))

    # 普通 typedef
    for m in re.finditer(r'typedef\s+(?:[\w\s\*\[\]]+)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*[;,](?!\s*\{)', content):
        identifiers.add(m.group(1))

    # 变量声明
    types_pat = (r'\b(?:int|char|float|double|void|long|short|unsigned|signed|static|extern|const|'
                 r'volatile|struct|union|enum|u8|u16|u32|s8|s16|s32|f32|f64)\s+'
                 r'(?:\*\s*|const\s+|volatile\s+)*(?:\w+\s+)*([a-zA-Z_][a-zA-Z0-9_]*)\s*'
                 r'(?:\[.*?\]|\s*[;=,\[])')
    for m in re.finditer(types_pat, content):
        identifiers.add(m.group(1))

    # 结构体成员访问 (obj.member / ptr->member)
    for m in re.finditer(r'\b([a-zA-Z_][a-zA-Z0-9_]*)\s*(?:\.|->)\s*([a-zA-Z_][a-zA-Z0-9_]*)', content):
        identifiers.add(m.group(2))
        identifiers.add(m.group(1))

    # 过滤
    keywords = {
        'if', 'else', 'for', 'while', 'do', 'switch', 'case', 'default', 'break', 'continue',
        'return', 'goto', 'sizeof', 'main', 'printf', 'scanf',
        'malloc', 'free', 'memcpy', 'memset', 'strlen', 'strcpy', 'strcmp', 'strcat',
        'NULL', 'true', 'false', 'bool', 'int', 'char', 'float', 'double', 'void',
        'long', 'short', 'unsigned', 'signed', 'static', 'extern', 'const', 'volatile',
        'struct', 'union', 'enum', 'typedef', 'auto', 'register', 'inline', 'restrict',
        'u8', 'u16', 'u32', 'u64', 's8', 's16', 's32', 's64', 'f32', 'f64',
        'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t', 'int8_t', 'int16_t', 'int32_t', 'int64_t',
        'size_t', 'time_t', 'pid_t', 'ssize_t', 'off_t', 'FILE',
    }
    macro_keywords = {
        'NULL', 'TRUE', 'FALSE', 'EOF', 'BUFSIZ', 'FILENAME_MAX', 'FOPEN_MAX',
        'L_tmpnam', 'TMP_MAX', 'EXIT_SUCCESS', 'EXIT_FAILURE', 'RAND_MAX', 'CLOCKS_PER_SEC',
    }

    filtered_id = set()
    for ident in identifiers:
        # 跳过保留字和单个字符
        if ident in keywords or len(ident) <= 1:
            continue
        if ident.isupper():
            continue  # 全大写是宏风格，不处理
        # 已经是纯小写+下划线且不太长 → 跳过
        if re.match(r'^[a-z]+(_[a-z0-9]+)*$', ident) and len(ident) <= 15:
            continue
        filtered_id.add(ident)

    filtered_mac = set()
    for mac in macros:
        if mac in macro_keywords or len(mac) <= 1:
            continue
        if re.match(r'^[A-Z]+(_[A-Z0-9]+)*$', mac):
            continue
        filtered_mac.add(mac)

    return filtered_id, filtered_mac


def format_filename(filename):
    """Format filename to Linux style (lowercase with underscores)."""
    parts = filename.rsplit('.', 1)
    name_only = parts[0]
    ext = '.' + parts[1] if len(parts) == 2 else ''
    return to_linux_style(name_only) + ext


def format_filenames_in_project(project_root, dry_run=False):
    """Format all source filenames to Linux style."""
    print(f"Searching for files to rename in: {project_root}")
    source_files = find_source_files(project_root)
    if not source_files:
        print("No source code files found")
        return 0, 0, []

    total_files = len(source_files)
    rename_details = []

    for file_path in source_files:
        path_obj = Path(file_path)
        if not path_obj.exists():
            continue
        original = path_obj.name
        formatted = format_filename(original)
        if original == formatted:
            continue

        new_path = path_obj.parent / formatted

        # 处理重名冲突
        target_exists = False
        try:
            if new_path.parent.exists():
                existing_files = [f.name for f in new_path.parent.iterdir()]
                target_exists = formatted in existing_files
        except Exception:
            target_exists = new_path.exists()

        if target_exists and str(new_path) != str(path_obj):
            try:
                if new_path.stat().st_size == path_obj.stat().st_size:
                    if path_obj.stat().st_mtime > new_path.stat().st_mtime:
                        print(f"  Replacing older target: {formatted}")
                        new_path.unlink()
                    else:
                        print(f"  Skipping {original} (newer target exists: {formatted})")
                        continue
                else:
                    print(f"  Warning: Conflict - {formatted} exists with different size, skipping {original}")
                    continue
            except Exception as e:
                print(f"  Warning: Cannot check conflict for {formatted}, skipping {original}: {e}")
                continue

        rename_details.append({
            'original': file_path, 'new': str(new_path),
            'old_name': original, 'new_name': formatted,
        })

    renamed = 0
    if not dry_run:
        print(f"\nRenaming {len(rename_details)} files...")
        for detail in rename_details:
            try:
                os.rename(detail['original'], detail['new'])
                rel = Path(detail['original']).parent
                print(f"  Renamed: {detail['old_name']} -> {detail['new_name']}  [{rel}]")
                update_include_statements(project_root, detail['old_name'], detail['new_name'])
                renamed += 1
            except Exception as e:
                print(f"  Error renaming {detail['old_name']}: {e}")
    else:
        print(f"\nDry run - would rename {len(rename_details)} files:")
        for d in rename_details:
            print(f"  {d['old_name']} -> {d['new_name']}")

    return total_files, renamed if not dry_run else len(rename_details), rename_details


def update_include_statements(project_root, old_fn, new_fn):
    """Update #include references after filename rename."""
    sources = find_source_files(project_root)
    p1 = re.compile(r'#include\s*"' + re.escape(old_fn) + r'"')
    p2 = re.compile(r'#include\s*<' + re.escape(old_fn) + r'>')

    for fp in sources:
        content, enc = read_file_content(fp)
        if content is None:
            continue
        changed = False
        if p1.search(content) or p2.search(content):
            content = p1.sub(f'#include "{new_fn}"', content)
            content = p2.sub(f'#include <{new_fn}>', content)
            changed = True
        if changed and write_file(fp, content, enc):
            print(f"    Updated includes in: {fp}")


def to_linux_style(name):
    """Convert an identifier to Linux underscore style."""
    if name.isupper():
        return name
    name = split_camel_case(name)
    name = split_compound_words(name)
    name = correct_spelling_and_abbreviations(name)
    linux = name.lower()
    linux = re.sub(r'_+', '_', linux).strip('_')
    return linux


def split_camel_case(name):
    """Split camelCase into underscore-separated parts."""
    if '_' in name:
        return '_'.join(_split_single_camel_word(p) for p in name.split('_') if p)
    return _split_single_camel_word(name)


def _split_single_camel_word(word):
    if not any(c.isupper() for c in word):
        return word

    complete_abbrevs = [
        'USBHID', 'UART', 'GPIO', 'SPI', 'I2C', 'ADC', 'DAC', 'DMA', 'PWM', 'RTC', 'LED', 'LCD',
        'CPU', 'GPU', 'RAM', 'ROM', 'USB', 'HID', 'IRQ', 'ISR', 'CRC', 'MD5', 'SHA', 'AES', 'DES',
        'RSA', 'TLS', 'SSL', 'HTTP', 'HTTPS', 'FTP', 'SSH', 'DNS', 'DHCP', 'TCP', 'UDP',
        'MAC', 'VLAN', 'VPN', 'NAT', 'SNMP', 'SMTP', 'POP3', 'IMAP', 'MIME', 'URL', 'URI',
        'JSON', 'XML', 'HTML', 'CSS', 'SQL', 'API', 'SDK', 'GUI', 'CLI', 'IDE', 'PID', 'FFT',
        'AV', 'RGB', 'HSV', 'YUV', 'GPS', 'WIFI', 'JS', 'PHP', 'ASP', 'JSP', 'CGI', 'OOP',
        'OO', 'RDBMS', 'DBMS', 'ALU', 'FPU', 'EEPROM',
        'ANO', 'OF', 'UP', 'Fusion', 'Deco', 'Flow', 'IMU', 'ACC', 'GYR', 'MAG',
        'Drift', 'Calib', 'Bias', 'Scale', 'Sens', 'Offset', 'Data', 'Ctrl', 'Task',
        'BSP', 'STM32F4xx', 'STM32', 'RC', 'UWB', 'DT', 'MV', 'OPMV', 'FS', 'PID',
        'CALIBRATE', 'CALIBRATION', 'CONFIGURATION', 'INITIALIZATION', 'TRANSMISSION',
        'RECEIVER', 'TRANSMITTER', 'PERIPHERAL', 'INTERRUPT', 'CONTROLLER', 'PROCESSING',
    ]
    tech_abbrevs = [
        'Gyr', 'Av', 'Acc', 'Mag', 'Temp', 'Hum', 'Pres', 'Alt', 'Dist',
        'Vel', 'Pos', 'Rot', 'Ang', 'Freq', 'Phase', 'Gain', 'Offset',
        'Bias', 'Noise', 'Sig', 'Data', 'Info', 'Msg', 'Cmd', 'Req',
        'Resp', 'Err', 'Stat', 'Val', 'Idx', 'Cnt', 'Num', 'Len', 'Sz',
        'Cfg', 'Param', 'Ctrl', 'Reg', 'Buf', 'Ptr', 'Ref', 'Src', 'Dst',
        'Init', 'Deinit', 'Reset', 'Start', 'Stop', 'Pause', 'Resume',
        'Enable', 'Disable', 'Read', 'Write', 'Send', 'Recv', 'Get', 'Set',
        'Calc', 'Comp', 'Conv', 'Filt', 'Proc', 'Handle', 'Update', 'Sync',
        'Async', 'Lock', 'Unlock', 'Wait', 'Signal', 'Notify', 'Trigger',
        'Flight',
    ]

    result = []
    i, n = 0, len(word)
    while i < n:
        found = False
        for lst in (complete_abbrevs, tech_abbrevs):
            for a in lst:
                if word.startswith(a, i):
                    if result and result[-1] != '_':
                        result.append('_')
                    result.append(a)
                    i += len(a)
                    found = True
                    break
            if found:
                break
        if found:
            continue
        c = word[i]
        if c.isupper():
            if i > 0 and result and result[-1] != '_':
                result.append('_')
            result.append(c.lower())
        else:
            result.append(c)
        i += 1

    return re.sub(r'_+', '_', ''.join(result)).strip('_')


def split_compound_words(name):
    """Detect and split compound words in identifier names."""
    replacements = [
        ('adddata', 'add_data'), ('datainfo', 'data_info'), ('datavalid', 'data_valid'),
        ('datasize', 'data_size'), ('datacount', 'data_count'), ('databuffer', 'data_buffer'),
        ('datanext', 'data_next'), ('dataprev', 'data_prev'), ('datahead', 'data_head'),
        ('datatail', 'data_tail'), ('configparam', 'config_param'), ('configvalue', 'config_value'),
        ('configsize', 'config_size'), ('configmode', 'config_mode'), ('configstate', 'config_state'),
        ('configflag', 'config_flag'), ('counterreset', 'counter_reset'), ('countervalue', 'counter_value'),
        ('countermax', 'counter_max'), ('countermin', 'counter_min'), ('counterstart', 'counter_start'),
        ('counterstop', 'counter_stop'), ('buffersize', 'buffer_size'), ('buffernext', 'buffer_next'),
        ('bufferhead', 'buffer_head'), ('buffertail', 'buffer_tail'), ('bufferfull', 'buffer_full'),
        ('bufferempty', 'buffer_empty'), ('gpioctrl', 'gpio_ctrl'), ('gpiostate', 'gpio_state'),
        ('gpiomode', 'gpio_mode'), ('gpiovalue', 'gpio_value'), ('uartctrl', 'uart_ctrl'),
        ('uartconfig', 'uart_config'), ('uartstatus', 'uart_status'), ('timerctrl', 'timer_ctrl'),
        ('timervalue', 'timer_value'), ('timermode', 'timer_mode'), ('timerconfig', 'timer_config'),
        ('processinput', 'process_input'), ('processoutput', 'process_output'),
        ('handleread', 'handle_read'), ('handlewrite', 'handle_write'), ('handlevalue', 'handle_value'),
        ('updatestatus', 'update_status'), ('updatevalue', 'update_value'), ('updateconfig', 'update_config'),
        ('statusflag', 'status_flag'), ('statusvalue', 'status_value'), ('statusmode', 'status_mode'),
        ('statusready', 'status_ready'), ('statusbusy', 'status_busy'), ('statusidle', 'status_idle'),
        ('statusvalid', 'status_valid'), ('deviceinit', 'device_init'), ('deviceconfig', 'device_config'),
        ('devicestatus', 'device_status'), ('devicemode', 'device_mode'), ('devicevalue', 'device_value'),
        ('devicectrl', 'device_ctrl'), ('usbhid', 'usb_hid'), ('usbconfig', 'usb_config'),
        ('usbstatus', 'usb_status'), ('usbctrl', 'usb_ctrl'), ('usbdata', 'usb_data'),
        ('hidconfig', 'hid_config'), ('hidstatus', 'hid_status'), ('hiddata', 'hid_data'),
        ('recvdata', 'recv_data'), ('recvcount', 'recv_count'), ('recvsize', 'recv_size'),
        ('recvbuffer', 'recv_buffer'), ('senddata', 'send_data'), ('sendcount', 'send_count'),
        ('sendsize', 'send_size'), ('sendbuffer', 'send_buffer'), ('tempvalue', 'temp_value'),
        ('tempdata', 'temp_data'), ('temprange', 'temp_range'), ('lengthmax', 'length_max'),
        ('lengthmin', 'length_min'), ('indexnext', 'index_next'), ('indexprev', 'index_prev'),
        ('valuemax', 'value_max'), ('valuemin', 'value_min'), ('sizenext', 'size_next'),
        ('sizeprev', 'size_prev'),
    ]
    lower = name.lower()
    for pat, repl in replacements:
        lower = lower.replace(pat, repl)
    # 尝试在常见前缀后加下划线
    if '_' not in lower and len(lower) > 6:
        prefixes = ['usb', 'hid', 'uart', 'gpio', 'spi', 'i2c', 'adc', 'dac', 'dma',
                     'timer', 'counter', 'buffer', 'config', 'status', 'process',
                     'handle', 'update', 'device', 'send', 'recv', 'data', 'value',
                     'size', 'count', 'length', 'index', 'mode', 'state', 'ctrl',
                     'reg', 'addr', 'port']
        for pref in sorted(prefixes, key=len, reverse=True):
            if lower.startswith(pref) and len(lower) > len(pref):
                lower = pref + '_' + lower[len(pref):]
                break
    return lower


def correct_spelling_and_abbreviations(name):
    """Fix common spelling mistakes and abbreviations."""
    spelling_corrections = {
        'lenght': 'length', 'lengh': 'length', 'widht': 'width', 'heigth': 'height',
        'recieve': 'receive', 'recive': 'receive', 'reciever': 'receiver', 'reciver': 'receiver',
        'sucess': 'success', 'succes': 'success', 'seperate': 'separate', 'seprate': 'separate',
        'occured': 'occurred', 'occurence': 'occurrence', 'begining': 'beginning',
        'beggining': 'beginning', 'comming': 'coming', 'processsing': 'processing',
        'processig': 'processing', 'initialise': 'initialize', 'connexion': 'connection',
        'conection': 'connection', 'temperary': 'temporary', 'temperory': 'temporary',
        'intial': 'initial', 'initail': 'initial', 'disconect': 'disconnect',
        'conected': 'connected', 'adress': 'address', 'adresss': 'address',
        'regist': 'register', 'registor': 'register', 'regester': 'register',
        'interrup': 'interrupt', 'interupt': 'interrupt', 'periphial': 'peripheral',
        'periperal': 'peripheral', 'periferal': 'peripheral', 'controll': 'control',
        'contoller': 'controller', 'contorller': 'controller', 'transmition': 'transmission',
        'transmision': 'transmission', 'transmiter': 'transmitter', 'recevier': 'receiver',
        'recever': 'receiver', 'caclulate': 'calculate', 'calulate': 'calculate',
        'calculte': 'calculate', 'calculat': 'calculate', 'comparsion': 'comparison',
        'comparision': 'comparison', 'destory': 'destroy', 'distroy': 'destroy',
        'simular': 'similar', 'simillar': 'similar', 'diffrent': 'different',
        'diferent': 'different', 'defintion': 'definition', 'curent': 'current',
        'currnet': 'current', 'curren': 'current', 'previus': 'previous', 'previos': 'previous',
        'prevoius': 'previous', 'follwing': 'following', 'folowing': 'following',
        'allways': 'always', 'alway': 'always', 'alwasy': 'always', 'usualy': 'usually',
        'ususally': 'usually', 'szie': 'size', 'sze': 'size', 'lenth': 'length',
        'maxium': 'maximum', 'maximun': 'maximum', 'minium': 'minimum', 'minimun': 'minimum',
        'amout': 'amount', 'ammount': 'amount', 'amunt': 'amount', 'nummber': 'number',
        'numer': 'number', 'numbr': 'number', 'indx': 'index', 'inddex': 'index', 'inde': 'index',
        'configuraton': 'configuration', 'configration': 'configuration', 'paramater': 'parameter',
        'paramter': 'parameter', 'parametre': 'parameter', 'arguement': 'argument',
        'argumment': 'argument', 'initilize': 'initialize', 'initalize': 'initialize',
        'initiaze': 'initialize', 'initilisation': 'initialization', 'comunication': 'communication',
        'comunicaton': 'communication', 'comuniction': 'communication', 'protocal': 'protocol',
        'protocall': 'protocol', 'protoccol': 'protocol', 'paket': 'packet', 'packt': 'packet',
        'pakect': 'packet', 'mesage': 'message', 'messge': 'message', 'messsage': 'message',
        'menory': 'memory', 'memmory': 'memory', 'memoy': 'memory', 'memery': 'memory',
        'allocat': 'allocate', 'alocate': 'allocate', 'delocate': 'deallocate',
        'dealocate': 'deallocate', 'dealocat': 'deallocate', 'fre': 'free', 'fee': 'free',
    }
    abbrev_corrections = {
        'conf': 'configuration', 'cfg': 'configuration', 'inf': 'information',
        'mn': 'minimum', 'ave': 'average', 'rqst': 'request', 'rsp': 'response',
        'er': 'error', 'ex': 'exception', 'ini': 'initialize',
    }

    words = name.split('_') if '_' in name else [name]
    corrected = []
    for w in words:
        cw = w
        lw = w.lower()
        if lw in spelling_corrections:
            cw = spelling_corrections[lw]
        elif lw in abbrev_corrections:
            cw = abbrev_corrections[lw]
        corrected.append(cw)
    return '_'.join(corrected) if len(corrected) > 1 else corrected[0]


def to_macro_style(name):
    """Convert identifier to UPPER_CASE macro style."""
    if name.isupper():
        return name
    result = []
    for i, c in enumerate(name):
        if i == 0:
            result.append(c.upper())
        elif c.isupper():
            prev = name[i - 1]
            if ((prev.islower() or prev.isdigit()) or
               (i + 1 < len(name) and name[i + 1].islower())) and prev != '_':
                result.append('_' + c.upper())
            else:
                result.append(c.upper())
        elif c == '_':
            if i == 0 or result[-1] != '_':
                result.append('_')
        else:
            result.append(c.upper())
    return re.sub(r'_+', '_', ''.join(result))


def format_identifiers_in_project(project_root, file_path):
    """Format all identifiers in the given file across the whole project."""
    print(f"  开始格式化标识符...")
    identifiers, macros = find_identifiers_in_file(file_path)
    if not identifiers and not macros:
        print(f"  没有找到需要格式化的标识符")
        return True

    total = len(identifiers) + len(macros)
    print(f"  找到 {total} 个需要格式化的标识符（{len(identifiers)} 变量/函数，{len(macros)} 宏）")

    # 按长度倒序替换，避免子串冲突
    for o in sorted(identifiers, key=len, reverse=True):
        n = to_linux_style(o)
        if o == n:
            continue
        print(f"    变量/函数: {o} -> {n}")
        if not replace_identifier_in_project(project_root, o, n):
            print(f"      警告: 替换 {o} 时出错")
            return False

    for o in sorted(macros, key=len, reverse=True):
        n = to_macro_style(o)
        if o == n:
            continue
        print(f"    宏: {o} -> {n}")
        if not replace_macro_in_project(project_root, o, n):
            print(f"      警告: 替换宏 {o} 时出错")
            return False

    print(f"  标识符格式化完成")
    return True


def _replace_in_files(project_root, patterns_fn):
    """Generic helper: apply patterns_fn to each source file's content."""
    sources = find_source_files(project_root)
    for fp in sources:
        content, enc = read_file_content(fp)
        if content is None:
            continue
        new_content = patterns_fn(content)
        if new_content != content:
            write_file(fp, new_content, enc)


def replace_identifier_in_project(project_root, old, new):
    """Replace an identifier across all project source files."""
    pattern = re.compile(r'\b' + re.escape(old) + r'\b')
    try:
        _replace_in_files(project_root, lambda c: pattern.sub(new, c))
        return True
    except Exception as e:
        print(f"      替换 {old} 时出错: {e}")
        return False


def replace_macro_in_project(project_root, old, new):
    """Replace a macro name across all project source files."""
    try:
        def replacer(content):
            c = re.sub(r'^(#\s*define\s+)' + re.escape(old) + r'\b',
                       r'\1' + new, content, flags=re.MULTILINE)
            c = re.sub(r'^(#\s*(?:ifdef|ifndef|if\s+defined)\s*\(\s*)' + re.escape(old) + r'\s*\)',
                       r'\1' + new + r')', c, flags=re.MULTILINE)
            c = re.sub(r'^(#\s*undef\s+)' + re.escape(old) + r'\b',
                       r'\1' + new, c, flags=re.MULTILINE)
            c = re.sub(r'\b' + re.escape(old) + r'(?!\s*\()', new, c)
            return c
        _replace_in_files(project_root, replacer)
        return True
    except Exception as e:
        print(f"      替换宏 {old} 时出错: {e}")
        return False


# =============================================================================
# astyle Integration
# =============================================================================

def build_astyle_options(style='linux', indent=4, max_length=120):
    """Build astyle command-line options list."""
    return [
        f'--style={style}',
        f'--indent=force-tab={indent}',
        '--attach-closing-while',
        '--indent-col1-comments',
        '--max-continuation-indent=100',
        '--pad-comma',
        '--pad-include',
        '--squeeze-lines=1',
        '--break-after-logical',
        '--indent-preproc-define',
        '--pad-oper',
        '--break-one-line-headers',
        '--pad-header',
        '--indent-after-parens',
        '--unpad-paren',
        '--add-braces',
        '--align-pointer=name',
        '--align-reference=name',
        '--suffix=none',
    ]


def format_with_astyle(file_path, astyle_path='astyle', astyle_options=None):
    """Format a single file using astyle."""
    if astyle_options is None:
        astyle_options = build_astyle_options()

    cmd = [astyle_path] + astyle_options + [file_path]
    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True, timeout=30)
        return result.returncode == 0, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return False, "", "astyle processing timeout"
    except FileNotFoundError:
        return False, "", f"astyle not found: {astyle_path}"
    except Exception as e:
        return False, "", f"Error executing astyle: {e}"


# =============================================================================
# Directory Processing Pipeline
# =============================================================================

def process_file_pipeline(file_path, astyle_path, astyle_options, project_root, dry_run,
                          align_defines, move_comments, format_ids):
    """Run the full processing pipeline on a single file.

    Steps (dry_run=False):
      1. astyle pass 1
      2. #define alignment
      3. Move trailing comments
      4. Format identifiers (project-wide)
      5. astyle pass 2
    """
    if dry_run:
        return True, None

    # Step 1
    ok, out, err = format_with_astyle(file_path, astyle_path, astyle_options)
    if not ok:
        print(f"  astyle error: {err}")
        return False, None

    # Step 2
    if align_defines:
        align_defines_in_file(file_path)

    # Step 3
    if move_comments:
        move_trailing_comments(file_path)

    # Step 4
    if format_ids:
        format_identifiers_in_project(project_root, file_path)

    # Step 5
    ok2, _, err2 = format_with_astyle(file_path, astyle_path, astyle_options)
    if not ok2:
        print(f"  Second astyle error: {err2}")
        return False, None

    return True, out


def format_directory(directory, astyle_path='astyle', astyle_options=None, dry_run=False,
                     align_defines=True, move_comments=True, format_ids=True):
    """Format all C/C++ files in directory."""
    print(f"Searching directory: {directory}")
    sources = find_source_files(directory)
    if not sources:
        print("No source code files found")
        return 0, 0, [], {}

    total = len(sources)
    print(f"Found {total} source code files")

    success_count = 0
    failed_files = []
    stats = {
        'align_defines': 0,
        'comments_moved': 0,
        'files_with_ids': 0,
        'total_ids': 0,
        'total_funcs': 0,
        'total_vars': 0,
        'total_macros': 0,
    }

    for idx, fp in enumerate(sources, 1):
        print(f"[{idx}/{total}] Processing: {fp}")
        ok, _ = process_file_pipeline(
            fp, astyle_path, astyle_options, directory, dry_run,
            align_defines, move_comments, format_ids)
        if ok:
            success_count += 1
            if not dry_run:
                # 统计信息
                ids, macros = find_identifiers_in_file(fp)
                if ids or macros:
                    stats['files_with_ids'] += 1
                    stats['total_ids'] += len(ids) + len(macros)
                    stats['total_macros'] += len(macros)
                    try:
                        with open(fp, 'r', encoding='utf-8') as f:
                            content = f.read()
                        funcs = set()
                        for m in re.finditer(r'\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)\s*\{', content):
                            if m.group(1) not in {'if', 'for', 'while', 'switch', 'catch', 'try'}:
                                funcs.add(m.group(1))
                        in_ids = funcs & ids
                        stats['total_funcs'] += len(in_ids)
                        stats['total_vars'] += len(ids - in_ids)
                    except Exception:
                        stats['total_vars'] += len(ids)

        else:
            failed_files.append(fp)

    return total, success_count, failed_files, stats


def format_multiple_directories(directories, astyle_path='astyle', astyle_options=None,
                                dry_run=False, align_defines=True, move_comments=True,
                                format_ids=True):
    """Process multiple directories."""
    total_files = 0
    total_ok = 0
    all_failed = []
    combined = {
        'align_defines': 0, 'comments_moved': 0, 'files_with_ids': 0,
        'total_ids': 0, 'total_funcs': 0, 'total_vars': 0, 'total_macros': 0,
    }

    for d in directories:
        print(f"\n{'=' * 60}")
        print(f"Processing: {os.path.abspath(d)}")
        print(f"{'=' * 60}")

        if not os.path.isdir(d):
            print(f"Error: '{d}' is not a directory, skipping")
            continue

        t, ok, failed, s = format_directory(
            d, astyle_path, astyle_options, dry_run,
            align_defines, move_comments, format_ids)
        total_files += t
        total_ok += ok
        all_failed.extend(failed)
        for k in combined:
            combined[k] += s.get(k, 0)

    return total_files, total_ok, all_failed, combined


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description='Format C/C++ code in entire folders using astyle',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=r"""
Examples:
  python format_code.py ./src
  python format_code.py ./src ./lib --dry-run
  python format_code.py . --astyle /usr/bin/astyle --style=allman

Pipeline (order):
  1. astyle code formatting
  2. #define value alignment
  3. Move trailing comments to line above
  4. Format identifiers to Linux style
  5. Second astyle pass
  6. Format filenames to Linux style (optional)
""")

    parser.add_argument('directories', nargs='*', default=['.'],
                        help='Directories to format (default: current dir)')

    parser.add_argument('--astyle', default='astyle', help='astyle executable path')
    parser.add_argument('--style', choices=['attach', 'allman', 'gnu', 'google', 'linux'],
                        default='linux', help='Code style (default: linux)')
    parser.add_argument('--indent', type=int, default=4, help='Indent width (default: 4)')
    parser.add_argument('--max-length', type=int, default=120,
                        help='Max line length (default: 120)')

    parser.add_argument('--dry-run', action='store_true', help='Preview only, no changes')

    parser.add_argument('--extensions', nargs='+', default=C_EXTENSIONS,
                        help='File extensions to process')

    parser.add_argument('--align-defines', action='store_true', default=True,
                        help='Align #define values [default: enabled]')
    parser.add_argument('--no-align-defines', action='store_false', dest='align_defines',
                        help='Disable #define alignment')

    parser.add_argument('--move-comments', action='store_true', default=True,
                        help='Move trailing comments [default: enabled]')
    parser.add_argument('--no-move-comments', action='store_false', dest='move_comments')

    parser.add_argument('--format-ids', action='store_true', default=True,
                        help='Format identifiers [default: enabled]')
    parser.add_argument('--no-format-ids', action='store_false', dest='format_ids')

    parser.add_argument('--format-filenames', action='store_true', default=True,
                        help='Format filenames [default: enabled]')
    parser.add_argument('--no-format-filenames', action='store_false', dest='format_filenames')

    args = parser.parse_args()
    astyle_opts = build_astyle_options(args.style, args.indent, args.max_length)

    print("=" * 60)
    print("C/C++ Code Formatting Tool")
    print("=" * 60)
    for d in args.directories:
        print(f"  Target: {os.path.abspath(d)}")
    print(f"  Style: {args.style}, Indent: {args.indent}, Max-length: {args.max_length}")
    print(f"  Extensions: {', '.join(args.extensions)}")
    print(f"  Dry-run: {args.dry_run}")
    print(f"  Pipeline steps: astyle → #define-align → comments → identifiers → astyle")
    print(f"  Filename formatting: {args.format_filenames}")
    print("=" * 60)

    # Steps 1-5
    total, success, failed, stats = format_multiple_directories(
        args.directories, args.astyle, astyle_opts, args.dry_run,
        args.align_defines, args.move_comments, args.format_ids)

    # Step 6: filenames
    renamed_total = 0
    found_total = 0
    if args.format_filenames:
        for d in args.directories:
            if not os.path.isdir(d):
                continue
            print(f"\n{'=' * 60}")
            print(f"Filename formatting: {os.path.abspath(d)}")
            print(f"{'=' * 60}")
            ft, fr, _ = format_filenames_in_project(d, args.dry_run)
            found_total += ft
            renamed_total += fr

    # Summary
    print("\n" + "=" * 60)
    print("Completed!")
    print(f"  Code files: {total}")
    print(f"  Success: {success}")
    print(f"  Failed: {len(failed)}")
    if args.format_filenames:
        print(f"  Filenames found: {found_total}, renamed: {renamed_total}")

    if not args.dry_run:
        print(f"\n  Statistics:")
        print(f"    #define alignment: enabled" if args.align_defines else "    #define alignment: disabled")
        print(f"    Comments moved: {stats.get('comments_moved', 0)}")
        print(f"    Files with ID formatting: {stats.get('files_with_ids', 0)}")
        print(f"    IDs processed: {stats.get('total_ids', 0)} "
              f"(funcs {stats.get('total_funcs', 0)}, "
              f"vars {stats.get('total_vars', 0)}, "
              f"macros {stats.get('total_macros', 0)})")

    if failed:
        print("\nFailed files:")
        for fp in failed:
            print(f"  - {fp}")

    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
