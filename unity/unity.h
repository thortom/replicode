

#include <r_exec/mem.h>             // for _Mem, Mem, MemStatic, etc
#include <r_comp/segments.h>        // for Image, ObjectNames, Metadata
#include <r_code/image.h>           // for Image
#include <r_comp/decompiler.h>      // for Decompiler
#include <stdint.h>


/////////////////////////////////////////////////////////////
// compiler fuckery
#ifndef __has_declspec_attribute         // Optional of course.
#define __has_declspec_attribute(x) 0  // Compatibility with non-clang compilers.
#endif

#if __has_declspec_attribute(dllexport)
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

//#define DllExport   __declspec( dllexport )


/////////////////////////////////////////////////////////////
//

extern "C" int DLLEXPORT inline foo() { return 27; }

typedef void(/*__stdcall*/ * LogCallback) (const char * str);
//LogCallback gDebugCallback, gDebugErrorCallback;

extern "C" void DLLEXPORT RegisterCoutCallback(LogCallback callback);

extern "C" void DLLEXPORT RegisterCerrCallback(LogCallback callback);


/////////////////////////////////////////////////////////////
// slight future proofing for vis stuff

//namespace jerk {
//    class Atom {
//        // wtf is your problem ??
//        typedef enum { YOU_MOMMA=0x50FA7 };
//
//    };
//}

//class VisContext
//{
//
//
//public:
//    VisContext() : m_mem(NULL), m_seed_image(NULL), m_metadata(NULL)
//    {
//        current = this;
//    }
//
//    ~VisContext()
//    {
//        current = NULL;
//    }
//
//    r_exec::_Mem *m_mem;
//    r_comp::Image *m_seed_image;
//    r_comp::Metadata *m_metadata;
//
//    static VisContext *current;
//
//};


typedef uint64_t (/*__stdcall*/ *time_base_callback_t)();

typedef void(/*__stdcall*/ * VisualizerAddNodeCallback) (uint32_t oid, const char* _group, const char* _type, const char* _name, const char* _src);
typedef void(/*__stdcall*/ * VisualizerAddEdgeCallback) (const char* src, const char* target, bool is_view);


#include "settings.h"
class ExecutionContext
{
protected:
    Settings settings;
    r_comp::Image seed;
    r_comp::Metadata metadata;
    r_exec::_Mem *mem;
    r_code::vector<r_code::Code *> ram_objects; // since mem appears to keep references to these...

    time_base_callback_t time_base_callback;
    r_comp::Decompiler decompiler;

    uint64_t last_starting_time;

    uint32_t last_unregistered_oid;
public:
    ExecutionContext() : mem(NULL),
                         time_base_callback(NULL),
                         last_starting_time(0),
                         last_unregistered_oid(UNDEFINED_OID-1)
    {

    }

    int init(std::string settings_path);

    int init(std::string settings_path, time_base_callback_t tcb);


    ~ExecutionContext()
    {
        if (mem != NULL) {
//            if (r_exec::_Mem::RUNNING == mem->check_state())
//            {
//                mem->stop();
//            }
            delete mem;
            mem = NULL;
        }
    }

    // hack: avoid dealing w/ swig and just put this here
    typedef enum {
        NOT_STARTED = 0,
        RUNNING = 1,
        STOPPED = 2
    } State;


    // TODO: a bit hacky, probably want to have another state to indicate 'not initialized'
    State state()
    {
        if (mem == NULL) return NOT_STARTED;
        return (State)(int)mem->check_state();
    }

    uint64_t start()
    {
        debug("EC::start") << "this=" <<(std::hex,(void*)this) << "mem=" << (void*)mem << (std::dec,"") << "\n";
        uint64_t starting_time = mem->start();
        last_starting_time = starting_time;
        return starting_time;
    }

    void stop()
    {
        debug("EC::stop") << "shutting rMem down...";
        mem->stop();
    }

    void dump_memory(std::string decompiled_output_path = "");

    void visualize(VisualizerAddNodeCallback ancb, VisualizerAddEdgeCallback aecb);

    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////

    // injection helpers

    uint64_t get_object_id(std::string name) {
        for (auto symbol : seed.object_names.symbols)
        {
            if (symbol.second == name)
            {
                return symbol.first;
            }
        }
        return UNDEFINED_OID;
    }

    uint64_t get_next_unregistered_oid()
    {
        return last_unregistered_oid--;
    }

    void inject_mk_val_vec3(uint32_t object_id, uint32_t attribute_id,
                            float x, float y, float z);



    Code* find_ram_object(uint32_t oid);



};

