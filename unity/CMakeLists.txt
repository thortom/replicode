set(unity_SRC
        unity.cpp
        inifile.cpp
#        class.cpp
#        replistruct.cpp
#        compiler.cpp
#        decompiler.cpp
#        preprocessor.cpp
#        segments.cpp
#        structure_member.cpp
        callbackstream.cpp callbackstream.h)
#set(unity_HDR
#        class.h
#        compiler.h
#        decompiler.h
#        out_stream.h
#        preprocessor.h
#        replistruct.h
#        segments.h
#        structure_member.h
#        )
#

add_library(unity SHARED ${unity_SRC} ${unity_HDR})
set_property(TARGET unity PROPERTY CXX_STANDARD 11)
set_property(TARGET unity PROPERTY CXX_STANDARD_REQUIRED ON)
install(TARGETS unity DESTINATION lib)
install(FILES ${unity_HDR} DESTINATION include/unity)

target_link_libraries(unity r_exec r_comp r_code dl pthread ${PLATFORM_LIBS})
