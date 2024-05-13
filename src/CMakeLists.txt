
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("chip8emu")

# Add source to this project's executable.
add_executable (chip8emu "chip8emu.cpp" "chip8emu.hpp" "memory.cpp" "memory.hpp" "chip8.cpp" "chip8.hpp" "display.cpp" "display.hpp")

# path to raylib
if (WIN32)
	set(RAYLIB_DIR "../lib/raylib-5.0_win64_msvc16")
elseif (UNIX)
	set(RAYLIB_DIR "../lib/raylib-5.0_linux_amd64")
endif()

target_include_directories(chip8emu PRIVATE ${RAYLIB_DIR}/include)
target_link_directories(chip8emu PRIVATE ${RAYLIB_DIR}/lib)

# link raylib and it's dependencies
if(WIN32)
	target_link_libraries(chip8emu PRIVATE raylib opengl32 gdi32 winmm)
elseif (UNIX)
	target_link_libraries(chip8emu PRIVATE raylib GL m pthread dl rt)
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET chip8emu PROPERTY CXX_STANDARD 20)
endif()
