"""Format only project-owned application files, excluding vendor code."""
import argparse
from pathlib import Path
import subprocess
import shutil

parser = argparse.ArgumentParser()
parser.add_argument('--check', action='store_true')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
files = ['Audio_Test.ino', 'Speaker_Test.ino']
tool = shutil.which('clang-format-18') or shutil.which('clang-format')
if not tool:
    raise SystemExit('Install clang-format 18.1.8 first.')
version = subprocess.check_output([tool, '--version'], text=True)
if 'version 18.' not in version:
    raise SystemExit('Use clang-format 18 to match CI.')
subprocess.run([tool, *(['--dry-run', '--Werror'] if args.check else ['-i']), *files], cwd=root, check=True)
