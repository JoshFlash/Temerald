cmake_minimum_required(VERSION 3.25)
project(temerald LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(temerald)

# Per-folder sources — add files inside each subdirectory's CMakeLists.txt
add_subdirectory(src/app)
add_subdirectory(src/engine)
add_subdirectory(src/render)
add_subdirectory(src/scene)
add_subdirectory(src/world)
add_subdirectory(src/util)

target_include_directories(temerald PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/external"
)

# Windows / DirectX system libraries
target_link_libraries(temerald PRIVATE
    d3d12
    dxgi
    dxguid
    d3dcompiler
)

# Unicode Win32 API
target_compile_definitions(temerald PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)
