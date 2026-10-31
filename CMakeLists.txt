cmake_minimum_required(VERSION 3.22)

project(flappybird LANGUAGES C CXX VERSION 1.0.0)

set(FLAPPYBIRD_ASSETS_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    CACHE PATH "Directory containing game assets"
)

set(FLAPPYBIRD_ICON_ICO
    "${FLAPPYBIRD_ASSETS_DIR}/favicon.ico"
    CACHE FILEPATH "Icon embedded into the Windows executable"
)

# ---------------------------------------------------------------- build type

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)

    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel
    )
endif()

# ---------------------------------------------------------------- output

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

foreach(_cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg}
        "${CMAKE_BINARY_DIR}/bin"
    )
endforeach()

# ---------------------------------------------------------------- MSVC runtime

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

# ---------------------------------------------------------------- standards

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------- raylib

set(FLAPPYBIRD_RAYLIB_VERSION "6.2" CACHE STRING "raylib release to fetch")

set(FLAPPYBIRD_RAYLIB_SOURCE_DIR ""
    CACHE PATH "Optional local raylib source checkout"
)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

if(FLAPPYBIRD_RAYLIB_SOURCE_DIR)

    if(NOT EXISTS "${FLAPPYBIRD_RAYLIB_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "raylib source not found at ${FLAPPYBIRD_RAYLIB_SOURCE_DIR}"
        )
    endif()

    set(FLAPPYBIRD_RAYLIB_SRC "${FLAPPYBIRD_RAYLIB_SOURCE_DIR}")

    add_subdirectory(
        "${FLAPPYBIRD_RAYLIB_SRC}"
        "${CMAKE_CURRENT_BINARY_DIR}/raylib"
        EXCLUDE_FROM_ALL
    )

else()

    include(FetchContent)

    set(FETCHCONTENT_BASE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/.cache"
        CACHE PATH "Persistent FetchContent cache"
    )

    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG "${FLAPPYBIRD_RAYLIB_VERSION}"
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )

    FetchContent_MakeAvailable(raylib)

    set(FLAPPYBIRD_RAYLIB_SRC "${raylib_SOURCE_DIR}")

endif()

# ---------------------------------------------------------------- Windows resources

set(FLAPPYBIRD_RC_SOURCES "")

if(MSVC OR MINGW OR WIN32)

    if(EXISTS "${FLAPPYBIRD_ICON_ICO}")

        file(TO_NATIVE_PATH
            "${FLAPPYBIRD_ICON_ICO}"
            FLAPPYBIRD_ICON_WIN_PATH
        )

        set(FLAPPYBIRD_RC_IN
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/flappy.rc.in"
        )

        set(FLAPPYBIRD_RC
            "${CMAKE_CURRENT_BINARY_DIR}/resources/flappy.rc"
        )

        file(MAKE_DIRECTORY
            "${CMAKE_CURRENT_BINARY_DIR}/resources"
        )

        configure_file(
            "${FLAPPYBIRD_RC_IN}"
            "${FLAPPYBIRD_RC}"
            @ONLY
        )

        set(FLAPPYBIRD_RC_SOURCES "${FLAPPYBIRD_RC}")

    else()

        message(WARNING
            "favicon.ico not found; Windows executable will not carry a custom icon"
        )

    endif()

endif()

# ---------------------------------------------------------------- platform sources

if(WIN32)

    set(FLAPPYBIRD_PLATFORM_SOURCES
        src/platform/windows/platform_windows.cpp
    )

elseif(APPLE)

    set(FLAPPYBIRD_PLATFORM_SOURCES
        src/platform/macos/platform_macos.cpp
    )

else()

    set(FLAPPYBIRD_PLATFORM_SOURCES
        src/platform/linux/platform_linux.cpp
    )

endif()

