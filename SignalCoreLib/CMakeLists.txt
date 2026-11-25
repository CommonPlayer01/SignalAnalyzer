cmake_minimum_required(VERSION 3.16)

# SignalCoreLib 静态库工程
# 核心算法库：FFT、窗函数、谱分析、有效值、文件读取。
# 硬性要求：不依赖任何 Qt 界面模块，可在工作线程中安全调用。
project(SignalCoreLib LANGUAGES CXX)

# C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 源码列表
set(SIGNALCORELIB_SOURCES
    src/Fft.cpp
    src/SpectrumAnalyzer.cpp
    src/SignalReader.cpp
    src/WindowFactory.cpp
)

# 公共头文件列表（用于 IDE 展示，非编译必需）
set(SIGNALCORELIB_HEADERS
    include/SignalCoreLib/SignalData.h
    include/SignalCoreLib/RmsCalculator.h
    include/SignalCoreLib/Fft.h
    include/SignalCoreLib/IWindow.h
    include/SignalCoreLib/WindowFactory.h
    include/SignalCoreLib/SpectrumAnalyzer.h
    include/SignalCoreLib/SignalReader.h
)

add_library(SignalCoreLib STATIC
    ${SIGNALCORELIB_SOURCES}
    ${SIGNALCORELIB_HEADERS}
)

# 仅依赖标准库，不链接 Qt。
# 头文件位于 include/，公共接口使用 SignalCoreLib/ 前缀。
target_include_directories(SignalCoreLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 静态库默认开启位置无关代码，便于被共享库/插件链接。
set_target_properties(SignalCoreLib PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# 编译告警（与原有 qmake 的 -Wall -Wextra 保持一致）。
# 源文件含 UTF-8 中文注释：MSVC 需显式指定 /utf-8，避免按系统代码页(GBK)误解析。
if(MSVC)
    target_compile_options(SignalCoreLib PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(SignalCoreLib PRIVATE -Wall -Wextra)
endif()

# 安装规则：头文件与静态库。
include(GNUInstallDirs)
install(TARGETS SignalCoreLib
    EXPORT SignalCoreLibTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/SignalCoreLib
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ---------------------------------------------------------------------------
# 调用冒烟测试：控制台程序，直接调用库的公开接口（无 Qt / 无第三方依赖）。
# 用于验证静态库可正常链接且函数返回结果正确，默认开启并注册到 CTest。
# 可用 -DSIGNALCORELIB_BUILD_SMOKE_TEST=OFF 关闭。
# ---------------------------------------------------------------------------
option(SIGNALCORELIB_BUILD_SMOKE_TEST "Build SignalCoreLib call smoke test" ON)

if(SIGNALCORELIB_BUILD_SMOKE_TEST)
    add_executable(SignalCoreLibSmokeTest tests/SmokeTest.cpp)

    target_link_libraries(SignalCoreLibSmokeTest PRIVATE SignalCoreLib)

    if(MSVC)
        target_compile_options(SignalCoreLibSmokeTest PRIVATE /W4 /utf-8)
    else()
        target_compile_options(SignalCoreLibSmokeTest PRIVATE -Wall -Wextra)
    endif()

    add_test(NAME SignalCoreLibSmokeTest COMMAND SignalCoreLibSmokeTest)
endif()

# ---------------------------------------------------------------------------
# 单元测试：基于 GoogleTest 框架，CTest 驱动。
#
# 默认关闭：避免配置阶段联网下载 googletest，导致 VS/CMake 配置直接失败
# （即 “Build step for googletest failed” 一类错误）。
# 开启方式（任选其一）：
#   1) 已安装 GoogleTest（如 vcpkg）：-DSIGNALCORELIB_BUILD_TESTS=ON
#   2) 有本地 googletest 源码：
#        -DSIGNALCORELIB_BUILD_TESTS=ON
#        -DSIGNALCORELIB_GTEST_SOURCE_DIR=<googletest 源码目录>
#   3) 允许联网下载：
#        -DSIGNALCORELIB_BUILD_TESTS=ON
# 库接口调用验证不依赖本开关，由始终可用的 SignalCoreLibSmokeTest 覆盖。
# ---------------------------------------------------------------------------
option(SIGNALCORELIB_BUILD_TESTS "Build SignalCoreLib unit tests (GoogleTest)" OFF)
set(SIGNALCORELIB_GTEST_SOURCE_DIR "" CACHE PATH
    "本地 googletest 源码目录（离线构建单元测试时指定；留空则联网下载）")

if(SIGNALCORELIB_BUILD_TESTS)
    # 优先使用系统/包管理器已安装的 GoogleTest，其次使用本地源码，最后才联网下载。
    find_package(GTest QUIET)

    if(NOT GTest_FOUND)
        include(FetchContent)
        if(SIGNALCORELIB_GTEST_SOURCE_DIR)
            if(NOT EXISTS "${SIGNALCORELIB_GTEST_SOURCE_DIR}/CMakeLists.txt")
                message(FATAL_ERROR
                    "SIGNALCORELIB_GTEST_SOURCE_DIR 无效（未找到 CMakeLists.txt）："
                    "${SIGNALCORELIB_GTEST_SOURCE_DIR}")
            endif()
            FetchContent_Declare(googletest
                SOURCE_DIR "${SIGNALCORELIB_GTEST_SOURCE_DIR}"
            )
            message(STATUS "GoogleTest: 使用本地源码 ${SIGNALCORELIB_GTEST_SOURCE_DIR}")
        else()
            FetchContent_Declare(googletest
                URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            )
            message(STATUS "GoogleTest: 本地未提供源码，将联网下载 v1.15.2")
        endif()

        # MSVC 下强制 gtest 使用动态 CRT（/MD），避免与本工程运行时库不匹配。
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(SignalCoreLibTests
        tests/FftTest.cpp
        tests/WindowTest.cpp
        tests/SpectrumAnalyzerTest.cpp
        tests/RmsTest.cpp
    )
    target_link_libraries(SignalCoreLibTests PRIVATE SignalCoreLib GTest::gtest_main)

    if(MSVC)
        target_compile_options(SignalCoreLibTests PRIVATE /W4 /utf-8)
    else()
        target_compile_options(SignalCoreLibTests PRIVATE -Wall -Wextra)
    endif()

    include(GoogleTest)
    gtest_discover_tests(SignalCoreLibTests)
endif()