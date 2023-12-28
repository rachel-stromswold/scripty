cmake_minimum_required(VERSION 3.15)
project(dotgen VERSION 0.1.0 DESCRIPTION "A utility for text manipulation and output")

set(CMAKE_BUILD_TYPE DEBUG)

#set package module loading to include the cmake directory
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

#set(CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)


set(LIB_NAME dotgen)
set(MAIN_EXE scripty)
set(TEST_EXE scripty_test)

set(BUILD_EXE 1)

#build the library
add_library(${LIB_NAME} SHARED
    src/errors.c
    src/utils.c
    src/values.c
    src/operations.c
)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${LIB_NAME} m)
set_target_properties(${LIB_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

#make testing executable
if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    #find_package(Catch2 REQUIRED)
    add_executable( ${TEST_EXE} src/errors.c src/utils.c src/values.c src/operations.c src/tests.cpp )
    #add_executable( ${TEST_EXE} src/tests.cpp )
    #target_link_libraries( ${TEST_EXE} PRIVATE ${LIB_NAME} Catch2::Catch2 )
    target_link_libraries(${TEST_EXE} PRIVATE ${LIB_NAME})
    target_include_directories( ${TEST_EXE} PRIVATE "/usr/include/doctest" )
endif()

#make main executable
if(${BUILD_EXE} MATCHES 1)
    add_executable(${MAIN_EXE} src/main.c)
    target_link_libraries(${MAIN_EXE} ${LIB_NAME})
endif()
