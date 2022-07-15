cmake_minimum_required(VERSION 3.15)
project(multimedia)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/bindings/)

# library
add_library(multimedia
        base/containers/queue.cc
        bindings/imgui_impl_glfw.cpp
        bindings/imgui_impl_opengl3.cpp
        )

# tutorials
set(TUTORIALS
        tutorials/cmake_introduction.cc
        tutorials/conan_getting_started.cc
        tutorials/google_glog.cc
        tutorials/unicode_org_icu.cc
        tutorials/singly_linked_list.cc
        # tutorials/google_test.cc
        tutorials/constructor.cc
        tutorials/ocornut_imgui.cc
        tutorials/decode_audio.cc
        tutorials/encode_audio.cc
        )

foreach (TUTORIAL ${TUTORIALS})
    string(REGEX MATCHALL "[0-9A-Za-z_]*.cc" temp ${TUTORIAL})
    string(REGEX REPLACE ".cc" "" EXE ${temp})
    add_executable(${EXE} ${TUTORIAL})
    target_link_libraries(${EXE} multimedia ${CONAN_LIBS})
endforeach ()

# google tests
enable_testing()

add_executable(multimedia_test
        base/containers/queue_unittest.cc
        tutorials/google_unittest.cc
        )
target_link_libraries(
        multimedia_test
        multimedia
        ${CONAN_LIBS}
)

include(GoogleTest)
gtest_discover_tests(multimedia_test)
