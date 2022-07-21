cmake_minimum_required(VERSION 3.21)
project(csgo)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(menu/imgui)
FILE(GLOB ExternalSource cheat/csgoExternal.cpp cheat/menu.cpp cheat/features/*.cpp cheat/hwid/*.cpp cheat/mem/*.cpp cheat/proc/*.cpp)
add_executable(csgoExternal ${ExternalSource})
#add_executable(menu cheat/menu.cpp)
#add_executable(menu menu/imguiMain.cpp)
#target_link_libraries(menu PUBLIC imgui imgui-win32 imgui-dx9)
target_link_libraries(csgoExternal PUBLIC imgui imgui-win32 imgui-dx9)



#add_library(csgoInternal SHARED cheat/csgo.hpp cheat/internal/csgoInternal.cpp cheat/features/aimbot.cpp cheat/features/aimbot.h)