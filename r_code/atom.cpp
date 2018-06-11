//	atom.cpp
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

#include "atom.h"

#include <r_code/atom.h>  // for Atom, Atom::::COMPOSITE_STATE, etc
#include <iostream>       // for operator<<, cout, ostream, basic_ostream, etc
#include <string>         // for string
#include <common_logger.h>// for logger


namespace r_code
{

Atom Atom::Float(float f)
{
    uint32_t a = *reinterpret_cast<uint32_t *>(&f);
    return Atom(a >> 1);
}

Atom Atom::PlusInfinity()
{
    return Atom(0x3FC00000);
}

Atom Atom::MinusInfinity()
{
    return Atom(0x7FC00000);
}

Atom Atom::UndefinedFloat()
{
    return Atom(0xFFFFFFF);
}

Atom Atom::Nil()
{
    return Atom(NIL << 24);
}

Atom Atom::Boolean(bool value)
{
    return Atom((BOOLEAN_ << 24) + value);
}

Atom Atom::UndefinedBoolean()
{
    return Atom(0x81FFFFFF);
}

Atom Atom::Wildcard(uint16_t opcode)
{
    return Atom((WILDCARD << 24) + ((opcode & 0x0FFF) << 8));
}

Atom Atom::TailWildcard()
{
    return Atom(T_WILDCARD << 24);
}

Atom Atom::IPointer(uint16_t index)
{
    return Atom((I_PTR << 24) + (index & 0x0FFF));
}

Atom Atom::VLPointer(uint16_t index, uint16_t cast_opcode)
{
    return Atom((VL_PTR << 24) + ((cast_opcode & 0x0FFF) << 12) + (index & 0x0FFF));
}

Atom Atom::RPointer(uint16_t index)
{
    return Atom((R_PTR << 24) + (index & 0x0FFF));
}

Atom Atom::IPGMPointer(uint16_t index)
{
    return Atom((IPGM_PTR << 24) + (index & 0x0FFF));
}

Atom Atom::InObjPointer(uint8_t inputIndex, uint16_t index)
{
    return Atom((IN_OBJ_PTR << 24) + (inputIndex << 12) + (index & 0x0FFF));
}

Atom Atom::DInObjPointer(uint8_t relativeIndex, uint16_t index)
{
    return Atom((D_IN_OBJ_PTR << 24) + (relativeIndex << 12) + (index & 0x0FFF));
}

Atom Atom::OutObjPointer(uint16_t index)
{
    return Atom((OUT_OBJ_PTR << 24) + (index & 0x0FFF));
}

Atom Atom::ValuePointer(uint16_t index)
{
    return Atom((VALUE_PTR << 24) + (index & 0x0FFF));
}

Atom Atom::ProductionPointer(uint16_t index)
{
    return Atom((PROD_PTR << 24) + (index & 0x0FFF));
}

Atom Atom::AssignmentPointer(uint8_t variable_index, uint16_t index)
{
    return Atom((ASSIGN_PTR << 24) + (variable_index << 16) + (index & 0x0FFF));
}

Atom Atom::This()
{
    return Atom(THIS << 24);
}

Atom Atom::View()
{
    return Atom(VIEW << 24);
}

Atom Atom::Mks()
{
    return Atom(MKS << 24);
}

Atom Atom::Vws()
{
    return Atom(VWS << 24);
}

Atom Atom::SSet(uint16_t opcode, uint8_t elementCount)
{
    return Atom((S_SET << 24) + ((opcode & 0x0FFF) << 8) + elementCount);
}

Atom Atom::Set(uint8_t elementCount)
{
    return Atom((SET << 24) + elementCount);
}

Atom Atom::CPointer(uint8_t elementCount)
{
    return Atom((C_PTR << 24) + elementCount);
}

Atom Atom::Object(uint16_t opcode, uint8_t arity)
{
    return Atom((OBJECT << 24) + ((opcode & 0x0FFF) << 8) + arity);
}

Atom Atom::Marker(uint16_t opcode, uint8_t arity)
{
    return Atom((MARKER << 24) + ((opcode & 0x0FFF) << 8) + arity);
}

Atom Atom::Operator(uint16_t opcode, uint8_t arity)
{
    return Atom((OPERATOR << 24) + ((opcode & 0x0FFF) << 8) + arity);
}

Atom Atom::Node(uint8_t nodeID)
{
    return Atom((NODE << 24) + (nodeID << 8));
}

Atom Atom::UndefinedNode()
{
    return Atom(0xA0FFFFFF);
}

Atom Atom::Device(uint8_t nodeID, uint8_t classID, uint8_t devID)
{
    return Atom((DEVICE << 24) + (nodeID << 16) + (classID << 8) + devID);
}

Atom Atom::UndefinedDevice()
{
    return Atom(0xA1FFFFFF);
}

Atom Atom::DeviceFunction(uint16_t opcode)
{
    return Atom((DEVICE_FUNCTION << 24) + (opcode << 8));
}

Atom Atom::UndefinedDeviceFunction()
{
    return Atom(0xA2FFFFFF);
}

Atom Atom::String(uint8_t characterCount)
{
    uint8_t blocks = characterCount / 4;

    if (characterCount % 4) {
        ++blocks;
    }

    return Atom((STRING << 24) + (blocks << 8) + characterCount);
}

Atom Atom::UndefinedString()
{
    return Atom(0xC6FFFFFF);
}

Atom Atom::Timestamp()
{
    return Atom(TIMESTAMP << 24);
}

Atom Atom::UndefinedTimestamp()
{
    return Atom(0xC7FFFFFF);
}

Atom Atom::InstantiatedProgram(uint16_t opcode, uint8_t arity)
{
    return Atom((INSTANTIATED_PROGRAM << 24) + ((opcode & 0x0FFF) << 8) + arity);
}

Atom Atom::Group(uint16_t opcode, uint8_t arity)
{
    return Atom((GROUP << 24) + ((opcode & 0x0FFF) << 8) + arity);
}

Atom Atom::InstantiatedCPPProgram(uint16_t opcode, uint8_t arity)
{
    return Atom((INSTANTIATED_CPP_PROGRAM << 24) + ((opcode & 0x0FFF) << 8) + arity);
}

Atom Atom::InstantiatedAntiProgram(uint16_t opcode, uint8_t arity)
{
    return Atom((INSTANTIATED_ANTI_PROGRAM << 24) + ((opcode & 0x0FFF) << 8) + arity);
}

Atom Atom::InstantiatedInputLessProgram(uint16_t opcode, uint8_t arity)
{
    return Atom((INSTANTIATED_INPUT_LESS_PROGRAM << 24) + ((opcode & 0x0FFF) << 8) + arity);
}

Atom Atom::CompositeState(uint16_t opcode, uint8_t arity)
{
    return Atom((COMPOSITE_STATE << 24) + ((opcode & 0x0FFF) << 8) + arity);
}

Atom Atom::Model(uint16_t opcode, uint8_t arity)
{
    return Atom((MODEL << 24) + ((opcode & 0x0FFF) << 8) + arity);
}

Atom Atom::NullProgram(bool take_past_inputs)
{
    return Atom((NULL_PROGRAM << 24) + (take_past_inputs ? 1 : 0));
}

Atom::Atom(uint32_t  a): atom(a)
{
}

Atom::~Atom()
{
}

Atom &Atom::operator =(const Atom& a)
{
    atom = a.atom;
    return *this;
}

bool Atom::operator ==(const Atom& a) const
{
    return atom == a.atom;
}

bool Atom::operator !=(const Atom& a) const
{
    return atom != a.atom;
}

bool Atom::operator !() const
{
    return isUndefined();
}

Atom::operator size_t () const
{
    return (size_t)atom;
}

bool Atom::isUndefined() const
{
    return atom == 0xFFFFFFFF;
}

uint8_t Atom::getDescriptor() const
{
    return atom >> 24;
}

bool Atom::isStructural() const
{
    return ((atom & 0xC0000000) == 0xC0000000 || (atom & 0xD0000000) == 0xD0000000);
}

bool Atom::isFloat() const
{
    return atom >> 31 == 0;
}

bool Atom::readsAsNil() const
{
    return atom == 0x80000000 ||
           atom == 0x3FFFFFFF ||
           atom == 0x81FFFFFF ||
           atom == 0xC1000000 ||
           atom == 0xA0FFFFFF ||
           atom == 0xA1FFFFFF ||
           atom == 0xA2FFFFFF ||
           atom == 0xC6FFFFFF;
}

float Atom::asFloat() const
{
    uint32_t _f = atom << 1;
    return *reinterpret_cast<const float *>(&_f);
}

bool Atom::asBoolean() const
{
    return atom & 0x000000FF;
}

uint16_t Atom::asIndex() const
{
    return atom & 0x00000FFF;
}

uint8_t Atom::asInputIndex() const
{
    return (uint8_t)((atom & 0x000FF000) >> 12);
}

uint8_t Atom::asRelativeIndex() const
{
    return (uint8_t)((atom & 0x000FF000) >> 12);
}

uint16_t Atom::asOpcode() const
{
    return (atom >> 8) & 0x00000FFF;
}

uint16_t Atom::asCastOpcode() const
{
    return (uint16_t)((atom & 0x00FFF000) >> 12);
}

uint8_t Atom::getNodeID() const
{
    return (uint8_t)((atom & 0x00FF0000) >> 16);
}

uint8_t Atom::getClassID() const
{
    return (atom & 0x0000FF00) >> 8;
}

uint8_t Atom::getDeviceID() const
{
    return atom & 0x000000FF;
}

uint8_t Atom::asAssignmentIndex() const
{
    return (atom & 0x00FF0000) >> 16;
}

uint8_t Atom::getAtomCount() const
{
    switch (getDescriptor()) {
    case SET:
    case OBJECT:
    case MARKER:
    case C_PTR:
    case OPERATOR:
    case INSTANTIATED_PROGRAM:
    case INSTANTIATED_CPP_PROGRAM:
    case INSTANTIATED_INPUT_LESS_PROGRAM:
    case INSTANTIATED_ANTI_PROGRAM:
    case COMPOSITE_STATE:
    case MODEL:
    case GROUP:
    case S_SET:
        return atom & 0x000000FF;

    case STRING:
        return (atom & 0x0000FF00) >> 8;

    case TIMESTAMP:
        return 2;

    default:
        return 0;
    }
}

bool Atom::takesPastInputs() const
{
    return atom & 0x00000001;
}

uint8_t Atom::Members_to_go = 0;
uint8_t Atom::Timestamp_data = 0;
uint8_t Atom::String_data = 0;
uint8_t Atom::Char_count = 0;
void Atom::trace() const
{
    write_indents();

    switch (getDescriptor()) {
    case NIL:
        LOG_TRACE << "nil";
        return;

    case BOOLEAN_:
        LOG_TRACE << "bl: " << std::boolalpha << asBoolean();
        return;

    case WILDCARD:
        LOG_TRACE << ":";
        return;

    case T_WILDCARD:
        LOG_TRACE << "::";
        return;

    case I_PTR:
        LOG_TRACE << "iptr: " << std::dec << asIndex();
        return;

    case VL_PTR:
        LOG_TRACE << "vlptr: " << std::dec << asIndex();
        return;

    case R_PTR:
        LOG_TRACE << "rptr: " << std::dec << asIndex();
        return;

    case IPGM_PTR:
        LOG_TRACE << "ipgm_ptr: " << std::dec << asIndex();
        return;

    case IN_OBJ_PTR:
        LOG_TRACE << "in_obj_ptr: " << std::dec << (uint32_t)asInputIndex() << " " << asIndex();
        return;

    case D_IN_OBJ_PTR:
        LOG_TRACE << "d_in_obj_ptr: " << std::dec << (uint32_t)asRelativeIndex() << " " << asIndex();
        return;

    case OUT_OBJ_PTR:
        LOG_TRACE << "out_obj_ptr: " << std::dec << asIndex();
        return;

    case VALUE_PTR:
        LOG_TRACE << "value_ptr: " << std::dec << asIndex();
        return;

    case PROD_PTR:
        LOG_TRACE << "prod_ptr: " << std::dec << asIndex();
        return;

    case ASSIGN_PTR:
        LOG_TRACE << "assign_ptr: " << std::dec << (uint16_t)asAssignmentIndex() << " " << asIndex();
        return;

    case THIS:
        LOG_TRACE << "this";
        return;

    case VIEW:
        LOG_TRACE << "view";
        return;

    case MKS:
        LOG_TRACE << "mks";
        return;

    case VWS:
        LOG_TRACE << "vws";
        return;

    case NODE:
        LOG_TRACE << "nid: " << std::dec << (uint32_t)getNodeID();
        return;

    case DEVICE:
        LOG_TRACE << "did: " << std::dec << (uint32_t)getNodeID() << " " << (uint32_t)getClassID() << " " << (uint32_t)getDeviceID();
        return;

    case DEVICE_FUNCTION:
        LOG_TRACE << "fid: " << std::dec << asOpcode();
        return;

    case C_PTR:
        LOG_TRACE << "cptr: " << std::dec << (uint16_t)getAtomCount();
        Members_to_go = getAtomCount();
        return;

    case SET:
        LOG_TRACE << "set: " << std::dec << (uint16_t)getAtomCount();
        Members_to_go = getAtomCount();
        return;

    case OBJECT:
        LOG_TRACE << "obj: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount();
        Members_to_go = getAtomCount();
        return;

    case S_SET:
        LOG_TRACE << "s_set: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount();
        Members_to_go = getAtomCount();
        return;

    case MARKER:
        LOG_TRACE << "mk: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount();
        Members_to_go = getAtomCount();
        return;

    case OPERATOR:
        LOG_TRACE << "op: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount();
        Members_to_go = getAtomCount();
        return;

    case STRING:
        LOG_TRACE << "st: " << std::dec << (uint16_t)getAtomCount();
        Members_to_go = String_data = getAtomCount();
        Char_count = (atom & 0x000000FF);
        return;

    case TIMESTAMP:
        LOG_TRACE << "us";
        Members_to_go = Timestamp_data = 2;
        return;

    case GROUP:
        LOG_TRACE << "grp: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount();
        Members_to_go = getAtomCount();
        return;

    case INSTANTIATED_PROGRAM:
    case INSTANTIATED_ANTI_PROGRAM:
    case INSTANTIATED_INPUT_LESS_PROGRAM:
        LOG_TRACE << "ipgm: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount();
        Members_to_go = getAtomCount();
        return;

    case COMPOSITE_STATE:
        LOG_TRACE << "cst: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount();
        Members_to_go = getAtomCount();
        return;

    case MODEL:
        LOG_TRACE << "mdl: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount();
        Members_to_go = getAtomCount();
        return;

    case NULL_PROGRAM:
        LOG_TRACE << "null pgm " << (takesPastInputs() ? "all inputs" : "new inputs");
        return;

    default:
        if (Timestamp_data) {
            --Timestamp_data;
            LOG_TRACE << atom;
        } else if (String_data) {
            --String_data;
            std::string s;
            char *content = (char *)&atom;

            for (uint8_t i = 0; i < 4; ++i) {
                if (Char_count-- > 0) {
                    s += content[i];
                } else {
                    break;
                }
            }

            LOG_TRACE << s.c_str();
        } else if (isFloat()) {
            LOG_TRACE << "nb: " << std::dec << asFloat();
            return;
        } else {
            LOG_TRACE << "undef";
        }

        return;
    }
}

void Atom::write_indents() const
{
    if (Members_to_go) {
        LOG_TRACE << "   ";
        --Members_to_go;
    }
}

void Atom::Trace(Atom *base, uint16_t count)
{
    std::string trace = "--------\n";
    for (uint16_t i = 0; i < count; ++i) {
        trace = trace + std::to_string(i) + "\t";
        base[i].trace();
    }
    LOG_TRACE << trace;
}
}
