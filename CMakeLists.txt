cmake_minimum_required(VERSION 3.13)
set(CMAKE_TOOLCHAIN_FILE "/usr/local/dev/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

include("${CMAKE_CURRENT_SOURCE_DIR}/extern/core/manifest.cmake")

string(TOLOWER ${cross_target} project_name)
project(${project_name} VERSION ${cross_version})

add_compile_definitions(PROJECT_NAME="${CMAKE_PROJECT_NAME}")
add_compile_definitions(PROJECT_VERSION="${CMAKE_PROJECT_VERSION}")
add_compile_definitions(APPLICATION_ID="${cross_identifier}")

add_executable("${CMAKE_PROJECT_NAME}" "bridge.cpp")
target_include_directories("${CMAKE_PROJECT_NAME}" PRIVATE "extern/core/src")
set_target_properties("${CMAKE_PROJECT_NAME}" PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "www"
    LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ui.js"
)
target_compile_options("${CMAKE_PROJECT_NAME}" PRIVATE
    -sDISABLE_EXCEPTION_CATCHING=0
    -sUSE_PTHREADS=1
)
target_link_options("${CMAKE_PROJECT_NAME}" PRIVATE
    --pre-js "${CMAKE_CURRENT_SOURCE_DIR}/ui.js"
    -sEXPORTED_FUNCTIONS=[_main,_PostJsMessage,_NeedStart,_NeedStop,_NeedEscape,_NeedExit,_PickFunction,_PopFunction,_malloc]
    -sEXPORTED_RUNTIME_METHODS=['ccall','stringToUTF8']
    -sPROXY_TO_PTHREAD=1
    -sEXIT_RUNTIME=1
    -sALLOW_MEMORY_GROWTH=1
    -sDISABLE_EXCEPTION_CATCHING=0
    -sUSE_PTHREADS=1
)
set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 17)
file(GLOB core_sources "extern/core/src/*.cpp")
target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${core_sources})
file(GLOB main_sources "../../src/*.cpp")
target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${main_sources})

configure_file("index.html" "www/index.html")
configure_file("index.css" "www/index.css")

set(RESOLUTIONS 16 32 48 64)
foreach(RESOLUTION ${RESOLUTIONS})
    add_custom_target("${RESOLUTION}-png" ALL DEPENDS "${RESOLUTION}.png")
    add_custom_command(OUTPUT "${RESOLUTION}.png"
        COMMAND inkscape -o "${RESOLUTION}.png" -w ${RESOLUTION} -h ${RESOLUTION} "${CMAKE_CURRENT_SOURCE_DIR}/../../icon.svg"
        MAIN_DEPENDENCY "../../icon.svg"
    )
    list(APPEND png_targets "${RESOLUTION}-png")
    list(APPEND png_files "${RESOLUTION}.png")
endforeach()
add_custom_target("favicon_icon" ALL DEPENDS "www/favicon.ico")
add_custom_command(OUTPUT "www/favicon.ico"
    COMMAND convert ${png_files} "${CMAKE_CURRENT_BINARY_DIR}/www/favicon.ico"
    DEPENDS ${png_targets}
)

add_custom_target("assets" ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../assets" "${CMAKE_CURRENT_BINARY_DIR}/www/assets"
)

add_custom_target("version" ALL
    COMMAND ${CMAKE_COMMAND} -E echo ${CMAKE_PROJECT_VERSION} > "${CMAKE_CURRENT_BINARY_DIR}/www/version.txt"
)
