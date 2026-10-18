"""Preview prepared local artifacts; --publish and an explicit new release plan enable network writes."""
# Copyright (c) 2026 DialogueOutline contributors; SPDX-License-Identifier: MIT
import argparse
from hashlib import sha256
import io
import json
import os
from pathlib import Path
from urllib.error import HTTPError
from urllib.parse import quote
from urllib.request import Request, urlopen
from zipfile import ZipFile
import sys
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
import build


def digest(data):
    return sha256(data).hexdigest()


def validate(folder):
    manifest = json.loads((folder/'release_manifest.json').read_text(encoding='utf-8'))
    raw = (folder/manifest['native_build_report']).read_bytes()
    if digest(raw) != manifest['native_build_report_sha256'] or json.loads(raw) != manifest['native_build_report_data']:
        raise ValueError('Original native report differs')
    assets, sources = [], None
    for asset in manifest['artifacts']:
        name = asset['file']
        if Path(name).name != name or not name.endswith('.zip'):
            raise ValueError('Expected a ZIP leaf name')
        data = (folder/name).read_bytes()
        if digest(data) != asset['sha256'] or len(data) != asset['bytes']:
            raise ValueError('Prepared artifact hash or size differs: ' + name)
        with ZipFile(io.BytesIO(data)) as archive:
            files = {n:archive.read(n) for n in archive.namelist()}
            if len(files) != asset['entries'] or len(files) != len(archive.namelist()):
                raise ValueError('Prepared ZIP member count differs')
        if 'SOURCE_MANIFEST.json' in files:
            if sources is not None:
                raise ValueError('Expected one current source snapshot')
            sources = files
        elif set(files) != {row['path'] for row in asset['files']}:
            raise ValueError('Installation members differ')
        else:
            for row in asset['files']:
                payload = files[row['path']]
                if len(payload) != row['bytes'] or digest(payload) != row['sha256']:
                    raise ValueError('Installation member differs: ' + row['path'])
        assets.append((asset,data))
    if sources is None:
        raise ValueError('Current source snapshot missing')
    snapshot = json.loads(sources['SOURCE_MANIFEST.json'])
    if snapshot['source_commit'] != manifest['source_commit'] or set(snapshot['files']) != set(sources)-{'SOURCE_MANIFEST.json'}:
        raise ValueError('Source snapshot identity differs')
    for name, expected in snapshot['files'].items():
        if digest(sources[name]) != expected:
            raise ValueError('Source snapshot hash differs: ' + name)
    report = manifest['native_build_report_data']
    if report['dll_sha256'] != manifest['core_sha256']:
        raise ValueError('Binary identity differs')
    inputs = report['compile_inputs']
    if set(inputs) != set(build.COMPILE_INPUTS) or inputs != manifest['compile_inputs']:
        raise ValueError('Actual compile input record differs')
    for name, expected in inputs.items():
        if digest(sources[name]) != expected:
            raise ValueError('Compiled input differs: ' + name)
    return manifest, assets


def publish(plan, manifest, assets):
    if plan['version'] != manifest['version'] or plan['tag'] != 'v'+manifest['version']:
        raise ValueError('Explicit release plan differs from prepared version')
    repository = os.environ.get('GITHUB_REPOSITORY', plan['repository'])
    if repository != plan['repository']:
        raise ValueError('Repository differs from explicit release plan')
    token = os.environ.get('GH_TOKEN') or os.environ['GITHUB_TOKEN']
    api = 'https://api.github.com/repos/'+repository

    def request(path, method='GET', payload=None, binary=None):
        url = path if path.startswith('https://') else api+path
        data = binary if binary is not None else (json.dumps(payload).encode() if payload is not None else None)
        headers = {'Authorization':'Bearer '+token,'Accept':'application/vnd.github+json',
                   'User-Agent':'SoraMod-release','X-GitHub-Api-Version':'2022-11-28'}
        if data is not None:
            headers['Content-Type'] = 'application/zip' if binary is not None else 'application/json'
        with urlopen(Request(url,data=data,headers=headers,method=method),timeout=90) as response:
            return json.load(response)

    def optional(path):
        try:
            return request(path)
        except HTTPError as error:
            if error.code != 404:
                raise
            return None

    tag, commit = plan['tag'], manifest['source_commit']
    ref = optional('/git/ref/tags/'+quote(tag,safe=''))
    if ref and ref['object']['sha'] != commit:
        raise ValueError('Existing tag points elsewhere; retained')
    release = optional('/releases/tags/'+quote(tag,safe=''))
    if release and not release['draft']:
        raise ValueError('Existing public release is immutable; retained')
    if release and release['target_commitish'] != commit and ref is None:
        raise ValueError('Existing draft targets another commit; retained')
    existing = {a['name']:a for a in release.get('assets',[])} if release else {}
    expected = {a['file']:a['sha256'] for a,_ in assets}
    if set(existing)-set(expected):
        raise ValueError('Existing draft contains other attachments; retained')
    for name, asset in existing.items():
        if asset.get('digest') != 'sha256:'+expected[name]:
            raise ValueError('Existing attachment has different content; retained: '+name)
    body = plan['body']
    if release is None:
        release = request('/releases','POST',dict(tag_name=tag,target_commitish=commit,name=plan['name'],body=body,draft=True))
    for asset,data in assets:
        if asset['file'] not in existing:
            uploaded = request(release['upload_url'].split('{')[0]+'?name='+quote(asset['file']),'POST',binary=data)
            if uploaded.get('digest') != 'sha256:'+asset['sha256']:
                raise ValueError('Uploaded attachment differs; draft retained')
    current = request('/releases/'+str(release['id'])+'/assets')
    if {a['name']:a.get('digest') for a in current} != {n:'sha256:'+h for n,h in expected.items()}:
        raise ValueError('Uploaded asset set differs; draft retained')
    release = request('/releases/'+str(release['id']),'PATCH',dict(name=plan['name'],body=body,draft=False,prerelease=False))
    print(release['html_url'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--packages',type=Path,default=ROOT/'build/packages')
    parser.add_argument('--plan',type=Path,help='Release repository, version, title and player-facing notes')
    parser.add_argument('--publish',action='store_true')
    args = parser.parse_args()
    manifest, assets = validate(args.packages)
    preview = dict(version=manifest['version'],current_source_commit=manifest['source_commit'],
                   binary_source_commit=manifest['binary_source_commit'],
                   binary_sha256=manifest['core_sha256'],source_only=manifest['source_only'],
                   artifacts=[a for a,_ in assets])
    if args.publish:
        if args.plan is None:
            parser.error('--publish requires an explicit --plan')
        publish(json.loads(args.plan.read_text(encoding='utf-8')),manifest,assets)
    else:
        print('Local preview only; no network or token required.')
        print(json.dumps(preview,indent=2,ensure_ascii=False))


if __name__ == '__main__':
    main()
