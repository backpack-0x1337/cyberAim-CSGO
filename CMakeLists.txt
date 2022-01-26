cmake_minimum_required(VERSION 3.21)
project(csgo)

set(CMAKE_CXX_STANDARD 20)

#add_executable(hwid hwid/hwid.cpp hwid/hwid.h)
#add_executable(csgoExternal features/bhop.cpp proc/proc.cpp proc/proc.h mem/mem.cpp mem/mem.h hwid/hwid.cpp hwid/hwid.h external/csgoExternal.cpp features/bhop.h external/csgoExternal.h)
add_library(csgoInternal SHARED csgo.hpp internal/csgoInternal.cpp)