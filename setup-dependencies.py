#!/usr/bin/env python3
"""Prepare the public library tags and the project's ZDS II compatibility patches."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent
DEPENDENCIES = (
    ('kernel', 'https://github.com/FreeRTOS/FreeRTOS-Kernel.git', 'V10.6.1',
     ('kernel-V10.6.1.patch',)),
    ('network', 'https://github.com/FreeRTOS/FreeRTOS-Plus-TCP.git', 'V3.1.0',
     ('network-V3.1.0.patch', 'network-zds-tick-print.patch')),
    ('uzlib', 'https://github.com/pfalcon/uzlib.git', 'v2.9.5',
     ('uzlib-v2.9.5.patch', 'uzlib-zds-inline.patch', 'uzlib-zds-checksum.patch')),
)


def git(repo, *args, check=True):
    return subprocess.run(['git', '-C', str(repo), *args], check=check,
                          text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def main():
    for name, url, tag, patches in DEPENDENCIES:
        repo = ROOT / name
        if not (repo / '.git').exists():
            subprocess.run(['git', 'clone', '--branch', tag, '--depth', '1', url, str(repo)], check=True)
        expected = git(repo, 'rev-parse', tag + '^{commit}').stdout.strip()
        current = git(repo, 'rev-parse', 'HEAD').stdout.strip()
        if current != expected:
            # Keep committed compatibility fixes referenced by this project's
            # gitlinks; checking out the upstream tag would discard them.
            descendant = git(repo, 'merge-base', '--is-ancestor', expected, current,
                             check=False).returncode == 0
            applied = all(git(repo, 'apply', '--reverse', '--check',
                              str(ROOT / 'patches' / filename),
                              check=False).returncode == 0 for filename in patches)
            if descendant and applied:
                print(f'{name}: committed {tag} + ZDS patches ready', flush=True)
                continue
            if git(repo, 'status', '--porcelain').stdout:
                raise RuntimeError(f'{name}: local changes; refusing to change version')
            git(repo, 'checkout', '--detach', tag)
        for filename in patches:
            patch = str(ROOT / 'patches' / filename)
            if git(repo, 'apply', '--reverse', '--check', patch, check=False).returncode == 0:
                continue
            result = git(repo, 'apply', '--check', patch, check=False)
            if result.returncode:
                raise RuntimeError(f'{name}: {filename}: {result.stderr}')
            git(repo, 'apply', '--whitespace=nowarn', patch)
        print(f'{name}: {tag} + ZDS patches ready', flush=True)


if __name__ == '__main__':
    main()
