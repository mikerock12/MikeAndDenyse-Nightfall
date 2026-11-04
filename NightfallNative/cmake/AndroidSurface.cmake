# Keep upstream raylib 5.5 immutable; compile a narrowly patched rcore copy.
# Android may publish an initial 1x1 NativeWindow. Its buffer geometry must be
# set before raylib caches display dimensions and computes screenScale.
function(slt_android_buffer_geometry raylib_source)
    if(NOT ANDROID)
        return()
    endif()
    set(_slt_dir "${CMAKE_CURRENT_BINARY_DIR}/slt-raylib-platform")
    file(READ "${raylib_source}/src/platforms/rcore_android.c" _slt_backend)
    string(REPLACE "\r\n" "\n" _slt_backend "${_slt_backend}")
    string(SHA256 _slt_hash "${_slt_backend}")
    if(NOT _slt_hash STREQUAL "a17a8c75ffa9710dec042769191f07d0352af7de0c197adbcfa883d78d2234d8")
        message(FATAL_ERROR "Unexpected raylib Android backend; review the pinned 5.5 geometry patch")
    endif()
    set(_slt_needle "                    CORE.Window.display.width = ANativeWindow_getWidth(platform.app->window);")
    string(FIND "${_slt_backend}" "${_slt_needle}" _slt_position)
    if(_slt_position LESS 0)
        message(FATAL_ERROR "Pinned Android initialization site missing")
    endif()
    set(_slt_patch "                    // SLT alteration: use the physical size supplied by the Activity before querying buffers.
                    if ((CORE.Window.screen.width > 1) && (CORE.Window.screen.height > 1))
                    {
                        int geometry = ANativeWindow_setBuffersGeometry(app->window, CORE.Window.screen.width, CORE.Window.screen.height, 0);
                        TRACELOG(LOG_INFO, \"ANDROID: SLT_BUFFER_GEOMETRY: %i x %i result=%i\", CORE.Window.screen.width, CORE.Window.screen.height, geometry);
                    }
${_slt_needle}")
    string(REPLACE "${_slt_needle}" "${_slt_patch}" _slt_backend "${_slt_backend}")
    file(MAKE_DIRECTORY "${_slt_dir}/platforms")
    configure_file("${raylib_source}/src/rcore.c" "${_slt_dir}/rcore.c" COPYONLY)
    file(CONFIGURE OUTPUT "${_slt_dir}/platforms/rcore_android.c" CONTENT "${_slt_backend}" @ONLY NEWLINE_STYLE UNIX)
    get_target_property(_slt_sources raylib SOURCES)
    list(FILTER _slt_sources EXCLUDE REGEX "(^|[/\\\\])rcore[.]c$")
    set_property(TARGET raylib PROPERTY SOURCES "${_slt_sources}")
    target_sources(raylib PRIVATE "${_slt_dir}/rcore.c")
    target_include_directories(raylib PRIVATE "${raylib_source}/src")
    message(STATUS "SLT physical Android buffer geometry fix enabled (pinned raylib 5.5)")
endfunction()
