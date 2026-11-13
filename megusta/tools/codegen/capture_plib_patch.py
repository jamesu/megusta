#!/usr/bin/env python3
"""Explicit maintainer operation: capture working vendor edits as a unified patch."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess,tempfile
root=Path(__file__).resolve().parents[2];base=root/'upstream/plib-1.3.2';work=root/'vendor/plib'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
patch_dir=root/'patches/plib'
later_patches=sorted(p for p in patch_dir.glob('*.patch') if p.name!='0001-megusta.patch')
# Rewind the working tree to the exact state after 0001; later patches can
# intentionally modify files also changed by the baseline patch.
with tempfile.TemporaryDirectory(prefix='plib-patch-capture-') as temporary:
    intermediate=Path(temporary)/'plib';shutil.copytree(work,intermediate,
        ignore=shutil.ignore_patterns('megusta-patches.json','.DS_Store'))
    for later in reversed(later_patches):
        subprocess.run(['patch','--batch','-R','-p1','-i',str(later)],cwd=intermediate,
                       check=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    # `patch -R` can leave backup files in its temporary working tree when
    # later patches touch the same source. They are tool artifacts, not PLIB.
    ignored=lambda p:p.name=='.DS_Store' or p.suffix in ('.orig','.rej')
    names={str(p.relative_to(base)) for p in base.rglob('*') if p.is_file() and not ignored(p)}|{str(p.relative_to(intermediate)) for p in intermediate.rglob('*') if p.is_file() and not ignored(p)}
    pieces=[]
    for name in sorted(names):
        a=base/name;b=intermediate/name
        old=a.read_bytes() if a.exists() else b'';new=b.read_bytes() if b.exists() else b''
        if old!=new:
            # Keep upstream and modified bytes intact, including legacy comments.
            pieces.extend(difflib.diff_bytes(difflib.unified_diff,old.splitlines(True),new.splitlines(True),fromfile=('a/'+name).encode(),tofile=('b/'+name).encode()))
patch=root/'patches/plib/0001-megusta.patch';patch.write_bytes(b''.join(pieces))
patches=sorted((root/'patches/plib').glob('*.patch'))
data={'base':'PLIB 1.3.2','patches':{str(p.relative_to(root)):sha(p) for p in patches},'files':{str(p.relative_to(work)):sha(p) for p in sorted(work.rglob('*')) if p.is_file() and p.name not in ('megusta-patches.json','.DS_Store')}}
(work/'megusta-patches.json').write_text(json.dumps(data,indent=2)+'\n')
print('Captured working PLIB changes:',patch)
