# Qt 6.8.3 LTS（C++/QML）开发环境与 Windows 交叉编译搭建指南（Ubuntu 22.04）

> 本文档记录在 **Ubuntu 22.04 LTS** 上从零搭建 Qt 6.8（C++17 + QML/Qt Quick）开发环境、构建 Linux 安装包（deb / AppImage）、交叉编译并打包 Windows NSIS 安装包的完整过程。所有命令均在本机实际执行验证通过。
> 适用工程：`xiangqi-qt`（中国象棋，纯 C++ 规则引擎 + AI + QML 界面）。

---

## 0. 版本事实（先看，避免踩坑）

| 事项 | 结论 |
|---|---|
| Qt 6.8.9 LTS | **开源渠道不存在**。Qt 6.8 系列开源版只发布到 **6.8.3**；6.8.4 及以后（含 6.8.9）仅商业授权渠道提供。开源用户应安装 **6.8.3 LTS** |
| 安装方式 | 官方在线安装器需图形界面且要登录账号；无头/CI 环境统一用 **aqtinstall**（pip 包，命令行安装官方开源包） |
| Linux 架构名 | `linux_gcc_64`（**不是** `gcc_64`，写错会报 qt_base 包解析错误） |
| Windows 架构名 | `win64_mingw`（MinGW/UCRT）、`win64_msvc2022_64`（MSVC ABI） |
| Windows 交叉编译器 | 不能用 winlibs 原生 Windows 编译器（PE 程序 Linux 无法执行）；Linux 上必须用 **LLVM-MinGW**（clang 目标为 mingw-w64）或 clang-cl+xwin |
| C++ 标准库 ABI | Qt `win64_mingw` 的 DLL 用 GCC/libstdc++ 编译；LLVM-MinGW 默认 libc++（`std::__1`），**必须显式切到 libstdc++**，否则链接报 `QTimer::singleShotImpl(std::__1::chrono::...)` 未定义 |
| QML 模块部署 | `qt_add_qml_module` 生成的 `build/<URI>/qmldir` 必须与可执行文件同目录分发，否则运行时报 `Module "Xiangqi" contains no type named "Main"` |
| AppImage 工具 | 无 FUSE 的容器内必须 `export APPIMAGE_EXTRACT_AND_RUN=1` |
| Qt6 xcb 插件 | 依赖 `libxcb-cursor0`（系统默认不装） |

---

## 1. 系统基础依赖

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build git wget curl unzip file \
  python3 python3-pip \
  libgl1-mesa-dev libglu1-mesa-dev libxkbcommon-dev \
  libfontconfig1-dev libfreetype6-dev libwayland-dev \
  libxcb-cursor0 \
  nsis wine64 p7zip-full
```

说明：
- `ninja-build`：推荐的生成器（比 make 快）。
- `libgl*/libxkbcommon/libfontconfig/libfreetype/libwayland`：Qt6 Gui/Quick 的构建与运行依赖。
- `libxcb-cursor0`：Qt6 xcb 平台插件运行时依赖，不装则 linuxdeploy 报 `Could not find dependency: libxcb-cursor.so.0`。
- `nsis`：Linux 原生 NSIS，可直接生成 Windows `.exe` 安装包（输出与平台无关）。
- `wine64`：在 Linux 上运行 Qt 官方的 Windows 版 `windeployqt.exe` 收集 DLL。

---

## 2. 安装 Qt 6.8.3（aqtinstall）

### 2.1 安装 aqtinstall

```bash
python3 -m pip install --upgrade pip
python3 -m pip install aqtinstall
# 验证
aqt version
```

### 2.2 查询可用版本与架构（可选，用于核实）

```bash
# 列出 6.8 系列版本
aqt list-qt linux desktop --spec "6.8"
# 列出某版本的架构
aqt list-qt linux desktop --arch 6.8.3
aqt list-qt windows desktop --arch 6.8.3
```

### 2.3 安装三套 Qt（Linux 本机 / Windows 目标 / Linux host 工具）

```bash
# Linux 本机开发与打包（约 1~2 GB，3~4 分钟）
sudo mkdir -p /opt/qt && sudo chown -R $USER:$USER /opt/qt
aqt install-qt linux desktop 6.8.3 linux_gcc_64 -O /opt/qt
# 安装结果：/opt/qt/6.8.3/gcc_64（qmake、cmake 配置、lib、plugins、qml）

