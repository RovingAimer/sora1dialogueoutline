"""Apply reviewed layout edits to user-supplied workspace copies. MIT licensed.

No game installation or extraction is performed. Inputs live in ../inputs and
all derived files are written under ../build/resources.
"""
from hashlib import sha256
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
INPUT = ROOT / 'inputs'
OUTPUT = ROOT / 'build/resources'


def digest(data):
    return sha256(data).hexdigest()


def main():
    spec = json.loads((ROOT / 'resource_patches.json').read_text(encoding='utf-8'))
    outputs, report = [], []
    for item in spec['layouts']:
        relative = Path(item['input'])
        original = (INPUT / relative).read_bytes()
        if digest(original) != item['source_sha256']:
            raise ValueError(f'Unsupported source layout: {relative}')
        patched = bytearray(original)
        allowed = set()
        for change in item['changes']:
            offset = change['offset']
            if struct.unpack_from('<d', original, offset)[0] != change['before']:
                raise ValueError(f'Original field differs: {relative}:{offset}')
            struct.pack_into('<d', patched, offset, change['after'])
            allowed.update(range(offset, offset + 8))
        changed = {i for i, (a, b) in enumerate(zip(original, patched)) if a != b}
        if len(original) != len(patched) or not changed <= allowed:
            raise ValueError(f'Unexpected byte edits: {relative}')
        if digest(patched) != item['patched_sha256']:
            raise ValueError(f'Patched layout differs: {relative}')
        outputs.append((item['output'], bytes(patched)))
        report.append(dict(path=item['output'], source_sha256=digest(original),
                           sha256=digest(patched), changed_bytes=len(changed)))
    for item in spec['reference_textures']:
        data = (INPUT / item['input']).read_bytes()
        if digest(data) != item['sha256']:
            raise ValueError(f'Reference texture differs: {item["input"]}')
        outputs.append((item['output'], data))
        report.append(dict(path=item['output'], sha256=digest(data), copied_unchanged=True))
    # Read and verify every input before publishing any derived output.
    for relative, data in outputs:
        path = OUTPUT / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        if digest(path.read_bytes()) != digest(data):
            raise ValueError(f'Output reread differs: {relative}')
    result = dict(game_runtime_verified=False, files=report,
                  historical_scope_evidence='85 original layouts / 820 text nodes; no marker collisions',
                  current_check='Exact source hashes and reviewed byte edits; not a new full-layout audit')
    (OUTPUT / 'report.json').write_text(json.dumps(result, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2, ensure_ascii=False))


if __name__ == '__main__':
    main()
