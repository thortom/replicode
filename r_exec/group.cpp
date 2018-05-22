//	group.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel nor the
//     names of their contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
//	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "group.h"

#include <math.h>                   // for fabs
#include <r_code/atom.h>            // for Atom, Atom::::COMPOSITE_STATE, etc
#include <r_code/list.h>            // for list<>::const_iterator, list, etc
#include <r_code/replicode_defs.h>  // for GRP_ACT_THR, GRP_C_ACT, etc
#include <r_code/utils.h>           // for Utils, Utils::MaxTime
#include <r_exec/cpp_programs.h>    // for CPPPrograms
#include <r_exec/cst_controller.h>  // for CSTController
#include <r_exec/factory.h>         // for MkActChg, MkHighAct, MkHighSln, etc
#include <r_exec/group.h>           // for Group, Group::GroupState, etc
#include <r_exec/init.h>            // for Now
#include <r_exec/mdl_controller.h>  // for MDLController, etc
#include <r_exec/mem.h>             // for _Mem, OUTPUT
#include <r_exec/opcodes.h>         // for Opcodes, Opcodes::MkGrpPair
#include <r_exec/overlay.h>         // for Controller
#include <r_exec/pgm_controller.h>  // for AntiPGMController, PGMController, etc
#include <r_exec/time_job.h>        // for AntiPGMSignalingJob, etc
#include <cstdint>                  // for uint64_t, uint16_t, uint32_t
#include <ostream>                  // for operator<<, basic_ostream, etc
#include <string>                   // for operator<<, char_traits, string
#include <unordered_set>            // for unordered_set
#include <utility>                  // for pair

#include <replicode_common.h>      // for DebugStream, debug


