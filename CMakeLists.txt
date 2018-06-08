project(replicode)

cmake_minimum_required(VERSION 3.0)
find_package (Threads)

include_directories(${PROJECT_SOURCE_DIR})

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++11 -fPIC -g")
endif()

# Boost Log
set(Boost_USE_MULTITHREADED ON)
ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)
find_package(Boost COMPONENTS system thread log log_setup REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})
add_library(common_logger common_logger.cpp)
target_link_libraries(common_logger ${Boost_LIBRARIES})

add_subdirectory(r_code)
add_subdirectory(r_comp)
add_subdirectory(r_exec)
add_subdirectory(usr_operators)
add_subdirectory(tests)

add_executable(replicode executor/main.cpp executor/inifile.cpp replicode_common.h)
set_property(TARGET replicode PROPERTY CXX_STANDARD 11)
set_property(TARGET replicode PROPERTY CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
configure_file(executor/settings.ini ${CMAKE_BINARY_DIR}/settings.ini COPYONLY)
target_link_libraries(replicode r_exec r_comp r_code dl pthread rt common_logger)

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