# Windows MinGW 版（交叉编译目标，约 1 GB）
sudo mkdir -p /opt/qtwin && sudo chown -R $USER:$USER /opt/qtwin
aqt install-qt windows desktop 6.8.3 win64_mingw -O /opt/qtwin
# 安装结果：/opt/qtwin/6.8.3/mingw_64（含 bin/windeployqt.exe）

# （可选，MSVC ABI 路线才需要）
# aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 -O /opt/qtwin
```

> **QT_HOST_PATH 的作用**：交叉编译时 Qt CMake 要在 configure 阶段运行 QML import 扫描器（`qmlimportscanner`，是个**主机**可执行程序）。目标版 Qt（Windows）里的扫描器是 `.exe`，Linux 跑不了；需要再装一套**同版本 Linux Qt** 作为 host 工具链，configure 时传 `-DQT_HOST_PATH=/opt/qt/6.8.3/gcc_64`。
> 注意 `qmlimportscanner` 位于 `libexec/`（不是 `bin/`），CMake 会自动查找。

---

## 3. 安装 Windows 交叉编译工具链（LLVM-MinGW + libstdc++）

Qt 的 `win64_mingw` 是 **UCRT + GCC/libstdc++ ABI**。我们在 Linux 上用 **LLVM-MinGW**（内含可在 Linux 运行的 clang，目标三元组 `x86_64-w64-mingw32`，自带 mingw-w64 UCRT 头库和 lld），再配一份 **winlibs 的 libstdc++ 头与运行时**以匹配 Qt 的 ABI。

### 3.1 下载并解压 LLVM-MinGW（Ubuntu 22.04 x86_64，UCRT 版）

发布页：<https://github.com/mstorsjo/llvm-mingw/releases>

```bash
# 以 20260922 版为例（请替换为最新 tag）
cd /tmp
wget https://github.com/mstorsjo/llvm-mingw/releases/download/20260922/\
llvm-mingw-20260922-ucrt-ubuntu-22.04-x86_64.tar.xz
sudo mkdir -p /opt/llvm-mingw && sudo chown $USER:$USER /opt/llvm-mingw
tar -xf llvm-mingw-*-ucrt-ubuntu-22.04-x86_64.tar.xz -C /opt/llvm-mingw --strip-components=1
# 验证（应输出 clang version xx）
/opt/llvm-mingw/bin/x86_64-w64-mingw32-clang --version
```

### 3.2 下载 winlibs（仅取 libstdc++ 头库与运行时 DLL）

> 为什么不直接用 winlibs 的编译器？winlibs 发行包里的 `gcc.exe/g++.exe` 是 **Windows PE 程序**，Linux 上无法执行（`Exec format error`）。我们只用它的 **libstdc++ 头文件、import 库和运行时 DLL**（这些是目标平台文件，与编译器宿主无关）。

发布页：<https://github.com/brechtsanders/winlibs_mingw/releases>（选 **UCRT**、POSIX threads、SEH、x86_64 的 zip）

```bash
cd /tmp
wget https://github.com/brechtsanders/winlibs_mingw/releases/download/\
16.2.0posix-14.0.0-ucrt-r2/\
winlibs-x86_64-posix-seh-gcc-16.2.0-mingw-w64ucrt-14.0.0-r2.zip
sudo mkdir -p /opt/winlibs && sudo chown $USER:$USER /opt/winlibs
unzip -q winlibs-*.zip -d /opt/winlibs
# 结果：/opt/winlibs/mingw64/{include/c++/16.2.0, lib, bin}
```

把 libstdc++ 的 import 库单独放一个目录（避免 winlibs 全套库与 LLVM-MinGW 冲突）：

```bash
sudo mkdir -p /opt/stdcpp/lib && sudo chown -R $USER:$USER /opt/stdcpp
cp /opt/winlibs/mingw64/lib/libstdc++.dll.a /opt/stdcpp/lib/
# 头文件在编译时直接通过 -isystem 引用：
#   /opt/winlibs/mingw64/include/c++/16.2.0
#   /opt/winlibs/mingw64/include/c++/16.2.0/x86_64-w64-mingw32
#   /opt/winlibs/mingw64/include/c++/16.2.0/backward
```

> **版本兼容性**：libstdc++ 保持向后 ABI 兼容。Qt 6.8.3 的 DLL 用 GCC 13.x 构建，用 GCC 16 的头编译应用、随包携带 GCC 16 的 `libstdc++-6.dll` 运行没有问题（新版运行时可承载旧版编译的库）。

### 3.3 交叉编译 toolchain 文件

工程内已提供 `cmake/mingw-llvm-toolchain.cmake`：

```cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(LLVM_MINGW "/opt/llvm-mingw")
set(WINLIBS "/opt/winlibs/mingw64")
set(QT_MINGW_ROOT "/opt/qtwin/6.8.3/mingw_64")
set(GCC_VER "16.2.0")