namespace r_exec
{

Group::Group(r_code::Mem *m): LObject(m)
{
    reset_ctrl_values();
    reset_stats();
    reset_decay_values();
}

Group::Group(r_code::SysObject *source): LObject(source)
{
    reset_ctrl_values();
    reset_stats();
    reset_decay_values();
}

Group::~Group()
{
    invalidate();
}

bool Group::invalidate()
{
    if (LObject::invalidate()) {
        return true;
    }

    // unregister from all groups it views.
    std::unordered_map<uint64_t, P<View> >::const_iterator gv;

    for (gv = group_views.begin(); gv != group_views.end(); ++gv) {
        Group *group = (Group *)gv->second->object;
        std::lock_guard<std::mutex> guard(group->mutex);
        group->viewing_groups.erase(this);
    }

    /* We keep the group intact: the only thing is now the group will not be updated anymore.
    // remove all views that are hosted by this group.
    FOR_ALL_VIEWS_BEGIN(this,v)

    v->second->object->acq_views();
    v->second->object->views.erase(v->second); // delete view from object's views.
    v->second->object->rel_views();

    FOR_ALL_VIEWS_END

    notification_views.clear();
    ipgm_views.clear();
    anti_ipgm_views.clear();
    input_less_ipgm_views.clear();
    other_views.clear();
    group_views.clear();
    */
    return false;
}

uint32_t Group::get_upr() const
{
    return code(GRP_UPR).asFloat();
}

float Group::get_sln_thr() const
{
    return code(GRP_SLN_THR).asFloat();
}

float Group::get_act_thr() const
{
    return code(GRP_ACT_THR).asFloat();
}

float Group::get_vis_thr() const
{
    return code(GRP_VIS_THR).asFloat();
}

float Group::get_c_sln_thr() const
{
    return code(GRP_C_SLN_THR).asFloat();
}

float Group::get_c_act_thr() const
{
    return code(GRP_C_ACT_THR).asFloat();
}

float Group::get_c_sln() const
{
    return code(GRP_C_SLN).asFloat();
}

float Group::get_c_act() const
{
    return code(GRP_C_ACT).asFloat();
}

void Group::mod_sln_thr(double value)
{
    ++sln_thr_changes;
    acc_sln_thr += value;
}

void Group::set_sln_thr(double value)
{
    ++sln_thr_changes;
    acc_sln_thr += value - get_sln_thr();
}

void Group::mod_act_thr(double value)
{
    ++act_thr_changes;
    acc_act_thr += value;
}

void Group::set_act_thr(double value)
{
    ++act_thr_changes;
    acc_act_thr += value - get_act_thr();
}

void Group::mod_vis_thr(double value)
{
    ++vis_thr_changes;
    acc_vis_thr += value;
}

void Group::set_vis_thr(double value)
{
    ++vis_thr_changes;
    acc_vis_thr += value - get_vis_thr();
}

void Group::mod_c_sln(double value)
{
    ++c_sln_changes;
    acc_c_sln += value;
}

void Group::set_c_sln(double value)
{
    ++c_sln_changes;
    acc_c_sln += value - get_c_sln();
}

void Group::mod_c_act(double value)
{
    ++c_act_changes;
    acc_c_act += value;
}

void Group::set_c_act(double value)
{
    ++c_act_changes;
    acc_c_act += value - get_c_act();
}

void Group::mod_c_sln_thr(double value)
{
    ++c_sln_thr_changes;
    acc_c_sln_thr += value;
}

void Group::set_c_sln_thr(double value)
{
    ++c_sln_thr_changes;
    acc_c_sln_thr += value - get_c_sln_thr();
}

void Group::mod_c_act_thr(double value)
{
    ++c_act_thr_changes;
    acc_c_act_thr += value;
}

void Group::set_c_act_thr(double value)
{
    ++c_act_thr_changes;
    acc_c_act_thr += value - get_c_act_thr();
}

float Group::get_sln_chg_thr()
{
    return code(GRP_SLN_CHG_THR).asFloat();
}

float Group::get_sln_chg_prd()
{
    return code(GRP_SLN_CHG_PRD).asFloat();
}

float Group::get_act_chg_thr()
{
    return code(GRP_ACT_CHG_THR).asFloat();
}

float Group::get_act_chg_prd()
{
    return code(GRP_ACT_CHG_PRD).asFloat();
}

float Group::get_avg_sln()
{
    return code(GRP_AVG_SLN).asFloat();
}

float Group::get_high_sln()
{
    return code(GRP_HIGH_SLN).asFloat();
}

float Group::get_low_sln()
{
    return code(GRP_LOW_SLN).asFloat();
}

float Group::get_avg_act()
{
    return code(GRP_AVG_ACT).asFloat();
}

float Group::get_high_act()
{
    return code(GRP_HIGH_ACT).asFloat();
}

float Group::get_low_act()
{
    return code(GRP_LOW_ACT).asFloat();
}

float Group::get_high_sln_thr()
{
    return code(GRP_HIGH_SLN_THR).asFloat();
}

float Group::get_low_sln_thr()
{
    return code(GRP_LOW_SLN_THR).asFloat();
}

float Group::get_sln_ntf_prd()
{
    return code(GRP_SLN_NTF_PRD).asFloat();
}

float Group::get_high_act_thr()
{
    return code(GRP_HIGH_ACT_THR).asFloat();
}

float Group::get_low_act_thr()
{
    return code(GRP_LOW_ACT_THR).asFloat();
}

float Group::get_act_ntf_prd()
{
    return code(GRP_ACT_NTF_PRD).asFloat();
}

float Group::get_low_res_thr()
{
    return code(GRP_LOW_RES_THR).asFloat();
}

float Group::get_ntf_new()
{
    return code(GRP_NTF_NEW).asFloat();
}

uint16_t Group::get_ntf_grp_count()
{
    return code(code(GRP_NTF_GRPS).asIndex()).getAtomCount();
}

Group *Group::get_ntf_grp(uint16_t i)
{
    if (code(code(GRP_NTF_GRPS).asIndex() + i).readsAsNil()) {
        return this;
    }

    uint16_t index = code(code(GRP_NTF_GRPS).asIndex() + i).asIndex();
    return (Group *)get_reference(index);
}

void Group::_mod_0_positive(uint16_t member_index, float value)
{
    float v = code(member_index).asFloat() + value;

    if (v < 0) {
        v = 0;
    }

    code(member_index) = Atom::Float(v);
}

void Group::_mod_0_plus1(uint16_t member_index, float value)
{
    float v = code(member_index).asFloat() + value;

    if (v < 0) {
        v = 0;
    } else if (v > 1) {
        v = 1;
    }

    code(member_index) = Atom::Float(v);
}

void Group::_mod_minus1_plus1(uint16_t member_index, float value)
{
    float v = code(member_index).asFloat() + value;

    if (v < -1) {
        v = -1;
    } else if (v > 1) {
        v = 1;
    }

    code(member_index) = Atom::Float(v);
}

void Group::_set_0_positive(uint16_t member_index, float value)
{
    if (value < 0) {
        code(member_index) = Atom::Float(0);
    } else {
        code(member_index) = Atom::Float(value);
    }
}

void Group::_set_0_plus1(uint16_t member_index, float value)
{
    if (value < 0) {
        code(member_index) = Atom::Float(0);
    } else if (value > 1) {
        code(member_index) = Atom::Float(1);
    } else {
        code(member_index) = Atom::Float(value);
    }
}

void Group::_set_minus1_plus1(uint16_t member_index, float value)
{
    if (value < -1) {
        code(member_index) = Atom::Float(-1);
    } else if (value > 1) {
        code(member_index) = Atom::Float(1);
    } else {
        code(member_index) = Atom::Float(value);
    }
}

void Group::_set_0_1(uint16_t member_index, float value)
{
    if (value == 0 || value == 1) {
        code(member_index) = Atom::Float(value);
    }
}

void Group::mod(uint16_t member_index, float value)
{
    switch (member_index) {
    case GRP_UPR:
    case GRP_DCY_PRD:
    case GRP_SLN_CHG_PRD:
    case GRP_ACT_CHG_PRD:
    case GRP_SLN_NTF_PRD:
    case GRP_ACT_NTF_PRD:
        _mod_0_positive(member_index, value);
        return ;

    case GRP_SLN_THR:
        mod_sln_thr(value);
        return;

    case GRP_ACT_THR:
        mod_act_thr(value);
        return;

    case GRP_VIS_THR:
        mod_vis_thr(value);
        return;

    case GRP_C_SLN:
        mod_c_sln(value);
        return;

    case GRP_C_SLN_THR:
        mod_c_sln_thr(value);
        return;

    case GRP_C_ACT:
        mod_c_act(value);
        return;

    case GRP_C_ACT_THR:
        mod_c_act_thr(value);
        return;

    case GRP_DCY_PER:
        _mod_minus1_plus1(member_index, value);
        return;

    case GRP_SLN_CHG_THR:
    case GRP_ACT_CHG_THR:
    case GRP_HIGH_SLN_THR:
    case GRP_LOW_SLN_THR:
    case GRP_HIGH_ACT_THR:
    case GRP_LOW_ACT_THR:
    case GRP_LOW_RES_THR:
        _mod_0_plus1(member_index, value);
        return;
    }
}

void Group::set(uint16_t member_index, float value)
{
    switch (member_index) {
    case GRP_UPR:
    case GRP_DCY_PRD:
    case GRP_SLN_CHG_PRD:
    case GRP_ACT_CHG_PRD:
    case GRP_SLN_NTF_PRD:
    case GRP_ACT_NTF_PRD:
        _set_0_positive(member_index, value);
        return;

    case GRP_SLN_THR:
        set_sln_thr(value);
        return;

    case GRP_ACT_THR:
        set_act_thr(value);
        return;

    case GRP_VIS_THR:
        set_vis_thr(value);
        return;

    case GRP_C_SLN:
        set_c_sln(value);
        return;

    case GRP_C_SLN_THR:
        set_c_sln_thr(value);
        return;

    case GRP_C_ACT:
        set_c_act(value);
        return;

    case GRP_C_ACT_THR:
        set_c_act_thr(value);
        return;

    case GRP_DCY_PER:
        _set_minus1_plus1(member_index, value);
        return;

    case GRP_SLN_CHG_THR:
    case GRP_ACT_CHG_THR:
    case GRP_HIGH_SLN_THR:
    case GRP_LOW_SLN_THR:
    case GRP_HIGH_ACT_THR:
    case GRP_LOW_ACT_THR:
    case GRP_LOW_RES_THR:
        _set_0_plus1(member_index, value);
        return;

    case GRP_NTF_NEW:
    case GRP_DCY_TGT:
    case GRP_DCY_AUTO:
        _set_0_1(member_index, value);
        return;
    }
}


inline bool Group::is_active_pgm(View *view)
{
    return view->get_act() > get_act_thr() &&
           get_c_sln() > get_c_sln_thr() &&
           get_c_act() > get_c_act_thr(); // active ipgm/icpp_pgm/cst/mdl in a c-salient and c-active group.
}

inline bool Group::is_eligible_input(View *view)
{
    return view->get_sln() > get_sln_thr() &&
           get_c_sln() > get_c_sln_thr() &&
           get_c_act() > get_c_act_thr(); // active ipgm/icpp_pgm/cst/mdl in a c-salient and c-active group.
}

View *Group::get_view_for_object(uint64_t OID)
{
    std::unordered_map<uint64_t, P<View> >::const_iterator it = other_views.find(OID);

    if (it != other_views.end()) {
        return it->second;
    }

    it = group_views.find(OID);

    if (it != group_views.end()) {
        return it->second;
    }

    it = ipgm_views.find(OID);

    if (it != ipgm_views.end()) {
        return it->second;
    }

    it = anti_ipgm_views.find(OID);

    if (it != anti_ipgm_views.end()) {
        return it->second;
    }

    it = input_less_ipgm_views.find(OID);

    if (it != input_less_ipgm_views.end()) {
        return it->second;
    }

    it = notification_views.find(OID);

    if (it != notification_views.end()) {
        return it->second;
    }

    return nullptr;
}

void Group::reset_ctrl_values()
{
    sln_thr_changes = 0;
    acc_sln_thr = 0;
    act_thr_changes = 0;
    acc_act_thr = 0;
    vis_thr_changes = 0;
    acc_vis_thr = 0;
    c_sln_changes = 0;
    acc_c_sln = 0;
    c_act_changes = 0;
    acc_c_act = 0;
    c_sln_thr_changes = 0;
    acc_c_sln_thr = 0;
    c_act_thr_changes = 0;
    acc_c_act_thr = 0;
}

void Group::reset_stats()
{
    avg_sln = 0;
    high_sln = 0;
    low_sln = 1;
    avg_act = 0;
    high_act = 0;
    low_act = 1;
    sln_updates = 0;
    act_updates = 0;

    if (sln_change_monitoring_periods_to_go <= 0) { // 0:reactivate, -1: activate.
        if (get_sln_chg_thr() < 1) { // activate monitoring.
            sln_change_monitoring_periods_to_go = get_sln_chg_prd();
        }
    } else if (get_sln_chg_thr() == 1) { // deactivate monitoring.
        sln_change_monitoring_periods_to_go = -1;
    } else {
        --sln_change_monitoring_periods_to_go;    // notification will occur when =0.
    }

    if (act_change_monitoring_periods_to_go <= 0) { // 0:reactivate, -1: activate.
        if (get_act_chg_thr() < 1) { // activate monitoring.
            act_change_monitoring_periods_to_go = get_act_chg_prd();
        }
    } else if (get_act_chg_thr() == 1) { // deactivate monitoring.
        act_change_monitoring_periods_to_go = -1;
    } else {
        --act_change_monitoring_periods_to_go;    // notification will occur when =0.
    }
}

void Group::update_stats()
{
    if (decay_periods_to_go > 0) {
        --decay_periods_to_go;
    }

    // auto restart: iff dcy_auto==1.
    if (Utils::Equal(code(GRP_DCY_AUTO).asFloat(), 1)) {
        double period = code(GRP_DCY_PRD).asFloat();

        if (decay_periods_to_go <= 0 && period > 0) {
            decay_periods_to_go = period;
        }
    }

    if (sln_updates) {
        avg_sln = avg_sln / (double)sln_updates;
    }

    if (act_updates) {
        avg_act = avg_act / (double)act_updates;
    }

    code(GRP_AVG_SLN) = Atom::Float(avg_sln);
    code(GRP_AVG_ACT) = Atom::Float(avg_act);
    code(GRP_HIGH_SLN) = Atom::Float(high_sln);
    code(GRP_LOW_SLN) = Atom::Float(low_sln);
    code(GRP_HIGH_ACT) = Atom::Float(high_act);
    code(GRP_LOW_ACT) = Atom::Float(low_act);

    if (sln_change_monitoring_periods_to_go == 0) {
        FOR_ALL_NON_NTF_VIEWS_BEGIN(this, v)
        double change = v->second->update_sln_delta();

        if (fabs(change) > get_sln_chg_thr()) {
            uint16_t ntf_grp_count = get_ntf_grp_count();

            for (uint16_t i = 1; i <= ntf_grp_count; ++i) {
                _Mem::Get()->inject_notification(new NotificationView(this, get_ntf_grp(i), new MkSlnChg(_Mem::Get(), v->second->object, change)), false);
            }
        }

        FOR_ALL_NON_NTF_VIEWS_END
    }

    if (act_change_monitoring_periods_to_go == 0) {
        FOR_ALL_NON_NTF_VIEWS_BEGIN(this, v)
        double change = v->second->update_act_delta();

        if (fabs(change) > get_act_chg_thr()) {
            uint16_t ntf_grp_count = get_ntf_grp_count();

            for (uint16_t i = 1; i <= ntf_grp_count; ++i) {
                _Mem::Get()->inject_notification(new NotificationView(this, get_ntf_grp(i), new MkActChg(_Mem::Get(), v->second->object, change)), false);
            }
        }

        FOR_ALL_NON_NTF_VIEWS_END
    }
}

void Group::reset_decay_values()
{
    sln_thr_decay = 0;
    sln_decay = 0;
    decay_periods_to_go = -1;
    decay_percentage_per_period = 0;
    decay_target = -1;
}

double Group::update_sln_thr()
{
    float percentage = code(GRP_DCY_PER).asFloat();
    float period = code(GRP_DCY_PRD).asFloat();

    if (percentage == 0 || period == 0) {
        reset_decay_values();
    } else {
        double percentage_per_period = percentage / period;

        if (percentage_per_period != decay_percentage_per_period || code(GRP_DCY_TGT).asFloat() != decay_target) { // recompute decay.
            decay_periods_to_go = period;
            decay_percentage_per_period = percentage_per_period;
            decay_target = code(GRP_DCY_TGT).asFloat();

            if (code(GRP_DCY_TGT).asFloat() == 0) {
                sln_thr_decay = 0;
                sln_decay = percentage_per_period;
            } else {
                sln_decay = 0;
                sln_thr_decay = percentage_per_period;
            }
        }
    }

    if (decay_periods_to_go > 0) {
        if (sln_thr_decay != 0) {
            mod_sln_thr(get_sln_thr()*sln_thr_decay);
        }
    }

    if (sln_thr_changes) {
        double new_sln_thr = get_sln_thr() + acc_sln_thr / sln_thr_changes;

        if (new_sln_thr < 0) {
            new_sln_thr = 0;
        } else if (new_sln_thr > 1) {
            new_sln_thr = 1;
        }

        code(GRP_SLN_THR) = r_code::Atom::Float(new_sln_thr);
    }

    acc_sln_thr = 0;
    sln_thr_changes = 0;
    return get_sln_thr();
}

double Group::update_act_thr()
{
    if (act_thr_changes) {
        double new_act_thr = get_act_thr() + acc_act_thr / act_thr_changes;

        if (new_act_thr < 0) {
            new_act_thr = 0;
        } else if (new_act_thr > 1) {
            new_act_thr = 1;
        }

        code(GRP_ACT_THR) = r_code::Atom::Float(new_act_thr);
    }

    acc_act_thr = 0;
    act_thr_changes = 0;
    return get_act_thr();
}

double Group::update_vis_thr()
{
    if (vis_thr_changes) {
        double new_vis_thr = get_vis_thr() + acc_vis_thr / vis_thr_changes;

        if (new_vis_thr < 0) {
            new_vis_thr = 0;
        } else if (new_vis_thr > 1) {
            new_vis_thr = 1;
        }

        code(GRP_VIS_THR) = r_code::Atom::Float(new_vis_thr);
    }

    acc_vis_thr = 0;
    vis_thr_changes = 0;
    return get_vis_thr();
}

double Group::update_c_sln()
{
    if (c_sln_changes) {
        double new_c_sln = get_c_sln() + acc_c_sln / c_sln_changes;

        if (new_c_sln < 0) {
            new_c_sln = 0;
        } else if (new_c_sln > 1) {
            new_c_sln = 1;
        }

        code(GRP_C_SLN) = r_code::Atom::Float(new_c_sln);
    }

    acc_c_sln = 0;
    c_sln_changes = 0;
    return get_c_sln();
}

double Group::update_c_act()
{
    if (c_act_changes) {
        double new_c_act = get_c_act() + acc_c_act / c_act_changes;

        if (new_c_act < 0) {
            new_c_act = 0;
        } else if (new_c_act > 1) {
            new_c_act = 1;
        }

        code(GRP_C_ACT) = r_code::Atom::Float(new_c_act);
    }

    acc_c_act = 0;
    c_act_changes = 0;
    return get_c_act();
}

double Group::update_c_sln_thr()
{
    if (c_sln_thr_changes) {
        double new_c_sln_thr = get_c_sln_thr() + acc_c_sln_thr / c_sln_thr_changes;

        if (new_c_sln_thr < 0) {
            new_c_sln_thr = 0;
        } else if (new_c_sln_thr > 1) {
            new_c_sln_thr = 1;
        }

        code(GRP_C_SLN_THR) = r_code::Atom::Float(new_c_sln_thr);
    }

    acc_c_sln_thr = 0;
    c_sln_thr_changes = 0;
    return get_c_sln_thr();
}

double Group::update_c_act_thr()
{
    if (c_act_thr_changes) {
        double new_c_act_thr = get_c_act_thr() + acc_c_act_thr / c_act_thr_changes;

        if (new_c_act_thr < 0) {
            new_c_act_thr = 0;
        } else if (new_c_act_thr > 1) {
            new_c_act_thr = 1;
        }

        code(GRP_C_ACT_THR) = r_code::Atom::Float(new_c_act_thr);
    }

    acc_c_act_thr = 0;
    c_act_thr_changes = 0;
    return get_c_act_thr();
}

double Group::update_res(View *v)
{
    if (v->object->is_invalidated()) {
        return 0;
    }

    float res = v->update_res();

    if (!v->isNotification() && res > 0 && res < get_low_res_thr()) {
        uint16_t ntf_grp_count = get_ntf_grp_count();

        for (uint16_t i = 1; i <= ntf_grp_count; ++i) {
            _Mem::Get()->inject_notification(new NotificationView(this, get_ntf_grp(i), new MkLowRes(_Mem::Get(), v->object)), false);
        }
    }

    return res;
}

double Group::update_sln(View *v)
{
    if (decay_periods_to_go > 0 && sln_decay != 0) {
        v->mod_sln(v->get_sln()*sln_decay);
    }

    double sln = v->update_sln(get_low_sln_thr(), get_high_sln_thr());
    avg_sln += sln;

    if (sln > high_sln) {
        high_sln = sln;
    } else if (sln < low_sln) {
        low_sln = sln;
    }

    ++sln_updates;

    if (!v->isNotification()) {
        if (v->periods_at_high_sln == get_sln_ntf_prd()) {
            v->periods_at_high_sln = 0;
            uint16_t ntf_grp_count = get_ntf_grp_count();

            for (uint16_t i = 1; i <= ntf_grp_count; ++i) {
                _Mem::Get()->inject_notification(new NotificationView(this, get_ntf_grp(i), new MkHighSln(_Mem::Get(), v->object)), false);
            }
        } else if (v->periods_at_low_sln == get_sln_ntf_prd()) {
            v->periods_at_low_sln = 0;
            uint16_t ntf_grp_count = get_ntf_grp_count();

            for (uint16_t i = 1; i <= ntf_grp_count; ++i) {
                _Mem::Get()->inject_notification(new NotificationView(this, get_ntf_grp(i), new MkLowSln(_Mem::Get(), v->object)), false);
            }
        }
    }

    return sln;
}

double Group::update_act(View *v)
{
    double act = v->update_act(get_low_act_thr(), get_high_act_thr());
    avg_act += act;

    if (act > high_act) {
        high_act = act;
    } else if (act < low_act) {
        low_act = act;
    }

    ++act_updates;

    if (!v->isNotification()) {
        if (v->periods_at_high_act == get_act_ntf_prd()) {
            v->periods_at_high_act = 0;
            uint16_t ntf_grp_count = get_ntf_grp_count();

            for (uint16_t i = 1; i <= ntf_grp_count; ++i) {
                _Mem::Get()->inject_notification(new NotificationView(this, get_ntf_grp(i), new MkHighAct(_Mem::Get(), v->object)), false);
            }
        } else if (v->periods_at_low_act == get_act_ntf_prd()) {
            v->periods_at_low_act = 0;
            uint16_t ntf_grp_count = get_ntf_grp_count();

            for (uint16_t i = 1; i <= ntf_grp_count; ++i) {
                _Mem::Get()->inject_notification(new NotificationView(this, get_ntf_grp(i), new MkLowAct(_Mem::Get(), v->object)), false);
            }
        }
    }

    return act;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Group::load(View *view, Code *object)
{
    switch (object->code(0).getDescriptor()) {
    case Atom::GROUP: {
        group_views[view->get_oid()] = view;
        // init viewing_group.
        bool viewing_c_active = get_c_act() > get_c_act_thr();
        bool viewing_c_salient = get_c_sln() > get_c_sln_thr();
        bool viewed_visible = view->get_vis() > get_vis_thr();

        if (viewing_c_active && viewing_c_salient && viewed_visible) { // visible group in a c-salient, c-active group.
            ((Group *)object)->viewing_groups[this] = view->get_cov();    // init the group's viewing groups.
        }

        break;
    }

    case Atom::INSTANTIATED_PROGRAM: {
        ipgm_views[view->get_oid()] = view;
        PGMController *c = new PGMController(view); // now will be added to the deadline at start time.
        view->controller = c;

        if (is_active_pgm(view)) {
            c->gain_activation();
        }

        break;
    }

    case Atom::INSTANTIATED_INPUT_LESS_PROGRAM: {
        input_less_ipgm_views[view->get_oid()] = view;
        InputLessPGMController *c = new InputLessPGMController(view); // now will be added to the deadline at start time.
        view->controller = c;

        if (is_active_pgm(view)) {
            c->gain_activation();
        }

        break;
    }

    case Atom::INSTANTIATED_ANTI_PROGRAM: {
        anti_ipgm_views[view->get_oid()] = view;
        AntiPGMController *c = new AntiPGMController(view); // now will be added to the deadline at start time.
        view->controller = c;

        if (is_active_pgm(view)) {
            c->gain_activation();
        }

        break;
    }

    case Atom::INSTANTIATED_CPP_PROGRAM: {
        ipgm_views[view->get_oid()] = view;
        std::string str = Utils::GetString<Code>(view->object, ICPP_PGM_NAME);

        std::cout << "Loading ICCP_PGM_NME" << str << "\n";
        std::cout << "before -   r_exec::_Mem::Get() = " << r_exec::_Mem::Get() << "\n";

        Controller *c = CPPPrograms::New(str, view); // now will be added to the deadline at start time.
        std::cout << "after -    r_exec::_Mem::Get() = " << r_exec::_Mem::Get() << "\n";

        if (!c) {
            return false;
        }

        view->controller = c;

        if (is_active_pgm(view)) {
            c->gain_activation();
        }

        break;
    }

    case Atom::COMPOSITE_STATE: {
        ipgm_views[view->get_oid()] = view;
        CSTController *c = new CSTController(view);
        view->controller = c;
        c->set_secondary_host(get_secondary_group());

        if (is_active_pgm(view)) {
            c->gain_activation();
        }

        break;
    }

    case Atom::MODEL: {
        ipgm_views[view->get_oid()] = view;
        bool inject_in_secondary_group;
        MDLController *c = MDLController::New(view, inject_in_secondary_group);
        view->controller = c;

        if (inject_in_secondary_group) {
            get_secondary_group()->load_secondary_mdl_controller(view);
        }

        if (is_active_pgm(view)) {
            c->gain_activation();
        }

        break;
    }

    case Atom::MARKER: // populate the marker set of the referenced objects.
        for (uint64_t i = 0; i < object->references_size(); ++i) {
            object->get_reference(i)->markers.push_back(object);
        }

        other_views[view->get_oid()] = view;
        break;

    case Atom::OBJECT:
        other_views[view->get_oid()] = view;
        break;
    }

    return true;
}

void Group::update(uint64_t planned_time)
{
    std::lock_guard<std::mutex> guard(mutex);

    if (this != _Mem::Get()->get_root() && views.size() == 0) {
        invalidate();
        return;
    }

    uint64_t now = Now();
    //if(get_secondary_group()!=NULL)
    // std::cout<<Utils::RelativeTime(Now())<<" UPR\n";
    //if(this==_Mem::Get()->get_stdin())
    // std::cout<<Utils::RelativeTime(Now())<<" ----------------------------------------------------------------\n";
    newly_salient_views.clear();

    // execute pending operations.
    for (Operation *pending_operation : pending_operations) {
        pending_operation->execute(this);
        delete pending_operation;
    }

    pending_operations.clear();
    // update group's ctrl values.
    update_sln_thr(); // applies decay on sln thr.
    update_act_thr();
    update_vis_thr();
    GroupState state(get_sln_thr(), get_c_act() > get_c_act_thr(), update_c_act() > get_c_act_thr(), get_c_sln() > get_c_sln_thr(), update_c_sln() > get_c_sln_thr());
    reset_stats();
    FOR_ALL_VIEWS_BEGIN_NO_INC(this, v)

    if (v->second->object->is_invalidated()) { // no need to update the view set.
        delete_view(v);
    } else {
        uint64_t ijt = v->second->get_ijt();

        if (ijt >= planned_time) { // in case the update happens later than planned, don't touch views that were injected after the planned update time: update next time.
            ++v;
            continue;
        }

        double res = update_res(v->second); // update resilience: decrement res by 1 in addition to the accumulated changes.

        if (res > 0) {
            _update_saliency(&state, v->second); // apply decay.

            switch (v->second->object->code(0).getDescriptor()) {
            case Atom::GROUP:
                _update_visibility(&state, v->second);
                break;

            case Atom::NULL_PROGRAM:
            case Atom::INSTANTIATED_PROGRAM:
            case Atom::INSTANTIATED_ANTI_PROGRAM:
            case Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
            case Atom::INSTANTIATED_CPP_PROGRAM:
            case Atom::COMPOSITE_STATE:
            case Atom::MODEL:
                _update_activation(&state, v->second);
                break;
            }

            ++v;
        } else { // view has no resilience: delete it from the group.
            v->second->delete_from_object();
            delete_view(v);
        }
    }

    FOR_ALL_VIEWS_END

    if (state.is_c_salient) {
        cov();
    }

    // build reduction jobs.
    std::multiset<P<View>, r_code::View::Less>::const_iterator v;

    for (v = newly_salient_views.begin(); v != newly_salient_views.end(); ++v) {
        inject_reduction_jobs(*v);
    }

    if (state.is_c_active && state.is_c_salient) { // build signaling jobs for new ipgms.
        for (Controller *new_controller : new_controllers) {
            switch (new_controller->getObject()->code(0).getDescriptor()) {
            case Atom::INSTANTIATED_ANTI_PROGRAM: { // inject signaling jobs for |ipgm (tsc).
                P<TimeJob> j = new AntiPGMSignalingJob(new_controller->getView(), now + Utils::GetTimestamp<Code>(new_controller->getObject(), IPGM_TSC));
                _Mem::Get()->pushTimeJob(j);
                break;
            }

            case Atom::INSTANTIATED_INPUT_LESS_PROGRAM: { // inject a signaling job for an input-less pgm.
                P<TimeJob> j = new InputLessPGMSignalingJob(new_controller->getView(), now + Utils::GetTimestamp<Code>(new_controller->getObject(), IPGM_TSC));
                _Mem::Get()->pushTimeJob(j);
                break;
            }
            }
        }

        new_controllers.clear();
    }

    update_stats(); // triggers notifications.

    if (get_upr() > 0) { // inject the next update job for the group.
        _Mem::Get()->pushTimeJob(new UpdateJob(this, planned_time + get_upr()*Utils::GetBasePeriod()));
    }

    //if(get_secondary_group()!=NULL)
    //if(this==_Mem::Get()->get_stdin())
    // std::cout<<Utils::RelativeTime(Now())<<" ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
}

void Group::_update_saliency(GroupState *state, View *view)
{
    double view_old_sln = view->get_sln();
    bool wiew_was_salient = view_old_sln > state->former_sln_thr;
    double view_new_sln = update_sln(view);
    bool wiew_is_salient = view_new_sln > get_sln_thr();

    if (state->is_c_salient) {
        if (wiew_is_salient) {
            switch (view->get_sync()) {
            case View::SYNC_ONCE:
            case View::SYNC_ONCE_AXIOM:
            case View::SYNC_PERIODIC:
                if (!wiew_was_salient) { // sync on front: crosses the threshold upward: record as a newly salient view.
                    newly_salient_views.insert(view);
                }

                break;

            case View::SYNC_HOLD:
            case View::SYNC_AXIOM: // sync on state: treat as if it was a new injection.
                view->set_ijt(Now());
                newly_salient_views.insert(view);
                break;
            }
        }

        // inject sln propagation jobs.
        // the idea is to propagate sln changes when a view "occurs to the mind", i.e. becomes more salient in a group and is eligible for reduction in that group.
        // - when a view is now salient because the group becomes c-salient, no propagation;
        // - when a view is now salient because the group's sln_thr gets lower, no propagation;
        // - propagation can occur only if the group is c_active. For efficiency reasons, no propagation occurs even if some of the group's viewing groups are c-active and c-salient.
        if (state->is_c_active) {
            _initiate_sln_propagation(view->object, view_new_sln - view_old_sln, get_sln_thr());
        }
    }
}

void Group::_update_visibility(GroupState *state, View *view)
{
    bool view_was_visible = view->get_vis() > get_vis_thr();
    bool view_is_visible = view->update_vis() > get_vis_thr();
    bool cov = view->get_cov();

    // update viewing groups.
    if (state->was_c_active && state->was_c_salient) {
        if (!state->is_c_active || !state->is_c_salient) { // group is not c-active and c-salient anymore: unregister as a viewing group.
            ((Group *)view->object)->viewing_groups.erase(this);
        } else { // group remains c-active and c-salient.
            if (!view_was_visible) {
                if (view_is_visible) { // newly visible view.
                    ((Group *)view->object)->viewing_groups[this] = cov;
                }
            } else {
                if (!view_is_visible) { // the view is no longer visible.
                    ((Group *)view->object)->viewing_groups.erase(this);
                } else { // the view is still visible, cov might have changed.
                    ((Group *)view->object)->viewing_groups[this] = cov;
                }
            }
        }
    } else if (state->is_c_active && state->is_c_salient) { // group becomes c-active and c-salient.
        if (view_is_visible) { // update viewing groups for any visible group.
            ((Group *)view->object)->viewing_groups[this] = cov;
        }
    }
}

void Group::_update_activation(GroupState *state, View *view)
{
    bool view_was_active = view->get_act() > get_act_thr();
    bool view_is_active = update_act(view) > get_act_thr();

    // kill newly inactive controllers, register newly active ones.
    if (state->was_c_active && state->was_c_salient) {
        if (!state->is_c_active || !state->is_c_salient) { // group is not c-active and c-salient anymore: kill the view's controller.
            view->controller->lose_activation();
        } else { // group remains c-active and c-salient.
            if (!view_was_active) {
                if (view_is_active) { // register the controller for the newly active ipgm view.
                    view->controller->gain_activation();
                    new_controllers.push_back(view->controller);
                }
            } else {
                if (!view_is_active) { // kill the newly inactive ipgm view's overlays.
                    view->controller->lose_activation();
                }
            }
        }
    } else if (state->is_c_active && state->is_c_salient) { // group becomes c-active and c-salient.
        if (view_is_active) { // register the controller for any active ipgm view.
            view->controller->gain_activation();
            new_controllers.push_back(view->controller);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Group::_initiate_sln_propagation(Code *object, double change, double source_sln_thr) const
{
    if (fabs(change) > object->get_psln_thr()) {
        std::vector<Code *> path;
        path.push_back(object);

        if (object->code(0).getDescriptor() == Atom::MARKER) { // if marker, propagate to references.
            for (uint16_t i = 0; i < object->references_size(); ++i) {
                _propagate_sln(object->get_reference(i), change, source_sln_thr, path);
            }
        }

        // propagate to markers
        object->acq_markers();
        r_code::list<Code *>::const_iterator m;

        for (m = object->markers.begin(); m != object->markers.end(); ++m) {
            _propagate_sln(*m, change, source_sln_thr, path);
        }

        object->rel_markers();
    }
}

void Group::_initiate_sln_propagation(Code *object, double change, double source_sln_thr, std::vector<Code *> &path) const
{
    if (fabs(change) > object->get_psln_thr()) {
        // prevent loops.
        for (Code  *node : path) {
            if (node == object) {
                return;
            }
        }

        path.push_back(object);

        if (object->code(0).getDescriptor() == Atom::MARKER) // if marker, propagate to references.
            for (uint16_t i = 0; i < object->references_size(); ++i) {
                _propagate_sln(object->get_reference(i), change, source_sln_thr, path);
            }

        // propagate to markers
        object->acq_markers();
        r_code::list<Code *>::const_iterator m;

        for (m = object->markers.begin(); m != object->markers.end(); ++m) {
            _propagate_sln(*m, change, source_sln_thr, path);
        }

        object->rel_markers();
    }
}

void Group::_propagate_sln(Code *object, double change, double source_sln_thr, std::vector<Code *> &path) const
{
    if (object == _Mem::Get()->get_root()) {
        return;
    }

    // prevent loops.
    for (Code *node : path) {
        if (node == object) {
            return;
        }
    }

    path.push_back(object);
    P<TimeJob> j = new SaliencyPropagationJob(object, change, source_sln_thr, 0);
    _Mem::Get()->pushTimeJob(j);
    _initiate_sln_propagation(object, change, source_sln_thr, path);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Group::inject_hlps(std::vector<View *> &views)
{
    std::lock_guard<std::mutex> guard(mutex);
    std::vector<View *>::const_iterator view;

    for (view = views.begin(); view != views.end(); ++view) {
        Atom a = (*view)->object->code(0);

        switch (a.getDescriptor()) {
        case Atom::COMPOSITE_STATE: {
            debug("group inject hlp") << DebugStream::timestamp(Now() - Utils::GetTimeReference()) << " -> cst " << (*view)->object->get_oid();
            ipgm_views[(*view)->get_oid()] = *view;
            CSTController *c = new CSTController(*view);
            (*view)->controller = c;
            c->set_secondary_host(get_secondary_group());
            break;
        }

        case Atom::MODEL: {
            debug("group inject hlp") << DebugStream::timestamp(Now() - Utils::GetTimeReference()) << " -> mdl " << (*view)->object->get_oid();
            ipgm_views[(*view)->get_oid()] = *view;
            bool inject_in_secondary_group;
            MDLController *c = MDLController::New(*view, inject_in_secondary_group);
            (*view)->controller = c;

            if (inject_in_secondary_group) {
                get_secondary_group()->inject_secondary_mdl_controller(*view);
            }

            break;
        }
        }

        (*view)->set_ijt(Now());
    }

    for (view = views.begin(); view != views.end(); ++view) {
        if (is_active_pgm(*view)) {
            (*view)->controller->gain_activation();
            std::multiset<P<View>, r_code::View::Less>::const_iterator v;

            for (v = newly_salient_views.begin(); v != newly_salient_views.end(); ++v) {
                (*view)->controller->_take_input(*v);    // view will be copied.
            }
        }
    }
}

void Group::inject(View *view)
{
    std::lock_guard<std::mutex> guard(mutex);
    Atom a = view->object->code(0);
    uint64_t now = Now();
    view->set_ijt(now);

    switch (a.getDescriptor()) {
    case Atom::NULL_PROGRAM: // the view comes with a controller.
        ipgm_views[view->get_oid()] = view;

        if (is_active_pgm(view)) {
            view->controller->gain_activation();

            if (a.takesPastInputs()) {
                std::multiset<P<View>, r_code::View::Less>::const_iterator v;

                for (v = newly_salient_views.begin(); v != newly_salient_views.end(); ++v) {
                    view->controller->_take_input(*v);    // view will be copied.
                }
            }
        }

        break;

    case Atom::INSTANTIATED_PROGRAM: {
        ipgm_views[view->get_oid()] = view;
        PGMController *c = new PGMController(view);
        view->controller = c;

        if (is_active_pgm(view)) {
            c->gain_activation();
            std::multiset<P<View>, r_code::View::Less>::const_iterator v;

            for (v = newly_salient_views.begin(); v != newly_salient_views.end(); ++v) {
                c->_take_input(*v);    // view will be copied.
            }
        }

        break;
    }

    case Atom::INSTANTIATED_CPP_PROGRAM: {
        ipgm_views[view->get_oid()] = view;
        std::string str = Utils::GetString<Code>(view->object, ICPP_PGM_NAME);
        Controller *c = CPPPrograms::New(str, view);

        if (!c) {
            return;
        }

        view->controller = c;

        if (is_active_pgm(view)) {
            c->gain_activation();
            std::multiset<P<View>, r_code::View::Less>::const_iterator v;

            for (v = newly_salient_views.begin(); v != newly_salient_views.end(); ++v) {
                c->_take_input(*v);    // view will be copied.
            }
        }

        break;
    }

    case Atom::INSTANTIATED_ANTI_PROGRAM: {
        anti_ipgm_views[view->get_oid()] = view;
        AntiPGMController *c = new AntiPGMController(view);
        view->controller = c;

        if (is_active_pgm(view)) {
            c->gain_activation();
            std::multiset<P<View>, r_code::View::Less>::const_iterator v;

            for (v = newly_salient_views.begin(); v != newly_salient_views.end(); ++v) {
                c->_take_input(*v);    // view will be copied.
            }

            _Mem::Get()->pushTimeJob(new AntiPGMSignalingJob(view, now + Utils::GetTimestamp<Code>(c->getObject(), IPGM_TSC)));
        }

        break;
    }

    case Atom::INSTANTIATED_INPUT_LESS_PROGRAM: {
        input_less_ipgm_views[view->get_oid()] = view;
        InputLessPGMController *c = new InputLessPGMController(view);
        view->controller = c;

        if (is_active_pgm(view)) {
            c->gain_activation();
            _Mem::Get()->pushTimeJob(new InputLessPGMSignalingJob(view, now + Utils::GetTimestamp<Code>(view->object, IPGM_TSC)));
        }

        break;
    }

    case Atom::MARKER: // the marker has already been added to the mks of its references.
        other_views[view->get_oid()] = view;
        cov(view);
        break;

    case Atom::OBJECT:
        other_views[view->get_oid()] = view;
        cov(view);
        break;

    case Atom::COMPOSITE_STATE: {
        OUTPUT(HLP_INJ) << Utils::RelativeTime(Now()) << " cst " << view->object->get_oid() << " injected" << std::endl;
        ipgm_views[view->get_oid()] = view;
        CSTController *c = new CSTController(view);
        view->controller = c;
        c->set_secondary_host(get_secondary_group());

        if (is_active_pgm(view)) {
            c->gain_activation();
            std::multiset<P<View>, r_code::View::Less>::const_iterator v;

            for (v = newly_salient_views.begin(); v != newly_salient_views.end(); ++v) {
                c->_take_input(*v);    // view will be copied.
            }
        }

        break;
    }

    case Atom::MODEL: {
        OUTPUT(HLP_INJ) << Utils::RelativeTime(Now()) << " mdl " << view->object->get_oid() << " injected" << std::endl;
        ipgm_views[view->get_oid()] = view;
        bool inject_in_secondary_group;
        MDLController *c = MDLController::New(view, inject_in_secondary_group);
        view->controller = c;

        if (inject_in_secondary_group) {
            get_secondary_group()->inject_secondary_mdl_controller(view);
        }

        if (is_active_pgm(view)) {
            c->gain_activation();
            std::multiset<P<View>, r_code::View::Less>::const_iterator v;

            for (v = newly_salient_views.begin(); v != newly_salient_views.end(); ++v) {
                c->Controller::_take_input(*v);    // view will be copied.
            }
        }

        break;
    }
    }

    if (is_eligible_input(view)) { // have existing programs reduce the new view.
        newly_salient_views.insert(view);
        inject_reduction_jobs(view);
    }

    //if(get_oid()==2)
    // std::cout<<Utils::RelativeTime(Now())<<" stdin <- "<<view->object->get_oid()<<std::endl;
}

void Group::inject_new_object(View *view)   // the view can hold anything but groups and notifications.
{
    //uint64_t t0=Now();
    switch (view->object->code(0).getDescriptor()) {
    case Atom::MARKER: // the marker does not exist yet: add it to the mks of its references.
        for (uint64_t i = 0; i < view->object->references_size(); ++i) {
            Code *ref = view->object->get_reference(i);
            ref->acq_markers();
            ref->markers.push_back(view->object);
            ref->rel_markers();
        }

        break;

    default:
        break;
    }

    inject(view);
    notifyNew(view);
    //uint64_t t1=Now();
    //std::cout<<"injection: "<<t1-t0<<std::endl;
}

void Group::inject_existing_object(View *view)   // the view can hold anything but groups and notifications.
{
    Code *object = view->object;
    object->acq_views();
    View *existing_view = (View *)object->get_view(this, false);

    if (!existing_view) { // no existing view: add the view to the object's view_map and inject.
        object->views.insert(view);
        object->rel_views();
        inject(view);
    } else { // call set on the ctrl values of the existing view with the new view's ctrl values, including sync. NB: org left unchanged.
        object->rel_views();
        std::lock_guard<std::mutex> guard(mutex);
        pending_operations.push_back(new Group::Set(existing_view->get_oid(), VIEW_RES, view->get_res()));
        pending_operations.push_back(new Group::Set(existing_view->get_oid(), VIEW_SLN, view->get_sln()));

        switch (object->code(0).getDescriptor()) {
        case Atom::INSTANTIATED_PROGRAM:
        case Atom::INSTANTIATED_ANTI_PROGRAM:
        case Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
        case Atom::INSTANTIATED_CPP_PROGRAM:
        case Atom::COMPOSITE_STATE:
        case Atom::MODEL:
            pending_operations.push_back(new Group::Set(existing_view->get_oid(), VIEW_ACT, view->get_act()));
            break;
        }

        existing_view->code(VIEW_SYNC) = view->code(VIEW_SYNC);
        existing_view->set_ijt(Now());
        bool wiew_is_salient = view->get_sln() > get_sln_thr();
        bool wiew_was_salient = existing_view->get_sln() > get_sln_thr();
        bool reduce_view = (!wiew_was_salient && wiew_is_salient);
        // give a chance to ipgms to reduce the new view.
        bool group_is_c_active = update_c_act() > get_c_act_thr();
        bool group_is_c_salient = update_c_sln() > get_c_sln_thr();

        if (group_is_c_active && group_is_c_salient && reduce_view) {
            newly_salient_views.insert(view);
            inject_reduction_jobs(view);
        }
    }
}

void Group::inject_group(View *view)
{
    std::lock_guard<std::mutex> guard(mutex);
    group_views[view->get_oid()] = view;

    if (get_c_sln() > get_c_sln_thr() && view->get_sln() > get_sln_thr()) { // group is c-salient and view is salient.
        if (view->get_vis() > get_vis_thr()) { // new visible group in a c-active and c-salient host.
            ((Group *)view->object)->viewing_groups[this] = view->get_cov();
        }

        inject_reduction_jobs(view);
    }

    if (((Group *)view->object)->get_upr() > 0) { // inject the next update job for the group.
        _Mem::Get()->pushTimeJob(new UpdateJob((Group *)view->object, ((Group *)view->object)->get_next_upr_time(Now())));
    }

    notifyNew(view);
}

void Group::inject_notification(View *view, bool lock)
{
    if (lock) {
        mutex.lock();
    }

    notification_views[view->get_oid()] = view;

    for (uint64_t i = 0; i < view->object->references_size(); ++i) {
        Code *ref = view->object->get_reference(i);
        ref->acq_markers();
        ref->markers.push_back(view->object);
        ref->rel_markers();
    }

    view->set_ijt(Now());

    if (get_c_sln() > get_c_sln_thr() && view->get_sln() > get_sln_thr()) { // group is c-salient and view is salient.
        inject_reduction_jobs(view);
    }

    if (lock) {
        mutex.unlock();
    }
}

void Group::inject_reduction_jobs(View *view)
{
    if (get_c_act() > get_c_act_thr()) { // host is c-active.
        // build reduction jobs from host's own inputs and own overlays.
        FOR_ALL_VIEWS_WITH_INPUTS_BEGIN(this, v)

        if (v->second->get_act() > get_act_thr()) { //{ // active ipgm/icpp_pgm/rgrp view.
            v->second->controller->_take_input(view);    // view will be copied.
        }

        //std::cout<<std::hex<<(void *)v->second->controller<<std::dec<<" <- "<<view->object->get_oid()<<std::endl;}
        FOR_ALL_VIEWS_WITH_INPUTS_END
    }

    // build reduction jobs from host's own inputs and overlays from viewing groups, if no cov and view is not a notification.
    // NB: visibility is not transitive;
    // no shadowing: if a view alresady exists in the viewing group, there will be twice the reductions: all of the identicals will be trimmed down at injection time.
    std::unordered_map<Group *, bool>::const_iterator vg;

    for (vg = viewing_groups.begin(); vg != viewing_groups.end(); ++vg) {
        if (vg->second || view->isNotification()) { // no reduction jobs when cov==true or view is a notification.
            continue;
        }

        FOR_ALL_VIEWS_WITH_INPUTS_BEGIN(vg->first, v)

        if (v->second->get_act() > vg->first->get_act_thr()) { // active ipgm/icpp_pgm/rgrp view.
            v->second->controller->_take_input(view);    // view will be copied.
        }

        FOR_ALL_VIEWS_WITH_INPUTS_END
    }
}

void Group::notifyNew(View *view)
{
    if (get_ntf_new() == 1) {
        uint16_t ntf_grp_count = get_ntf_grp_count();

        for (uint16_t i = 1; i <= ntf_grp_count; ++i) {
            _Mem::Get()->inject_notification(new NotificationView(this, get_ntf_grp(i), new MkNew(_Mem::Get(), view->object)), get_ntf_grp(i) != this);    // the object appears for the first time in the group: notify.
        }
    }
}

void Group::cov(View *view)
{
    std::unordered_map<Group *, bool>::const_iterator vg;

    for (vg = viewing_groups.begin(); vg != viewing_groups.end(); ++vg) {
        if (vg->second) { // cov==true, viewing group c-salient and c-active (otherwise it wouldn't be a viewing group).
            _Mem::Get()->inject_copy(view, vg->first);
        }
    }
}

void Group::cov()
{
    // cov, i.e. injecting now newly salient views in the viewing groups from which the group is visible and has cov.
    // reduction jobs will be added at each of the eligible viewing groups' own update time.
    std::unordered_map<Group *, bool>::const_iterator vg;

    for (vg = viewing_groups.begin(); vg != viewing_groups.end(); ++vg) {
        if (vg->second) { // cov==true.
            std::multiset<P<View>, r_code::View::Less>::const_iterator v;

            for (v = newly_salient_views.begin(); v != newly_salient_views.end(); ++v) { // no cov for pgm (all sorts), groups, notifications.
                if ((*v)->isNotification()) {
                    continue;
                }

                switch ((*v)->object->code(0).getDescriptor()) {
                case Atom::GROUP:
                case Atom::NULL_PROGRAM:
                case Atom::INSTANTIATED_PROGRAM:
                case Atom::INSTANTIATED_CPP_PROGRAM:
                case Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
                case Atom::INSTANTIATED_ANTI_PROGRAM:
                case Atom::COMPOSITE_STATE:
                case Atom::MODEL:
                    break;

                default:
                    _Mem::Get()->inject_copy(*v, vg->first); // no need to protect group->newly_salient_views[i] since the support values for the ctrl values are not even read.
                    break;
                }
            }
        }
    }
}

void Group::delete_view(View *v)
{
    if (v->isNotification()) {
        notification_views.erase(v->get_oid());
    } else switch (v->object->code(0).getDescriptor()) {
        case Atom::NULL_PROGRAM:
        case Atom::INSTANTIATED_PROGRAM:
        case Atom::INSTANTIATED_CPP_PROGRAM:
        case Atom::COMPOSITE_STATE:
        case Atom::MODEL:
            ipgm_views.erase(v->get_oid());
            break;

        case Atom::INSTANTIATED_ANTI_PROGRAM:
            anti_ipgm_views.erase(v->get_oid());
            break;

        case Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
            input_less_ipgm_views.erase(v->get_oid());
            break;

        case Atom::OBJECT:
        case Atom::MARKER:
            other_views.erase(v->get_oid());
            break;

        case Atom::GROUP:
            group_views.erase(v->get_oid());
            break;
        }
}

void Group::delete_view(std::unordered_map<uint64_t, P<View> >::const_iterator &v)
{
    if (v->second->isNotification()) {
        v = notification_views.erase(v);
    } else switch (v->second->object->code(0).getDescriptor()) {
        case Atom::NULL_PROGRAM:
        case Atom::INSTANTIATED_PROGRAM:
        case Atom::INSTANTIATED_CPP_PROGRAM:
        case Atom::COMPOSITE_STATE:
        case Atom::MODEL:
            v = ipgm_views.erase(v);
            break;

        case Atom::INSTANTIATED_ANTI_PROGRAM:
            v = anti_ipgm_views.erase(v);
            break;

        case Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
            v = input_less_ipgm_views.erase(v);
            break;

        case Atom::OBJECT:
        case Atom::MARKER:
            v = other_views.erase(v);
            break;

        case Atom::GROUP:
            v = group_views.erase(v);
            break;
        }
}

Group *Group::get_secondary_group()
{
    Group *secondary = nullptr;
    r_code::list<Code *>::const_iterator m;
    acq_markers();

    for (m = markers.begin(); m != markers.end(); ++m) {
        if ((*m)->code(0).asOpcode() == Opcodes::MkGrpPair) {
            if ((Group *)(*m)->get_reference((*m)->code(GRP_PAIR_FIRST).asIndex()) == this) {
                secondary = (Group *)(*m)->get_reference((*m)->code(GRP_PAIR_SECOND).asIndex());
                break;
            }
        }
    }

    rel_markers();
    return secondary;
}

void Group::load_secondary_mdl_controller(View *view)
{
    PrimaryMDLController *p = (PrimaryMDLController *)view->controller;
    View *_view = new View(view, true);
    _view->code(VIEW_ACT) = Atom::Float(0);
    _view->references[0] = this;
    ipgm_views[_view->get_oid()] = _view;
    SecondaryMDLController *s = new SecondaryMDLController(_view);
    _view->controller = s;
    view->object->views.insert(_view);
    p->set_secondary(s);
    s->set_primary(p);
}

void Group::inject_secondary_mdl_controller(View *view)
{
    PrimaryMDLController *p = (PrimaryMDLController *)view->controller;
    View *_view = new View(view, true);
    _view->code(VIEW_ACT) = Atom::Float(0);
    _view->references[0] = this;
    ipgm_views[_view->get_oid()] = _view;
    SecondaryMDLController *s = new SecondaryMDLController(_view);
    _view->controller = s;
    view->object->views.insert(_view);
    p->set_secondary(s);
    s->set_primary(p);
}

uint64_t Group::get_next_upr_time(uint64_t now) const
{
    uint64_t __upr = get_upr();

    if (__upr == 0) {
        return Utils::MaxTime;
    }

    uint64_t _upr = __upr * Utils::GetBasePeriod();
    uint64_t delta = (now - Utils::GetTimeReference()) % _upr;
    return now - delta + _upr;
}

uint64_t Group::get_prev_upr_time(uint64_t now) const
{
    uint64_t __upr = get_upr();

    if (__upr == 0) {
        return Utils::MaxTime;
    }

    uint64_t _upr = __upr * Utils::GetBasePeriod();
    uint64_t delta = (now - Utils::GetTimeReference()) % _upr;
    return now - delta;
}
}
