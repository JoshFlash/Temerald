cmake_minimum_required(VERSION 3.25)
project(temerald LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ── External dependencies (header-only, fetched at configure time) ────────────
include(FetchContent)

FetchContent_Declare(DirectXMath
    GIT_REPOSITORY https://github.com/microsoft/DirectXMath.git
    GIT_TAG        oct2024
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(DirectXHeaders
    GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
    GIT_TAG        v1.619.1
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(DirectXMath DirectXHeaders)

# ── Target ────────────────────────────────────────────────────────────────────
add_executable(temerald WIN32)

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

target_link_libraries(temerald PRIVATE
    d3d12
    dxgi
    dxguid
    d3dcompiler
    DirectXMath
    Microsoft::DirectX-Headers
)

target_compile_definitions(temerald PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

# ── DXC post-build copy ───────────────────────────────────────────────────────
# Place dxcompiler.dll and dxil.dll in external/dxc/ (download from
# https://github.com/microsoft/DirectXShaderCompiler/releases)
set(DXC_DIR "${CMAKE_SOURCE_DIR}/external/dxc")

foreach(DXC_DLL dxcompiler.dll dxil.dll)
    if(EXISTS "${DXC_DIR}/${DXC_DLL}")
        add_custom_command(TARGET temerald POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DXC_DIR}/${DXC_DLL}"
                "$<TARGET_FILE_DIR:temerald>/${DXC_DLL}"
            COMMENT "Copying ${DXC_DLL}"
        )
    else()
        message(WARNING
            "DXC binary not found: ${DXC_DIR}/${DXC_DLL}\n"
            "Download from https://github.com/microsoft/DirectXShaderCompiler/releases "
            "and place dxcompiler.dll + dxil.dll in external/dxc/")
    endif()
endforeach()
