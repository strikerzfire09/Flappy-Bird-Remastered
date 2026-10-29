cmake_minimum_required(VERSION 3.22)
project(flappybird LANGUAGES C CXX VERSION 1.0.0)

set(FLAPPYBIRD_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets" CACHE PATH "Directory containing game assets")
set(FLAPPYBIRD_ICON_ICO "${FLAPPYBIRD_ASSETS_DIR}/favicon.ico" CACHE FILEPATH "Icon embedded into the Windows executable")

# ---------------------------------------------------------------- build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
               Debug Release RelWithDebInfo MinSizeRel)
endif()

# Always put the runnable game (and its assets) in <build-dir>/bin. This keeps
# the path identical for single-config (Make/Ninja) and multi-config (Visual
# Studio) generators, while the wrapper scripts keep each platform in its own
# build directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(_cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg} "${CMAKE_BINARY_DIR}/bin")
endforeach()

# Windows: use the static CRT so the exe does not need the VC++ redistributable.
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ---------------------------------------------------------------- standards
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------- raylib
# raylib is not vendored in this repository. CMake downloads the pinned release
# on the first configure, or uses a local checkout when FLAPPYBIRD_RAYLIB_SOURCE_DIR
# is set (useful for offline builds).
set(FLAPPYBIRD_RAYLIB_VERSION "6.2" CACHE STRING "raylib release to fetch")
set(FLAPPYBIRD_RAYLIB_SOURCE_DIR "" CACHE PATH "Optional local raylib source checkout")

# Build one static, self-contained game binary. Keep examples/tests out of the
# build so a fresh configure is fast.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

if(FLAPPYBIRD_RAYLIB_SOURCE_DIR)
  if(NOT EXISTS "${FLAPPYBIRD_RAYLIB_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "raylib source not found at ${FLAPPYBIRD_RAYLIB_SOURCE_DIR}")
  endif()
  set(FLAPPYBIRD_RAYLIB_SRC "${FLAPPYBIRD_RAYLIB_SOURCE_DIR}")
  add_subdirectory("${FLAPPYBIRD_RAYLIB_SRC}" "${CMAKE_CURRENT_BINARY_DIR}/raylib" EXCLUDE_FROM_ALL)
else()
  include(FetchContent)
  # Keep downloads outside the per-platform build directory: the build wrappers
  # delete that directory for clean builds and must not re-fetch raylib each time.
  set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/.cache" CACHE PATH "Persistent FetchContent cache")
  FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG "${FLAPPYBIRD_RAYLIB_VERSION}"
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
  )
  FetchContent_MakeAvailable(raylib)
  set(FLAPPYBIRD_RAYLIB_SRC "${raylib_SOURCE_DIR}")
endif()

# ---------------------------------------------------------------- windows resources
# Generate the .rc from the real absolute icon path. This is robust with both
# MSVC (rc.exe) and MinGW (windres), regardless of the build directory.
set(FLAPPYBIRD_RC_SOURCES "")
if(MSVC OR MINGW OR WIN32)
  if(EXISTS "${FLAPPYBIRD_ICON_ICO}")
    file(TO_NATIVE_PATH "${FLAPPYBIRD_ICON_ICO}" FLAPPYBIRD_ICON_WIN_PATH)
    set(FLAPPYBIRD_RC_IN "${CMAKE_CURRENT_SOURCE_DIR}/resources/flappy.rc.in")
    set(FLAPPYBIRD_RC "${CMAKE_CURRENT_BINARY_DIR}/resources/flappy.rc")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/resources")
    configure_file("${FLAPPYBIRD_RC_IN}" "${FLAPPYBIRD_RC}" @ONLY)
    set(FLAPPYBIRD_RC_SOURCES "${FLAPPYBIRD_RC}")
  else()
    message(WARNING "favicon.ico not found; Windows executable will not carry a custom icon")
  endif()
endif()

# ---------------------------------------------------------------- platform sources
# Keep OS-specific code in its own folder so Linux and Windows never pull in
# each other's headers (notably <windows.h>, which collides with raylib).
if(WIN32)
  set(FLAPPYBIRD_PLATFORM_SOURCES
      src/platform/windows/platform_windows.cpp)
