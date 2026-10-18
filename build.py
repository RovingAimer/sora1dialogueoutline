"""Build the standalone x64 core offline; writes only beside this file in build/."""
# Copyright (c) 2026 DialogueOutline contributors
# SPDX-License-Identifier: MIT
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / 'src'
OUTPUT = ROOT / 'build'
COMPILE_INPUTS = ('src/core.cpp', 'src/bridge.asm', 'src/DialogueOutline.def', 'src/DialogueOutline.h', 'src/LicenseNotice.h',
                  'src/runtime_config.cpp', 'src/runtime_config.h', 'src/dialogue_layout.cpp', 'src/dialogue_layout.h')
TARGET_HASH = '9585d3d993874d65156aba633c00b8a25faab3afde8953a8a183c80fb8f657ac'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def pe_info(data):
    """Inspect our freshly linked PE without loading it or running its entry point."""
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[:2] != b'MZ' or data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Invalid PE')
    machine, count = struct.unpack_from('<HH', data, pe + 4)
    optional_size = struct.unpack_from('<H', data, pe + 20)[0]
    optional = pe + 24
    if machine != 0x8664 or struct.unpack_from('<H', data, optional)[0] != 0x20b:
        raise ValueError('Expected x64 PE32+')
    sections = []
    for index in range(count):
        at = optional + optional_size + index * 40
        _, rva, size, raw = struct.unpack_from('<4I', data, at + 8)
        sections.append((rva, size, raw))

    def offset(rva):
        for start, size, raw in sections:
            if start <= rva < start + size and raw + rva - start < len(data):
                return raw + rva - start
        raise ValueError(f'Unmapped RVA: {rva:#x}')

    def name(rva):
        at = offset(rva)
        return data[at:data.index(0, at)].decode('ascii')

    export_rva, export_size = struct.unpack_from('<2I', data, optional + 112)
    exports = []
    if export_rva:
        base, functions, names, function_table, name_table, ordinal_table = struct.unpack_from(
            '<6I', data, offset(export_rva) + 16)
        for index in range(names):
            name_rva = struct.unpack_from('<I', data, offset(name_table) + 4 * index)[0]
            ordinal = struct.unpack_from('<H', data, offset(ordinal_table) + 2 * index)[0]
            if ordinal >= functions:
                raise ValueError('Invalid export ordinal')
            rva = struct.unpack_from('<I', data, offset(function_table) + 4 * ordinal)[0]
            exports.append(dict(name=name(name_rva), ordinal=base + ordinal, rva=rva,
                                forwarded=export_rva <= rva < export_rva + export_size))
    import_rva, _ = struct.unpack_from('<2I', data, optional + 120)
    imports = []
    if import_rva:
        at = offset(import_rva)
        while any(data[at:at + 20]):
            lookup, _, _, name_rva, thunk = struct.unpack_from('<5I', data, at)
            table = offset(lookup or thunk)
            symbols = []
            while True:
                entry = struct.unpack_from('<Q', data, table)[0]
                if not entry:
                    break
                symbols.append(f'ordinal:{entry & 0xffff}' if entry >> 63 else name(entry + 2))
                table += 8
            imports.append(dict(module=name(name_rva), symbols=symbols))
            at += 20
    delay_rva, delay_size = struct.unpack_from('<2I', data, optional + 112 + 13 * 8)
    if delay_rva or delay_size:
        raise ValueError('Unexpected delay imports')
    return exports, imports


