cmake_minimum_required(VERSION 3.15)
project(multimedia)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(TUTORIALS
        tutorials/cmake_introduction.cc
        tutorials/conan_getting_started.cc
        tutorials/google_glog.cc
        tutorials/unicode_org_icu.cc
        tutorials/singly_linked_list.cc
        )

foreach (TUTORIAL ${TUTORIALS})
    string(REGEX MATCHALL "[0-9A-Za-z_]*.cc" temp ${TUTORIAL})
    string(REGEX REPLACE ".cc" "" EXE ${temp})
    add_executable(${EXE} ${TUTORIAL})
    target_link_libraries(${EXE} ${CONAN_LIBS})
endforeach ()