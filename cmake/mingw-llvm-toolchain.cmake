# Windows x86_64 交叉编译：LLVM-MinGW clang + winlibs libstdc++（匹配 Qt win64_mingw 的 libstdc++ ABI）
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(LLVM_MINGW "/opt/llvm-mingw")
set(WINLIBS "/opt/winlibs/mingw64")
set(QT_MINGW_ROOT "/opt/qtwin/6.8.3/mingw_64")
set(GCC_VER "16.2.0")

set(CMAKE_C_COMPILER   "${LLVM_MINGW}/bin/x86_64-w64-mingw32-clang")
set(CMAKE_CXX_COMPILER "${LLVM_MINGW}/bin/x86_64-w64-mingw32-clang++")
set(CMAKE_RC_COMPILER  "${LLVM_MINGW}/bin/x86_64-w64-mingw32-windres")

set(CMAKE_CXX_FLAGS "-stdlib=libstdc++ -isystem ${WINLIBS}/include/c++/${GCC_VER} -isystem ${WINLIBS}/include/c++/${GCC_VER}/x86_64-w64-mingw32 -isystem ${WINLIBS}/include/c++/${GCC_VER}/backward")
set(CMAKE_EXE_LINKER_FLAGS "-L/opt/stdcpp/lib")
set(CMAKE_SHARED_LINKER_FLAGS "-L/opt/stdcpp/lib")

set(CMAKE_FIND_ROOT_PATH "${QT_MINGW_ROOT}" "${LLVM_MINGW}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_PREFIX_PATH "${QT_MINGW_ROOT}")
