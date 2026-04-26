#!/usr/bin/env python3
import os
import sys

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    xml_path = os.path.join(script_dir, '..', 'include', 'kinetis', 'mavlink', 'mavlink_dialect.xml')
    out_dir = os.path.join(script_dir, '..', 'include', 'kinetis', 'mavlink')

    xml_path = os.path.normpath(xml_path)
    out_dir = os.path.normpath(out_dir)

    print(f"XML path: {xml_path}")
    print(f"Output dir: {out_dir}")

    try:
        from pymavlink.generator import mavgen
        from pymavlink.generator.mavgen import Opts
    except ImportError:
        print("Error: pymavlink not installed correctly. Run: pip install --force-reinstall pymavlink")
        sys.exit(1)

    os.makedirs(out_dir, exist_ok=True)

    opts = Opts(out_dir, wire_protocol='2.0', language='C')
    mavgen.mavgen(opts, [os.path.abspath(xml_path)])

    print(f"Generated files in: {out_dir}")
    for f in sorted(os.listdir(out_dir)):
        print(f"  - {f}")

if __name__ == '__main__':
    main()
