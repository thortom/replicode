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



