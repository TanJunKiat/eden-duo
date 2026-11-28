#!/usr/bin/env python3
"""Build the opt-in EO III EXP trial without changing the normal manifest."""
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
COMPANION = HERE.parent
ROOT = COMPANION.parent.parent
TITLE = '010084A0176AA000'
BUILD = '3D0322EDA242DEC85321974C126F43AC00000000000000000000000000000000'
VERSION = '0.1.21'
HOOK = 0x1CF0A50


def run(*args):
    subprocess.run(args, check=True)


def add_room_widgets(manifest, widgets):
    def button(key, binding, rect, action=None, argument=None):
        manifest['module_outputs'].append(binding)
        entry = {'kind': 'module', 'action': action or key, 'enabled_bind': binding}
        if argument is not None:
            entry['argument'] = argument
        manifest['actions'][key] = entry
        widgets.append({'type': 'rect', 'id': key, 'rect': rect, 'color': '#00000000',
                        'need_bind': binding, 'on_tap': key})

    button('town_settings', 'town.settings_open', [428, 502, 140, 24])
    button('town_settings_back', 'town.settings_back', [24, 449, 144, 32])
    button('guild_prev', 'town.guild_prev', [24, 459, 114, 34])
    button('guild_next', 'town.guild_next', [454, 459, 114, 34])
    button('bistro_board', 'town.bistro_board', [404, 29, 164, 33], 'town_board')
    button('toggle_bounty', 'town.bounty_toggle', [24, 252, 210, 42])
    button('toggle_weekly', 'town.weekly_toggle', [244, 252, 244, 42])
    for i in range(4):
        button(f'bistro_offer_{i}', f'town.bistro_offer_{i}', [24, 117 + 67*i, 544, 59], 'bistro_offer', i)
    button('bistro_abandon', 'town.bistro_abandon', [24, 421, 150, 33])
    button('bistro_report', 'town.bistro_report', [400, 421, 168, 33])


