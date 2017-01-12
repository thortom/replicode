//
// Created by Daniel MacDonald on 1/10/17.
//

#ifndef REPLICODE_UNITYMEM_H
#define REPLICODE_UNITYMEM_H


#include <r_exec/object.h>
#include <r_exec/mem.h>
#include "unity.h"


class UnityMem : public r_exec::Mem<r_exec::LObject, r_exec::MemStatic>
{
private:


//    mBrane::sdk::module::_Module *module;

//    UNORDERED_MAP<uint32_t, P<Code>> entity_map; // first n objects: left side: OID assigned by rMem, right side: object (ent and ont). n=number of ent and ont objects.
// from n+1 on: left side: OID assigned by I/O devices (foreign objects, i.e.new entites in the environment), right side: the corresponding replicode object.
// foreign objects are never referred to in commands from rMem.
//    r_exec::View *build_view(uint64_t time, uint8_t nodeID) const;
public:
    //DMem(mBrane::sdk::module::_Module *m);
    UnityMem() : Mem() {}

//    void eject(View *view, uint16_t nodeID); // RMem to RMem.
    void eject(Code *command) override; // RMem to I/O device.
//    void inject(Code *object, uint8_t nodeID);

//    void add_entity_map_entry(Code *entity); // called for each axiomatic entity.

//    Code *get_object(uint32_t OID, uint8_t NID); // creates a new entity if the OID is not found.



    // hacks to support command callbacks
    FireCommandCallback fire_command_callback;
    RotateToCommandCallback rotate_to_command_callback;

};


#endif //REPLICODE_UNITYMEM_H
