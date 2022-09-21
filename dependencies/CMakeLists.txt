include(FetchContent)
include(ExternalProject)

# set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
FetchContent_Declare(GSL
    GIT_REPOSITORY "https://github.com/microsoft/GSL"
    GIT_TAG "v3.1.0"
)

FetchContent_Declare(DirectXMath
    GIT_REPOSITORY "https://github.com/microsoft/DirectXMath"
    GIT_TAG "may2022"
)

FetchContent_Declare(DirectXTK
    GIT_REPOSITORY "https://github.com/microsoft/DirectXTK"
    GIT_TAG "jul2022"
)

FetchContent_Declare(FMath
    GIT_REPOSITORY "https://github.com/BrennenTaylor/FMath"
)

FetchContent_Declare(tinyobjloader
    GIT_REPOSITORY "https://github.com/tinyobjloader/tinyobjloader"
    GIT_TAG "v2.0.0rc9"
)

FetchContent_Declare(D3D11_SMAA
    GIT_REPOSITORY "https://github.com/BrennenTaylor/D3D11_SMAA"
)

# Fetch Bullet3
FetchContent_Declare(
    bullet
    GIT_REPOSITORY "https://github.com/bulletphysics/bullet3"
    GIT_TAG "3.24"
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_Declare(Imath
    GIT_REPOSITORY "https://github.com/AcademySoftwareFoundation/Imath"
    GIT_TAG "main"
)

FetchContent_Declare(openexr
    GIT_REPOSITORY "https://github.com/AcademySoftwareFoundation/openexr"
    GIT_TAG "main"
)

FetchContent_Declare(spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog"
    GIT_TAG "v1.10.0"
)

FetchContent_Declare(xxHash
    GIT_REPOSITORY "https://github.com/Cyan4973/xxHash"
    GIT_TAG "v0.8.1"
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/xxHash/
)

FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/stb/
)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/BrennenTaylor/imgui
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/imgui/
)

FetchContent_MakeAvailable(GSL)
FetchContent_MakeAvailable(DirectXMath)

set(BUILD_XAUDIO_WIN10 ON CACHE BOOL "ON" FORCE)
set(BUILD_XAUDIO_WIN8 OFF CACHE BOOL "" FORCE)
set(BUILD_XAUDIO_WIN7 OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(DirectXTK)

FetchContent_MakeAvailable(FMath)

FetchContent_MakeAvailable(tinyobjloader)

FetchContent_MakeAvailable(D3D11_SMAA)

FetchContent_GetProperties(bullet)

if(NOT bullet_POPULATED)
    FetchContent_Populate(bullet)
    add_subdirectory(${bullet_SOURCE_DIR} ${bullet_BINARY_DIR})
    target_include_directories(Bullet3Collision PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
    target_include_directories(Bullet3Common PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
    target_include_directories(Bullet3Dynamics PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
    target_include_directories(Bullet3Geometry PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
    target_include_directories(Bullet3OpenCL_clew PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
    target_include_directories(Bullet2FileLoader PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
    target_include_directories(BulletCollision PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
    target_include_directories(BulletDynamics PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
    target_include_directories(BulletInverseDynamics PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
    target_include_directories(BulletSoftBody PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
    target_include_directories(LinearMath PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
endif()

set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_PYBULLET OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)

# set(XXHASH_BUILD_ENABLE_INLINE_API OFF) # optional
# set(XXHASH_BUILD_XXHSUM OFF) # optional
# add_subdirectory(xxHash/cmake_unofficial EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(tinyobjloader)

FetchContent_MakeAvailable(Imath)

FetchContent_MakeAvailable(openexr)

FetchContent_MakeAvailable(spdlog)

set(XXHASH_BUILD_ENABLE_INLINE_API OFF) # optional
set(XXHASH_BUILD_XXHSUM OFF) # optional
FetchContent_MakeAvailable(xxHash)
add_subdirectory(xxHash/cmake_unofficial EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(stb)
FetchContent_MakeAvailable(imgui)