def compiler_environment():
    finder_name = shutil.which('vswhere.exe')
    finder = Path(finder_name) if finder_name else Path(os.environ.get(
        'ProgramFiles(x86)', r'C:\Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    if not finder.is_file():
        raise RuntimeError('Install Visual Studio Build Tools with C++ x64 tools and a Windows SDK')
    install = subprocess.check_output([str(finder), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath', '-utf8'],
        encoding='utf-8').strip()
    if not install:
        raise RuntimeError('MSVC x64 tools unavailable')
    setup = Path(install) / 'VC/Auxiliary/Build/vcvars64.bat'
    environment = os.environ.copy()
    environment['PATH'] = environment.get('PATH', '').replace('"', '')
    environment.update(VSLANG='1033', TEMP=str(OUTPUT), TMP=str(OUTPUT))
    command = OUTPUT / 'core_toolchain_env.cmd'
    command.write_text('@echo off\nchcp 65001 >nul\ncall "' + str(setup) +
                       '" >nul\nif errorlevel 1 exit /b 1\nset\n', encoding='utf-8')
    result = subprocess.run(['cmd.exe', '/d', '/c', str(command)], cwd=OUTPUT, env=environment,
                            capture_output=True, check=True, encoding='utf-8')
    for line in result.stdout.splitlines():
        key, separator, value = line.partition('=')
        if separator and key:
            environment[key] = value
    environment.update(TEMP=str(OUTPUT), TMP=str(OUTPUT))
    return environment


def run(arguments, environment, log_name):
    result = subprocess.run(arguments, cwd=OUTPUT, env=environment, capture_output=True,
                            encoding='utf-8', errors='replace')
    output = result.stdout + result.stderr
    (OUTPUT / log_name).write_text(output, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(f'{Path(arguments[0]).name} failed ({result.returncode}):\n{output}')
    if output.strip():
        print(output.strip())
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    if os.name != 'nt':
        parser.error('The native build requires Windows, MSVC x64 tools, and a Windows SDK')
    sys.stdout.reconfigure(encoding='utf-8')
    OUTPUT.mkdir(exist_ok=True)
    (OUTPUT / 'core_report.json').unlink(missing_ok=True)
    environment = compiler_environment()
    compiler, assembler, linker, dumper = (shutil.which(name, path=environment['PATH'])
        for name in ('cl.exe', 'ml64.exe', 'link.exe', 'dumpbin.exe'))
    if not all((compiler, assembler, linker, dumper)):
        raise RuntimeError('Incomplete MSVC x64 toolchain')
    compiler_version = run([compiler, '/nologo', '/Bv', '/c', '/std:c++17', '/O2', '/W4', '/MT', '/EHsc', '/utf-8',
         '/Fo' + str(OUTPUT / 'core.obj'), '/Fd' + str(OUTPUT / 'core.pdb'),
         str(SOURCE / 'core.cpp')], environment, 'core_compile.log')
    for source in ('runtime_config', 'dialogue_layout'):
        run([compiler, '/nologo', '/c', '/std:c++17', '/O2', '/W4', '/MT', '/EHsc', '/utf-8',
             '/Fo' + str(OUTPUT / (source + '.obj')), '/Fd' + str(OUTPUT / 'core.pdb'),
             str(SOURCE / (source + '.cpp'))], environment, source + '_compile.log')
    run([assembler, '/nologo', '/c', '/Fo' + str(OUTPUT / 'bridge.obj'),
         str(SOURCE / 'bridge.asm')], environment, 'core_assemble.log')
    dll_path = OUTPUT / 'DialogueOutline.dll'
    run([linker, '/NOLOGO', '/DLL', '/Brepro', '/MACHINE:X64', '/INCREMENTAL:NO', '/DYNAMICBASE',
         '/NXCOMPAT', '/OPT:REF', '/OPT:ICF', '/OUT:' + str(dll_path),
         '/DEF:' + str(SOURCE / 'DialogueOutline.def'),
         '/IMPLIB:' + str(OUTPUT / 'DialogueOutline.lib'),
         '/PDB:' + str(OUTPUT / 'DialogueOutline.pdb'), '/MAP:' + str(OUTPUT / 'DialogueOutline.map'),
         str(OUTPUT / 'core.obj'), str(OUTPUT / 'bridge.obj'),
         str(OUTPUT / 'runtime_config.obj'), str(OUTPUT / 'dialogue_layout.obj'), 'kernel32.lib', 'bcrypt.lib'],
        environment, 'core_link.log')
    run([dumper, '/NOLOGO', '/DISASM', str(OUTPUT / 'bridge.obj')], environment, 'core_bridge_disasm.txt')
    dll = dll_path.read_bytes()
    exports, imports = pe_info(dll)
    expected = {'InitializeASI', 'SoraChooseOutline', 'SoraDialogueInitialize', 'SoraGetStatus', 'SoraOutlineCallback'}
    if {row['name'] for row in exports} != expected or any(row['forwarded'] for row in exports):
        raise ValueError('Export contract mismatch')
    # Static CRT: the complete direct DLL dependency set is limited to Windows APIs.
    if {row['module'].lower() for row in imports} - {'kernel32.dll', 'bcrypt.dll'}:
        raise ValueError('Unexpected imported DLL')
    source_hashes = {name: digest((ROOT / name).read_bytes()) for name in COMPILE_INPUTS}
    if (ROOT / '.git').exists():
        source_commit = subprocess.check_output(['git','-C',str(ROOT),'rev-parse','HEAD'],text=True).strip()
        source_dirty = bool(subprocess.check_output(['git','-C',str(ROOT),'status','--porcelain']))
    else:
        source_commit, source_dirty = None, None
    report = dict(artifact=dll_path.name, dll_sha256=digest(dll), dll_bytes=len(dll),
        compile_inputs=source_hashes, source_commit=source_commit, source_dirty=source_dirty,
        compiler=compiler, compiler_version=compiler_version,
        msvc_version=environment.get('VCToolsVersion'), windows_sdk_version=environment.get('WindowsSDKVersion'),
        build_flags=['/MT', '/O2', '/W4', '/std:c++17', '/EHsc', '/utf-8', '/Brepro'],
        exports=exports, imports=imports, export_contract_match=True,
        runtime_verified=False, offline_dll_executed=False, game_files_required_for_build=False,
        target_exe_name='sora_1st.exe', target_exe_sha256=TARGET_HASH,
        hook_rva='0x46BDF9', atomic_patch_expected8='00ff868803000048',
        layout_hooks=[dict(rva='0x46B1E0', span=5, expected8='48895c2408488974'),
                      dict(rva='0x46ACE0', span=7, expected8='80b9e10600000048'),
                      dict(rva='0x458539', span=7, prefix=1, expected8='004d89b01c010000'),
                      dict(rva='0x469D80', span=5, expected8='48895c2408488974')],
        configuration='DLL directory/DialogueOutline.ini; ReadDirectoryChangesW; apply at native text preparation',
        marker_float=0.28125, marker_float_bits='0x3E900000', outline_item_mode='0x0100',
        initialization='Explicit synchronous InitOnce outside DllMain; only matching host is pinned; no dependency loader or logging.',
        status_bits={'0x001': 'HostNameMatch', '0x002': 'HostHashMatch', '0x004': 'OriginalBytesMatch',
            '0x008': 'HookActive', '0x010': 'HostHashRejected', '0x020': 'HookConflict',
            '0x040': 'CorePinned', '0x080': 'HookFailed', '0x100': 'InitializationFinished',
            '0x200': 'CorePinFailed', '0x400': 'ForeignHost', '0x800': 'MemoryFinalizeFailed',
            '0x1000': 'LayoutActive', '0x2000': 'LayoutFailed', '0x4000': 'ConfigurationUnavailable'},
        limitations=[
            'No game, original loader, or produced DLL was executed by this build.',
            'Only the exact target hash and original instruction bytes are supported.',
            'Effective fill RGB times node RGB is treated as sRGB; fill and alpha are unchanged.',
            'Selection occurs at item construction; later tint/brightness changes without rebuild may not reselect.',
            'The bridge preserves GPR volatile state, flags, MXCSR and XMM0-XMM5; no synthetic SEH unwind metadata or AVX upper-register preservation.',
            'Marked-node persistence, material behavior, visual results and performance require game validation.',
        ])
    (OUTPUT / 'core_report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'Built {dll_path.name}: {len(dll)} bytes; SHA256 {digest(dll)}')
    print(OUTPUT / 'core_report.json')


if __name__ == '__main__':
    main()
