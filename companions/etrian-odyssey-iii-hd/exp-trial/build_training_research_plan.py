#!/usr/bin/env python3
"""Generate unpackaged Expedition Rules probes for a disposable lab profile."""
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
import json
import subprocess
import tempfile
from pathlib import Path

from build_package import BUILD, HERE, HOOK, TITLE

PATCHES = (
    (HOOK, 'e83e4339', 0),
    (0x17BAC20, '086940b9', 'training_split'),
    (0x1F9D1C4, '6f0cf097', 'training_result'),
    (0x1BA04C8, 'a0fbff54', 'training_untrained'),
    (0x1BA05FC, '4901090b', 'training_trained'),
    (0x17BA7F0, '14386494', 'rotation_party'),
    (0x17BA8B0, 'f44f46a9', 'rotation_finish'),
    (0x2151920, 'ea0f1bfc', 'favor_paid_am'),
    (0x2152080, 'ea0f1bfc', 'favor_paid_pm'),
    (0x2DADD1C, '3d000094', 'favor_sale'),
    (0x2DB2E0C, '01ecff97', 'favor_sale'),
    (0x17BCDF0, 'fd7bbda9', 'bounty_battle_init'),
    (0x25C1A00, '78264079', 'bounty_encounter'),
    (0x31DB0CC, 'a975ac97', 'bounty_death'),
    (0x17BCDF0, 'fd7bbda9', 'regular_battle_init'),
    (0x17BCE80, '7c2c3894', 'regular_normal_encounter'),
    (0x31DB0EC, 'd550ac97', 'regular_death'),
    (0x1CE7870, 'fd7bbda9', 'bistro_claim'),
)
WEEKLY_CALLS = ((0x2DACC88, '8a24d397'), (0x2DAFBAC, 'c118d397'),
                (0x2DAFDD8, '3618d397'), (0x2DB0F3C, 'dd13d397'),
                (0x2DB1168, '5213d397'), (0x2DB1D2C, '6110d397'),
                (0x2DB4D60, '5404d397'), (0x2DB5BCC, 'b900d397'),
                (0x362ACA4, '832cb197'), (0x362D0B4, '7f23b197'), (0x362EB84, 'cb1cb197'),
                # Obfuscated copies call the equivalent pointer-taking 0x2275460 wrapper.
                (0x2DAD8D8, 'e21ed397'), (0x2DAEEEC, '5d19d397'), (0x2DB3EAC, '6d05d397'),
                (0x362A974, 'bb2ab197'), (0x362E134, 'cb1cb197'))
BRANCHES = {
    'weekly_native_price': 0x2275EB0,
    'reward_return': HOOK + 4,
    'training_split_return': 0x17BAC24,
    'training_result_call': 0x1BA0380,
    'training_result_return': 0x1F9D1C8,
    'training_untrained_store': 0x1BA0600,
    'training_untrained_return': 0x1BA04CC,
    'training_untrained_skip': 0x1BA043C,
    'training_trained_store': 0x1BA0600,
    'rotation_party_call': 0x30C8840,
    'rotation_finish_return': 0x17BA8B4,
    'favor_paid_am_return': 0x2151924,
    'favor_paid_pm_return': 0x2152084,
    'favor_material_call': 0x2276CC0,
    'favor_sale_call': 0x2DADE10,
    'bounty_battle_init_return': 0x17BCDF4,
    'bounty_encounter_return': 0x25C1A04,
    'bounty_death_call': 0x1CF8770,
    'regular_battle_init_return': 0x17BCDF4,
    'regular_normal_encounter_call': 0x25C8070,
    'regular_death_call': 0x1CEF440,
    'bistro_party_call': 0x1E13980,
    'bistro_packet_call': 0x2EBD7F0,
    'bistro_current_exp_call': 0x1CF4ED0,
    'bistro_next_exp_call': 0x1CF4ED0,
    'bistro_resolve_town_call': 0x3A3970,
    'bistro_town_instance_call': 0x3142270,
    'bistro_resolve_result_call': 0x3A3970,
    'bistro_alloc_action_call': 0x3A3B00,
    'bistro_action_ctor_call': 0x2BD7BB0,
    'bistro_instantiate_call': 0x18A4300,
    'bistro_roster_barrier_call': 0x3A38C0,
    'bistro_packet_barrier_call': 0x3A38C0,
    'bistro_callback_barrier_call': 0x3A38C0,
    'bistro_original_return': 0x1CE7874,
}
BISTRO_METADATA = {
    'bistro_town_metadata': 0x63B09A0,
    'bistro_result_metadata': 0x63F8F60,
    'bistro_action_type': 0x63991F8,
    'bistro_report_metadata': 0x63B09B8,
}
LINKED_HELPER_BRANCH = {'training_result_call', 'favor_material_call', 'weekly_native_price'} | {
    name for name in BRANCHES if name.startswith('bistro_') and name.endswith('_call')
}
LINKED_GAME_PATCH = {'rotation_party', 'favor_sale', 'bounty_death', 'regular_death', 'regular_normal_encounter', 'weekly_price'}


