//
// Created by Daniel MacDonald on 1/4/17.
//



#include <r_code/atom.h>            // for Atom
#include <r_code/image.h>           // for Image
#include <r_code/image_impl.h>      // for ImageImpl
#include <r_code/object.h>          // for Code, View, View::::SYNC_ONCE
#include <r_code/replicode_defs.h>  // for VIEW_ARITY, VIEW_HOST, VIEW_IJT, etc
#include <r_code/utils.h>           // for Utils
#include <r_code/vector.h>          // for vector
#include <r_comp/decompiler.h>      // for Decompiler
#include <r_comp/segments.h>        // for Image, ObjectNames, Metadata
#include <r_exec/factory.h>         // for Fact
#include <r_exec/init.h>            // for Now, Compile, Init
#include <r_exec/mem.h>             // for _Mem, Mem, MemStatic, etc
#include <r_exec/object.h>          // for LObject
#include <r_exec/opcodes.h>         // for Opcodes, Opcodes::MkVal
#include <r_exec/view.h>            // for View, View::ViewOpcode
#include <stdint.h>                 // for uint64_t, uint16_t, uintptr_t
#include <stdlib.h>                 // for srand


#include <replicode_common.h>      // for debug, DebugStream
#include "settings.h"               // for Settings

#include "callbackstream.h"         // for hooking std::cout

#include "unity.h"                   // for unity context





////////////////////////////////////////////////////////////
// helpers

using namespace r_comp;

r_exec::View *build_view(uint64_t time, Code* rstdin)   // this is application dependent WRT view->sync.
{
    r_exec::View *view = new r_exec::View();
    const uint64_t arity = VIEW_ARITY; // reminder: opcode not included in the arity.
//    uint16_t write_index = 0;
    uint16_t extent_index = arity + 1;
    view->code(VIEW_OPCODE) = Atom::SSet(r_exec::View::ViewOpcode, arity);
    view->code(VIEW_SYNC) = Atom::Float(View::SYNC_ONCE); // sync on front.
    view->code(VIEW_IJT) = Atom::IPointer(extent_index); // iptr to injection time.
    view->code(VIEW_SLN) = Atom::Float(1.0); // sln.
    view->code(VIEW_RES) = Atom::Float(1); // res is set to 1 upr of the destination group.
    view->code(VIEW_HOST) = Atom::RPointer(0); // stdin/stdout is the only reference.
    view->code(VIEW_ORG) = Atom::Nil(); // org.
    Utils::SetTimestamp(&view->code(extent_index), time);
    view->references[0] = rstdin;
    return view;
}

Code *make_object(r_exec::_Mem *mem, Code* rstdin, double i)
{
    Code *object = new r_exec::LObject(mem);
    //object->code(0)=Atom::Marker(r_exec::GetOpcode("mk.val"),4); // Caveat: arity does not include the opcode.
    object->code(0) = Atom::Marker(r_exec::Opcodes::MkVal, 4); // Caveat: arity does not include the opcode.
    object->code(1) = Atom::RPointer(0);
    object->code(2) = Atom::RPointer(1);
    object->code(3) = Atom::Float(i);
    object->code(4) = Atom::Float(1); // psln_thr.
    object->set_reference(0, rstdin);
    object->set_reference(1, rstdin);
    return object;
}


void do_injection(r_exec::_Mem *mem, Code* object)
{
    Code* rstdin = mem->get_stdin();
//    uint64_t t0 = r_exec::Now();
    {
//        Code* object = make_object(mem, rstdin, i);
        uint64_t now = r_exec::Now();
        // Build a fact.
        Code *fact = new r_exec::Fact(object, now, now, 1, 1);

        // Build a default view for the fact.
        r_exec::View *view = build_view(now, rstdin);

        // Inject the view.
        view->set_object(fact);
        mem->inject(view);
    }

}




/////////////////////////////////////////////////////////////////////////////////////////////////
// public interface




