cmake_minimum_required(VERSION 3.22)
project(flappybird LANGUAGES C CXX VERSION 1.0.0)

set(FLAPPYBIRD_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets" CACHE PATH "Directory containing game assets")
set(FLAPPYBIRD_ICON_ICO "${FLAPPYBIRD_ASSETS_DIR}/favicon.ico" CACHE FILEPATH "Icon embedded into the Windows executable")

# ---------------------------------------------------------------- standards
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------- raylib (vendored)
set(RAYLIB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib" CACHE PATH "Vendored raylib source")
if(NOT EXISTS "${RAYLIB_SRC_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "Vendored raylib not found at ${RAYLIB_SRC_DIR}. See README.md")
endif()

# Build raylib as a static lib, desktop OpenGL, X11 on Linux (works under XWayland/Wayland),
# no examples, no shared lib.
set(RAYLIB_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/raylib" CACHE PATH "")
add_subdirectory("${RAYLIB_SRC_DIR}" "${RAYLIB_BUILD_DIR}" EXCLUDE_FROM_ALL)

# Force raylib static + PIC so it links cleanly into our executable on every platform.
set_property(TARGET raylib PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET raylib PROPERTY BUILD_SHARED_LIBS OFF)

# ---------------------------------------------------------------- windows icon resource
set(FLAPPYBIRD_WIN_RC "${CMAKE_CURRENT_SOURCE_DIR}/resources/flappy.rc")
if(MSVC OR CMAKE_CROSSCOMPILING OR WIN32)
  if(EXISTS "${FLAPPYBIRD_ICON_ICO}")
    configure_file("${FLAPPYBIRD_ICON_ICO}" "${CMAKE_CURRENT_BINARY_DIR}/resources/favicon.ico" COPYONLY)
    set(FLAPPYBIRD_RC_SOURCES "${FLAPPYBIRD_WIN_RC}")
  else()
    message(WARNING "favicon.ico not found; Windows executable will not carry a custom icon")
  endif()
endif()

# ---------------------------------------------------------------- executable
add_executable(flappybird
  src/main.cpp
  src/config.cpp
  src/assets.cpp
  src/icon.cpp
  src/storage.cpp
  src/game.cpp
  src/ui.cpp
  src/fx.cpp
  src/daynight.cpp
  src/view.cpp
  ${FLAPPYBIRD_RC_SOURCES}
)

target_include_directories(flappybird PRIVATE src ${RAYLIB_SRC_DIR}/src)
target_link_libraries(flappybird PRIVATE raylib)

# Platform libraries
if(UNIX AND NOT APPLE)
  find_package(Threads REQUIRED)
  target_link_libraries(flappybird PRIVATE Threads::Threads dl m)
endif()

# Windows: mark as GUI application (no console popup) and embed the icon via .rc
if(MSVC)
  target_compile_options(flappybird PRIVATE /W4 /WX- /EHsc /permissive-)
  set_target_properties(flappybird PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(MINGW OR CMAKE_COMPILER_IS_GNUCXX AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_compile_options(flappybird PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
  set_target_properties(flappybird PROPERTIES WIN32_EXECUTABLE TRUE)
else()
  target_compile_options(flappybird PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
endif()

target_compile_definitions(flappybird PRIVATE
  FLAPPYBIRD_ASSETS_DIR="${FLAPPYBIRD_ASSETS_DIR}"
)

# ---------------------------------------------------------------- copy assets next to the binary
# So the game can run straight out of the build directory on every platform.
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
