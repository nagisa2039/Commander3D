cmake_minimum_required(VERSION 3.20)
project(CommanderCoreTests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

# Domain / Application sources are UE-free, so they are built here without Unreal.
set(PROJ_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../../Proj/Source)

add_library(CommanderDomain INTERFACE)
target_include_directories(CommanderDomain INTERFACE ${PROJ_SOURCE}/CommanderDomain/Public)

add_library(CommanderApplication INTERFACE)
target_include_directories(CommanderApplication INTERFACE ${PROJ_SOURCE}/CommanderApplication/Public)
target_link_libraries(CommanderApplication INTERFACE CommanderDomain)

# One executable per test file; each is registered as one CTest test.
function(add_core_test Name)
	add_executable(${Name} ${ARGN})
	target_link_libraries(${Name} PRIVATE CommanderDomain CommanderApplication)
	target_include_directories(${Name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
	if(MSVC)
		target_compile_options(${Name} PRIVATE /W4 /WX)
	else()
		target_compile_options(${Name} PRIVATE -Wall -Wextra -Werror)
	endif()
	add_test(NAME ${Name} COMMAND ${Name})
endfunction()

add_core_test(GridCoordTest Domain/GridCoordTest.cpp)
add_core_test(GridTest Domain/GridTest.cpp)
add_core_test(BattleMapTest Domain/BattleMapTest.cpp)
add_core_test(ResultTest Application/ResultTest.cpp)