def make_trial_assets(stage, code, rules_plan=None, bistro_bounty=False, armoroad_rooms=False):
    if rules_plan is None and (not 16 <= len(code) <= 4096 or len(code) % 4 or
                               code[-4:] != bytes.fromhex('00000014')):
        raise ValueError('unexpected guest helper size or branch placeholder')
    destination = stage / 'dualscreen'
    shutil.copytree(COMPANION / 'dualscreen', destination)
    path = destination / 'manifest.json'
    try:
        manifest = json.loads(path.read_text())
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f'cannot load base manifest: {error}') from error
    manifest['name'] += (' — Armoroad Rooms Trial' if armoroad_rooms else
                         ' — Expedition Rules Trial' if rules_plan else ' — Deep Run Trial')
    manifest['load_plan'] = 'guest/<build>/load-plan.json'
    manifest['module_outputs'].append('town.exp_controls')
    widgets = manifest['pages'][0]['widgets']
    for index, rate in enumerate((0, 2, 5, 10)):
        key = f'exp_rate_{rate}'
        manifest['actions'][key] = {'kind': 'module', 'action': 'exp_rate', 'argument': rate,
                                    'enabled_bind': 'town.exp_controls'}
        widgets.append({'type': 'rect', 'id': key, 'rect': [302+67*index, 34, 64, 30],
                        'color': '#00000000', 'need_bind': 'town.exp_controls', 'on_tap': key})
    manifest['actions']['deep_run'] = {'kind': 'module', 'action': 'deep_run',
                                       'enabled_bind': 'town.exp_controls'}
    widgets.append({'type': 'rect', 'id': 'deep_run', 'rect': [424, 204, 144, 30],
                    'color': '#00000000', 'need_bind': 'town.exp_controls', 'on_tap': 'deep_run'})
    if rules_plan:
        rules = [(124, 'guild_training'), (224, 'guild_rotation')]
        if not armoroad_rooms:
            rules.append((324, 'merchant_favor'))
        for x, key in rules:
            manifest['actions'][key] = {'kind': 'module', 'action': key,
                                        'enabled_bind': 'town.exp_controls'}
            widgets.append({'type': 'rect', 'id': key, 'rect': [x, 204, 96, 30],
                            'color': '#00000000', 'need_bind': 'town.exp_controls', 'on_tap': key})
    if armoroad_rooms:
        if not bistro_bounty or not rules_plan:
            raise ValueError('room widgets require the bounty and expedition rules module')
        add_room_widgets(manifest, widgets)
    elif bistro_bounty:
        manifest['name'] += ' + Bistro Bounty'
        manifest['module_outputs'].append('town.bounty_action')
        manifest['actions']['bistro_bounty'] = {'kind': 'module', 'action': 'bistro_bounty',
                                               'enabled_bind': 'town.bounty_action'}
        widgets.append({'type': 'rect', 'id': 'bistro_bounty', 'rect': [408, 502, 160, 24],
                        'color': '#00000000', 'need_bind': 'town.bounty_action', 'on_tap': 'bistro_bounty'})
    path.write_text(json.dumps(manifest, indent=2) + '\n')

    def reloc(offset, kind, target, addend=0):
        return {'offset': offset, 'kind': kind, 'target': target, 'addend': addend}

    layout = {'segments': [{'location': location, 'size': size} for location, size in
                           [(0, 68559992), (68562944, 31062004), (99627008, 5292376)]],
              'bss_size': 3612328}
    plan = {
        'title_id': TITLE, 'build_id': BUILD, 'module': 'main', 'format': 2, 'layout': layout,
        'mailbox_size': 4096, 'mailbox_epoch_offset': 4092,
        'patches': [{'offset': HOOK, 'expected': 'e83e4339', 'replacement': '00000014',
                     'relocations': [reloc(0, 'aarch64_branch26', 'code')]}],
        'guest_code': {'bytes': code.hex(), 'relocations': [
            reloc(0, 'aarch64_adrp', 'mailbox'), reloc(4, 'aarch64_add_lo12', 'mailbox'),
            reloc(len(code)-4, 'aarch64_branch26', 'main', HOOK+4)]},
    }
    path = destination / 'guest' / BUILD / 'load-plan.json'
    path.parent.mkdir(parents=True)
    path.write_text(json.dumps(rules_plan if rules_plan is not None else plan, indent=2) + '\n')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ndk-bin', type=Path, required=True)
    p.add_argument('--packager', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    variants = p.add_mutually_exclusive_group()
    variants.add_argument('--expedition-rules', action='store_true', help='Build the v0.1.22 rules trial without Bistro Bounty')
    variants.add_argument('--bistro-bounty', action='store_true', help='Build the v0.1.23 regular bounty trial')
    variants.add_argument('--armoroad-rooms', action='store_true', help='Build the v0.1.25 Armoroad Rooms trial')
    args = p.parse_args()
    bistro_bounty = args.bistro_bounty or args.armoroad_rooms
    version = '0.1.25' if args.armoroad_rooms else '0.1.23' if args.bistro_bounty else '0.1.22' if args.expedition_rules else VERSION
    args.output.mkdir(parents=True, exist_ok=True)
    final = args.output / f'{TITLE}-v{version}.dsmod.zip'
    if final.exists():
        p.error(f'refusing to overwrite {final}')
    with tempfile.TemporaryDirectory(prefix='eo3-exp-trial-') as directory:
        work = Path(directory)
        clang = args.ndk_bin / 'aarch64-linux-android24-clang++'
        stage = work/'stage'
        defines = ['-DETRIAN_EXP_TRIAL']
        if args.expedition_rules or bistro_bounty:
            from build_training_research_plan import make_plan
            plan = make_plan(args.ndk_bin, bounty=False, production_bounty=bistro_bounty, weekly=args.armoroad_rooms)
            make_trial_assets(stage, b'', plan, bistro_bounty=bistro_bounty, armoroad_rooms=args.armoroad_rooms)
            defines.append('-DETRIAN_EXPEDITION_RULES')
            if bistro_bounty:
                defines.append('-DETRIAN_BISTRO_BOUNTY')
            if args.armoroad_rooms:
                defines.append('-DETRIAN_ARMOROAD_ROOMS')
        else:
            run(clang, '-c', HERE/'reward.s', '-o', work/'reward.o')
            run(args.ndk_bin/'llvm-objcopy', f'--dump-section=.text={work / "reward.bin"}', work/'reward.o')
            make_trial_assets(stage, (work/'reward.bin').read_bytes())
        module = work/f'{TITLE}.so'
        run(clang, '-std=c++20', '-O2', '-shared', '-fPIC', '-static-libstdc++', *defines,
            '-I', ROOT/'src', COMPANION/f'{TITLE}.cpp', '-o', module)
        run(args.ndk_bin/'llvm-strip', '--strip-all', module)
        run(sys.executable, args.packager, '--package', stage, '--output', work/'archive',
            '--version', version, '--module', f'android-arm64-v8a={module}', '--build-id', BUILD,
            '--min-runtime', '25')
        with (work/'archive'/f'{TITLE}.dsmod.zip').open('rb') as source, final.open('xb') as target:
            shutil.copyfileobj(source, target)
    print(final)


if __name__ == '__main__':
    main()
