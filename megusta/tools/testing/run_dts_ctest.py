#!/usr/bin/env python3
"""Run a reconstructed Win32 check through the existing C-only prefix.

Optional arguments after the executable are passed to that reconstructed check.
"""
import os,sys
from pathlib import Path
from wine_stage import WineStage
exe=Path(sys.argv[1])
with exe.open('rb') as stream:
    dos=stream.read(64)
    if len(dos)<64 or dos[:2]!=b'MZ': raise RuntimeError(f'Not a PE executable: {exe}')
    stream.seek(int.from_bytes(dos[60:64],'little'))
    pe=stream.read(6)
    if pe[:4]!=b'PE\0\0': raise RuntimeError(f'Invalid PE executable: {exe}')
    machine=int.from_bytes(pe[4:6],'little')
if machine==0x8664:
    architecture='win64'; toolchain='toolchain-x86_64/x86_64-w64-mingw32'
    runtime=('lib/libstdc++-6.dll','lib/libgcc_s_seh-1.dll','bin/libwinpthread-1.dll')
elif machine==0x014c:
    architecture='win32'; toolchain='toolchain-i686/i686-w64-mingw32'
    runtime=('lib/libstdc++-6.dll','lib/libgcc_s_sjlj-1.dll','bin/libwinpthread-1.dll')
else:
    raise RuntimeError(f'Unsupported PE machine 0x{machine:04x}: {exe}')
stage=WineStage(Path(os.environ.get('WINEPREFIX','/tmp/mgm-wineprefix')))
stage.check_architecture(architecture)
stage.check_pe_architecture(exe,architecture)
sdk=Path(os.environ.get('MEGUSTA_MINGW_ROOT','/opt/homebrew/opt/mingw-w64'))/toolchain
for rel in runtime:
    stage.copy(sdk/rel,Path('bin')/Path(rel).name)
staged=stage.copy(exe,Path('bin')/exe.name)
check_arguments=sys.argv[2:]
if exe.stem=='megusta_scene_inspect' and '--check-network-bindings' in sys.argv[2:]:
    # Normal game startup disables class exceptions. Enable and stage the
    # pinned hierarchy only when the caller explicitly requests this test mode.
    class_exceptions='--class-exceptions' in check_arguments
    check_arguments=[arg for arg in check_arguments if arg!='--class-exceptions']
    stage.env['MEGUSTA_TEST_CLASS_EXCEPTIONS']='1' if class_exceptions else '0'
    stage.env['PYTHONPATH']=''
    if class_exceptions:
        library=Path(__file__).resolve().parents[2]/'vendor/Python-1.5.2/Lib'
        stage.copy(library/'exceptions.py',Path('python-lib/exceptions.py'))
        stage.env['PYTHONPATH']=r'C:\megusta-tests\python-lib'
if exe.stem=='dts_dsq_import_check':
    shipped=Path(__file__).resolve().parents[3]/'MGM'/'out'
    shape=stage.copy(shipped/'BGY.DTS',Path('fixtures/BGY.DTS'))
    sequence=stage.copy(shipped/'BGYBIRD.DSQ',Path('fixtures/BGYBIRD.DSQ'))
    result=stage.run(staged,shape,sequence)
elif exe.stem=='dts_dsq_catalog_check':
    shipped=Path(__file__).resolve().parents[3]/'MGM'/'out'
    shapes={path.stem:path for path in shipped.glob('*.DTS')}
    rows=[]
    for sequence_file in sorted(shipped.glob('*.DSQ')):
        shape_name=sequence_file.stem[:3]
        if shape_name not in shapes:
            raise RuntimeError(f'No shipped DTS matches {sequence_file.name}')
        stage.copy(shapes[shape_name],Path('fixtures')/shapes[shape_name].name)
        stage.copy(sequence_file,Path('fixtures')/sequence_file.name)
        rows.append(f'{shapes[shape_name].name} {sequence_file.name}')
    manifest=stage.directory/'fixtures/manifest.tsv'
    manifest.write_text('\n'.join(rows)+'\n')
    result=stage.run(staged,r'C:\megusta-tests\fixtures\manifest.tsv')
else:
    result=stage.run(staged,*check_arguments)
sys.stdout.write(result.stdout); sys.stderr.write(result.stderr)
raise SystemExit(result.returncode)
