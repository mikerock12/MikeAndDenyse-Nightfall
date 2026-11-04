cmake_minimum_required(VERSION 3.22)
project(NightfallNative VERSION 1.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # Distributables run without a separately installed Visual C++ redistributable.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
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
    target_link_libraries(NightfallCore PUBLIC ws2_32 iphlpapi)
endif()

set(RAYLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/raylib")
if(ANDROID)
    if(NOT RAYLIB_SOURCE_DIR)
        message(FATAL_ERROR "Set RAYLIB_SOURCE_DIR to the pinned raylib 5.5 source tree")
    endif()
    set(PLATFORM Android CACHE STRING "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(WITH_PIC ON CACHE BOOL "" FORCE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set_target_properties(NightfallCore PROPERTIES POSITION_INDEPENDENT_CODE ON)
    add_subdirectory("${RAYLIB_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/raylib")
    add_library(nightfall SHARED src/main.cpp)
    target_compile_definitions(nightfall PRIVATE PLATFORM_ANDROID)
    target_include_directories(nightfall PRIVATE "${ANDROID_NDK}/sources/android/native_app_glue")
    target_link_libraries(nightfall PRIVATE NightfallCore raylib)
    target_link_options(nightfall PRIVATE "-u" "ANativeActivity_onCreate" "-Wl,-z,max-page-size=16384")
endif()
if(WIN32 AND EXISTS "${RAYLIB_ROOT}/lib/raylibdll.lib")
    add_executable(NightfallNative WIN32
        src/main.cpp
    )
    enable_language(RC)
    set(SLT_BRANDING_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Platform/Windows/Branding")
    set(SLT_PRODUCT_NAME "Mike & Denyse: Nightfall")
    set(SLT_VERSION "${PROJECT_VERSION}")
    string(REPLACE "." "," SLT_VERSION_COMMAS "${PROJECT_VERSION}.0")
    configure_file(Platform/Windows/GameResources.rc.in "${CMAKE_CURRENT_BINARY_DIR}/GameResources.rc" @ONLY)
    target_sources(NightfallNative PRIVATE Platform/Windows/WindowsBranding.cpp "${CMAKE_CURRENT_BINARY_DIR}/GameResources.rc")
    target_include_directories(NightfallNative PRIVATE Platform/Windows)
    if(MSVC)
        target_link_options(NightfallNative PRIVATE /ENTRY:mainCRTStartup)
    endif()
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

if(NOT ANDROID)
enable_testing()
if(WIN32)
    add_executable(NightfallPreferencesTests tests/PreferencesTests.cpp)
    target_include_directories(NightfallPreferencesTests PRIVATE include "${RAYLIB_ROOT}/include")
    target_link_libraries(NightfallPreferencesTests PRIVATE "${RAYLIB_ROOT}/lib/raylibdll.lib")
    add_test(NAME NightfallPreferencesTests COMMAND NightfallPreferencesTests)
endif()
add_executable(NightfallParityTrace tests/NightfallParityTrace.cpp)
target_link_libraries(NightfallParityTrace PRIVATE NightfallCore)

add_executable(NightfallSmokeTests
    tests/NightfallSmokeTests.cpp
)
target_link_libraries(NightfallSmokeTests PRIVATE NightfallCore)

add_test(NAME NightfallSmoke77Checks COMMAND NightfallSmokeTests)
if(TARGET NightfallNative)
    add_test(NAME NightfallHeadlessSmoke COMMAND NightfallNative --headless-smoke)
endif()

endif()
