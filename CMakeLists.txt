cmake_minimum_required(VERSION 3.16)
project(pocket35_sim CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(DEPS_DIR ${PROJECT_ROOT}/third_party)

if(NOT EXISTS ${DEPS_DIR}/LovyanGFX OR NOT EXISTS ${DEPS_DIR}/lvgl OR NOT EXISTS ${DEPS_DIR}/Mooncake)
    message(FATAL_ERROR "缺少第三方依赖，请先运行: bash sim/setup_deps.sh")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

add_compile_options(-O2 -Wno-unused-parameter -Wno-sign-compare -Wno-missing-field-initializers)
add_definitions(-DLGFX_SDL -DLV_CONF_INCLUDE_SIMPLE)

# ---------------------------------------------------------------- LovyanGFX (SDL)
file(GLOB LGFX_SRCS
    ${DEPS_DIR}/LovyanGFX/src/lgfx/v1/*.cpp
    ${DEPS_DIR}/LovyanGFX/src/lgfx/v1/panel/*.cpp
    ${DEPS_DIR}/LovyanGFX/src/lgfx/v1/touch/*.cpp
    ${DEPS_DIR}/LovyanGFX/src/lgfx/v1/platforms/sdl/*.cpp
    ${DEPS_DIR}/LovyanGFX/src/lgfx/v1/lv_font/*.c
    ${DEPS_DIR}/LovyanGFX/src/lgfx/utility/*.c
    ${DEPS_DIR}/LovyanGFX/src/lgfx/Fonts/efont/*.c
    ${DEPS_DIR}/LovyanGFX/src/lgfx/Fonts/IPA/*.c
    ${DEPS_DIR}/LovyanGFX/src/lgfx/Fonts/lvgl/*.c
)
add_library(lovyangfx STATIC ${LGFX_SRCS} ${PROJECT_ROOT}/sim/sdl_compat.cpp)
target_include_directories(lovyangfx PUBLIC ${DEPS_DIR}/LovyanGFX/src)
target_compile_definitions(lovyangfx PUBLIC SDL_KeyCode=SDL_Keycode)
target_compile_options(lovyangfx PUBLIC
    $<$<COMPILE_LANGUAGE:CXX>:-include${PROJECT_ROOT}/sim/sdl_compat.h>)
target_link_libraries(lovyangfx PUBLIC PkgConfig::SDL2 pthread)

# ---------------------------------------------------------------- LVGL
set(LV_BUILD_CONF_DIR ${PROJECT_ROOT}/include CACHE PATH "" FORCE)
add_subdirectory(${DEPS_DIR}/lvgl ${CMAKE_BINARY_DIR}/lvgl)

# ---------------------------------------------------------------- Mooncake
set(MOONCAKE_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory(${DEPS_DIR}/Mooncake ${CMAKE_BINARY_DIR}/mooncake)

# ---------------------------------------------------------------- smooth_ui_toolkit
file(GLOB_RECURSE SMOOTH_UI_SRCS ${PROJECT_ROOT}/lib/smooth_ui_toolkit/src/*.cpp)
add_library(smooth_ui_toolkit STATIC ${SMOOTH_UI_SRCS})
target_include_directories(smooth_ui_toolkit PUBLIC ${PROJECT_ROOT}/lib/smooth_ui_toolkit/src ${PROJECT_ROOT}/sim/compat)
target_link_libraries(smooth_ui_toolkit PUBLIC lvgl)

# ---------------------------------------------------------------- smooth_lvgl
file(GLOB_RECURSE SMOOTH_LVGL_SRCS ${PROJECT_ROOT}/lib/smooth_lvgl/src/*.cpp)
add_library(smooth_lvgl STATIC ${SMOOTH_LVGL_SRCS})
target_include_directories(smooth_lvgl PUBLIC ${PROJECT_ROOT}/lib/smooth_lvgl/src ${PROJECT_ROOT}/sim/compat)
target_link_libraries(smooth_lvgl PUBLIC lvgl)

# ---------------------------------------------------------------- app
file(GLOB_RECURSE APP_SRCS ${PROJECT_ROOT}/src/*.cpp ${PROJECT_ROOT}/src/*.c)
list(FILTER APP_SRCS EXCLUDE REGEX "/hal_board/")

add_executable(pocket35_sim ${APP_SRCS} ${PROJECT_ROOT}/sim/sdl_main.cpp)

target_compile_options(pocket35_sim PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-include${PROJECT_ROOT}/sim/compat/Arduino.h>
    -Wno-write-strings
)

target_include_directories(pocket35_sim PRIVATE
    ${PROJECT_ROOT}/sim/compat
    ${PROJECT_ROOT}/sim
    ${PROJECT_ROOT}/src
    ${PROJECT_ROOT}/src/pocket35
    ${PROJECT_ROOT}/include
    ${PROJECT_ROOT}/lib/smooth_ui_toolkit/src
    ${PROJECT_ROOT}/lib/smooth_lvgl/src
)

target_link_libraries(pocket35_sim PRIVATE
    lovyangfx
    lvgl
    mooncake
    smooth_ui_toolkit
    smooth_lvgl
    PkgConfig::SDL2
    pthread
)