set(CMAKE_C_COMPILER   "${LLVM_MINGW}/bin/x86_64-w64-mingw32-clang")
set(CMAKE_CXX_COMPILER "${LLVM_MINGW}/bin/x86_64-w64-mingw32-clang++")
set(CMAKE_RC_COMPILER  "${LLVM_MINGW}/bin/x86_64-w64-mingw32-windres")

# 关键：显式使用 libstdc++，并指向 winlibs 的 libstdc++ 头
set(CMAKE_CXX_FLAGS "-stdlib=libstdc++ \
  -isystem ${WINLIBS}/include/c++/${GCC_VER} \
  -isystem ${WINLIBS}/include/c++/${GCC_VER}/x86_64-w64-mingw32 \
  -isystem ${WINLIBS}/include/c++/${GCC_VER}/backward")
# 链接期让链接器找到 libstdc++.dll.a
set(CMAKE_EXE_LINKER_FLAGS "-L/opt/stdcpp/lib")
set(CMAKE_SHARED_LINKER_FLAGS "-L/opt/stdcpp/lib")

set(CMAKE_FIND_ROOT_PATH "${QT_MINGW_ROOT}" "${LLVM_MINGW}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_PREFIX_PATH "${QT_MINGW_ROOT}")
```

---

## 4. Linux 本机构建

```bash
cd xiangqi-qt
export PATH=/opt/qt/6.8.3/gcc_64/bin:$PATH

cmake -S . -B build -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/opt/qt/6.8.3/gcc_64
cmake --build build

# 单元测试（规则引擎 7 项，已集成 CTest）
ctest --test-dir build --output-on-failure
# 产物：build/xiangqi-qt；QML 模块目录：build/Xiangqi/
```

无头环境冒烟测试：

```bash
QT_QPA_PLATFORM=offscreen ./build/xiangqi-qt   # 能启动、无报错即正常
```

---

## 5. Linux 安装包（AppImage + deb）

### 5.1 准备图标与 desktop 文件

见 `tools/xiangqi-qt.png`（512×512）与 `tools/xiangqi-qt.desktop`：

```ini
[Desktop Entry]
Name=中国象棋
Exec=xiangqi-qt
Icon=xiangqi-qt
Type=Application
Categories=Game;BoardGame;
Terminal=false
```

### 5.2 下载 linuxdeploy 三件套

```bash
cd tools
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage -O linuxdeploy
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage -O linuxdeploy-plugin-qt
wget https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage -O linuxdeploy-plugin-appimage
chmod +x linuxdeploy linuxdeploy-plugin-qt linuxdeploy-plugin-appimage
cd ..
```

### 5.3 生成 AppDir 与 AppImage

```bash
export PATH=/opt/qt/6.8.3/gcc_64/bin:$PATH
export QML_SOURCES_PATHS=$PWD/qml          # 让 qt 插件扫描 QML 依赖
export APPIMAGE_EXTRACT_AND_RUN=1          # 无 FUSE 环境必需

./tools/linuxdeploy --appdir AppDir \
  --executable build/xiangqi-qt \
  --desktop-file tools/xiangqi-qt.desktop \
  --icon-file tools/xiangqi-qt.png \
  --plugin qt

# 无头环境验证用的 offscreen 平台插件（真实桌面用 xcb，已默认包含）
cp /opt/qt/6.8.3/gcc_64/plugins/platforms/libqoffscreen.so AppDir/usr/plugins/platforms/
# 关键：把 qt_add_qml_module 生成的模块目录放到可执行文件旁
cp -r build/Xiangqi AppDir/usr/bin/

OUTPUT=xiangqi-qt_0.1.0_amd64.AppImage \
  ./tools/linuxdeploy-plugin-appimage --appdir AppDir
```

### 5.4 生成 deb

```bash
mkdir -p debroot/DEBIAN
cp -r AppDir/usr debroot/
SIZE=$(du -sk debroot/usr | cut -f1)
cat > debroot/DEBIAN/control <<EOF
Package: xiangqi-qt
Version: 0.1.0
Section: games
Priority: optional
Architecture: amd64
Maintainer: Xiangqi Dev <dev@local>
Installed-Size: $SIZE
Depends: libc6 (>= 2.34), libxcb-cursor0
Description: 中国象棋（C++/QML 版）
 Chinese Chess built with Qt 6 Quick/QML, rule engine + AI.
EOF
dpkg-deb --build debroot xiangqi-qt_0.1.0_amd64.deb
```

> 若 `dpkg-deb` 偶发 `tar: file changed as we read it`，可删除 `debroot` 重新 `cp -r` 后再打包（文件系统时序问题，与内容无关）。

验证：

```bash
dpkg-deb -I xiangqi-qt_0.1.0_amd64.deb      # 查看元信息
mkdir /tmp/t && dpkg-deb -x xiangqi-qt_0.1.0_amd64.deb /tmp/t
QT_QPA_PLATFORM=offscreen /tmp/t/usr/bin/xiangqi-qt
```

---

## 6. Windows 交叉编译与打包

### 6.1 交叉编译

```bash
cd xiangqi-qt
cmake -S . -B build-win -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-llvm-toolchain.cmake \
  -DQT_HOST_PATH=/opt/qt/6.8.3/gcc_64
cmake --build build-win
# 产物：build-win/xiangqi-qt.exe（PE32+ x86-64）
# QML 模块：build-win/Xiangqi/
```

常见编译错误与处理：
- `no member named 'abs' in namespace 'std'`：源码缺 `#include <cstdlib>`（GCC 间接包含、clang 严格），补上头文件即可。
- `undefined symbol ... std::__1::chrono ...`：用了 libc++，检查 toolchain 是否加了 `-stdlib=libstdc++` 与 winlibs 头路径。

### 6.2 准备发布目录并用 windeployqt 收集 Qt DLL

```bash
mkdir -p win-dist
cp build-win/xiangqi-qt.exe win-dist/
cp -r build-win/Xiangqi win-dist/

# 初始化 64 位 wine 前缀（首次）
export WINEPREFIX=$HOME/.wine-xq WINEARCH=win64 WINEDEBUG=-all
wineboot -i

# 运行 Windows 版 windeployqt（经 wine）
wine /opt/qtwin/6.8.3/mingw_64/bin/windeployqt.exe \
  --release --no-translations --no-system-d3d-compiler \
  --dir win-dist win-dist/xiangqi-qt.exe
```

### 6.3 补齐 MinGW 运行时 DLL

```bash
# libstdc++（来自 winlibs，GCC 16）
cp /opt/winlibs/mingw64/bin/libstdc++-6.dll win-dist/
# libunwind / libwinpthread（来自 LLVM-MinGW，x86_64）
cp /opt/llvm-mingw/x86_64-w64-mingw32/bin/libunwind.dll \
   /opt/llvm-mingw/x86_64-w64-mingw32/bin/libwinpthread-1.dll win-dist/
```

依赖闭包自检（应只剩 Windows 自带系统库，如 `KERNEL32/DWrite/UxTheme/bcrypt/msvcrt/api-ms-win-crt-*`）：

```bash
for f in $(find win-dist -name '*.dll') win-dist/xiangqi-qt.exe; do
  /opt/llvm-mingw/bin/x86_64-w64-mingw32-objdump -p "$f" | grep 'DLL Name'
done | sort -u
```

最终 `win-dist/` 关键内容：
- `xiangqi-qt.exe`
- `Qt6Core.dll / Qt6Gui.dll / Qt6Qml.dll / Qt6Quick.dll / ...`
- `platforms/qwindows.dll`、`qml/QtQuick/...`
- `Xiangqi/qmldir`（+ `qml/Main.qml`、qmltypes）
- `libstdc++-6.dll / libunwind.dll / libwinpthread-1.dll`

### 6.4 NSIS 生成安装包

`tools/installer.nsi`（注意：NSIS 的 `File` 相对路径基于 **.nsi 所在目录**，打包前把脚本复制到工程根目录）：

```bash
cp tools/installer.nsi ./installer.nsi
makensis installer.nsi
# 产物：xiangqi-qt_0.1.0_x64-setup.exe（约 31 MB）
```

安装脚本特性：安装到 `%PROGRAMFILES64%\XiangqiQt`、创建桌面与开始菜单快捷方式（中文名）、写入注册表卸载项、生成 `uninstall.exe`。

---

## 7. 关于 MSVC ABI 路线（备选，记录备查）

若必须链接 `win64_msvc2022_64`（MSVC ABI），Linux 上可用 **clang-cl + lld-link + xwin SDK**：

```bash
# LLVM 19+（xwin 新版 MSVC STL 头要求 Clang 19+，clang 14 会报 STL1000）
wget https://apt.llvm.org/llvm.sh && sudo ./llvm.sh 19
sudo ln -sf /usr/lib/llvm-19/bin/clang /usr/lib/llvm-19/bin/clang-cl
# xwin 下载 Windows SDK + MSVC STL（约 1.2 GB）
cargo install xwin
xwin --accept-license splat --output $HOME/.xwin-cache
```

toolchain 要点（工程内 `cmake/msvc-toolchain.cmake`）：
- 编译器 `/usr/lib/llvm-19/bin/clang-cl`，链接器 `lld-link`，资源编译器 `llvm-rc`；
- 头路径用 `-imsvc <xwin>/crt/include`、`-imsvc <xwin>/sdk/include/<ver>/{ucrt,um,shared}`；
- 库路径用 `/LIBPATH:<xwin>/crt/lib/x86_64`、`/LIBPATH:<xwin>/sdk/lib/<ver>/{ucrt,um}/x86_64`；
- CMake flags 中含空格的多值参数要用 `string(JOIN " " ...)` 拼接，**不要直接写分号列表**（分号会被 shell 当命令分隔符）；
- 用 `export LIB='<path1>;<path2>'`（**分号**分隔，Windows 语义）覆盖编译器自检阶段的库搜索；
- 该路线最终还需微软原版 VC 运行时 DLL（vcruntime140/msvcp140），在纯 Linux 下获取较繁琐，故**推荐默认走 MinGW + libstdc++ 路线**。

---

## 8. Git 工作流

工程遵循小步提交：

```
chore: 项目骨架（CMake + Qt6 Quick/QML + gitignore）
feat(board): 规则引擎（走法生成/将军/九宫/蹩马腿/塞象眼/炮隔子/将帅对脸/FEN）
feat(engine): AI（minimax + alpha-beta + 走法排序）
feat(game): 对局控制器 + 入口（双人/人机/悔棋）
feat(ui): QML 棋盘界面
fix(board): 规则 bug 修复 + 单元测试（CTest，7 项全过）
```

规则测试也可不依赖 Qt 直接编译运行：

```bash
g++ -std=c++17 -I src tests/test_rules.cpp src/board.cpp src/engine.cpp -o /tmp/test_rules
/tmp/test_rules
```

---

## 9. GitHub Actions（三平台）

工作流：`.github/workflows/build-installers.yml`，在打 `v*` tag 或手动触发时执行：

| Job | Runner | 产物 |
|---|---|---|
| linux | ubuntu-22.04 | `*.AppImage`、`*.deb`（linuxdeploy + qt 插件） |
| windows | windows-latest | `*_x64-setup.exe`（MSVC 原生构建 + windeployqt + NSIS） |
| macos | macos-14 | `*_macos.dmg`（macdeployqt + hdiutil） |

CI 上 Windows 直接用 GitHub runner 自带的 MSVC（`vcvars64.bat`）+ aqt 安装的 `win64_msvc2022_64`，无需交叉编译；Linux 的交叉编译工具链用于**本地**产出 Windows 包。

---

## 10. 一键命令速查

```bash
# ---- Linux ----
export PATH=/opt/qt/6.8.3/gcc_64/bin:$PATH
cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/qt/6.8.3/gcc_64
cmake --build build && ctest --test-dir build --output-on-failure

# ---- Windows 交叉编译 ----
cmake -S . -B build-win -GNinja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-llvm-toolchain.cmake \
  -DQT_HOST_PATH=/opt/qt/6.8.3/gcc_64
cmake --build build-win
```

## 11. 磁盘占用参考

| 组件 | 约占用 |
|---|---|
| Qt 6.8.3 Linux（/opt/qt） | 1.2 GB |
| Qt 6.8.3 Windows mingw（/opt/qtwin） | 1.1 GB |
| LLVM-MinGW（/opt/llvm-mingw） | 1.3 GB |
| winlibs（/opt/winlibs，仅用其 libstdc++） | 1.8 GB（可只保留 include/c++ 与 bin 中 3 个 DLL，约 50 MB） |
| xwin SDK（MSVC 备选路线） | 1.2 GB |
