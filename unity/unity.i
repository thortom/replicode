//
//
//#include <r_code/atom.h>            // for Atom
//#include <r_code/image.h>           // for Image
//#include <r_code/image_impl.h>      // for ImageImpl
//#include <r_code/object.h>          // for Code, View, View::::SYNC_ONCE
//#include <r_code/replicode_defs.h>  // for VIEW_ARITY, VIEW_HOST, VIEW_IJT, etc
//#include <r_code/utils.h>           // for Utils


%include "swig_helpers.h"

%module unity



/////////////////////////////////////////////////////////////
// stuff to literally include in the wrapper
%{

// forward stuff to help swig generate proper code


// headers and other decls go here

#include "unity.h"


#include "replicode_common.h"

#ifndef REPLICODE_EXPORT
    #define REPLICODE_EXPORT // wtf asshole
#endif

#include <replicode_common.h>

//#include <cstdint>
//#include <cstddef>

#include <r_code/atom.h>
#include <r_code/object.h>
#include <r_exec/object.h>

//    extern "C" int init(const char* settings_path);

//    int foo() { return 27; }


%};


///////////////////////////////////////////////////////////
// stuff that will be processed by swig

%cs_callback(LogCallback, Native.StringCallback)

%cs_callback(VisualizerAddNodeCallback, Native.VisualizerAddNodeCallback)
%cs_callback(VisualizerAddEdgeCallback, Native.VisualizerAddEdgeCallback)

%typemap(imtype) const char * "string"
//%typemap(imtype) std::string "string"

%ignore s_debugSection; // ignore troublesome global mutex declared in replicode_common.h



%include "std_string.i"
%include "stdint.i"

%include "unity.h" // NB: must include this (with the typedefs referred to by any cs_callback defs) AFTER the cs_callback macro invocation



%include "replicode_common.h"


// these confuse SWIG
#undef REPLICODE_EXPORT
#define REPLICODE_EXPORT // wtf asshole



%rename(r_exec_LObject) r_exec::LObject;
//%rename(r_code_LObject) r_code::LObject;

//%include <stdint>
//%include <stddef>
%include "r_code.i"
%include "r_exec.i"

%template(the_LObject) r_exec::Object< r_code::LObject,r_exec::LObject >;

typedef r_exec::Object< r_code::LObject,r_exec::LObject > force_LObject;

//ExecutionContext *g_default_context = NULL;
//extern "C" int init(const char* settings_path);
//
//int foo();
//
//
//
//extern "C" void RegisterCoutCallback(LogCallback callback);
//extern "C" void RegisterCerrCallback(LogCallback callback);
