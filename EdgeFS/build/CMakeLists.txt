cmake_minimum_required(VERSION 3.14)
project(edgefs)

option(ENABLE_DEMO      "enable compile demo" on)

# 配置公共变量
set(CMAKE_POSITION_INDEPENDENT_CODE on)

# 目录变量
set(BASE_PATH       ${PROJECT_SOURCE_DIR}/../)
set(SRC_PATH        ${BASE_PATH}/src)
set(DEMO_PATH       ${BASE_PATH}/demo)
set(TEST_PATH       ${BASE_PATH}/test)
set(INSTALL_PATH    ${BASE_PATH}/build/output)
set(COMMON_BASE_PATH    ${BASE_PATH}/../mouse_base/project/)

# ========== 目录设置 ===================
include_directories(${COMMON_BASE_PATH})

# ========== 安装变量设置 ================
set(INCLUDE_INSTALL_PATH ${INSTALL_PATH}/include)
set(LIBRARY_INSTALL_PATH ${INSTALL_PATH}/lib)

# andro不同架构库文件生成路径
# if(CMAKE_ANDROID_NDK AND ANDROID_ABI)
#     set(LIBRARY_INSTALL_PATH ${LIBRARY_INSTALL_PATH}/${ANDROID_ABI})
# endif()

# if ("${PLATFORM}" STREQUAL "ios")
#     set(LIBRARY_INSTALL_PATH ${LIBRARY_INSTALL_PATH}/ios)
# elseif ("${PLATFORM}" STREQUAL "macos")
#     set(LIBRARY_INSTALL_PATH ${LIBRARY_INSTALL_PATH}/macos)
# endif()

# ========== 编译选项设置 ===============
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG     "-O0 -Wall -ggdb -D__STDC_FORMAT_MACROS")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG}")

#  -fvisibility=hidden 隐藏符号表
set(CMAKE_C_FLAGS_RELEASE       "-Os -Wall -ggdb -fvisibility=hidden -D__STDC_FORMAT_MACROS")
set(CMAKE_CXX_FLAGS_RELEASE     "${CMAKE_C_FLAGS_RELEASE} -fno-rtti -fno-exceptions")

#if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
    message("[!] Current Compile Mode is debug")
#endif()

set(ALL_SRC_FILES
    ${SRC_PATH}/common/AsyncLogging.cpp
    ${SRC_PATH}/common/logger.cpp
    ${SRC_PATH}/common/FileOper.cpp
    ${SRC_PATH}/common/sha1.cpp
    ${SRC_PATH}/common/Utils.cpp
    ${SRC_PATH}/Bitmap.cpp
    ${SRC_PATH}/DataMgr.cpp
    ${SRC_PATH}/IndexMgr.cpp
    ${SRC_PATH}/EdgeFS.cpp
    )

link_libraries(-pthread -ldl)

# ======== object files ======
set(OBJECT_FILES ${PROJECT_NAME}_obj_files)
add_library(${OBJECT_FILES} OBJECT ${ALL_SRC_FILES})

# ======== static library
add_library(${PROJECT_NAME} STATIC $<TARGET_OBJECTS:${OBJECT_FILES}>)
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    CLEAN_DIRECT_OUTPUT 1
    )
install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${INSTALL_PATH}/lib/
    LIBRARY DESTINATION ${INSTALL_PATH}/lib/
    ARCHIVE DESTINATION ${INSTALL_PATH}/lib/
    )

# ======== demo =========
if (ENABLE_DEMO)
    set(WRITE_ALL_SRC_FILES
        ${DEMO_PATH}/WriteMain.cpp
    )
    set(WRITE_DEMO_NAME "edgefs_write_main")
    add_executable(${WRITE_DEMO_NAME} ${WRITE_ALL_SRC_FILES})

    target_link_libraries(${WRITE_DEMO_NAME} PRIVATE
        edgefs
    )

    install(
        TARGETS ${WRITE_DEMO_NAME}
        RUNTIME DESTINATION ${DEMO_PATH}
        LIBRARY DESTINATION ${DEMO_PATH}
        ARCHIVE DESTINATION ${DEMO_PATH}
    )

    set(READ_ALL_SRC_FILES
        ${DEMO_PATH}/ReadMain.cpp
    )
    set(READ_DEMO_NAME "edgefs_read_main")
    add_executable(${READ_DEMO_NAME} ${READ_ALL_SRC_FILES})

    target_link_libraries(${READ_DEMO_NAME} PRIVATE
        edgefs
    )
    
    install(
        TARGETS ${READ_DEMO_NAME}
        RUNTIME DESTINATION ${DEMO_PATH}
        LIBRARY DESTINATION ${DEMO_PATH}
        ARCHIVE DESTINATION ${DEMO_PATH}
    )
endif ()



