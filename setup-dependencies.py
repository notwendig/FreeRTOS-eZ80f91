#!/usr/bin/env python3
"""Prepare pinned public libraries and the project's ZDS II compatibility patches."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parent
DEPENDENCIES = (
    ('kernel', 'https://github.com/FreeRTOS/FreeRTOS-Kernel.git', 'V11.3.2',
     ('kernel-V11.3.2.patch',)),
    ('network', 'https://github.com/FreeRTOS/FreeRTOS-Plus-TCP.git', 'V4.4.2',
     ('network-V4.4.2-zds.patch',)),
    ('uzlib', 'https://github.com/pfalcon/uzlib.git',
     '6d60d651a4499a64f2e5b21b4cc08d98cb84b5c1',
     ('uzlib-6d60d65-zds.patch',)),
)


def git(repo, *args, check=True):
    result = subprocess.run(['git', '-C', str(repo), *args],
                            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if check and result.returncode:
        raise RuntimeError(f'{repo.name}: git {args}: {result.stderr}')
    return result


def main():
    for name, url, revision, patches in DEPENDENCIES:
        repo = ROOT / name
        is_commit = re.fullmatch(r'[0-9a-f]{40}', revision) is not None
        fresh = not (repo / '.git').exists()
        if fresh:
            subprocess.run(['git', 'clone', '--no-checkout', '--depth', '1',
                            url, str(repo)], check=True)
        if git(repo, 'rev-parse', '--verify', revision + '^{commit}', check=False).returncode:
            refspec = [revision] if is_commit else ['tag', revision]
            git(repo, 'fetch', '--depth', '1', 'origin', *refspec)
        expected = git(repo, 'rev-parse', revision + '^{commit}').stdout.strip()
        if fresh:
            git(repo, 'checkout', '--detach', expected)
        current = git(repo, 'rev-parse', 'HEAD').stdout.strip()
        if current != expected:
            descendant = git(repo, 'merge-base', '--is-ancestor', expected, current,
                             check=False).returncode == 0
            applied = all(git(repo, 'apply', '--reverse', '--check',
                              str(ROOT / 'patches' / filename),
                              check=False).returncode == 0 for filename in patches)
            if descendant and applied:
                print(f'{name}: committed {revision} + ZDS patches ready', flush=True)
                continue
            if git(repo, 'status', '--porcelain').stdout:
                raise RuntimeError(f'{name}: local changes; refusing to change version')
            git(repo, 'checkout', '--detach', expected)
        for filename in patches:
            patch = str(ROOT / 'patches' / filename)
            if git(repo, 'apply', '--reverse', '--check', patch, check=False).returncode == 0:
                continue
            result = git(repo, 'apply', '--check', patch, check=False)
            if result.returncode:
                raise RuntimeError(f'{name}: {filename}: {result.stderr}')
            git(repo, 'apply', '--whitespace=nowarn', patch)
        print(f'{name}: {revision} + ZDS patches ready', flush=True)


if __name__ == '__main__':
    main()
