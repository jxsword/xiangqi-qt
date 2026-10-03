# Windows x86_64 交叉编译 toolchain：clang-cl + lld-link + xwin SDK（MSVC ABI）
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_SYSTEM_VERSION "10.0.26100")

set(LLVM_BIN "/usr/lib/llvm-19/bin")
set(XWIN_ROOT "/home/user/.xwin-cache/xwin")

set(CMAKE_C_COMPILER   "${LLVM_BIN}/clang-cl")
set(CMAKE_CXX_COMPILER "${LLVM_BIN}/clang-cl")
set(CMAKE_LINKER       "${LLVM_BIN}/lld-link")
set(CMAKE_RC_COMPILER  "${LLVM_BIN}/llvm-rc")

set(_XWIN_INC_ITEMS
  "-imsvc ${XWIN_ROOT}/crt/include"
  "-imsvc ${XWIN_ROOT}/sdk/include/10.0.26100/ucrt"
  "-imsvc ${XWIN_ROOT}/sdk/include/10.0.26100/um"
  "-imsvc ${XWIN_ROOT}/sdk/include/10.0.26100/shared")
string(JOIN " " _XWIN_INC ${_XWIN_INC_ITEMS})

set(CMAKE_C_FLAGS   "/MD /DWIN32 /D_WINDOWS -fuse-ld=lld ${_XWIN_INC}")
set(CMAKE_CXX_FLAGS "/MD /DWIN32 /D_WINDOWS /std:c++17 /EHsc -fuse-ld=lld ${_XWIN_INC}")

set(_XPATH_ITEMS
  "/LIBPATH:${XWIN_ROOT}/crt/lib/x86_64"
  "/LIBPATH:${XWIN_ROOT}/sdk/lib/10.0.26100/ucrt/x86_64"
  "/LIBPATH:${XWIN_ROOT}/sdk/lib/10.0.26100/um/x86_64")
string(JOIN " " _XPATHS ${_XPATH_ITEMS})
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_XPATHS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_XPATHS}")

set(CMAKE_SYSTEM_INCLUDE_PATH
  "${XWIN_ROOT}/crt/include"
  "${XWIN_ROOT}/sdk/include/10.0.26100/ucrt"
  "${XWIN_ROOT}/sdk/include/10.0.26100/um"
  "${XWIN_ROOT}/sdk/include/10.0.26100/shared"
)
set(CMAKE_SYSTEM_LIBRARY_PATH
  "${XWIN_ROOT}/crt/lib/x86_64"
  "${XWIN_ROOT}/sdk/lib/10.0.26100/ucrt/x86_64"
  "${XWIN_ROOT}/sdk/lib/10.0.26100/um/x86_64"
)

set(CMAKE_FIND_ROOT_PATH "/opt/qtwin/6.8.3/msvc2022_64")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_PREFIX_PATH "/opt/qtwin/6.8.3/msvc2022_64")
