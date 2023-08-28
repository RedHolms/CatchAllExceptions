cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)

option(BUILD_EXAMPLE "Build example project" OFF)

project("CatchAllExceptions")

if(WIN32)
  set(CAE_PLATFORM "win")
elseif(UNIX)
  message(FATAL_ERROR "Currently only win32 is supported")
endif()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(CAE_ARCH "x64")
else()
  set(CAE_ARCH "x86")
endif()

enable_language(ASM_MASM)
if(CMAKE_ASM_MASM_COMPILER_WORKS)
  set(CAE_ASM "masm")
  set(CAE_ASM_EXT "asm")
else()
  message(FATAL_ERROR "Current only masm is supported, and it's not present")
endif()

message(
  "[CAE] Using: Platform=${CAE_PLATFORM}; Arch=${CAE_ARCH}; Asm=${CAE_ASM}; AsmExt=${CAE_ASM_EXT}"
)

add_library(cae STATIC
  "CAE.h"
  "CAE_${CAE_PLATFORM}.cpp"
  "CAE_${CAE_ASM}_${CAE_ARCH}.${CAE_ASM_EXT}"
)

if(BUILD_EXAMPLE)
  add_executable(cae_example "example.cpp")
  target_link_libraries(cae_example PRIVATE cae)
endif()
