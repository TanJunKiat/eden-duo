// SPDX-License-Identifier: GPL-3.0-or-later
#include "etrian_exp_trial.h"
#include <cassert>
int main() {
    using etrian::exp_trial::command;
    using etrian::exp_trial::reset_required;
    assert(command(5,42)==(uint64_t{42}<<32 | 5));
    assert(command(10,42)==(uint64_t{42}<<32 | 10));
    assert(command(2,0)==2 && command(10,42)!=command(10,43));
    assert(command(3,42)==0 && command(11,42)==0);
    assert(command(0,42)==0 && command(1,42)==0 && command(4,42)==0);
    assert(command(-1,42)==0 && command(5,-1)==0);
    assert(reset_required(8,8,42,42)==false);
    assert(reset_required(8,9,42,42) && reset_required(8,0,42,42));
    assert(reset_required(8,8,42,43) && reset_required(8,8,42,-1));
    using etrian::exp_trial::RuleLifecycle;
    using etrian::exp_trial::rule_lifecycle;
    assert(rule_lifecycle(8,42,0,42,false)==RuleLifecycle::waiting); // inn fade
    assert(rule_lifecycle(8,42,8,-1,false)==RuleLifecycle::waiting);
    assert(rule_lifecycle(8,42,8,42,false)==RuleLifecycle::bound);
    assert(rule_lifecycle(8,42,0,42,true)==RuleLifecycle::reset); // reopen same slot from title
    assert(rule_lifecycle(8,42,9,-1,false)==RuleLifecycle::reset); // changed save still wins
    assert(rule_lifecycle(8,42,0,43,false)==RuleLifecycle::reset); // new epoch while loading
    assert(command(0,42,true)==(uint64_t{42}<<32 | 0x100));
    assert(command(10,42,true)==(uint64_t{42}<<32 | 0x10a));
    assert(command(10,-1,true)==0 && command(3,42,true)==0);
    assert(etrian::exp_trial::deep_run_quarters(0)==4 && etrian::exp_trial::deep_run_quarters(1)==5);
    assert(etrian::exp_trial::deep_run_quarters(2)==6 && etrian::exp_trial::deep_run_quarters(3)==7);
    assert(etrian::exp_trial::deep_run_quarters(4)==8 && etrian::exp_trial::deep_run_quarters(999)==8);

    using etrian::exp_trial::Rules;
    assert(command(10,42,true,Rules{100,true,true})==(uint64_t{42}<<32 | 0x1d0a));
    assert(command(0,42,false,Rules{50,false,false})==(uint64_t{42}<<32 | 0x200));
    assert(command(0,42,false,Rules{})==0);
    assert(command(0,42,false,Rules{75,true,true})==0);
    assert(command(0,-1,false,Rules{100,true,true})==0);

    using etrian::exp_trial::guild_training_award;
    // B1F baseline: 144 total, three survivors, one reserve earned 1 natively.
    assert(guild_training_award(1,144,3,0)==1);
    assert(guild_training_award(1,144,3,50)==24);
    assert(guild_training_award(0,144,3,100)==48);
    assert(guild_training_award(72,144,3,100)==72); // Do not replace a better native award.
    assert(guild_training_award(0,219,3,50)==36); // Fractional EXP rounds down.
    assert(guild_training_award(0,219,2,100)==109); // Only survivors count.
    assert(guild_training_award(0,2,3,100)==1); // Native ordinary battle minimum.
    assert(guild_training_award(0,2,3,50)==0);
    assert(guild_training_award(0,999999,1,100)==999999);
    assert(guild_training_award(1,0,3,50)==1 && guild_training_award(1,-1,3,100)==1);
    assert(guild_training_award(1,1000000,3,100)==1);
    assert(guild_training_award(1,144,0,100)==1 && guild_training_award(1,144,6,100)==1);
    assert(guild_training_award(1,144,3,-1)==1 && guild_training_award(1,144,3,75)==1);

    using etrian::exp_trial::guild_rotation_award;
    assert(guild_rotation_award(96,144,3,1,2)==120); // 2× Boost does not boost the bonus.
    assert(guild_rotation_award(48,144,3,2,2)==48); // Equal to strongest survivor.
    assert(guild_rotation_award(0,144,3,1,2)==0); // No native award (including KO).
    assert(guild_rotation_award(48,144,3,0,2)==48); // Unavailable level.
    assert(guild_rotation_award(999990,144,3,1,2)==999999);

    using etrian::exp_trial::merchant_favor_price;
    assert(merchant_favor_price(4,true,true)==6);
    assert(merchant_favor_price(5,true,true)==7);
    assert(merchant_favor_price(28,false,true)==28); // Equipment or mixed basket.
    assert(merchant_favor_price(4,true,false)==4);
    assert(merchant_favor_price(10001,true,true)==15001);
    assert(merchant_favor_price(11000,true,true)==16000); // Capped bonus.
    assert(merchant_favor_price(0,true,true)==0 && merchant_favor_price(-4,true,true)==-4);
}
