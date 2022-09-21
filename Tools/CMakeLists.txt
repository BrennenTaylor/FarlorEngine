add_executable(ExrMipGenerator
    ExrMipGenerator.cpp
)

target_link_libraries(ExrMipGenerator
    PUBLIC FMath::FMath
    PUBLIC spdlog::spdlog_header_only
    PUBLIC tinyobjloader
    PUBLIC AGZSMAA
    PUBLIC d3d11
    PUBLIC dxgi
    PUBLIC winmm
    PUBLIC d3dcompiler
    PUBLIC xinput
    PUBLIC Ws2_32
    PUBLIC dxguid
    PUBLIC DirectXTK
    PRIVATE xxHash::xxhash
    PUBLIC OpenEXR::OpenEXR
)

target_include_directories(ExrMipGenerator
    PUBLIC imgui/
)

target_compile_definitions(ExrMipGenerator
    PUBLIC _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
)