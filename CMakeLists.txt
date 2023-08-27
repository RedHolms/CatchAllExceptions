cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)

project("CatchAllExceptions")


if(WIN32)
  set(PLATFORM_SOURCES "CAE_win32.cpp")
  enable_language(ASM_MASM)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND PLATFORM_SOURCES "CAE_win32_x64.asm")
    message("Using win32_x64")
  else()
    list(APPEND PLATFORM_SOURCES "CAE_win32_x86.asm")
    message("Using win32_x86")
  endif()
elseif(UNIX)
  
endif()

add_executable("CAE" "test.cpp" ${PLATFORM_SOURCES})