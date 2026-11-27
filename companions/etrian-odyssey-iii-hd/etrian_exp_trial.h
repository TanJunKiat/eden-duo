// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>

namespace etrian::exp_trial {
constexpr bool supported_rate(int64_t rate) { return rate==0 || rate==2 || rate==5 || rate==10; }
inline constexpr uint32_t deep_run_mask=0x100, win_offset=8;
constexpr uint32_t deep_run_quarters(uint32_t wins) { return 4+(wins<4?wins:4); }
struct Rules {
    int training=0;
    bool rotation=false, favor=false;
};
inline constexpr uint32_t favor_rng_offset=64, favor_pending_offset=68, favor_epoch_offset=72;
constexpr uint64_t command(int64_t rate,int64_t epoch,bool deep_run=false,Rules rules={}) {
    if(!supported_rate(rate) || epoch<0 || epoch>UINT32_MAX ||
       (rules.training!=0 && rules.training!=50 && rules.training!=100)) return 0;
    const uint64_t bits=static_cast<uint64_t>(rate) | (deep_run?deep_run_mask:0) |
        (rules.training==50?0x200:rules.training==100?0x400:0) |
        (rules.rotation?0x800:0) | (rules.favor?0x1000:0);
    return bits?(static_cast<uint64_t>(epoch)<<32) | bits:0;
}
// Reference policy for one eligible reserve in an ordinary native victory.
// Inputs are award deltas, never accumulated character EXP. The native guest
// hook still has to establish eligibility and match this policy.
constexpr uint32_t guild_training_award(uint32_t combat_study,int32_t unboosted_total,
                                        int32_t survivors,int32_t percent) {
    if((percent!=50 && percent!=100) || unboosted_total<=0 || unboosted_total>999999 ||
       survivors<1 || survivors>5) return combat_study;
    const auto divided=static_cast<uint32_t>(unboosted_total/survivors);
    const auto per_member=divided?divided:1;
    const auto training=percent==50?per_member/2:per_member;
    return combat_study>training?combat_study:training;
}
// Reference policy for one active slot, after the native survivor split.
constexpr uint32_t guild_rotation_award(uint32_t native,uint32_t unboosted_total,
                                        int32_t survivors,int32_t level,int32_t leader) {
    if(!native || native>999999 || level<1 || leader>99 || level>=leader ||
       unboosted_total>999999) return native;
    const auto bonus=guild_training_award(0,static_cast<int32_t>(unboosted_total),survivors,50);
    return native+bonus>999999?999999:native+bonus;
}
inline constexpr int32_t favor_bonus_cap=5000, native_gold_cap=99999999;
constexpr int32_t merchant_favor_price(int32_t price,bool all_materials,bool ready) {
    if(!all_materials || !ready || price<=1 || price>native_gold_cap) return price;
    const auto half=price/2;
    return price+(half>favor_bonus_cap?favor_bonus_cap:half);
}
constexpr bool reset_required(uint64_t old_save,uint64_t save,int64_t old_epoch,int64_t epoch) {
    return !save || old_save!=save || old_epoch!=epoch || epoch<0 || epoch>UINT32_MAX;
}

enum class RuleLifecycle { waiting, bound, reset };
constexpr RuleLifecycle rule_lifecycle(uint64_t old_save,int64_t old_epoch,uint64_t save,int64_t epoch,bool title,
                                       bool continuous_inn=false) {
    const bool epoch_ok=epoch>=0 && epoch<=UINT32_MAX;
    if(title || (epoch_ok && old_epoch!=epoch)) return RuleLifecycle::reset;
    if(save && old_save!=save && !(old_save && epoch_ok && continuous_inn)) return RuleLifecycle::reset;
    return save && epoch_ok?RuleLifecycle::bound:RuleLifecycle::waiting;
}
} // namespace etrian::exp_trial