def make_plan(tools, bounty=True, regular_bounty=False, bistro_claim=False, production_bounty=False, weekly=False):
    if weekly and not production_bounty:
        raise ValueError('weekly pricing is specific to Armoroad Rooms')
    if production_bounty:
        if bounty or regular_bounty or bistro_claim:
            raise ValueError('production bounty cannot contain lab modes')
        regular_bounty = bistro_claim = True
    if bounty and regular_bounty:
        raise ValueError('the two lab probes share a battle-init patch')

    production_sites = ('bistro_party_mailbox', 'bistro_empty_mailbox', 'bistro_failed_mailbox')

    def included(site):
        name = str(site)
        return ((bounty or not name.startswith('bounty_'))
                and (regular_bounty or not name.startswith('regular_'))
                and (bistro_claim or not name.startswith('bistro_'))
                and (production_bounty or name not in production_sites)
                and (not weekly or not name.startswith('favor_'))
                and (weekly or not name.startswith('weekly_')))

    with tempfile.TemporaryDirectory(prefix='eo3-training-plan-') as tmp:
        source, obj, code = (Path(tmp) / p for p in ('helper.s', 'helper.o', 'helper.bin'))
        sources = ('reward.s', 'training-research.s', 'weekly-sale.s' if weekly else 'favor-research.s')
        defines = '.set TRAINING_RESEARCH, 1\n'
        if bounty:
            defines += '.set BOUNTY_RESEARCH, 1\n'
            sources += ('bounty-research.s',)
        if production_bounty:
            defines += '.set BISTRO_BOUNTY, 1\n'
        if weekly:
            defines += '.set ARMOROAD_ROOMS, 1\n'
        if regular_bounty:
            if not production_bounty:
                defines += '.set REGULAR_BOUNTY_RESEARCH, 1\n'
            sources += ('regular-bounty-research.s',)
        if bistro_claim:
            sources += ('bistro-claim-research.s',)
        source.write_text(defines + '\n'.join((HERE / name).read_text() for name in sources))
        subprocess.run([tools / 'aarch64-linux-android24-clang++', '-c', source, '-o', obj], check=True)
        subprocess.run([tools / 'llvm-objcopy', f'--dump-section=.text={code}', obj], check=True)
        names = subprocess.check_output([tools / 'llvm-nm', '--defined-only', '--format=posix', obj], text=True)
        symbols = {name: int(address, 16) for name, _, address, *_ in map(str.split, names.splitlines())}
        data = code.read_bytes()

    def reloc(offset, kind, target, addend=0):
        return {'offset': offset, 'kind': 'aarch64_' + kind, 'target': target, 'addend': addend}

    if len(data) % 4 or not 0 < len(data) <= 4096:
        raise ValueError('invalid helper size')
    relocations = []
    for site in (0, 'training_split', 'training_result', 'training_result_clear',
                 'training_untrained_mailbox', 'training_trained', 'rotation_party', 'rotation_finish',
                 'favor_paid_am', 'favor_paid_pm', 'favor_sale', 'favor_consume_mailbox', 'weekly_mailbox',
                 'bounty_battle_init', 'bounty_encounter', 'bounty_death',
                 'regular_battle_init', 'regular_normal_encounter', 'regular_death',
                 'bistro_claim', 'bistro_result_mailbox', *production_sites):
        if not included(site):
            continue
        offset = symbols[site] if isinstance(site, str) else site
        if data[offset:offset + 8].hex() != '1100009031020091':
            raise ValueError(f'invalid mailbox address placeholder: {site}')
        relocations += [reloc(offset, 'adrp', 'mailbox'), reloc(offset + 4, 'add_lo12', 'mailbox')]
    for site, addr in BRANCHES.items():
        if not included(site):
            continue
        offset = symbols[site]
        if data[offset:offset + 4].hex() != ('00000094' if site in LINKED_HELPER_BRANCH else '00000014'):
            raise ValueError(f'invalid branch placeholder: {site}')
        relocations.append(reloc(offset, 'branch26', 'main', addr))

    if bistro_claim:
        for site, addr in BISTRO_METADATA.items():
            offset = symbols[site]
            adrp = int.from_bytes(data[offset:offset + 4], 'little')
            ldr = int.from_bytes(data[offset + 4:offset + 8], 'little')
            if adrp & 0xffffffe0 != 0x90000000 or ldr != 0xf9400000 | (adrp & 31) * 33:
                raise ValueError(f'invalid metadata placeholders: {site}')
            relocations += [reloc(offset, 'adrp', 'main', addr), reloc(offset + 4, 'ldr64_lo12', 'main', addr)]

    segments = [{'location': offset, 'size': size} for offset, size in
                [(0, 68559992), (68562944, 31062004), (99627008, 5292376)]]
    patches = [{'offset': addr, 'expected': expect,
                'replacement': '00000094' if site in LINKED_GAME_PATCH else '00000014',
                'relocations': [reloc(0, 'branch26', 'code', symbols[site] if isinstance(site, str) else site)]}
               for addr, expect, site in PATCHES + (tuple((addr, expect, 'weekly_price') for addr, expect in WEEKLY_CALLS)
                                                   if weekly else ()) if included(site)]
    return {
        'title_id': TITLE, 'build_id': BUILD, 'module': 'main', 'format': 2,
        'layout': {'segments': segments, 'bss_size': 3612328},
        'mailbox_size': 4096, 'mailbox_epoch_offset': 4092, 'patches': patches,
        'guest_code': {'bytes': data.hex(), 'relocations': relocations},
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ndk-bin', type=Path, required=True)
    probes = parser.add_mutually_exclusive_group()
    probes.add_argument('--regular-bounty', action='store_true', help='Observe regular kills instead of FOEs')
    probes.add_argument('--bistro-claim', action='store_true', help='Lab-only native town result')
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    plan = make_plan(args.ndk_bin, bounty=not (args.regular_bounty or args.bistro_claim),
                     regular_bounty=args.regular_bounty, bistro_claim=args.bistro_claim)
    with args.output.open('x') as handle:
        json.dump(plan, handle, indent=2)
        handle.write('\n')
