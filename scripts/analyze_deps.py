#!/usr/bin/env python3
"""
Analyze header file dependencies for TARGET using GCC generated .d files
"""

import os
import re
import sys
from pathlib import Path


def parse_dependency_file(dep_file_path):
    """Parse a single .d dependency file and extract source and header dependencies"""
    try:
        with open(dep_file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        # GCC .d file format: target.o: source.c header1.h header2.h
        # Remove line continuations (\ )
        content = content.replace('\\\n', ' ').replace('\\\r\n', ' ')

        # Split by colon to get the dependency list
        parts = content.split(':')
        if len(parts) < 2:
            return None, []

        # Extract source file (first part after target)
        dep_string = ':'.join(parts[1:]).strip()
        files = [f.strip() for f in dep_string.split() if f.strip()
                 and not f.strip().endswith(':')]

        if not files:
            return None, []

        # First file is the source file, rest are headers
        source_file = os.path.abspath(
            files[0]) if files[0].endswith('.c') else None
        headers = [os.path.abspath(f) for f in files[1:] if f.endswith('.h')]

        return source_file, headers

    except Exception as e:
        print(f"Warning: Cannot parse {dep_file_path}: {e}")
        return None, []


def build_dependency_tree(dep_files, max_header_depth=2):
    """Build simplified dependency tree using .d files only"""
    dependency_tree = {}
    all_headers = set()

    # Collect direct dependencies from .d files
    source_to_headers = {}
    for dep_file in dep_files:
        source_file, headers = parse_dependency_file(dep_file)
        if source_file:
            source_to_headers[source_file] = headers
            all_headers.update(headers)

    # Use simple categorization instead of recursive parsing
    dependency_tree = source_to_headers
    return all_headers, dependency_tree


def find_dependency_files(build_dir):
    """Find all .d dependency files in the build directory"""
    dep_files = []
    obj_dir = os.path.join(build_dir, 'obj')

    if not os.path.exists(obj_dir):
        print(f"Warning: Object directory {obj_dir} does not exist")
        print("Please run 'make' first to generate dependency files")
        return dep_files

    for file in os.listdir(obj_dir):
        if file.endswith('.d'):
            dep_files.append(os.path.join(obj_dir, file))

    return sorted(dep_files)


def analyze_dependencies_from_d_files(build_dir):
    """Analyze dependencies from all .d files"""
    if not os.path.exists(build_dir):
        print(f"Error: Build directory {build_dir} does not exist")
        print("Please run 'make' first to generate build artifacts")
        return set(), {}

    dep_files = find_dependency_files(build_dir)

    if not dep_files:
        print("No dependency files found. Running 'make' first to generate them...")
        # Try to run make to generate dependency files
        make_cmd = "make"
        if os.name == 'nt':  # Windows
            make_cmd = "make"
        try:
            result = os.system(make_cmd)
            if result != 0:
                print("Failed to run make. Please run 'make' manually first.")
                return set(), {}
            dep_files = find_dependency_files(build_dir)
        except Exception as e:
            print(f"Failed to run make: {e}")
            return set(), {}

    if not dep_files:
        print("Still no dependency files found after attempting to build.")
        return set(), {}

    print(f"Found {len(dep_files)} dependency files")
    return build_dependency_tree(dep_files, max_header_depth=3)


def get_source_files_from_makefile(makefile_path):
    """Extract source file list from Makefile"""
    source_files = []

    try:
        with open(makefile_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Find C_SOURCES definitions
        pattern = r'C_SOURCES\s*\+=?\s*\$\(wildcard\s+([^\)]+)\)'
        matches = re.findall(pattern, content)

        makefile_dir = os.path.dirname(os.path.abspath(makefile_path))

        for match in matches:
            # Handle relative paths
            if match.startswith('../'):
                pattern_path = os.path.join(makefile_dir, match)
            else:
                pattern_path = match

            # Expand wildcard
            import glob
            files = glob.glob(pattern_path)
            source_files.extend(files)

        # Handle EXCLUDE_SOURCES
        exclude_pattern = r'EXCLUDE_SOURCES\s*\+=?\s*(.+)'
        exclude_matches = re.findall(exclude_pattern, content, re.MULTILINE)
        exclude_files = []

        for exclude in exclude_matches:
            if '$(wildcard' in exclude:
                # Handle wildcard exclusions
                wildcard_pattern = r'\$\(wildcard\s+([^\)]+)\)'
                wildcard_matches = re.findall(wildcard_pattern, exclude)
                for match in wildcard_matches:
                    if match.startswith('../'):
                        pattern_path = os.path.join(makefile_dir, match)
                    else:
                        pattern_path = match
                    files = glob.glob(pattern_path)
                    exclude_files.extend(files)
            else:
                # Handle direct path exclusions
                exclude_files.append(exclude.strip())

        # Filter out excluded files
        source_files = [f for f in source_files if f not in exclude_files]

    except Exception as e:
        print(f"Error reading Makefile: {e}")

    return source_files


def find_all_source_files(project_root):
    """Find all .c files in the project recursively"""
    all_c_files = set()
    try:
        for root, dirs, files in os.walk(project_root):
            # Skip build directories and hidden directories
            dirs[:] = [d for d in dirs if not d.startswith(
                '.') and 'build' not in d.lower() and 'output' not in d.lower()]

            for file in files:
                if file.endswith('.c'):
                    abs_path = os.path.abspath(os.path.join(root, file))
                    all_c_files.add(abs_path)
    except Exception as e:
        print(f"Error scanning for .c files: {e}")

    return all_c_files


def find_all_header_files(project_root):
    """Find all .h files in the project recursively"""
    all_h_files = set()
    try:
        for root, dirs, files in os.walk(project_root):
            # Skip build directories and hidden directories
            dirs[:] = [d for d in dirs if not d.startswith(
                '.') and 'build' not in d.lower() and 'output' not in d.lower()]

            for file in files:
                if file.endswith('.h'):
                    abs_path = os.path.abspath(os.path.join(root, file))
                    all_h_files.add(abs_path)
    except Exception as e:
        print(f"Error scanning for .h files: {e}")

    return all_h_files


def find_all_project_files(project_root):
    """Find all files in the project recursively"""
    all_files = set()
    try:
        for root, dirs, files in os.walk(project_root):
            # Skip build directories and hidden directories
            dirs[:] = [d for d in dirs if not d.startswith(
                '.') and 'build' not in d.lower() and 'output' not in d.lower()]

            for file in files:
                abs_path = os.path.abspath(os.path.join(root, file))
                all_files.add(abs_path)
    except Exception as e:
        print(f"Error scanning for all project files: {e}")

    return all_files


def generate_unused_report(project_root, all_headers, dependency_tree):
    """Generate report for unused files"""
    print("=== Unused Files Analysis ===")

    # Find all files in project
    project_all_files = find_all_project_files(project_root)
    project_headers = find_all_header_files(project_root)
    project_sources = find_all_source_files(project_root)

    # Convert dependent sets to absolute paths for comparison
    dependent_header_paths = set(os.path.abspath(h) for h in all_headers)
    dependent_source_paths = set()

    # Extract source files from dependency tree
    for source_file in dependency_tree.keys():
        dependent_source_paths.add(os.path.abspath(source_file))

    # Find unused headers
    unused_headers = project_headers - dependent_header_paths
    # Find unused sources (excluding main project files that might not be in build)
    unused_sources = project_sources - dependent_source_paths

    # Find all other unused files (not .c or .h)
    used_files = dependent_header_paths | dependent_source_paths
    all_code_files = project_headers | project_sources
    other_files = project_all_files - all_code_files
    # Since we don't have dependency info for other files
    unused_other_files = other_files

    # Categorize other files by type
    file_extensions = {}
    for file_path in unused_other_files:
        ext = os.path.splitext(file_path)[1].lower()
        if ext == '':
            ext = 'no_extension'
        if ext not in file_extensions:
            file_extensions[ext] = []
        file_extensions[ext].append(file_path)

    print(f"📊 Files Summary:")
    print(f"  Total headers found in project: {len(project_headers)}")
    print(f"  Headers used in dependencies: {len(dependent_header_paths)}")
    print(f"  Unused headers: {len(unused_headers)}")
    print(f"  Total source files found in project: {len(project_sources)}")
    print(f"  Source files used in build: {len(dependent_source_paths)}")
    print(f"  Potentially unused sources: {len(unused_sources)}")
    print(f"  Other files in project: {len(other_files)}")

    # Show file type breakdown
    if file_extensions:
        print(f"\n📁 Other Files by Type:")
        for ext, files in sorted(file_extensions.items()):
            print(f"  {ext or 'no_ext'}: {len(files)} files")

    return unused_headers, unused_sources, project_headers, project_sources, dependent_header_paths, dependent_source_paths, unused_other_files, file_extensions


def print_unused_files_report(unused_headers, unused_sources, unused_other_files=None, file_extensions=None):
    """Print detailed unused files report"""
    if unused_headers:
        print(f"\n🔍 Unused Header Files ({len(unused_headers)}):")
        for header in sorted(unused_headers):
            relative_path = os.path.relpath(
                header, start=os.path.dirname(os.path.abspath('.')))
            print(f"  ❌ {relative_path}")
    else:
        print(f"✅ All header files are being used!")

    if unused_sources:
        print(f"\n🔍 Potentially Unused Source Files ({len(unused_sources)}):")
        for source in sorted(unused_sources):
            relative_path = os.path.relpath(
                source, start=os.path.dirname(os.path.abspath('.')))
            # Filter out obvious non-build files like templates, examples, etc.
            if not any(keyword in source.lower() for keyword in ['example', 'template', 'test', 'demo']):
                print(f"  ⚠️  {relative_path}")

        if len(unused_sources) > 0:
            filtered_unused = [s for s in unused_sources if not any(
                keyword in s.lower() for keyword in ['example', 'template', 'test', 'demo'])]
            if len(filtered_unused) != len(unused_sources):
                print(f"  📝 Note: Some files filtered out (examples, tests, templates)")
                print(
                    f"  🎯 Potentially unused for production: {len(filtered_unused)}")
    else:
        print(f"✅ All source files are being used!")

    # Print other files
    if unused_other_files and file_extensions:
        print(f"\n🔍 Other Files in Project ({len(unused_other_files)}):")
        for ext, files in sorted(file_extensions.items()):
            ext_name = ext if ext != 'no_extension' else 'no extension'
            print(f"\n  📄 {ext_name} files ({len(files)}):")
            for file_path in sorted(files)[:10]:  # Show max 10 per type
                relative_path = os.path.relpath(
                    file_path, start=os.path.dirname(os.path.abspath('.')))
                print(f"    📝 {relative_path}")
            if len(files) > 10:
                print(f"    ... and {len(files) - 10} more {ext_name} files")


def delete_unused_files(unused_headers, unused_sources, unused_other_files=None, exclude_paths=None):
    """Delete unused files with certain exclusions"""
    if exclude_paths is None:
        exclude_paths = []

    # Convert exclude paths to absolute paths for comparison
    exclude_abs_paths = [os.path.abspath(path) for path in exclude_paths]

    deleted_files = []
    failed_deletes = []

    # Delete unused headers
    for header in unused_headers:
        # Check if file should be excluded
        should_exclude = False
        for exclude_path in exclude_abs_paths:
            if header.startswith(exclude_path):
                should_exclude = True
                print(f"  🚫 Excluding header: {header}")
                break

        if not should_exclude and os.path.exists(header):
            try:
                os.remove(header)
                deleted_files.append(header)
                print(f"  🗑️  Deleted: {header}")
            except Exception as e:
                failed_deletes.append((header, str(e)))
                print(f"  ❌ Failed to delete {header}: {e}")

    # Delete unused sources
    for source in unused_sources:
        # Check if file should be excluded
        should_exclude = False
        for exclude_path in exclude_abs_paths:
            if source.startswith(exclude_path):
                should_exclude = True
                print(f"  🚫 Excluding source: {source}")
                break

        if not should_exclude and os.path.exists(source):
            try:
                os.remove(source)
                deleted_files.append(source)
                print(f"  🗑️  Deleted: {source}")
            except Exception as e:
                failed_deletes.append((source, str(e)))
                print(f"  ❌ Failed to delete {source}: {e}")

    # Delete other unused files
    if unused_other_files:
        for other_file in unused_other_files:
            # Check if file should be excluded
            should_exclude = False
            for exclude_path in exclude_abs_paths:
                if other_file.startswith(exclude_path):
                    should_exclude = True
                    print(f"  🚫 Excluding other file: {other_file}")
                    break

            if not should_exclude and os.path.exists(other_file):
                try:
                    os.remove(other_file)
                    deleted_files.append(other_file)
                    print(f"  🗑️  Deleted: {other_file}")
                except Exception as e:
                    failed_deletes.append((other_file, str(e)))
                    print(f"  ❌ Failed to delete {other_file}: {e}")

    return deleted_files, failed_deletes


def delete_empty_directories():
    """Delete empty directories in the project"""
    print()
    print("=== Cleaning Up Empty Directories ===")

    project_root = os.path.abspath("..")
    deleted_dirs = []
    failed_dirs = []

    # Define exclude paths
    exclude_paths = [
        r"E:\Code\Kinetis\drivers\kinetis",
        r"E:\Code\Kinetis\include\kinetis",
        r"E:\Code\Kinetis\.git",
        r"E:\Code\Kinetis\.vscode",
        r"E:\Code\Kinetis\scripts"
    ]
    exclude_abs_paths = [os.path.abspath(path) for path in exclude_paths]

    # Walk from bottom up to handle nested directories
    for root, dirs, files in os.walk(project_root, topdown=False):
        # Skip build directories and hidden directories
        dirs[:] = [d for d in dirs if not d.startswith(
            '.') and 'build' not in d.lower() and 'output' not in d.lower()]

        # Skip scripts directory itself and excluded paths
        if root == os.path.abspath("."):
            continue

        # Skip directories in excluded paths
        should_skip = False
        for exclude_path in exclude_abs_paths:
            if root.startswith(exclude_path):
                should_skip = True
                break

        if should_skip:
            continue

        for dir_name in dirs:
            dir_path = os.path.join(root, dir_name)

            # Check if directory is empty (no files and no subdirectories)
            try:
                if not os.listdir(dir_path):
                    os.rmdir(dir_path)
                    deleted_dirs.append(dir_path)
                    print(f"  🗂️  Deleted empty directory: {dir_path}")
            except Exception as e:
                failed_dirs.append((dir_path, str(e)))
                print(f"  ❌ Failed to delete directory {dir_path}: {e}")

    return deleted_dirs, failed_dirs


def save_unused_files_report(unused_headers, unused_sources, unused_other_files=None, file_extensions=None, output_file="unused_files_report.txt"):
    """Save unused files report to file"""
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("=== Unused Files Analysis Report ===\n\n")

        f.write(f"Summary:\n")
        f.write(f"  Unused headers: {len(unused_headers)}\n")
        f.write(f"  Potentially unused sources: {len(unused_sources)}\n")
        if unused_other_files:
            f.write(f"  Other files: {len(unused_other_files)}\n")
        f.write("\n")

        if unused_headers:
            f.write("Unused Header Files:\n")
            for header in sorted(unused_headers):
                f.write(f"  {header}\n")
            f.write("\n")

        if unused_sources:
            f.write("Potentially Unused Source Files:\n")
            for source in sorted(unused_sources):
                f.write(f"  {source}\n")
            f.write("\n")

        if unused_other_files and file_extensions:
            f.write("Other Files by Type:\n")
            for ext, files in sorted(file_extensions.items()):
                ext_name = ext if ext != 'no_extension' else 'no extension'
                f.write(f"\n  {ext_name} files ({len(files)}):\n")
                for file_path in sorted(files):
                    f.write(f"    {file_path}\n")

    print(f"\n📄 Unused files report saved to: {output_file}")


def delete_unused_files_from_report():
    """Delete files listed in unused_files_report.txt with exclusions"""
    report_file = "unused_files_report.txt"

    if not os.path.exists(report_file):
        print(
            f"❌ Report file {report_file} not found. Please run dependency analysis first.")
        return

    # Define exclude paths
    exclude_paths = [
        r"E:\Code\Kinetis\drivers\kinetis",
        r"E:\Code\Kinetis\include\kinetis",
        r"E:\Code\Kinetis\fs\fatfs",
        r"E:\Code\Kinetis\.git",
        r"E:\Code\Kinetis\.vscode",
        r"E:\Code\Kinetis\scripts"
    ]

    print("=== Deleting Unused Files ===")
    print(f"Reading from: {report_file}")
    print(f"Excluding paths: {exclude_paths}")
    print()

    # Parse unused files from report
    unused_headers = set()
    unused_sources = set()
    unused_other_files = set()

    current_section = None
    try:
        with open(report_file, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.rstrip('\n\r')  # Remove only line endings
                stripped_line = line.lstrip()
                
                # Identify section headers
                if stripped_line.startswith("Unused Header Files:"):
                    current_section = "headers"
                elif stripped_line.startswith("Potentially Unused Source Files:"):
                    current_section = "sources"
                elif stripped_line.startswith("Other Files in Project ("):
                    current_section = "others"
                elif line.startswith("  ") and not line.startswith("  Summary:") and not line.startswith("  Unused headers:") and not line.startswith("  Potentially unused sources:") and not line.startswith("  Other files:"):
                    # This is a file entry line (indented with at least 2 spaces)
                    # Remove leading spaces
                    file_entry = line.lstrip()
                    
                    # Handle emoji prefixes if present
                    if file_entry.startswith('❌ ') or file_entry.startswith('⚠️  ') or file_entry.startswith('📝 '):
                        file_path = file_entry.split(' ', 1)[1]  # Get everything after the emoji
                    else:
                        file_path = file_entry
                    
                    # Convert to absolute path
                    abs_path = os.path.abspath(file_path)
                    
                    # Add to appropriate collection based on current section
                    if current_section == "headers":
                        unused_headers.add(abs_path)
                    elif current_section == "sources":
                        unused_sources.add(abs_path)
                    elif current_section == "others":
                        # For other files, they have 4-space indent
                        if line.startswith("    "):
                            unused_other_files.add(abs_path)

    except Exception as e:
        print(f"❌ Error processing report file: {e}")
        return

    print(f"Found {len(unused_headers)} unused headers, {len(unused_sources)} unused sources, and {len(unused_other_files)} other files in report")

    # Delete files with exclusions
    deleted_files, failed_deletes = delete_unused_files(
        unused_headers, unused_sources, unused_other_files, exclude_paths)

    print()
    print("=== Deletion Summary ===")
    print(f"  ✅ Successfully deleted: {len(deleted_files)} files")
    print(f"  ❌ Failed to delete: {len(failed_deletes)} files")

    if failed_deletes:
        print()
        print("Failed deletions:")
        for file_path, error in failed_deletes:
            print(f"  {file_path}: {error}")

    if deleted_files:
        print()
        print("Successfully deleted files:")
        for file_path in deleted_files:
            print(f"  {file_path}")

    # Clean up empty directories
    deleted_dirs, failed_dirs = delete_empty_directories()

    print()
    print("=== Final Cleanup Summary ===")
    print(f"  🗑️  Files deleted: {len(deleted_files)}")
    print(f"  🗂️  Empty directories deleted: {len(deleted_dirs)}")
    if failed_deletes or failed_dirs:
        print(
            f"  ❌ Failed operations: {len(failed_deletes) + len(failed_dirs)}")


def main():
    """Main function"""
    # Check for command line arguments
    if len(sys.argv) > 1:
        if sys.argv[1] == "--delete-unused":
            delete_unused_files_from_report()
            return
        else:
            print("Usage:")
            print("  python analyze_deps.py              - Run dependency analysis")
            print(
                "  python analyze_deps.py --delete-unused - Delete unused files from report")
            return

    # Get Makefile path
    makefile_path = "Makefile"
    if not os.path.exists(makefile_path):
        print("Error: Makefile not found in current directory")
        sys.exit(1)

    print("=== Analyzing TARGET Header Dependencies (from GCC .d files) ===")
    print(f"Makefile: {os.path.abspath(makefile_path)}")
    print()

    # Get build directory from Makefile
    try:
        with open(makefile_path, 'r', encoding='utf-8') as f:
            content = f.read()

        build_dir_match = re.search(r'BUILD_DIR\s*=\s*(\S+)', content)
        build_dir = build_dir_match.group(1) if build_dir_match else "output"
        print(f"Build directory: {os.path.abspath(build_dir)}")
    except:
        build_dir = "output"
        print(f"Using default build directory: {os.path.abspath(build_dir)}")

    print()

    # Analyze dependencies from .d files
    print("1. Analyzing dependencies from GCC generated .d files...")
    all_headers, dependency_tree = analyze_dependencies_from_d_files(build_dir)

    if not all_headers:
        print("No dependencies found. Please ensure:")
        print("1. You have run 'make' at least once to generate dependency files")
        print("2. The build completed successfully")
        print("3. The DEPFLAGS (-MMD -MP) is enabled in your Makefile")
        return

    print()
    print("=== Analysis Results ===")
    print(f"Total {len(all_headers)} header files are being depended on")
    print(f"Total {len(dependency_tree)} source files analyzed")
    print()

    # Show most dependent header files
    header_count = {}
    header_full_paths = {}  # To track which full path corresponds to each header name

    for headers in dependency_tree.values():
        for header in headers:
            header_name = os.path.basename(header)
            # Count by unique header name, but store the first full path we encounter
            if header_name not in header_count:
                header_count[header_name] = 0
                header_full_paths[header_name] = header
            header_count[header_name] += 1

    print("Most depended-on header files (top 15):")
    sorted_headers = sorted(header_count.items(),
                            key=lambda x: x[1], reverse=True)

    # Find the maximum count for alignment
    max_count = max(
        count for _, count in sorted_headers[:15]) if sorted_headers else 0
    max_count_width = len(str(max_count))

    # Find the maximum header name length for alignment
    max_header_name_width = max(len(
        header_name) for header_name, _ in sorted_headers[:15]) if sorted_headers else 0

    for header_name, count in sorted_headers[:15]:
        full_path = header_full_paths[header_name]
        print(
            f"  {header_name:<{max_header_name_width}}: {count:>{max_count_width}} times ({full_path})")

    print()
    print(f"Unique header files by location:")
    # Group headers by directory
    header_dirs = {}
    for header in all_headers:
        header_dir = os.path.dirname(header)
        header_name = os.path.basename(header)
        if header_dir not in header_dirs:
            header_dirs[header_dir] = []
        header_dirs[header_dir].append(header_name)

    for dir_path, headers in sorted(header_dirs.items()):
        print(f"  {dir_path}: {len(headers)} files")
        if headers:
            # Group headers by lines for compact display
            headers_per_line = 4  # Number of headers per line
            display_headers = sorted(headers)[:5] if len(
                headers) > 5 else sorted(headers)

            for i in range(0, len(display_headers), headers_per_line):
                line_headers = display_headers[i:i + headers_per_line]
                if len(line_headers) == headers_per_line:
                    # Full line - display all in one line with alignment
                    print(f"    - " +
                          "  ".join(f"{h:<20}" for h in line_headers))
                else:
                    # Last line with fewer items - display separately
                    for header in line_headers:
                        print(f"    - {header}")

            if len(headers) > 5:
                print(f"    ... and {len(headers) - 5} more files")

    print()
    print("=== Source Files Being Used ===")

    # Find the maximum lengths for alignment
    max_source_name_width = max(len(os.path.basename(
        source)) for source in dependency_tree.keys()) if dependency_tree else 0
    max_path_width = 0
    for source_file in dependency_tree.keys():
        relative_path = os.path.relpath(
            source_file, start=os.path.dirname(os.path.abspath('..')))
        max_path_width = max(max_path_width, len(relative_path))

    max_header_count_width = max(len(str(len(
        dependency_tree[source]))) for source in dependency_tree.keys()) if dependency_tree else 0

    for source_file in sorted(dependency_tree.keys()):
        source_name = os.path.basename(source_file)
        # Get relative path from project root (parent of scripts directory)
        relative_path = os.path.relpath(
            source_file, start=os.path.dirname(os.path.abspath('..')))
        header_count = len(dependency_tree[source_file])
        print(f"  {source_name:<{max_source_name_width}} ({relative_path:<{max_path_width}}): depends on {header_count:>{max_header_count_width}} headers")

    print()
    print("=== Dependencies Grouped by Source Files ===")
    for source_file, headers in sorted(dependency_tree.items()):
        if headers:
            source_name = os.path.basename(source_file)
            print(f"\n{source_name}: {len(headers)} headers")
            header_names = [os.path.basename(header)
                            for header in sorted(headers)]

            # Group header names by lines for compact display
            headers_per_line = 4  # Number of headers per line
            for i in range(0, len(header_names), headers_per_line):
                line_headers = header_names[i:i + headers_per_line]
                if len(line_headers) == headers_per_line:
                    # Full line - display all in one line with alignment
                    print(
                        f"  - " + "  ".join(f"{h:<20}" for h in line_headers))
                else:
                    # Last line with fewer items - display separately
                    for header_name in line_headers:
                        print(f"  - {header_name}")

    print()
    print("=== Dependencies Grouped by Source Files (Flat List) ===")
    for source_file, headers in sorted(dependency_tree.items()):
        if isinstance(headers, dict):
            # Convert tree back to flat list for summary
            flat_headers = []

            def collect_headers(tree):
                for name, data in tree.items():
                    flat_headers.append(data['path'])
                    if data.get('children'):
                        collect_headers(data['children'])
            collect_headers(headers)
            headers = flat_headers

            if headers:
                source_name = os.path.basename(source_file)
                print(f"\n{source_name}: {len(headers)} headers")
                header_names = [os.path.basename(
                    header) for header in sorted(headers)]

                # Group header names by lines for compact display
                headers_per_line = 4  # Number of headers per line
                for i in range(0, len(header_names), headers_per_line):
                    line_headers = header_names[i:i + headers_per_line]
                    if len(line_headers) == headers_per_line:
                        # Full line - display all in one line with alignment
                        print(
                            f"  - " + "  ".join(f"{h:<20}" for h in line_headers))
                    else:
                        # Last line with fewer items - display separately
                        for header_name in line_headers:
                            print(f"  - {header_name}")

    # Save results to file
    output_file = "dependency_analysis_report.txt"
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(
            "=== TARGET Header File Dependency Analysis Results (from GCC .d files) ===\n\n")
        f.write(
            f"Total {len(all_headers)} header files are being depended on\n")
        f.write(f"Total {len(dependency_tree)} source files analyzed\n\n")

        # Add list of source files that are being depended on
        f.write("=== Source Files Being Used ===\n")

        # Find the maximum lengths for alignment
        max_source_name_width = max(len(os.path.basename(
            source)) for source in dependency_tree.keys()) if dependency_tree else 0
        max_path_width = 0
        for source_file in dependency_tree.keys():
            relative_path = os.path.relpath(
                source_file, start=os.path.dirname(os.path.abspath('..')))
            max_path_width = max(max_path_width, len(relative_path))

        max_header_count_width = max(len(str(len(
            dependency_tree[source]))) for source in dependency_tree.keys()) if dependency_tree else 0

        for source_file in sorted(dependency_tree.keys()):
            source_name = os.path.basename(source_file)
            # Get relative path from project root (parent of scripts directory)
            relative_path = os.path.relpath(
                source_file, start=os.path.dirname(os.path.abspath('..')))
            header_count = len(dependency_tree[source_file])
            f.write(f"  {source_name:<{max_source_name_width}} ({relative_path:<{max_path_width}}): depends on {header_count:>{max_header_count_width}} headers\n")
        f.write("\n")

        f.write("Most depended-on header files:\n")

        # Find the maximum count for alignment in the file output
        max_count = max(
            count for _, count in sorted_headers) if sorted_headers else 0
        max_count_width = len(str(max_count))

        # Find the maximum header name length for alignment in file output
        max_header_name_width = max(
            len(header_name) for header_name, _ in sorted_headers) if sorted_headers else 0

        for header_name, count in sorted_headers:
            full_path = header_full_paths[header_name]
            f.write(
                f"  {header_name:<{max_header_name_width}}: {count:>{max_count_width}} times ({full_path})\n")

        f.write(f"\nUnique header files by location:\n")
        for dir_path, headers in sorted(header_dirs.items()):
            f.write(f"\n  {dir_path}: {len(headers)} files\n")
            if headers:
                # Group headers by lines for compact display
                headers_per_line = 4  # Number of headers per line
                for i in range(0, len(headers), headers_per_line):
                    line_headers = headers[i:i + headers_per_line]
                    if len(line_headers) == headers_per_line:
                        # Full line - display all in one line
                        f.write(
                            f"    - " + ", ".join(f"{h:<20}" for h in line_headers) + "\n")
                    else:
                        # Last line with fewer items - display separately
                        for header in line_headers:
                            f.write(f"    - {header}\n")

        f.write("\nDependencies grouped by source files:\n")
        for source_file, headers in sorted(dependency_tree.items()):
            if headers:
                source_name = os.path.basename(source_file)
                f.write(f"\n{source_name}: {len(headers)} headers\n")
                header_names = [os.path.basename(
                    header) for header in sorted(headers)]

                # Group header names by lines for compact display
                headers_per_line = 4  # Number of headers per line
                for i in range(0, len(header_names), headers_per_line):
                    line_headers = header_names[i:i + headers_per_line]
                    if len(line_headers) == headers_per_line:
                        # Full line - display all in one line with alignment
                        f.write(
                            "  - " + "  ".join(f"{h:<20}" for h in line_headers) + "\n")
                    else:
                        # Last line with fewer items - display separately
                        for header_name in line_headers:
                            f.write(f"  - {header_name}\n")

        f.write("\nDependencies grouped by source files (flat list):\n")
        for source_file, headers in sorted(dependency_tree.items()):
            if isinstance(headers, dict):
                # Convert tree back to flat list for summary
                flat_headers = []

                def collect_headers(tree):
                    for name, data in tree.items():
                        if 'path' in data:
                            flat_headers.append(data['path'])
                            if data.get('children'):
                                collect_headers(data['children'])
                collect_headers(headers)
                headers = flat_headers

            if headers:
                source_name = os.path.basename(source_file)
                f.write(f"\n{source_name}: {len(headers)} headers\n")
                header_names = [os.path.basename(
                    header) for header in sorted(headers)]

                # Group header names by lines for compact display
                headers_per_line = 4  # Number of headers per line
                for i in range(0, len(header_names), headers_per_line):
                    line_headers = header_names[i:i + headers_per_line]
                    if len(line_headers) == headers_per_line:
                        # Full line - display all in one line with alignment
                        f.write(
                            "  - " + "  ".join(f"{h:<20}" for h in line_headers) + "\n")
                    else:
                        # Last line with fewer items - display separately
                        for header_name in line_headers:
                            f.write(f"  - {header_name}\n")

        f.write(f"\n\nFull paths of all dependent headers:\n")
        for header in sorted(all_headers):
            f.write(f"  {header}\n")

    print(f"Results saved to: {output_file}")
    print("Note: This analysis includes nested dependencies found by GCC's preprocessor.")

    # === NEW: Unused Files Analysis ===
    print("2. Analyzing unused files...")

    # Get project root directory (parent of scripts directory)
    project_root = os.path.abspath("..")
    print(f"Scanning project root: {project_root}")

    # Generate unused files report
    unused_headers, unused_sources, project_headers, project_sources, dependent_header_paths, dependent_source_paths, unused_other_files, file_extensions = generate_unused_report(
        project_root, all_headers, dependency_tree)

    # Print detailed unused files report
    print_unused_files_report(
        unused_headers, unused_sources, unused_other_files, file_extensions)

    # Save unused files report
    unused_report_file = "unused_files_report.txt"
    save_unused_files_report(
        unused_headers, unused_sources, unused_other_files, file_extensions, unused_report_file)
    
    print("=== Analysis Complete ===")
    print("Files generated:")
    print(f"  📄 {output_file} - Dependency analysis report")
    print(f"  📄 {unused_report_file} - Unused files analysis report")


if __name__ == "__main__":
    main()