Code* get_code_mk_val_vec3(r_exec::_Mem *m,
                          //uint32_t object_id, uint32_t attribute_id,
                          Code* _object, Code* _attribute,
                          float x, float y, float z)
{

    // X problem: how do i resolve oids to entities?
    //      -- how do i even get the oid of my seed entities in the first place??
    //      metadata->getObjectName(i)
    //      looks like the index should correspond to the ordering in the Mem::objects private member
    //      may need another method to access it, can get oid's out of those objects i think
    //      for seed objects, this->ram_objects should have the right correspondence? assuming oids have been assigned at load time??
    //      ...yes, confirmed

    // for tracking of entities injected later, will need another way...basically replicate the entity_map in the other project
    // injector will need to return oids or something (or add string name metadata?? for new things...if thats even allowed?)
    uint16_t vec3_opcode = m->metadata->opcodes["vec3"];


    Code *object = new r_exec::LObject(m);
    object->code(0) = Atom::Marker(r_exec::Opcodes::MkVal, 4); // Caveat: arity does not include the opcode.
    object->code(1) = Atom::RPointer(0);
    object->code(2) = Atom::RPointer(1);
    object->code(3) = Atom::IPointer(5); // points to the vector.
    object->code(4) = Atom::Float(1); // psln_thr.
    object->code(5) = Atom::Object(vec3_opcode /*r_comp::ClassRegister::GetOpcode("vec3")*/, 3); // Caveat: arity does not include the opcode.
    object->code(6) = Atom::Float(x); //this->value[0]);
    object->code(7) = Atom::Float(y); //this->value[1]);
    object->code(8) = Atom::Float(z); //this->value[2]);
//    object->set_reference(0, m->get_object(this->object, this->senderNodeID()));
//    object->set_reference(1, m->get_object(this->attribute, this->senderNodeID()));

    // need to retrieve actual objects (Code* ) from mem
    object->set_reference(0, _object);
    object->set_reference(1, _attribute);


    // TODO: maybe need to manually hack the OID?
    // yep. fuck. ok.

    return object;


/*
    Code *object = new r_exec::LObject(m);
    object->code(0) = Atom::Marker(r_comp::ClassRegister::GetOpcode("mk.val"), 4); // Caveat: arity does not include the opcode.
    object->code(1) = Atom::RPointer(0);
    object->code(2) = Atom::RPointer(1);
    object->code(3) = Atom::IPointer(5); // points to the vector.
    object->code(4) = Atom::Float(1); // psln_thr.
    object->code(5) = Atom::Object(r_comp::ClassRegister::GetOpcode("vec3"), 3); // Caveat: arity does not include the opcode.
    object->code(6) = Atom::Float(x); //this->value[0]);
    object->code(7) = Atom::Float(y); //this->value[1]);
    object->code(8) = Atom::Float(z); //this->value[2]);
    object->set_reference(0, m->get_object(this->object, this->senderNodeID()));
    object->set_reference(1, m->get_object(this->attribute, this->senderNodeID()));
    return object;

*/
}

Code* ExecutionContext::find_ram_object(uint32_t oid)
{
//    ::debug("find_ram_objects") << "obj:" << (std::hex, obj) << (std::dec, "") << "\n";

    // TODO: may be unnecessary, to search, if ram_object order always matches oid assignment

   // for (auto obj : ram_objects)
    ::debug("find_ram_objects") << "ram_objects:" << ram_objects.size();

    if (oid < ram_objects.size())
    {
        Code *obj = ram_objects[oid];
        ::debug("find_ram_objects") << "obj->get_oid():" << (std::hex, obj->get_oid()) << (std::dec, "");
        return obj;

    } else
    {
        ::debug("find_ram_objects") << "ERROR:" << "oid (" << oid << ") > ram_objects.size():" << ram_objects.size();
        return NULL;
    }

//    for (uint64_t i = 0; i < ram_objects.size(); i++)
//    {
//        Code *obj = ram_objects[i];
//        ::debug("find_ram_objects") << "obj:" << (std::hex, obj) << (std::dec, "");
//        ::debug("find_ram_objects") << "obj->get_oid():" << (std::hex, obj->get_oid()) << (std::dec, "");
//
//        if (obj->get_oid() == oid)
//        {
//            return obj;
//        }
//    }
//
//    return NULL;
}

void ExecutionContext::inject_mk_val_vec3(uint32_t object_id, uint32_t attribute_id,
                                          float x, float y, float z)
{
    ::debug("inject_mk_val_vec3") << "oid:" << object_id << ", attrid:" << attribute_id << "\n";

    Code *object = find_ram_object(object_id);
    Code *attribute = find_ram_object(attribute_id);

    if (object == NULL || attribute == NULL) {
        ::debug("inject_mk_val_vec3") << "object or attribute was null, aborting\n";
        return;
    }

    Code *marker_object = get_code_mk_val_vec3(mem, object, attribute, x, y, z);

    // SO ANNOYING -- cant set the fucking oid on this or it vanishes immediately (!!!)
//    static int count = 0;
//    marker_object->set_oid(get_next_unregistered_oid());
//    marker_object->set_oid(10000 + (count++)); //get_next_unregistered_oid());

    ::debug("inject_mk_val_vec3") << "marker obj oid" << marker_object->get_oid();
    Code* rstdin = mem->get_stdin();

    uint64_t now = r_exec::Now();
    // Build a fact.
    Code *fact = new r_exec::Fact(marker_object, now, now + 10000000, 1, 1);

    // Build a default view for the fact.
    r_exec::View *view = build_view(now, rstdin);

    // Inject the view.
    view->set_object(fact);
    mem->inject(view);


}

