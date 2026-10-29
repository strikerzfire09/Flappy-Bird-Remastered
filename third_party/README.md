## Third-party libraries

raylib is not vendored in this repository. CMake downloads raylib 6.2 from the
official GitHub repository on the first configure.

For offline builds, clone raylib here (as `third_party/raylib`) or configure
with `-DFLAPPYBIRD_RAYLIB_SOURCE_DIR=/path/to/raylib`. Local checkouts are
ignored by Git.

