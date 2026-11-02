cmake_minimum_required(VERSION 3.22)
project(NightfallNative VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8 /W3 /EHsc)
endif()

add_library(NightfallCore STATIC
    src/core/NightfallCore.cpp
    src/network/NightfallNet.cpp
)
target_include_directories(NightfallCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
if(WIN32)
    target_link_libraries(NightfallCore PUBLIC ws2_32)
endif()

set(RAYLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/raylib")
if(WIN32 AND EXISTS "${RAYLIB_ROOT}/lib/raylibdll.lib")
    add_executable(NightfallNative
        src/main.cpp
    )
    target_include_directories(NightfallNative PRIVATE "${RAYLIB_ROOT}/include")
    target_link_libraries(NightfallNative PRIVATE
        NightfallCore
        "${RAYLIB_ROOT}/lib/raylibdll.lib"
        winmm gdi32 opengl32 user32 shell32
    )
    add_custom_command(TARGET NightfallNative POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RAYLIB_ROOT}/lib/raylib.dll"
            "$<TARGET_FILE_DIR:NightfallNative>/raylib.dll"
    )
endif()

enable_testing()

add_executable(NightfallSmokeTests
    tests/NightfallSmokeTests.cpp
)
target_link_libraries(NightfallSmokeTests PRIVATE NightfallCore)

add_test(NAME NightfallSmoke77Checks COMMAND NightfallSmokeTests)
if(TARGET NightfallNative)
    add_test(NAME NightfallHeadlessSmoke COMMAND NightfallNative --headless-smoke)
endif()
