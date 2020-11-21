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
target_compile_options("${CMAKE_PROJECT_NAME}" PRIVATE
    -msse -msse2 -msse3 -msimd128
    -sDISABLE_EXCEPTION_CATCHING=0
)
target_link_options("${CMAKE_PROJECT_NAME}" PRIVATE
    -sEXPORTED_FUNCTIONS=[_Begin,_End,_Create,_Destroy,_Start,_Stop,_Restart,_Escape,_Handle,_HandleAsync]
    -sEXTRA_EXPORTED_RUNTIME_METHODS=['ccall']
    -sDISABLE_EXCEPTION_CATCHING=0
)
set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 17)
file(GLOB core_sources "../core/src/*.cpp")
target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${core_sources})
file(GLOB main_sources "../../src/*.cpp")
target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${main_sources})

configure_file("index.html" "index.html")
configure_file("index.js" "index.js")
configure_file("index.css" "index.css")
