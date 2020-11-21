cmake_minimum_required(VERSION 3.13)
set(CMAKE_TOOLCHAIN_FILE "~/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../../manifest.txt" manifest)
foreach(key_value ${manifest})
    string(REGEX MATCH "^[^=]+" key ${key_value})
    string(REPLACE "${key}=" "" value ${key_value})
    set(${key} "${value}")
endforeach()

string(TOLOWER ${cross_target} project_name)
project(${project_name} VERSION ${cross_version}.${cross_release})

add_executable("${CMAKE_PROJECT_NAME}" "bridge.cpp")
set_target_properties("${CMAKE_PROJECT_NAME}" PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "www"
)
target_compile_options("${CMAKE_PROJECT_NAME}" PRIVATE
    -msse -msse2 -msse3 -msimd128
    -sALLOW_MEMORY_GROWTH=1
    -sDISABLE_EXCEPTION_CATCHING=0
    -sUSE_PTHREADS=1
)
target_link_options("${CMAKE_PROJECT_NAME}" PRIVATE
    -sEXPORTED_FUNCTIONS=[_main,_PickFunction,_PopFunction,_PostJsMessage,_CreatePixels,_LockPixels,_UnlockPixels,_Escape,_Handle]
    -sEXTRA_EXPORTED_RUNTIME_METHODS=['ccall']
    -sPROXY_TO_PTHREAD=1
    -sEXIT_RUNTIME=1
    -sALLOW_MEMORY_GROWTH=1
    -sDISABLE_EXCEPTION_CATCHING=0
    -sUSE_PTHREADS=1
)
set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 17)
file(GLOB core_sources "../core/src/*.cpp")
target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${core_sources})
file(GLOB main_sources "../../src/*.cpp")
target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${main_sources})

configure_file("index.html" "www/index.html")
configure_file("index.js" "www/index.js")
configure_file("index.css" "www/index.css")

set(RESOLUTIONS 16 32 48 64)
foreach(RESOLUTION ${RESOLUTIONS})
    add_custom_target("${RESOLUTION}-png" ALL DEPENDS "${RESOLUTION}.png")
    add_custom_command(OUTPUT "${RESOLUTION}.png"
        COMMAND inkscape -e "${RESOLUTION}.png" -w ${RESOLUTION} -h ${RESOLUTION} "${CMAKE_CURRENT_SOURCE_DIR}/../../icon.svg"
        MAIN_DEPENDENCY "../../icon.svg"
    )
    list(APPEND png_targets "${RESOLUTION}-png")
    list(APPEND png_files "${RESOLUTION}.png")
endforeach()
add_custom_target("favicon_icon" ALL DEPENDS "www/favicon.ico")
add_custom_command(OUTPUT "www/favicon.ico"
    COMMAND convert ${png_files} "www/favicon.ico"
    DEPENDS ${png_targets}
)

add_custom_target("assets" ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/../../assets" "www/assets")