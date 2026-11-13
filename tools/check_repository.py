"""Offline source checks; does not import applications or access hardware."""
import ast
from pathlib import Path
import re
import subprocess
import sys
from urllib.parse import unquote, urlsplit

root = Path(__file__).resolve().parents[1]
paths = subprocess.check_output(['git', 'ls-files', '-z'], cwd=root).decode().split('\0')
errors = []
checked = 0
for name in filter(None, paths):
    path = root / name
    if path.suffix.lower() not in {'.py', '.md', '.c', '.h', '.cpp', '.ino', '.sv', '.yml', '.yaml', '.html'}:
        continue
    text = path.read_text(encoding='utf-8-sig')
    checked += 1
    if re.search(r'^(?:<{7} |>{7} )', text, re.M):
        errors.append(f'{name}: unresolved merge conflict')
    if path.suffix == '.py':
        try:
            ast.parse(text, filename=name)
        except SyntaxError as exc:
            errors.append(f'{name}:{exc.lineno}: {exc.msg}')
    if path.suffix == '.md':
        # Inline local Markdown links; anchors and remote URLs are deliberately excluded.
        text = re.sub(r'```.*?```', '', text, flags=re.S)
        for target in re.findall(r'\]\((<[^>]+>|[^\s)]+)(?:\s+"[^"]*")?\)', text):
            target = target.strip('<>')
            parsed = urlsplit(target)
            if parsed.scheme or parsed.netloc or not parsed.path or target.startswith('/'):
                continue
            if not (path.parent / unquote(parsed.path)).exists():
                errors.append(f'{name}: missing local link target {target}')
print(f'Checked {checked} source/documentation files.')
for error in errors:
    print(error)
sys.exit(bool(errors))
