#!/usr/bin/env python3
"""
Generate sl_status_strings.h and sl_status_strings.c from sl_status.h using Jinja2 template.
Usage: main.py <input_dir> <output_dir>
"""
import argparse
import logging
import re
import subprocess
import sys
from pathlib import Path
from jinja2 import Environment, FileSystemLoader

logging.basicConfig(level=logging.WARNING, format='%(levelname)s: %(message)s')


class ClangFormat:
    def __init__(self):
        self._clang_format_path = self._find_clang_format()

    def _find_clang_format(self):
        """Find clang-format executable in the system PATH."""
        try:
            result = subprocess.run(
                ["which", "clang-format"], capture_output=True, text=True, check=True
            )
            return result.stdout.strip()
        except subprocess.CalledProcessError:
            logging.warning(
                "clang-format not found in PATH, formatting will be skipped"
            )
            return None

    def format(self, content, style=None, filename="file.h"):
        """Format C/C++ code using clang-format."""
        if not self._clang_format_path:
            logging.warning("clang-format not available, returning unformatted content")
            return content

        try:
            # Default style if none specified
            if style is None:
                style = "file"

            # Run clang-format with explicit language specification
            result = subprocess.run(
                [
                    self._clang_format_path,
                    f"--style={style}",
                    f"--assume-filename={filename}",
                ],
                input=content,
                capture_output=True,
                text=True,
                check=True,
            )
            return result.stdout
        except subprocess.CalledProcessError as e:
            logging.error(f"clang-format failed: {e}")
            logging.error(f"stderr: {e.stderr}")
            return content
        except Exception as e:
            logging.error(f"Error running clang-format: {e}")
            return content


def parse_sl_status_header(header_path):
    """
    Parse sl_status.h and extract status code definitions.
    Returns a list of dicts with 'name' and 'description' keys.
    """
    status_codes = []
    
    with open(header_path, 'r', encoding='utf-8') as f:
        for line in f:
            # Match: #define SL_STATUS_XXX ((sl_status_t)0xYYYY)  ///< Description.
            # Pattern breakdown:
            # - ^\s*#\s*define\s+ - start of line, optional whitespace, #define
            # - (SL_STATUS_[A-Z0-9_]+) - capture status name (bounded character class)
            # - \s+ - required whitespace
            # - [^/]+ - match cast expression (bounded, stops at first '/')
            # - ///<\s* - literal comment marker
            # - (.+)$ - capture description to end of line
            match = re.match(
                r'^\s*#\s*define\s+(SL_STATUS_[A-Z0-9_]+)\s+[^/]+///<\s*(.+)$',
                line
            )
            if match:
                name = match.group(1)
                description = match.group(2).strip()
                
                # Skip SL_STATUS_H (header guard)
                if name != 'SL_STATUS_H':
                    # Remove trailing period if present
                    if description.endswith('.'):
                        description = description[:-1]
                    # Ensure description ends with period
                    if not description.endswith('.'):
                        description += '.'
                    
                    status_codes.append({
                        'name': name,
                        'description': description
                    })
    
    return status_codes


def main():
    parser = argparse.ArgumentParser(
        description='Generate sl_status_strings.h and sl_status_strings.c from sl_status.h using Jinja2'
    )
    parser.add_argument(
        'input_dir',
        help='Directory containing sl_status.h'
    )
    parser.add_argument(
        'output_dir',
        help='Output directory for generated header and source files'
    )
    
    args = parser.parse_args()
    
    # Resolve paths
    input_dir = Path(args.input_dir).resolve()
    header_path = input_dir / 'sl_status.h'
    output_dir = Path(args.output_dir).resolve()
    
    # Check if input file exists
    if not header_path.exists():
        print(f"Error: File not found: {header_path}", file=sys.stderr)
        sys.exit(1)
    
    # Parse the header file
    status_codes = parse_sl_status_header(header_path)
    
    if not status_codes:
        print("Warning: No status codes found in header file", file=sys.stderr)
    
    # Setup Jinja2 environment
    # Template directory is relative to this script's location
    script_dir = Path(__file__).parent.resolve()
    template_dir = script_dir / 'templates'
    env = Environment(  # NOSONAR: no injection offline
        autoescape=False,  # NOSONAR: no injection offline - generating C code, not HTML
        loader=FileSystemLoader(str(template_dir)),
        trim_blocks=True,
        lstrip_blocks=True
    )
    
    # Ensure output directories exist
    include_dir = output_dir / 'include'
    src_dir = output_dir / 'src'
    include_dir.mkdir(parents=True, exist_ok=True)
    src_dir.mkdir(parents=True, exist_ok=True)
    
    # Initialize clang-format
    clang_format = ClangFormat()
    
    # Generate header file
    header_template = env.get_template('sl_status_strings.h.j2')
    header_content = header_template.render(status_codes=status_codes)
    header_formatted = clang_format.format(header_content, filename="file.h")
    
    header_path_out = include_dir / 'sl_status_strings.h'
    with open(header_path_out, 'w', encoding='utf-8') as f:
        f.write(header_formatted)
    
    # Generate source file
    source_template = env.get_template('sl_status_strings.c.j2')
    source_content = source_template.render(status_codes=status_codes)
    source_formatted = clang_format.format(source_content, filename="file.c")
    
    source_path_out = src_dir / 'sl_status_strings.c'
    with open(source_path_out, 'w', encoding='utf-8') as f:
        f.write(source_formatted)


if __name__ == '__main__':
    main()

