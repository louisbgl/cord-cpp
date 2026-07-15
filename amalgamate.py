#!/usr/bin/env python3
import re
from pathlib import Path

def amalgamate(src_dir, output_file):
    """Merge multiple headers into single file."""

    # Order matters - dependencies first
    files = [
        'exception.hpp',
        'field.hpp',
        'errors.hpp',
        'schema.hpp',
    ]

    output_lines = []

    # Header
    output_lines.append('// cord - Config Reader\n')
    output_lines.append('#pragma once\n\n')

    # Collect all system includes first
    system_includes = set()
    for file in files:
        path = Path(src_dir) / file
        with open(path) as f:
            for line in f:
                if line.startswith('#include <'):
                    system_includes.add(line)

    output_lines.extend(sorted(system_includes))
    output_lines.append('\nnamespace cord {\n')

    # Process each file - extract content inside namespace
    for file in files:
        path = Path(src_dir) / file

        with open(path) as f:
            lines = f.readlines()

        in_namespace = False
        content_lines = []

        for line in lines:
            # Skip pragma once
            if line.strip() == '#pragma once':
                continue
            # Skip includes
            if line.startswith('#include'):
                continue
            # Skip namespace declarations
            if line.strip().startswith('namespace cord'):
                in_namespace = True
                continue
            # Skip closing namespace brace at end
            if line.strip() == '} // namespace cord':
                break

            content_lines.append(line)

        # Add content, trimming excess blank lines
        for line in content_lines:
            output_lines.append(line)

    # Close namespace
    output_lines.append('} // namespace cord\n')

    # Clean up excess newlines
    cleaned = []
    prev_empty = False
    for line in output_lines:
        is_empty = line.strip() == ''
        if is_empty and prev_empty:
            continue
        cleaned.append(line)
        prev_empty = is_empty

    # Write output
    with open(output_file, 'w') as f:
        f.writelines(cleaned)

    print(f'Generated {output_file}')

if __name__ == '__main__':
    amalgamate('src', 'cord.hpp')
