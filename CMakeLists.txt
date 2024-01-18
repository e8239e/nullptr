cmake_minimum_required(VERSION 3.19)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release})
    add_compile_options(-O3)
endif()

add_compile_options(-Wall -pedantic)

project(nullptr LANGUAGES CXX)

set(NULLPTR_MAJOR 0)
set(NULLPTR_MINOR 1)
set(NULLPTR_PATCH 0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(FONTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/fonts)
set(MAIN_FONT_ARCHIVE ${FONTS_DIR}/Iosevka.tar.xz)
set(MAIN_FONT_VER 3.1.1)
set(MAIN_FONT_ARCHIVE_URL https://github.com/ryanoasis/nerd-fonts/releases/download/v${MAIN_FONT_VER}/Iosevka.tar.xz)
set(MAIN_FONT_ARCHIVE_SHA512 faf27db7c24538c08dad36bea3d71a7925e8f7a4a78c965fce421b8554c2f8bf6352361c096c6cfd9f587e9455855d3cf17f51fc8084d4ed5d6bec050167cd53)

set(CJK_FONT_ARCHIVE ${FONTS_DIR}/wqy-microhei-0.2.0-beta.tar.gz)
set(CJK_FONT_ARCHIVE_URL https://downloads.sourceforge.net/project/wqy/wqy-microhei/0.2.0-beta/wqy-microhei-0.2.0-beta.tar.gz)
set(CJK_FONT_ARCHIVE_SHA512 c1e09c83f6427f637b3c93c7f89e427bde7fb22cedcfbb806fb036f13a553551f28d985516c7ab4c2602b4159502de36b52e6cc9b41c9b6d6e36778a6de74037)
set(CJK_FONT_URL https://downloads.sourceforge.net/project/wqy/wqy-microhei/0.2.0-beta/wqy-microhei-0.2.0-beta.tar.gz)

macro(check_hash file hash_type hash)
    execute_process(
        COMMAND ${hash_type}sum ${file}
        COMMAND grep -q ${hash}
        RESULT_VARIABLE HASH_CHECK_RESULT
    )

    if (NOT (${HASH_CHECK_RESULT} STREQUAL "0"))
        message(FATAL_ERROR "Hash of ${file} is incorrect")
    endif()
endmacro(check_hash)

macro(dl_file_w_hash_check url path hash_type hash)
    message(STATUS "${path} download started")
    file(DOWNLOAD ${url} ${path} STATUS stat)
    list(GET stat 0 success)
    list(GET stat 1 error)
    if (${success})
        file(REMOVE ${path})
        message(FATAL_ERROR "${path}: download failed (${error})")
    else()
        message(STATUS "${path}: download succeeded")
    endif()
    check_hash(${path} ${hash_type} ${hash})
endmacro(dl_file_w_hash_check)

if (NOT EXISTS ${FONTS_DIR}/IosevkaNerdFontMono-Regular.h)
    if (NOT EXISTS ${FONTS_DIR}/binary_to_compressed_c)
        message(STATUS "${FONTS_DIR}/binary_to_compressed_c not found, compiling")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} ./third-party/imgui/misc/fonts/binary_to_compressed_c.cpp -O -o ${FONTS_DIR}/binary_to_compressed_c
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()

    if (NOT EXISTS ${FONTS_DIR}/IosevkaNerdFontMono-Regular.ttf)
        message(STATUS "${FONTS_DIR}/IosevkaNerdFontMono-Regular.ttf not found, extracting archive ${MAIN_FONT_ARCHIVE}")

        if (NOT EXISTS ${MAIN_FONT_ARCHIVE})
            message(STATUS "Main font archive not found, downloading")
            dl_file_w_hash_check(${MAIN_FONT_ARCHIVE_URL} ${MAIN_FONT_ARCHIVE} sha512 ${MAIN_FONT_ARCHIVE_SHA512})
        endif()

        check_hash(${MAIN_FONT_ARCHIVE} sha512 ${MAIN_FONT_ARCHIVE_SHA512})

        execute_process(
            COMMAND tar -xf ${MAIN_FONT_ARCHIVE} -C ${FONTS_DIR}
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()

    execute_process(
        COMMAND ${FONTS_DIR}/binary_to_compressed_c ${FONTS_DIR}/IosevkaNerdFontMono-Regular.ttf font_IosevkaNerdFontMono_Regular
        OUTPUT_FILE ${FONTS_DIR}/IosevkaNerdFontMono-Regular.h
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

if (NOT EXISTS ${FONTS_DIR}/wqy-microhei.h)
    if (NOT EXISTS ${FONTS_DIR}/binary_to_compressed_c)
        message(STATUS "${FONTS_DIR}/binary_to_compressed_c not found, compiling")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} ./third-party/imgui/misc/fonts/binary_to_compressed_c.cpp -O -o ${FONTS_DIR}/binary_to_compressed_c
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()

    if (NOT EXISTS ${FONTS_DIR}/wqy-microhei.ttf)
        message(STATUS "${FONTS_DIR}/wqy-microhei.ttf not found, extracting archive ${MAIN_FONT_ARCHIVE}")

        if (NOT EXISTS ${CJK_FONT_ARCHIVE})
            message(STATUS "CJK font archive not found, downloading")
            dl_file_w_hash_check(${CJK_FONT_ARCHIVE_URL} ${CJK_FONT_ARCHIVE} sha512 ${CJK_FONT_ARCHIVE_SHA512})
        endif()

        check_hash(${CJK_FONT_ARCHIVE} sha512 ${CJK_FONT_ARCHIVE_SHA512})

        execute_process(
            COMMAND tar -xf ${CJK_FONT_ARCHIVE} -C ${FONTS_DIR}
            COMMAND_ERROR_IS_FATAL ANY
        )

        execute_process(
            WORKING_DIRECTORY ${FONTS_DIR}
            COMMAND fontforge -script ttc2ttf.pe wqy-microhei/wqy-microhei.ttc
            OUTPUT_QUIET
            ERROR_QUIET
            COMMAND_ERROR_IS_FATAL ANY
        )

        execute_process(
            COMMAND mv ${FONTS_DIR}/WenQuanYiMicroHeiMono-02.ttf ${FONTS_DIR}/wqy-microhei.ttf
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()

    execute_process(
        COMMAND ${FONTS_DIR}/binary_to_compressed_c ${FONTS_DIR}/wqy-microhei.ttf font_wqy_microhei
        OUTPUT_FILE ${FONTS_DIR}/wqy-microhei.h
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

include_directories(${FONTS_DIR})

set(IMGUI_DIR ./third-party/imgui)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

find_package(glfw3 REQUIRED)
include_directories(${glfw3_INCLUDE_DIRS})

find_package(cpr REQUIRED)
include_directories(${cpr_INCLUDE_DIRS})

set(IMGUI ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp)
set(IMGUI_BACKEND ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
set(IMGUI_EXTENSIONS ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp)

add_executable(nullptr src/main.cc ${IMGUI} ${IMGUI_BACKEND} ${IMGUI_EXTENSIONS})
target_compile_features(nullptr PUBLIC cxx_std_20)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

if(supported)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET nullptr PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()

add_compile_definitions(IMGUI_IMPL_OPENGL_ES2=1)
target_link_libraries(nullptr PUBLIC glfw GLESv2 cpr)
