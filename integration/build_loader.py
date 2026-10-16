"""Build the optional integrated x64 loose loader; never load/install any DLL.

Run with Python 3 on Windows with MSVC x64 Build Tools + Windows SDK installed.
Vendored upstream files remain unchanged; generated files stay in build/optional_loader.
"""
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / 'third_party/sora1looseload'
OUTPUT = ROOT / 'build/optional_loader'
INTEGRATION = ROOT / 'integration'


def patched_loader():
    original = (VENDOR / 'dllmain.cpp').read_text(encoding='utf-8')
    anchor = '#include "pch.h"\n'
    if original.count(anchor) != 1:
        raise ValueError('Unexpected upstream include anchor')
    patched = original.replace(anchor, anchor + '#include "dialogue_outline_loader.h"\n', 1)
    old = '            DetourTransactionCommit();\n\n            Log("All detours attached successfully.");'
    new = '''            const LONG result = DetourTransactionCommit();
            if (result != NO_ERROR) {
                Log("Detours commit failed (%ld); DialogueOutline was not loaded.", result);
                return;
            }
            Log("All detours attached successfully.");
            LoadDialogueOutline(Log);'''
    if patched.count(old) != 1:
        raise ValueError('Unexpected upstream Detours commit anchor')
    return original, patched.replace(old, new, 1)


def integration_patch():
    original, patched = patched_loader()
    patch = ''.join(difflib.unified_diff(original.splitlines(True), patched.splitlines(True),
        fromfile='a/dllmain.cpp', tofile='b/dllmain.cpp'))
    helper = (INTEGRATION / 'dialogue_outline_loader.h').read_text(encoding='utf-8')
    return patch + ''.join(difflib.unified_diff([], helper.splitlines(True),
        fromfile='/dev/null', tofile='b/dialogue_outline_loader.h'))


def compiler_environment():
    finder = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)')) / \
        'Microsoft Visual Studio/Installer/vswhere.exe'
    install = subprocess.check_output([str(finder), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],
        text=True).strip()
    if not install:
        raise RuntimeError('MSVC x64 Build Tools and Windows SDK are required')
    environment = os.environ.copy()
    environment['PATH'] = environment.get('PATH', '').replace('"', '')
    environment.update(TEMP=str(OUTPUT), TMP=str(OUTPUT), VSLANG='1033', VSCMD_SKIP_SENDTELEMETRY='1')
    setup = Path(install) / 'VC/Auxiliary/Build/vcvars64.bat'
    command = OUTPUT / 'toolchain_env.cmd'
    command.write_text(f'@echo off\ncall "{setup}" >nul\nif errorlevel 1 exit /b 1\nset\n', encoding='ascii')
    result = subprocess.run(['cmd.exe', '/d', '/c', str(command)], cwd=OUTPUT, env=environment,
        capture_output=True, text=True, encoding='mbcs', check=True)
    for line in result.stdout.splitlines():
        key, separator, value = line.partition('=')
        if separator and key:
            environment[key] = value
    environment.update(TEMP=str(OUTPUT), TMP=str(OUTPUT))
    return environment


