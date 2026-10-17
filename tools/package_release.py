"""Package installation files separately from release evidence and MIT source."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import sys
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'build/packages'
sys.path.insert(0, str(ROOT))
import build
VERSION = '0.1.1'
TARGET_HASH = '9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac'


def digest(data):
    return sha256(data).hexdigest()


def source_files():
    files = {}
    for name in ('LICENSE', 'README.md', 'INSTALL.md', 'BUILDING.md', 'THIRD_PARTY.md', 'VISUAL_FIX.md',
                 '.gitignore', '.gitattributes', 'build.py', 'resource_patches.json', 'loader.lock.json', 'release_plan.json'):
        files[name] = (ROOT / name).read_bytes()
    for folder in ('src', 'tools', 'integration', 'mods', '.github'):
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
        if sorted(archive.namelist()) != sorted(files):
            raise ValueError('ZIP validation failed')
        for relative, data in files.items():
            if archive.read(relative) != data:
                raise ValueError(f'ZIP payload differs: {relative}')
    return dict(file=name, bytes=path.stat().st_size, sha256=digest(path.read_bytes()), entries=len(files))


def verify_native_report(report, artifact):
    if digest(artifact.read_bytes()) != report['dll_sha256']:
        raise ValueError('Native artifact differs from its historical build report')
    inputs = report.get('compile_inputs') or {name:report['sources'][name] for name in build.COMPILE_INPUTS}
    if set(inputs) != set(build.COMPILE_INPUTS):
        raise ValueError('Actual compiled input set differs')
    for name, expected in inputs.items():
        if digest((ROOT/name).read_bytes()) != expected:
            raise ValueError('Compiled input changed: ' + name)
    return inputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-only', action='store_true', help='Prepare only the current source snapshot')
    args = parser.parse_args()
    sources = source_files()
    if (ROOT/'.git').exists():
        source_commit = subprocess.check_output(['git','-C',str(ROOT),'rev-parse','HEAD'],text=True).strip()
        if subprocess.check_output(['git','-C',str(ROOT),'status','--porcelain']):
            raise ValueError('Commit current source before packaging')
    else:
        source_commit = json.loads((ROOT/'SOURCE_MANIFEST.json').read_text(encoding='utf-8'))['source_commit']
    sources['SOURCE_MANIFEST.json'] = (json.dumps(dict(source_commit=source_commit,files={n:digest(d) for n,d in sorted(sources.items())}),indent=2)+'\n').encode('utf-8')
    source_id = digest(''.join(f'{name}\0{digest(data)}\n' for name, data in sorted(sources.items())).encode('utf-8'))
    core_report = json.loads((ROOT / 'build/core_report.json').read_text(encoding='utf-8'))
    compile_inputs = verify_native_report(core_report, ROOT / 'build/DialogueOutline.dll')
    dll = (ROOT / 'build/DialogueOutline.dll').read_bytes()
    metadata = dict(version=VERSION, source_commit=source_commit, source_tree_sha256=source_id,
                    original_binary_source_commit=(core_report.get('source_commit') if 'compile_inputs' in core_report
                        else json.loads(sources['release_plan.json'])['release_source_commit']),
                    compile_inputs=compile_inputs, binary_reused='compile_inputs' not in core_report,
                    code_license='MIT', target_exe_sha256=TARGET_HASH,
                    loader_repository='https://github.com/RovingAimer/sora1looseload',
                    packaging_runs_game=False, sky_2nd_supported=False,
                    replaces_shared_shaders=False, preserves_original_text_color=True,
                    runtime_selection='Per glyph item generation; highest black/white sRGB contrast',
                    loader_support='Generic loader v0.2.0; bundled sora_mod.ini declares SoraDialogueInitialize',
                    core_sha256=digest(dll))
    plans = []
    if not args.source_only:
        config_files = {'mods/DialogueOutline/sora_mod.ini': (ROOT/'mods/DialogueOutline/sora_mod.ini').read_bytes()}
        main_files = {'mods/DialogueOutline/DialogueOutline.dll': dll, **config_files}
        resources = ROOT / 'build/resources'
        optional_files = {}
        spec = json.loads((ROOT / 'resource_patches.json').read_text(encoding='utf-8'))
        for item in spec['layouts'] + spec['reference_textures']:
            relative = Path(item['output'])
            data = (resources / relative).read_bytes()
            if digest(data) != item.get('patched_sha256', item.get('sha256')):
                raise ValueError(f'Resource differs from reviewed specification: {relative}')
            target = main_files if relative.parts[0] == 'main' else optional_files
            target[Path(*relative.parts[1:]).as_posix()] = data
        if set(main_files) != {'mods/DialogueOutline/DialogueOutline.dll', 'mods/DialogueOutline/sora_mod.ini', 'layout/subtitle.lay',
                               'asset/dx11/image/window8.dds'}:
            raise ValueError('Main installation file set differs')
        if set(optional_files) != {'layout/active_voice.lay', 'asset/dx11/image/active_voice.dds'}:
            raise ValueError('Active-voice installation file set differs')
        plans = [(f'DialogueOutline-v{VERSION}-config.zip', config_files, 'configuration-only-update'),
                 (f'DialogueOutline-v{VERSION}-main.zip', main_files, 'generic-loader-v0.2.0-required'),
                 (f'DialogueOutline-v{VERSION}-active-voice.zip', optional_files, 'optional-active-voice-requires-main')]
    OUTPUT.mkdir(parents=True, exist_ok=True)
    artifacts = [write_zip(f'DialogueOutline-v{VERSION}-source.zip', sources)]
    for name, files, route in plans:
        artifact = write_zip(name, files)
        artifact.update(route=route,
                        files=[dict(path=relative, bytes=len(data), sha256=digest(data))
                               for relative, data in sorted(files.items())])
        artifacts.append(artifact)
    (OUTPUT / 'native-build-report.json').write_bytes((ROOT / 'build/core_report.json').read_bytes())
    result = {**metadata, 'source_only':args.source_only, 'artifacts': artifacts,
              'native_build_report': 'native-build-report.json',
              'native_build_report_sha256': digest((ROOT / 'build/core_report.json').read_bytes()),
              'native_build_report_data': core_report}
    (OUTPUT / 'release_manifest.json').write_text(json.dumps(result, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2, ensure_ascii=False))


if __name__ == '__main__':
    main()
