#!/usr/bin/env python3
"""Stage local reference trees without overwriting an existing working copy."""
from pathlib import Path
import hashlib, json, shutil

root = Path(__file__).resolve().parents[3]
out = root / 'megusta/upstream'
sources = {
    'zlib-1.1.3': 'recomp/reference/image_libraries/sources/zlib-1.1.3',
    'libpng-1.0.8': 'recomp/reference/image_libraries/sources/libpng-1.0.8',
    'libglpng-1.45': 'recomp/reference/image_libraries/sources/libglpng-1.45',
    'plib-1.3.2': 'recomp/reference/plib/sources/plib-1.3.2',
    'Python-1.5.2': 'recomp/reference/python/sources/Python-1.5.2',
}
records = {}
for name, source in sources.items():
    src = root / source
    dst = out / name
    if not src.is_dir(): raise SystemExit(f'Missing reference: {src}')
    if not dst.exists(): shutil.copytree(src, dst, symlinks=False, ignore=shutil.ignore_patterns('.DS_Store'))
    entries = {}
    for file in sorted(src.rglob('*')):
        if file.is_file() and file.name!='.DS_Store':
            rel = str(file.relative_to(src))
            digest = hashlib.sha256(file.read_bytes()).hexdigest()
            target = dst / rel
            if not target.is_file() or hashlib.sha256(target.read_bytes()).hexdigest() != digest:
                raise SystemExit(f'Staged file differs: {target}; preserve it and review modifications')
            entries[rel] = digest
    records[name] = {'reference': source, 'files': entries}
(out / 'manifest.json').write_text(json.dumps(records, indent=2) + '\n')
print('Staged and verified', ', '.join(sources))
