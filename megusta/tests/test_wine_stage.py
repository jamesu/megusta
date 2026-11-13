"""Wine staging guards, exercised without starting Wine or mapping host paths."""
import sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/testing'))
from wine_stage import WineStage

class StagingTests(unittest.TestCase):
    def setUp(self):
        self.temporary=tempfile.TemporaryDirectory()
        self.prefix=Path(self.temporary.name)/'prefix';self.prefix.mkdir()
        (self.prefix/'drive_c').mkdir();(self.prefix/'dosdevices').mkdir()
        (self.prefix/'dosdevices/c:').symlink_to('../drive_c',target_is_directory=True)
    def tearDown(self):self.temporary.cleanup()
    def test_copy_uses_c_paths_and_real_files(self):
        source=Path(self.temporary.name)/'input';source.write_bytes(b'test input')
        stage=WineStage(self.prefix)
        self.assertEqual(stage.copy(source,Path('assets/input')),r'C:\megusta-tests\assets\input')
        destination=self.prefix/'drive_c/megusta-tests/assets/input'
        self.assertFalse(destination.is_symlink());self.assertEqual(destination.read_bytes(),source.read_bytes())
    def test_refuses_additional_mapping_before_execution(self):
        (self.prefix/'dosdevices/d:').symlink_to('../drive_c',target_is_directory=True)
        with self.assertRaises(RuntimeError):WineStage(self.prefix)
    def test_refuses_mapping_added_after_staging(self):
        stage=WineStage(self.prefix)
        (self.prefix/'dosdevices/d:').symlink_to('../drive_c',target_is_directory=True)
        with self.assertRaises(RuntimeError):stage.run('never-executed.exe')
    def test_refuses_staging_symlink_escape(self):
        stage=WineStage(self.prefix)
        source=Path(self.temporary.name)/'input';source.write_bytes(b'test')
        (stage.directory/'escape').symlink_to(Path(self.temporary.name),target_is_directory=True)
        with self.assertRaises(RuntimeError):stage.copy(source,Path('escape/output'))
    def test_missing_prefix_does_not_initialize_wine(self):
        with self.assertRaises(RuntimeError):WineStage(Path(self.temporary.name)/'missing')

if __name__=='__main__':unittest.main()