# ---------------------------------------------------------------- embedded assets

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(FLAPPYBIRD_GENERATED_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

set(FLAPPYBIRD_EMBEDDED_CPP
    "${FLAPPYBIRD_GENERATED_DIR}/embedded_assets.cpp"
)

set(FLAPPYBIRD_EMBEDDED_HPP
    "${FLAPPYBIRD_GENERATED_DIR}/embedded_assets.hpp"
)

file(GLOB_RECURSE FLAPPYBIRD_ASSET_FILES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${FLAPPYBIRD_ASSETS_DIR}/*"
)

add_custom_command(
    OUTPUT
        "${FLAPPYBIRD_EMBEDDED_CPP}"
        "${FLAPPYBIRD_EMBEDDED_HPP}"

    COMMAND
        ${CMAKE_COMMAND} -E make_directory
        "${FLAPPYBIRD_GENERATED_DIR}"

    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_assets.py"
        "${FLAPPYBIRD_ASSETS_DIR}"
        "${FLAPPYBIRD_EMBEDDED_CPP}"
        "${FLAPPYBIRD_EMBEDDED_HPP}"

    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_assets.py"
        ${FLAPPYBIRD_ASSET_FILES}

    COMMENT "Embedding Flappy Bird assets"

    VERBATIM
)

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

    ${FLAPPYBIRD_EMBEDDED_CPP}
    ${FLAPPYBIRD_EMBEDDED_HPP}
)

target_include_directories(flappybird PRIVATE
    src
    "${FLAPPYBIRD_RAYLIB_SRC}/src"
    "${FLAPPYBIRD_GENERATED_DIR}"
)

target_link_libraries(flappybird PRIVATE raylib)

# ---------------------------------------------------------------- platform libraries

if(UNIX AND NOT APPLE)

    find_package(Threads REQUIRED)

    target_link_libraries(flappybird PRIVATE
        Threads::Threads
        dl
        m
    )

endif()

# ---------------------------------------------------------------- platform settings

if(MSVC)

    target_compile_options(flappybird PRIVATE
        /W4
        /WX-
        /EHsc
        /permissive-
    )

    target_link_options(flappybird PRIVATE
        /ENTRY:mainCRTStartup
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )

    set_target_properties(flappybird PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

elseif(MINGW)

    target_compile_options(flappybird PRIVATE
        -Wall
        -Wextra
        -Wno-missing-field-initializers
    )

    target_link_options(flappybird PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )

    set_target_properties(flappybird PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

else()

    target_compile_options(flappybird PRIVATE
        -Wall
        -Wextra
        -Wno-missing-field-initializers
        -ffunction-sections
        -fdata-sections
    )

    if(APPLE)

        target_link_options(flappybird PRIVATE
            -Wl,-dead_strip
        )

    else()

        target_link_options(flappybird PRIVATE
            -Wl,--gc-sections
        )

        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            target_link_options(flappybird PRIVATE -s)
        endif()

    endif()

endif()

# ---------------------------------------------------------------- link-time optimisation

if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_CONFIGURATION_TYPES)

    include(CheckIPOSupported)

    check_ipo_supported(
        RESULT FLAPPYBIRD_IPO_OK
        OUTPUT FLAPPYBIRD_IPO_LOG
        LANGUAGES CXX
    )

    if(FLAPPYBIRD_IPO_OK)

        set_property(TARGET flappybird PROPERTY
            INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
        )

    endif()

endif()

# ---------------------------------------------------------------- development fallback

# This is only used if an asset is missing from the embedded asset table.
# It does NOT require an external assets folder for a normal packaged build.

file(TO_CMAKE_PATH
    "${FLAPPYBIRD_ASSETS_DIR}"
    FLAPPYBIRD_ASSETS_DIR_CMAKE
)

target_compile_definitions(flappybird PRIVATE
    "FLAPPYBIRD_ASSETS_DIR=\"${FLAPPYBIRD_ASSETS_DIR_CMAKE}\""
)

# ---------------------------------------------------------------- installation

install(TARGETS flappybird
    RUNTIME DESTINATION bin
)