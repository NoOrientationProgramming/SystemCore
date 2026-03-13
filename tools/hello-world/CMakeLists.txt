
cmake_minimum_required(VERSION 3.10)

project("SystemCore - Introduction" LANGUAGES CXX)

set(SRCS
    ../../Processing.cpp
    ../../Log.cpp
    ../../SystemDebugging.cpp
    ../../SystemCommanding.cpp
    ../../TcpListening.cpp
    ../../TcpTransfering.cpp
    main.cpp
    Introducing.cpp
    ChildExecuting.cpp
)

set(DEFS
    CONFIG_PROC_HAVE_LOG=1
    CONFIG_PROC_HAVE_CORE_LOG=1
)

if(WIN32)
    set(EXE_NAME Introducing)
else()
    set(EXE_NAME introducing)
endif()

add_executable(${EXE_NAME} ${SRCS})

target_include_directories(${EXE_NAME} PRIVATE ../..)

target_compile_definitions(${EXE_NAME} PRIVATE ${DEFS})

find_package(Threads REQUIRED)
target_link_libraries(${EXE_NAME} PRIVATE Threads::Threads)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

    target_compile_options(${EXE_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wfatal-errors
        -Wreorder
        -Wswitch-enum
        -Wuseless-cast
        -Wparentheses
        -Wshift-overflow
        -Wsign-compare
        -Wzero-as-null-pointer-constant
        -Wcast-align
        -Wcast-qual
        -Wcatch-value
        -Wchar-subscripts
        -Wswitch-default
        -Wctor-dtor-privacy
        -Wduplicated-branches
        -Wduplicated-cond
        -Wempty-body
        -Wextra-semi
        -Wfloat-equal
        -Wformat
        -Wformat-extra-args
        -Wimplicit-fallthrough
        -Wmissing-field-initializers
        -Wnull-dereference
        -Wshadow
    )

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")

    target_compile_options(${EXE_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wno-gnu-zero-variadic-macro-arguments
    )

endif()

if(WIN32)

    target_compile_definitions(${EXE_NAME} PRIVATE
        _WIN32_WINNT=_WIN32_WINNT_WIN10
        WINVER=_WIN32_WINNT_WIN10
    )

    target_link_libraries(${EXE_NAME} PRIVATE ws2_32)

else()

    target_compile_options(${EXE_NAME} PRIVATE -std=gnu++11)

endif()

if(MSVC)
    target_compile_options(${EXE_NAME} PRIVATE /std:c++17)
endif()

