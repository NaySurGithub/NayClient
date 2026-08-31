cmake_minimum_required(VERSION 3.20)
project(NayClient VERSION 0.1.0 LANGUAGES C)

file(GLOB_RECURSE NAY_MODULE_SOURCES CONFIGURE_DEPENDS src/modules/*.c)
file(GLOB_RECURSE NAY_LAUNCHER_SOURCES CONFIGURE_DEPENDS src/launcher/*.c)
file(GLOB NAY_PLATFORM_SOURCES CONFIGURE_DEPENDS src/platform/*.c)
file(GLOB NAY_NETWORK_SOURCES CONFIGURE_DEPENDS src/network/*.c)

add_library(nayclient SHARED
    src/client.c
    ${NAY_MODULE_SOURCES}
    ${NAY_NETWORK_SOURCES}
    ${NAY_PLATFORM_SOURCES}
)

target_include_directories(nayclient PRIVATE include src)
target_compile_features(nayclient PRIVATE c_std_11)
target_compile_definitions(nayclient PRIVATE NAYCLIENT_BUILD)
target_link_libraries(nayclient PRIVATE user32)

if(MSVC)
    target_compile_options(nayclient PRIVATE /W4 /permissive-)
else()
    target_compile_options(nayclient PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(naylauncher ${NAY_LAUNCHER_SOURCES} src/platform/minecraft_version.c)
target_include_directories(naylauncher PRIVATE include src)
target_compile_features(naylauncher PRIVATE c_std_11)
target_link_libraries(naylauncher PRIVATE user32)

if(MSVC)
    target_compile_options(naylauncher PRIVATE /W4 /permissive-)
else()
    target_compile_options(naylauncher PRIVATE -Wall -Wextra -Wpedantic)
endif()
