project(replicode)

cmake_minimum_required(VERSION 2.8)
find_package (Threads)
##############################
## ugh hacks to copy runtime code to a sane place

macro(copy_files GLOBPAT DESTINATION TARGETNAME)
    file(GLOB COPY_FILES
            RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            ${GLOBPAT})
    add_custom_target(${TARGETNAME} ALL
            COMMENT "Copying files: ${GLOBPAT}")

    foreach(FILENAME ${COPY_FILES})
        set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}")
        set(DST "${DESTINATION}/${FILENAME}")

        add_custom_command(
                TARGET ${TARGETNAME}
                COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
        )
    endforeach(FILENAME)
endmacro(copy_files)


##############################

include_directories(${PROJECT_SOURCE_DIR})

set(SANITIZE_ADDRESS Off)
set(SANITIZE_MEMORY On)

# from https://github.com/arsenm/sanitizers-cmake
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/externals/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})

### valgrind -- for when i want to integrate it in to test harness i guess

find_program( MEMORYCHECK_COMMAND valgrind )
set( MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full" )

#set( MEMORYCHECK_SUPPRESSIONS_FILE "${PROJECT_SOURCE_DIR}/valgrind_suppress.txt" )




#################


if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++11 -fPIC -g -Wlanguage-extension-token") # -fsanitize=address
endif()


if (UNIX AND NOT APPLE)
    set(PLATFORM_LIBS "rt")
elseif(APPLE)
    set(PLATFORM_LIBS "")
endif()

find_package(Sanitizers)


# add_compile_options("-stdlib=libstdc++")
# find_library (LIBSTDCXX NAMES stdc++)


add_subdirectory(r_code)
add_subdirectory(r_comp)
add_subdirectory(r_exec)
add_subdirectory(usr_operators)
add_subdirectory(tests)

add_sanitizers(r_code)
add_sanitizers(r_comp)
add_sanitizers(r_exec)
add_sanitizers(usr_operators)

add_executable(replicode executor/main.cpp executor/inifile.cpp replicode_common.h)
add_sanitizers(replicode)

set_property(TARGET replicode PROPERTY CXX_STANDARD 11)
set_property(TARGET replicode PROPERTY CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
#configure_file(executor/settings.ini ${CMAKE_BINARY_DIR}/settings.ini COPYONLY)
#file(COPY examples DESTINATION ./)
copy_files(examples/V1.2/* ${CMAKE_BINARY_DIR} copy_examples)

copy_files(executor/settings.ini ${CMAKE_BINARY_DIR} copy_settings)



target_link_libraries(replicode r_exec r_comp r_code dl pthread ${PLATFORM_LIBS})

install(FILES replicode_common.h DESTINATION include)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)






enable_testing()

foreach(test_file
        "hello.world.replicode"
        "pong.replicode"
        "test.1.replicode"
        "test.2.replicode"
        "test.3.replicode"
        "test.4.replicode")
    add_test(NAME Compiler-${test_file} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/compiler/ COMMAND compilertest ${test_file})
endforeach()
