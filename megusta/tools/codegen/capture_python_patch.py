#!/usr/bin/env python3
"""Capture CPython 1.5.2 changes and verify a byte-exact clean application.

Usage: capture_python_patch.py BASELINE [--check]
The baseline is supplied explicitly; normal builds do not use it or this tool.
"""
import argparse
import difflib
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('baseline',type=Path)
parser.add_argument('--check',action='store_true',help='Verify the recorded patch without replacing it')
args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
baseline=args.baseline.resolve()
working=root/'vendor/Python-1.5.2'
target=root/'patches/python/0001-megusta.patch'

def inventory(directory):
    result={}
    for folder,dirs,names in os.walk(directory,followlinks=False):
        dirs[:]=sorted(d for d in dirs if d not in {'.git','__pycache__'})
        for name in sorted(names):
            if name=='.DS_Store':continue
            path=Path(folder)/name
            if path.is_symlink():raise RuntimeError(f'Symlink needs explicit review: {path}')
            result[path.relative_to(directory).as_posix()]=path.read_bytes()
    return result

if baseline==working.resolve():raise SystemExit('Baseline and working tree must differ')
for directory in (baseline,working):
    if '"1.5.2"' not in (directory/'Include/patchlevel.h').read_text():
        raise SystemExit('Expected a CPython 1.5.2 source tree')
old=inventory(baseline);new=inventory(working)
changed=[name for name in sorted(old.keys()|new.keys()) if old.get(name)!=new.get(name)]
pieces=[]
for name in changed:
    before=old.get(name,b'');after=new.get(name,b'')
    if b'\0' in before or b'\0' in after:raise SystemExit(f'Binary patch requires explicit handling: {name}')
    # Preserve legacy encodings and CRLF rather than decoding/reformatting C.
    lines=difflib.diff_bytes(difflib.unified_diff,before.splitlines(True),after.splitlines(True),
        fromfile=('a/'+name).encode() if name in old else b'/dev/null',
        tofile=('b/'+name).encode() if name in new else b'/dev/null')
    for line in lines:
        pieces.append(line if line.endswith(b'\n') else line+b'\n\\ No newline at end of file\n')
patch=b''.join(pieces)
if args.check and (not target.is_file() or target.read_bytes()!=patch):
    raise SystemExit('Recorded patch differs from the supplied baseline/current vendor tree')
(root/'build').mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='python-patch-check-',dir=root/'build') as temporary:
    temporary=Path(temporary);clean=temporary/'Python-1.5.2'
    shutil.copytree(baseline,clean,ignore=shutil.ignore_patterns('.git','__pycache__','.DS_Store'))
    candidate=temporary/'changes.patch';candidate.write_bytes(patch)
    if patch:
        result=subprocess.run(['patch','--batch','-F','0','-p1','-i',str(candidate)],
            cwd=clean,capture_output=True,text=True)
        if result.returncode:raise SystemExit(result.stdout+result.stderr)
    reproduced=inventory(clean)
    # patch can leave its own backup/reject files; baseline-owned files with
    # those suffixes remain part of the comparison.
    reproduced={name:data for name,data in reproduced.items()
                if name in new or not name.endswith(('.orig','.rej'))}
    if reproduced!=new:raise SystemExit('Clean baseline + patch does not reproduce the complete vendor tree')
if not args.check:
    target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(patch)
print(f'Verified CPython 1.5.2 patch: {len(changed)} changed files; {len(new)} files reproduced byte-for-byte')
print('Patch SHA256:',hashlib.sha256(patch).hexdigest())
for name in changed:print(name)
