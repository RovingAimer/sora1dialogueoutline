"""Package the local initial candidate and its MIT source; never install it."""
from hashlib import sha256
import json
from pathlib import Path
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'build/packages'
TARGET_HASH = '9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac'


def digest(data):
    return sha256(data).hexdigest()


def source_files():
    files = {}
    for name in ('LICENSE', 'README.md', 'INSTALL.md', 'BUILDING.md', 'THIRD_PARTY.md',
                 '.gitignore', '.gitattributes', 'build.py', 'resource_patches.json', 'loader.lock.json'):
        files[name] = (ROOT / name).read_bytes()
    for folder in ('src', 'tools', 'integration', '.github'):
        directory = ROOT / folder
        if not directory.exists():
            continue
        for path in sorted(directory.rglob('*')):
            if path.is_file() and not any(part in {'__pycache__', 'build', '.git'} for part in path.relative_to(directory).parts):
                files[path.relative_to(ROOT).as_posix()] = path.read_bytes()
    if any(Path(name).suffix.lower() in {'.dll', '.exe', '.pac', '.dds', '.lay', '.asi', '.obj', '.lib', '.pdb'} for name in files):
        raise ValueError('Native outputs or game resources must not enter the source package')
    return files


def write_zip(name, files):
    path = OUTPUT / name
    with ZipFile(path, 'w', compression=ZIP_DEFLATED) as archive:
        for relative, data in sorted(files.items()):
            entry = ZipInfo(relative, date_time=(2026, 1, 1, 0, 0, 0))
            entry.compress_type = ZIP_DEFLATED
            archive.writestr(entry, data)
    with ZipFile(path) as archive:
        if archive.testzip() is not None or len(archive.namelist()) != len(files):
            raise ValueError('ZIP validation failed')
        for relative, data in files.items():
            if archive.read(relative) != data:
                raise ValueError(f'ZIP payload differs: {relative}')
    return dict(file=name, bytes=path.stat().st_size, sha256=digest(path.read_bytes()), entries=len(files))


def verify_native_report(path, artifact, source_hashes, expected_paths):
    report = json.loads(path.read_text(encoding='utf-8'))
    if digest(artifact.read_bytes()) != report['dll_sha256']:
        raise ValueError(f'Native artifact differs from its build report: {artifact.name}')
    if set(source_hashes) != set(expected_paths):
        raise ValueError(f'Build input set differs: {artifact.name}; rebuild before packaging')
    for relative, expected in source_hashes.items():
        if digest(expected_paths[relative].read_bytes()) != expected:
            raise ValueError(f'Build input changed: {relative}; rebuild before packaging')
    return report


def main():
    sources = source_files()
    source_id = digest(''.join(f'{name}\0{digest(data)}\n' for name, data in sorted(sources.items())).encode('utf-8'))
    core_report = json.loads((ROOT / 'build/core_report.json').read_text(encoding='utf-8'))
    core_inputs = {name: ROOT / name for name in ('build.py', 'BUILDING.md')}
    core_inputs.update({path.relative_to(ROOT).as_posix(): path for path in (ROOT / 'src').iterdir() if path.is_file()})
    verify_native_report(ROOT / 'build/core_report.json', ROOT / 'build/DialogueOutline.dll',
                         core_report['sources'], core_inputs)
    dll = (ROOT / 'build/DialogueOutline.dll').read_bytes()
    # Common docs and source identity travel with every native distribution.
    common = {name: sources[name] for name in ('LICENSE', 'README.md', 'INSTALL.md', 'THIRD_PARTY.md')}
    common['build_evidence/core_report.json'] = (ROOT / 'build/core_report.json').read_bytes()
    metadata = dict(version='initial-development-candidate', source_tree_sha256=source_id,
                    code_license='MIT', target_exe_sha256=TARGET_HASH,
                    loader_repository='https://github.com/RovingAimer/sora1looseload',
                    game_runtime_verified=False, sky_2nd_supported=False,
                    replaces_shared_shaders=False, preserves_original_text_color=True,
                    runtime_selection='Per glyph item generation; highest black/white sRGB contrast',
                    original_loader_support='Stock sora1looseload has no plugin interface; integration is required',
                    core_sha256=digest(dll))
    main_files = {**common, 'mods/DialogueOutline/DialogueOutline.dll': dll}
    resources = ROOT / 'build/resources'
    optional_files = {**common}
    spec = json.loads((ROOT / 'resource_patches.json').read_text(encoding='utf-8'))
    for item in spec['layouts'] + spec['reference_textures']:
        relative = Path(item['output'])
        data = (resources / relative).read_bytes()
        if digest(data) != item.get('patched_sha256', item.get('sha256')):
            raise ValueError(f'Resource differs from reviewed specification: {relative}')
        target = main_files if relative.parts[0] == 'main' else optional_files
        target[Path(*relative.parts[1:]).as_posix()] = data
    plans = [('1st_初版_主接入包.zip', main_files, 'original-loader-integration-required'),
             ('1st_初版_可选即时对话.zip', optional_files, 'optional-active-voice-requires-main')]
    asi = ROOT / 'build/DialogueOutline.asi'
    if asi.exists():
        if asi.read_bytes() != dll:
            raise ValueError('Optional ASI must contain the same reviewed core')
        asi_files = {**common, 'plugins/DialogueOutline.asi': dll}
        asi_files.update({name: data for name, data in main_files.items() if name.endswith(('.lay', '.dds'))})
        plans.append(('1st_初版_可选ASI入口.zip', asi_files, 'optional-ual-initializeasi'))
    OUTPUT.mkdir(parents=True, exist_ok=True)
    artifacts = [write_zip('DialogueOutline_初版_MIT源码.zip', sources)]
    for name, files, route in plans:
        manifest = {**metadata, 'route': route,
                    'files': [dict(path=relative, bytes=len(data), sha256=digest(data)) for relative, data in sorted(files.items())]}
        files['manifest.json'] = (json.dumps(manifest, indent=2, ensure_ascii=False) + '\n').encode('utf-8')
        artifacts.append(write_zip(name, files))
    result = {**metadata, 'artifacts': artifacts,
              'native_build_report': 'build/core_report.json',
              'native_build_report_sha256': digest(json.dumps(core_report, sort_keys=True).encode('utf-8'))}
    (OUTPUT / 'release_manifest.json').write_text(json.dumps(result, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2, ensure_ascii=False))


if __name__ == '__main__':
    main()
