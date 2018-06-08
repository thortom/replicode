//	ast_controller.cpp
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

#include "ast_controller.h"

#include <r_exec/ast_controller.h>      // for PASTController, etc
#include <r_exec/binding_map.h>         // for ::MATCH_FAILURE, etc
#include <r_exec/factory.h>             // for _Fact
#include <r_exec/pattern_extractor.h>   // for CTPX

#include <r_exec/overlay.tpl.h>         // for __take_input

namespace r_exec {
class AutoFocusController;
class View;
}  // namespace r_exec

namespace r_exec
{

PASTController::PASTController(AutoFocusController *auto_focus, View *target): ASTController<PASTController>(auto_focus, target)
{
    //std::cout<<Utils::Timestamp(Now())<<" created TPX PERIODIC"<<std::endl;
}

PASTController::~PASTController()
{
}

void PASTController::reduce(View *v, _Fact *input)
{
    switch (input->is_timeless_evidence(target)) {
    case MATCH_SUCCESS_POSITIVE:
        //std::cout<<Utils::Timestamp(Now())<<" "<<std::hex<<this<<std::dec<<" target: "<<target->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<" reduced: "<<input->get_oid()<<" positive\n";
        kill();
        target->invalidate();//std::cout<<Time::ToString_seconds(Now()-st)<<" "<<" ------------- "<<std::dec<<target->get_oid()<<std::endl;
        break;

    case MATCH_SUCCESS_NEGATIVE:
        //std::cout<<Utils::Timestamp(Now())<<" TPX"<<target->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<" reduced: "<<input->get_oid()<<" counter-evidence: "<<input->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<std::endl;
        kill();
        tpx->signal(v);
        target->invalidate();//std::cout<<Utils::Timestamp(Now())<<" "<<target->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<"|"<<std::dec<<target->get_oid()<<" invalidated"<<std::endl;
        break;

    case MATCH_FAILURE:
        //std::cout<<Utils::Timestamp(Now())<<" "<<std::hex<<this<<std::dec<<" target: "<<target->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<" stored: "<<input->get_oid()<<std::endl;
        tpx->store_input(v);
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////

HASTController::HASTController(AutoFocusController *auto_focus, View *target, _Fact *source): ASTController<HASTController>(auto_focus, target), source(source)
{
    //std::cout<<Utils::Timestamp(Now())<<" "<<std::hex<<this<<std::dec<<" created HOLD "<<target->get_oid()<<std::endl;
}

HASTController::~HASTController()
{
}

void HASTController::reduce(View *v, _Fact *input)
{
    switch (input->is_timeless_evidence(target)) {
    case MATCH_SUCCESS_POSITIVE:
        //std::cout<<Utils::Timestamp(Now())<<" "<<std::hex<<this<<std::dec<<" target: "<<target->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<" reduced: "<<input->get_oid()<<" positive\n";
        kill();
        break;

    case MATCH_SUCCESS_NEGATIVE:
        //std::cout<<Utils::Timestamp(Now())<<" "<<std::hex<<this<<std::dec<<" target: "<<target->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<" reduced: "<<input->get_oid()<<" counter-evidence: "<<input->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<std::endl;
        kill();
        tpx->signal(v);
        target->invalidate();//std::cout<<Utils::Timestamp(Now())<<" "<<std::hex<<this<<std::dec<<" ------------- "<<std::dec<<target->get_oid()<<std::endl;
        source->invalidate();
        break;

    case MATCH_FAILURE:
        //std::cout<<Utils::Timestamp(Now())<<" "<<std::hex<<this<<std::dec<<" target: "<<target->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<" stored: "<<input->get_oid()<<std::endl;
        tpx->store_input(v);
        break;
    }
}
}
