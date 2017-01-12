set(unity_SRC
        unity.cpp
        inifile.cpp
        visualizer.cpp
        callbackstream.cpp
        callbackstream.h
        injection.cpp UnityMem.cpp UnityMem.h)
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

#set(UNITY_SWIG_ASSET_PATH /Users/dmacd/agi/AERA/unity-AERA/unity-aera-proto1/Assets/AERA/Native/SWIG)
#
#set_target_properties(unity PROPERTIES
#        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${UNITY_SWIG_ASSET_PATH}
#        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${UNITY_SWIG_ASSET_PATH})


set_property(TARGET unity PROPERTY CXX_STANDARD 11)
set_property(TARGET unity PROPERTY CXX_STANDARD_REQUIRED ON)
install(TARGETS unity DESTINATION lib)
install(FILES ${unity_HDR} DESTINATION include/unity)

target_link_libraries(unity r_exec r_comp r_code dl pthread ${PLATFORM_LIBS})





##########################
## swig

# where to dump all the outputs
set(UNITY_SWIG_ASSET_PATH /Users/dmacd/agi/AERA/unity-AERA/unity-aera-proto1/Assets/AERA/Native/SWIG)

# looks like I HAVE NO CHOICE BUT TO USE A .i !!! otherwise fucking piece of shit swig module ignores it
SET(INTERFACE_FILES unity.i unity.h unity.cpp )

FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

#FIND_PACKAGE(PythonLibs)
#INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

# ???
#FIND_PACKAGE(CSharpLibs)
#INCLUDE_DIRECTORIES(${CSHARP_INCLUDE_PATH})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_INCLUDE_PATH})

SET(CMAKE_SWIG_FLAGS ${CMAKE_SWIG_FLAGS} -namespace AERA.Native.SWIG)  # NB: cant put this is quotes or weird escape spacing issues come up and fuck up the options. yay

#SET_SOURCE_FILES_PROPERTIES(example.i PROPERTIES CPLUSPLUS ON)
#SET_SOURCE_FILES_PROPERTIES(example.i PROPERTIES SWIG_FLAGS "-includeall")
SET_SOURCE_FILES_PROPERTIES(${INTERFACE_FILES} PROPERTIES CPLUSPLUS ON)
#SET_SOURCE_FILES_PROPERTIES(unity.i PROPERTIES SWIG_FLAGS "-includeall")
#SET_SOURCE_FILES_PROPERTIES(${unity_SRC} PROPERTIES CPLUSPLUS ON)
#SET_SOURCE_FILES_PROPERTIES(${unity_SRC} PROPERTIES SWIG_FLAGS "-includeall")

set_property(SOURCE ${INTERFACE_FILES} PROPERTY SWIG_MODULE_NAME unity_swig)


### ooohhkay. can add deps like this http://public.kitware.com/pipermail/cmake/2012-December/053045.html
SET(CMAKE_SWIG_OUTDIR ${UNITY_SWIG_ASSET_PATH}) #"csharp") #${"/tmp/swig") ### <------ GRRR NEXT STEP FIX THIS SHIT TO POINT TO UNITY ASSET FOLDER...OR POST BUILD STEP TO COPY


SWIG_ADD_MODULE(unity_swig csharp ${INTERFACE_FILES} ${unity_SRC})

set_target_properties(unity_swig PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SWIG_OUTDIR})


# not in our version apparently
#SWIG_ADD_LIBRARY(unity_swig
#                 TYPE SHARED #<SHARED|MODULE|STATIC|USE_BUILD_SHARED_LIBS>]
#                 LANGUAGE csharp
#                 SOURCES ${unity_SRC} unity.h unity.cpp)
#

# where language specific files go

SWIG_LINK_LIBRARIES(unity_swig r_exec r_comp r_code dl pthread ${PLATFORM_LIBS} ${CSHARP_LIBRARIES})



#add_custom_command(TARGET unity_swig POST_BUILD
#        COMMAND "${CMAKE_COMMAND}" -E copy
#        "$<TARGET_FILE:unity_swig>"
#        "${UNITY_SWIG_ASSET_PATH}/$<TARGET_FILE_NAME:unity_swig>"
#        COMMENT "Copying to output directory")
#

###############################
