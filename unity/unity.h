
#ifndef REPLICODE_UNITY_H
#define REPLICODE_UNITY_H



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

extern "C" DLLEXPORT void update_external_timescale(double timescale);


typedef uint64_t (/*__stdcall*/ *time_base_callback_t)();

typedef void(/*__stdcall*/ * VisualizerAddNodeCallback) (uint32_t oid, const char* _group, const char* _type, const char* _name, const char* _src);
typedef void(/*__stdcall*/ * VisualizerAddEdgeCallback) (const char* src, const char* target, bool is_view);

typedef void(/*__stdcall*/ * FireCommandCallback) (uint64_t deadline);
typedef void(/*__stdcall*/ * RotateToCommandCallback) (float angle, uint64_t deadline);

class UnityMem;

#include "settings.h"
class ExecutionContext
{
protected:
    Settings settings;
    r_comp::Image seed;
    r_comp::Metadata metadata;
    r_exec::_Mem *mem;
//    UnityMem *mem;
    r_code::vector<r_code::Code *> ram_objects; // since mem appears to keep references to these...

    time_base_callback_t time_base_callback;
    r_comp::Decompiler decompiler;

    uint64_t last_starting_time, paused_since;

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
        uint64_t starting_time = mem->start(false);
        last_starting_time = starting_time;
        return starting_time;
    }

    void pause()
    {
        // no special changes for pauseing at present
        paused_since = r_exec::Now();
        update_external_timescale(0);
        mem->stop();

    }

    uint64_t resume()
    {
        debug("EC::resume") << "this=" <<(std::hex,(void*)this) << "mem=" << (void*)mem << (std::dec,"") << "\n";
        // TODO: HACK: move timescale management inside the EC since we need to track it to implement pause/resume effectively
        update_external_timescale(1.0);  // HAAAAAAAAAAAAAAAAAAAAAAACK <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

        uint64_t resuming_time = mem->start(true);
        return resuming_time;
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

    uint16_t get_opcode(std::string name)
    {
        return metadata.opcodes[name];
    }

    void inject_mk_val_vec3(uint32_t object_id, uint32_t attribute_id,
                            float x, float y, float z);

    void inject_fact_for_object(r_code::Code *object);

    r_code::Code* find_ram_object(uint32_t oid);

    // ejection helpers
    // TODO: later expose enough of Code interface to pass the command code back to unity
    // so new device functions dont require C++ code changes

    void RegisterCommandCallbacks(FireCommandCallback fcb,
                                  RotateToCommandCallback rtcb);



};


#endif