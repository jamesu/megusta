#!/usr/bin/env python3
"""Reproduce or verify a complete PLIB distribution plus ordered local patches."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,tempfile
ap=argparse.ArgumentParser();ap.add_argument('--verify-rebuild',action='store_true');args=ap.parse_args()
root=Path(__file__).resolve().parents[2]
base=root/'upstream/plib-1.3.2';dest=root/'vendor/plib'
patches=sorted((root/'patches/plib').glob('*.patch'))
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
original=json.loads((root/'upstream/manifest.json').read_text())['plib-1.3.2']['files']
original={name:value for name,value in original.items() if Path(name).name!='.DS_Store'}
assert {str(p.relative_to(base)) for p in base.rglob('*') if p.is_file() and p.name!='.DS_Store'}==set(original),'Pristine source file set differs'
assert all(digest(base/name)==value for name,value in original.items()),'Pristine PLIB source differs'
patchpins={str(p.relative_to(root)):digest(p) for p in patches}
manifest=dest/'megusta-patches.json'
if dest.exists():
    data=json.loads(manifest.read_text())
    assert data['patches']==patchpins,'Patches changed; archive vendor/plib to trash, then prepare again'
    assert {str(p.relative_to(dest)) for p in dest.rglob('*') if p.is_file() and p.name not in ('megusta-patches.json','.DS_Store')}==set(data['files']),'Unrecorded vendor source file'
    assert all((dest/name).is_file() and digest(dest/name)==value for name,value in data['files'].items()),'Working vendor edits need to be captured in a patch'
else:
    shutil.copytree(base,dest,ignore=shutil.ignore_patterns('.DS_Store'))
    for patch in patches:
        subprocess.run(['patch','--batch','-p1','-i',str(patch)],cwd=dest,check=True)
    data={'base':'PLIB 1.3.2','patches':patchpins,'files':{str(p.relative_to(dest)):digest(p) for p in sorted(dest.rglob('*')) if p.is_file()}}
    manifest.write_text(json.dumps(data,indent=2)+'\n')
print('Verified complete PLIB 1.3.2 working tree:',dest)
if args.verify_rebuild:
    (root/'build').mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='plib-reproduction-',dir=root/'build') as temporary:
        clean=Path(temporary)/'plib';shutil.copytree(base,clean,ignore=shutil.ignore_patterns('.DS_Store'))
        for patch in patches:
            result=subprocess.run(['patch','--batch','-p1','-i',str(patch)],cwd=clean,
                                  stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            if result.returncode:
                raise RuntimeError(f'Patch failed: {patch.name}\n{result.stdout}')
        # BSD patch may leave source backups beside files it updates. Those
        # are transient patch-tool artifacts, not part of the reproduced tree.
        actual={str(p.relative_to(clean)):digest(p) for p in clean.rglob('*')
                if p.is_file() and p.suffix not in ('.orig','.rej')}
        assert actual==data['files'],'Clean patch application differs from working vendor tree'
    print('Clean distribution + patches reproduces every working source byte')
