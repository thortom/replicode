//
// Created by Daniel MacDonald on 1/10/17.
//

#include <r_comp/segments.h>
#include "UnityMem.h"






void UnityMem::eject(Code *command)
{
    uint16_t function = (uint16_t)((command->code(CMD_FUNCTION).atom >> 8) & 0x000000FF);

    if (function == metadata->opcodes["fire"]) {
        // extract args
        // invoke unity callbacks

        uint16_t args_set_index = command->code(CMD_ARGS).asIndex();
//        uint16_t hand_index = args_set_index + 1;
//        move_to->OID = command->get_reference(command->code(hand_index).asIndex())->get_oid();
//        uint16_t target_position_index = command->code(args_set_index + 2).asIndex();
//        move_to->target_position[0] = command->code(target_position_index + 1).asFloat();
//        move_to->target_position[1] = command->code(target_position_index + 2).asFloat();
//        move_to->target_position[2] = command->code(target_position_index + 3).asFloat();


        uint16_t deadline_index = (uint16_t)(args_set_index + 1);
        uint64_t deadline = Utils::GetTimestamp<Code>(command, deadline_index);

        fire_command_callback(deadline);
//        NODE->send(module, move_to, N::PRIMARY);

    } else if (function == metadata->opcodes["rotate_to"])
    {
        uint16_t args_set_index = command->code(CMD_ARGS).asIndex();

        uint16_t angle_index = (uint16_t)(args_set_index + 1);

        float angle = command->code(angle_index).asFloat();
        uint16_t deadline_index = (uint16_t)(args_set_index + 2);
        uint64_t deadline = Utils::GetTimestamp<Code>(command, deadline_index);

        rotate_to_command_callback(angle, deadline);

    } else
    {
        ::debug("eject") << "unrecognized cmd function " << function;
    }





#if false
    if (function == r_comp::ClassRegister::GetOpcode("speak")) {
        Speak *speak = new Speak();
        uint16_t args_set_index = command->code(CMD_ARGS).asIndex();
        uint16_t word_index = args_set_index + 1;
        std::string w = r_code::Utils::GetString<Code>(command, word_index);
        memcpy(speak->word, w.c_str(), w.length());
        uint16_t deadline_index = args_set_index + 2;
        speak->deadline = Utils::GetTimestamp<Code>(command, deadline_index);
        NODE->send(module, speak, N::PRIMARY);
    } else if (function == r_comp::ClassRegister::GetOpcode("move_hand")) {
        MoveTo *move_to = new MoveTo();
        uint16_t args_set_index = command->code(CMD_ARGS).asIndex();
        uint16_t hand_index = args_set_index + 1;
        move_to->OID = command->get_reference(command->code(hand_index).asIndex())->get_oid();
        uint16_t target_position_index = command->code(args_set_index + 2).asIndex();
        move_to->target_position[0] = command->code(target_position_index + 1).asFloat();
        move_to->target_position[1] = command->code(target_position_index + 2).asFloat();
        move_to->target_position[2] = command->code(target_position_index + 3).asFloat();
        uint16_t deadline_index = args_set_index + 3;
        move_to->deadline = Utils::GetTimestamp<Code>(command, deadline_index);
        NODE->send(module, move_to, N::PRIMARY);
    } else if (function == r_comp::ClassRegister::GetOpcode("grab_hand")) {
        Grab *grab_hand = new Grab();
        uint16_t args_set_index = command->code(CMD_ARGS).asIndex();
        uint16_t hand_index = args_set_index + 1;
        grab_hand->OID = command->get_reference(command->code(hand_index).asIndex())->get_oid();
        uint16_t deadline_index = args_set_index + 2;
        grab_hand->deadline = Utils::GetTimestamp<Code>(command, deadline_index);
        NODE->send(module, grab_hand, N::PRIMARY);
    } else if (function == r_comp::ClassRegister::GetOpcode("release_hand")) {
        Release *release_hand = new Release();
        uint16_t args_set_index = command->code(CMD_ARGS).asIndex();
        uint16_t hand_index = args_set_index + 1;
        release_hand->OID = command->get_reference(command->code(hand_index).asIndex())->get_oid();
        uint16_t deadline_index = args_set_index + 2;
        release_hand->deadline = Utils::GetTimestamp<Code>(command, deadline_index);
        NODE->send(module, release_hand, N::PRIMARY);
    } else if (function == r_comp::ClassRegister::GetOpcode("point_at")) {
        PointAt *point_at = new PointAt();
        uint16_t args_set_index = command->code(CMD_ARGS).asIndex();
        uint16_t hand_index = args_set_index + 1;
        point_at->OID = command->get_reference(command->code(hand_index).asIndex())->get_oid();
        uint16_t target_position_index = command->code(args_set_index + 2).asIndex();
        point_at->target_position[0] = command->code(target_position_index + 1).asFloat();
        point_at->target_position[1] = command->code(target_position_index + 2).asFloat();
        point_at->target_position[2] = command->code(target_position_index + 3).asFloat();
        uint16_t deadline_index = args_set_index + 3;
        point_at->deadline = Utils::GetTimestamp<Code>(command, deadline_index);
        NODE->send(module, point_at, N::PRIMARY);
    } else if (function == r_comp::ClassRegister::GetOpcode("look_at")) {
        LookAt *look_at = new LookAt();
        uint16_t args_set_index = command->code(CMD_ARGS).asIndex();
        uint16_t target_position_index = command->code(args_set_index + 1).asIndex();
        look_at->target_position[0] = command->code(target_position_index + 1).asFloat();
        look_at->target_position[1] = command->code(target_position_index + 2).asFloat();
        look_at->target_position[2] = command->code(target_position_index + 3).asFloat();
        uint16_t deadline_index = args_set_index + 2;
        look_at->deadline = Utils::GetTimestamp<Code>(command, deadline_index);
        NODE->send(module, look_at, N::PRIMARY);
    }
#endif
}