def run(arguments, environment, log):
    result = subprocess.run([str(value) for value in arguments], cwd=OUTPUT, env=environment,
        capture_output=True, text=True, encoding='utf-8', errors='replace')
    (OUTPUT / log).write_text(result.stdout + result.stderr, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(f'{Path(arguments[0]).name} failed; see {OUTPUT / log}')


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    report_path = OUTPUT / 'report.json'
    report_path.unlink(missing_ok=True)
    lock = json.loads((VENDOR / 'UPSTREAM_LOCK.json').read_text(encoding='utf-8'))
    for filename, record in lock['files'].items():
        digest = hashlib.sha256((VENDOR / filename).read_bytes()).hexdigest()
        if digest != record['sha256']:
            raise ValueError(f'Vendored source differs from locked source: {filename}')
    patch = integration_patch()
    if (INTEGRATION / 'sora1looseload-dialogue-outline.patch').read_text(encoding='utf-8') != patch:
        raise ValueError('Published patch does not match current integration sources')
    (_, patched) = patched_loader()
    generated = OUTPUT / 'dllmain.cpp'
    generated.write_text(patched, encoding='utf-8')
    environment = compiler_environment()
    locate = lambda name: shutil.which(name, path=environment['PATH'])
    compiler, librarian, linker, dumper = map(locate, ('cl.exe', 'lib.exe', 'link.exe', 'dumpbin.exe'))
    if not all((compiler, librarian, linker, dumper)):
        raise RuntimeError('Incomplete MSVC x64 toolchain')
    detours = VENDOR / 'external/detours/src'
    detour_sources = ['detours', 'modules', 'disasm', 'image', 'creatwth', 'disolx86',
        'disolx64', 'disolia64', 'disolarm', 'disolarm64']
    objects = []
    for name in detour_sources:
        obj = OUTPUT / (name + '.obj')
        run([compiler, '/nologo', '/c', '/O2', '/MT', '/EHsc', '/DWIN32_LEAN_AND_MEAN',
            '/DDETOUR_DEBUG=0', '/Fo' + str(obj), detours / (name + '.cpp')], environment, name + '.log')
        objects.append(obj)
    library = OUTPUT / 'detours.lib'
    run([librarian, '/NOLOGO', '/OUT:' + str(library), *objects], environment, 'library.log')
    loader_objects = []
    for name, source in [('dllmain', generated), ('proxy', VENDOR / 'proxy.cpp')]:
        obj = OUTPUT / (name + '.obj')
        run([compiler, '/nologo', '/c', '/std:c++17', '/O2', '/MT', '/W4', '/EHsc',
            '/I' + str(VENDOR), '/I' + str(INTEGRATION), '/I' + str(detours),
            '/Fo' + str(obj), source], environment, name + '.log')
        loader_objects.append(obj)
    artifact = OUTPUT / 'xinput1_4.dll'
    run([linker, '/NOLOGO', '/DLL', '/MACHINE:X64', '/INCREMENTAL:NO', '/DYNAMICBASE', '/Brepro',
        '/NXCOMPAT', '/OPT:REF', '/OUT:' + str(artifact), *loader_objects, library,
        'kernel32.lib', 'psapi.lib'], environment, 'link.log')
    run([dumper, '/NOLOGO', '/HEADERS', '/EXPORTS', '/DEPENDENTS', artifact], environment, 'pe_inspection.txt')
    inspection = (OUTPUT / 'pe_inspection.txt').read_text(encoding='utf-8')
    exports = sorted([[int(ordinal), name] for ordinal, name in re.findall(
        r'^\s*(\d+)\s+[0-9A-F]+\s+[0-9A-F]{8}\s+(\w+)\s*$', inspection, re.MULTILINE)])
    expected_exports = [[2, 'XInputGetState'], [3, 'XInputSetState'], [4, 'XInputGetCapabilities'],
        [5, 'XInputEnable'], [7, 'XInputGetBatteryInformation'], [8, 'XInputGetKeystroke'],
        [10, 'XInputGetAudioDeviceIds']]
    if exports != expected_exports:
        raise ValueError(f'Unexpected XInput exports: {exports}')
    imports = re.findall(r'^\s+([A-Za-z0-9_.-]+\.dll)\s*$', inspection, re.MULTILINE | re.IGNORECASE)
    report = dict(optional_replacement_loader=True, runtime_verified=False,
        dll_executed=False, game_started=False, upstream_commit=lock['commit'],
        upstream_lock_sha256=hashlib.sha256((VENDOR / 'UPSTREAM_LOCK.json').read_bytes()).hexdigest(),
        detours_commit=lock['detours']['commit'], compiler=compiler,
        build_flags=['/O2', '/MT', '/std:c++17', '/Brepro (link)'],
        exports=exports, expected_exports=expected_exports, export_contract_match=True, imports=imports,
        dll_sha256=hashlib.sha256(artifact.read_bytes()).hexdigest(),
        dll_bytes=artifact.stat().st_size,
        integration_sha256={path.name: hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(INTEGRATION.iterdir()) if path.is_file()},
        note='Stock loaders do not load this core; this binary is an explicit optional replacement.')
    report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(artifact)
    print(report_path)


if __name__ == '__main__':
    main()