elseif(APPLE)
  set(FLAPPYBIRD_PLATFORM_SOURCES
      src/platform/macos/platform_macos.cpp)
else()
  set(FLAPPYBIRD_PLATFORM_SOURCES
      src/platform/linux/platform_linux.cpp)
endif()

# ---------------------------------------------------------------- executable
add_executable(flappybird
  src/main.cpp
  src/assets.cpp
  src/icon.cpp
  src/storage.cpp
  src/game.cpp
  src/ui.cpp
  src/fx.cpp
  src/daynight.cpp
  src/view.cpp
  ${FLAPPYBIRD_PLATFORM_SOURCES}
  ${FLAPPYBIRD_RC_SOURCES}
)

target_include_directories(flappybird PRIVATE src ${FLAPPYBIRD_RAYLIB_SRC}/src)
target_link_libraries(flappybird PRIVATE raylib)

# Platform libraries.
if(UNIX AND NOT APPLE)
  find_package(Threads REQUIRED)
  target_link_libraries(flappybird PRIVATE Threads::Threads dl m)
endif()

# ---------------------------------------------------------------- platform settings
if(MSVC)
  target_compile_options(flappybird PRIVATE /W4 /WX- /EHsc /permissive-)
  # /SUBSYSTEM:WINDOWS + main() needs the console entry symbol overridden.
  target_link_options(flappybird PRIVATE /ENTRY:mainCRTStartup)
  target_link_options(flappybird PRIVATE
                      $<$<CONFIG:Release>:/OPT:REF>
                      $<$<CONFIG:Release>:/OPT:ICF>)
  set_target_properties(flappybird PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(MINGW)
  target_compile_options(flappybird PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
  # Fully static runtime: without -static, MinGW may add a dependency on
  # libwinpthread-1.dll, which is not present on a normal Windows install.
  target_link_options(flappybird PRIVATE -static -static-libgcc -static-libstdc++)
  set_target_properties(flappybird PROPERTIES WIN32_EXECUTABLE TRUE)
else()
  target_compile_options(flappybird PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
  target_compile_options(flappybird PRIVATE -ffunction-sections -fdata-sections)
  if(APPLE)
    target_link_options(flappybird PRIVATE -Wl,-dead_strip)
  else()
    target_link_options(flappybird PRIVATE -Wl,--gc-sections)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
      target_link_options(flappybird PRIVATE -s)
    endif()
  endif()
endif()

# Link-time optimisation for our code in Release. Safe to skip if unsupported.
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_CONFIGURATION_TYPES)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT FLAPPYBIRD_IPO_OK OUTPUT FLAPPYBIRD_IPO_LOG
                      LANGUAGES CXX)
  if(FLAPPYBIRD_IPO_OK)
    set_property(TARGET flappybird PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  endif()
endif()

file(TO_CMAKE_PATH "${FLAPPYBIRD_ASSETS_DIR}" FLAPPYBIRD_ASSETS_DIR_CMAKE)
target_compile_definitions(flappybird PRIVATE
  FLAPPYBIRD_ASSETS_DIR="${FLAPPYBIRD_ASSETS_DIR_CMAKE}"
)

# ---------------------------------------------------------------- copy assets next to the binary
function(_copy_dir src dst)
  file(GLOB children RELATIVE "${src}" "${src}/*")
  foreach(child ${children})
    if(IS_DIRECTORY "${src}/${child}")
      _copy_dir("${src}/${child}" "${dst}/${child}")
    else()
      add_custom_command(TARGET flappybird POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${src}/${child}" "${dst}/${child}"
        COMMENT "Copying ${child} -> ${dst}/")
    endif()
  endforeach()
endfunction()

if(EXISTS "${FLAPPYBIRD_ASSETS_DIR}")
  _copy_dir("${FLAPPYBIRD_ASSETS_DIR}" "$<TARGET_FILE_DIR:flappybird>/assets")
endif()

install(TARGETS flappybird RUNTIME DESTINATION bin)
install(DIRECTORY "${FLAPPYBIRD_ASSETS_DIR}/" DESTINATION "share/flappybird/assets")
