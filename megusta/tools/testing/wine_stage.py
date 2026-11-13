"""Copy test inputs into an existing C:-only Wine prefix; never map host paths."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,struct,subprocess,sys

DEFAULT_PREFIX=Path('/tmp/mgm-wineprefix')

class WineStage:
    def __init__(self,prefix=DEFAULT_PREFIX):
        self.prefix=Path(prefix).resolve()
        self.check_drives()
        self.directory=self.prefix/'drive_c/megusta-tests'
        if not self.directory.resolve().is_relative_to(self.prefix/'drive_c'):
            raise RuntimeError('Wine staging directory escapes C:')
        self.directory.mkdir(exist_ok=True)
        self.env=os.environ.copy()
        self.env.update(WINEPREFIX=str(self.prefix),WINEDEBUG='-all',WINEPATH=r'C:\megusta-tests\bin')
        # Do not inherit another process's Wine DLL overrides or host search path.
        self.env.pop('WINEDLLOVERRIDES',None)
        self.files={}

    def check_drives(self):
        drives=self.prefix/'dosdevices'
        if not drives.is_dir():
            raise RuntimeError(f'Expected existing Wine prefix at {self.prefix}; automatic wineboot is disabled')
        names={p.name.lower() for p in drives.iterdir()}
        if names!={'c:'} or (drives/'c:').resolve()!=(self.prefix/'drive_c').resolve():
            raise RuntimeError(f'Wine must have only C: mapped to its own drive_c; found {sorted(names)}')
        if (self.prefix/'drive_c').is_symlink():
            raise RuntimeError('Wine drive_c must be a real directory inside its prefix')

    def check_architecture(self,architecture):
        if architecture=='win64':
            registry=self.prefix/'system.reg'
            if registry.is_file():
                with registry.open(errors='replace') as stream:
                    header=stream.read(512)
            else:
                header=''
            if '#arch=win64' not in header:
                raise RuntimeError(f'Expected an existing win64 Wine prefix at {self.prefix}')

    @staticmethod
    def check_pe_architecture(executable,architecture):
        expected=0x8664 if architecture=='win64' else 0x014c
        with Path(executable).open('rb') as stream:
            header=stream.read(64)
            if len(header)<64 or header[:2]!=b'MZ':
                raise RuntimeError(f'Not a PE executable: {executable}')
            pe_offset=struct.unpack_from('<I',header,0x3c)[0]
            stream.seek(pe_offset)
            pe=stream.read(6)
        if len(pe)<6 or pe[:4]!=b'PE\0\0' or struct.unpack_from('<H',pe,4)[0]!=expected:
            label='x64' if architecture=='win64' else 'x86'
            raise RuntimeError(f'Expected an {label} PE executable: {executable}')

    def copy(self,source,relative):
        source=Path(source).resolve();destination=self.directory/relative
        if not destination.resolve().is_relative_to(self.directory):
            raise RuntimeError('Staged path escapes test directory')
        destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(source,destination)
        digest=hashlib.sha256(source.read_bytes()).hexdigest()
        if hashlib.sha256(destination.read_bytes()).hexdigest()!=digest:
            raise RuntimeError('Staged copy checksum differs')
        self.files[str(relative)]=digest
        return 'C:\\megusta-tests\\'+str(relative).replace('/','\\')

    def copy_tree(self,source,relative,exclude_dirs=(),exclude_suffixes=()):
        source=Path(source).resolve(); destination=self.directory/relative
        if not destination.resolve().is_relative_to(self.directory):
            raise RuntimeError('Staged tree escapes test directory')
        excluded_dirs={name.casefold() for name in exclude_dirs}
        excluded_suffixes={suffix.casefold() for suffix in exclude_suffixes}
        records={}
        for current,dirs,files in os.walk(source,followlinks=False):
            current=Path(current)
            if current.is_symlink(): raise RuntimeError(f'Refusing symlink in staged tree: {current}')
            target_directory=destination/current.relative_to(source)
            if not target_directory.resolve().is_relative_to(self.directory):
                raise RuntimeError('Staged directory escapes test directory')
            target_directory.mkdir(parents=True,exist_ok=True)
            dirs[:]=sorted(name for name in dirs if not (current==source and name.casefold() in excluded_dirs) and
                           not (current/name).is_symlink())
            for name in sorted(files):
                path=current/name
                if path.is_symlink(): raise RuntimeError(f'Refusing symlink in staged tree: {path}')
                if path.suffix.casefold() in excluded_suffixes: continue
                relative_file=path.relative_to(source)
                destination_file=destination/relative_file
                if not destination_file.resolve().is_relative_to(self.directory):
                    raise RuntimeError('Staged file escapes test directory')
                destination_file.parent.mkdir(parents=True,exist_ok=True)
                shutil.copyfile(path,destination_file)
                digest=hashlib.sha256(path.read_bytes()).hexdigest()
                if hashlib.sha256(destination_file.read_bytes()).hexdigest()!=digest:
                    raise RuntimeError(f'Staged copy checksum differs: {relative_file}')
                key=str(Path(relative)/relative_file)
                records[key]=digest; self.files[key]=digest
        return records

    def run(self,executable,*arguments):
        self.check_drives()
        try:
            try:
                return subprocess.run(['wine',executable,*arguments],cwd=self.directory,env=self.env,
                                      capture_output=True,text=True,
                                      timeout=float(os.environ.get('MEGUSTA_WINE_TIMEOUT','90')))
            except subprocess.TimeoutExpired as error:
                if error.stdout:
                    print(error.stdout.decode(errors='replace') if isinstance(error.stdout,bytes) else error.stdout,end='')
                if error.stderr:
                    print(error.stderr.decode(errors='replace') if isinstance(error.stderr,bytes) else error.stderr,end='',file=sys.stderr)
                raise
        finally:
            self.check_drives()

if __name__=='__main__':
    parser=argparse.ArgumentParser(description='Stage shipped game data into the existing C-only Wine prefix.')
    parser.add_argument('mode',choices=['stage-game'])
    parser.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[3]/'MGM')
    parser.add_argument('--wine-prefix',type=Path,default=DEFAULT_PREFIX)
    parser.add_argument('--architecture',choices=['win32','win64'],default='win32')
    parser.add_argument('--executable',type=Path)
    parser.add_argument('--mingw-root',type=Path,default=Path(os.environ.get('MEGUSTA_MINGW_ROOT','/opt/homebrew/opt/mingw-w64')))
    args=parser.parse_args()
    stage=WineStage(args.wine_prefix)
    stage.check_architecture(args.architecture)
    build_name='win32-clang' if args.architecture=='win32' else 'win64-clang'
    executable=(args.executable or Path(__file__).resolve().parents[2]/'build'/build_name/'megusta_game.exe').resolve()
    if not executable.is_file(): raise SystemExit(f'Build the reconstructed Windows target first: {executable}')
    stage.check_pe_architecture(executable,args.architecture)
    records=stage.copy_tree(args.source,'game',exclude_dirs=('out',),exclude_suffixes=('.exe',))
    records['game/megusta_game.exe']=hashlib.sha256(executable.read_bytes()).hexdigest()
    stage.copy(executable,Path('game/megusta_game.exe'))
    if args.architecture=='win32':
        sdk=args.mingw_root/'toolchain-i686/i686-w64-mingw32'
        runtime=('lib/libstdc++-6.dll','lib/libgcc_s_sjlj-1.dll','bin/libwinpthread-1.dll')
    else:
        sdk=args.mingw_root/'toolchain-x86_64/x86_64-w64-mingw32'
        runtime=('lib/libstdc++-6.dll','lib/libgcc_s_seh-1.dll','bin/libwinpthread-1.dll')
    for relative in runtime:
        library=sdk/relative
        if not library.is_file(): raise SystemExit(f'Missing MinGW runtime dependency: {library}')
        destination=Path('game')/library.name
        records[str(destination)]=hashlib.sha256(library.read_bytes()).hexdigest()
        stage.copy(library,destination)
    manifest={'source':str(args.source.resolve()),'destination':'C:\\megusta-tests\\game',
              'excluded':['top-level out directory','shipped .exe files'],
              'executable':str(executable),'mapped_drives':['C:'],'files':records}
    manifest_path=stage.directory/'game-staging-manifest.json'
    manifest_path.write_text(json.dumps(manifest,indent=2)+'\n')
    local_manifest=Path(__file__).resolve().parents[2]/'build'/build_name/'game-staging-manifest.json'
    local_manifest.parent.mkdir(parents=True,exist_ok=True)
    local_manifest.write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps({'destination':manifest['destination'],'files_copied':len(records),
                      'manifest':str(manifest_path),'mapped_drives':['C:']},indent=2))
