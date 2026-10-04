# 本机 Windows x86_64 交叉编译 toolchain
# 工具链：系统 mingw-w64 GCC 13.2-posix（/usr/bin/x86_64-w64-mingw32-g++-posix）
# Qt：aqt 下载的 Qt 6.8.3 win64_mingw（~/qtwin/6.8.3/mingw_64）
# Host Qt（提供 qmlimportscanner/qmlsc 等编译工具）：~/Qt/6.8.3/gcc_64
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(MINGW_SYSROOT "/usr/x86_64-w64-mingw32")
set(QT_MINGW_ROOT "$ENV{HOME}/qtwin/6.8.3/mingw_64")

set(CMAKE_C_COMPILER   "/usr/bin/x86_64-w64-mingw32-gcc-posix")
set(CMAKE_CXX_COMPILER "/usr/bin/x86_64-w64-mingw32-g++-posix")
set(CMAKE_RC_COMPILER  "/usr/bin/x86_64-w64-mingw32-windres")

set(CMAKE_FIND_ROOT_PATH "${QT_MINGW_ROOT}" "${MINGW_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_PREFIX_PATH "${QT_MINGW_ROOT}")
set(QT_HOST_PATH "$ENV{HOME}/Qt/6.8.3/gcc_64")
