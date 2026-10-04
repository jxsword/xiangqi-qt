# Qt 6（C++/QML）开发环境与 Windows 交叉编译搭建指南（WSL Ubuntu 26.04，本机实测）

> 本文档记录**本机实际环境**：WSL Ubuntu 26.04.1 LTS 下 Qt 6（C++17 + QML/Qt Quick）开发环境、Linux 构建运行、以及交叉编译 Windows x64 版并分发（zip 免安装）的完整过程。**所有命令均在 /home/ssy 下以普通用户执行验证通过**（本机 sudo 需密码不可用，全程走用户级安装）。
> 适用工程：`xiangqi-qt`（中国象棋，纯 C++ 规则引擎 + AI + QML 界面）。
> 交叉编译遇到的坑与解决方案速查：见 [cross_compiling_win_on_linux_faq.md](./cross_compiling_win_on_linux_faq.md)。

---

## 0. 版本事实（先看，避免踩坑）

| 事项 | 结论 |
|---|---|
| Qt 6.8 LTS 开源版 | 开源渠道只发布到 **6.8.3**；6.8.4 及以后仅商业授权。本机交叉目标用 **6.8.3 win64_mingw** |
| Qt 6.12.0 | **没有 Windows mingw 包**（aqt 查询架构报 checksum 下载失败）——Windows 交叉请勿用 6.12.0 |
| Linux Qt | 本机为 Qt 官方在线安装器安装：`~/Qt/6.12.0/gcc_64`（主环境）+ `~/Qt/6.8.3/gcc_64`（交叉编译 host，**必须与目标同版本**） |
| Windows Qt（交叉目标） | aqt 安装：`~/qtwin/6.8.3/mingw_64`（含 qml/plugins/lib/cmake 全套） |
| Windows 交叉编译器 | **系统 apt 的 mingw-w64-posix**（`x86_64-w64-mingw32-g++-posix`，GCC 13.2）；Qt `win64_mingw` DLL 为 GCC/libstdc++ ABI，与系统 mingw 匹配 |
| QML 模块加载 | **Windows 交叉版 `engine.loadFromModule("Xiangqi","Main")` 不可靠**（报 `contains no type named "Main"`），必须直接 `engine.load(QUrl("qrc:/qml/Main.qml"))`（AOT 缓存直命中） |
| 控制台黑框 | exe 必须用 GUI 子系统：`qt_add_executable(... WIN32 ...)`；**不要手动链接 `Qt6::EntryPoint`**（该目标不存在），Qt6::Core 会在 `WIN32_EXECUTABLE` 为真时自动注入 `Qt6::EntryPointPrivate`（qtmain，WinMain→main） |
| DLL 收集 | **交叉环境下 windeployqt 不可用**（Windows 版 exe Linux 跑不了）；统一用「全量拷贝 `bin/Qt6*.dll` + objdump 闭包验证」 |
| 分发形态 | 本机实际用 **zip 免安装**（解压即用）；NSIS 安装包见附录（备选/CI） |
| 系统版本 | WSL Ubuntu **26.04.1 LTS**；Python 3.14.4 / pip 25.1.1；CMake 4.4.3；Ninja 1.13.2 |

---

## 1. 国内镜像加速（先配，下载快 10 倍以上）

### 1.1 apt 换清华源

本机已配置；新环境参考（Ubuntu 26.04）：

```bash
# 备份原源
sudo cp /etc/apt/sources.list.d/ubuntu.sources /etc/apt/sources.list.d/ubuntu.sources.bak
# 用清华镜像替换（URIs: http://mirrors.tuna.tsinghua.edu.cn/ubuntu/）
# Ubuntu 24.04+ 使用 deb822 格式 /etc/apt/sources.list.d/ubuntu.sources
sudo sed -i 's|http://archive.ubuntu.com/ubuntu/|http://mirrors.tuna.tsinghua.edu.cn/ubuntu/|g' \
  /etc/apt/sources.list.d/ubuntu.sources
sudo apt-get update
```

其他可选镜像：阿里云 `http://mirrors.aliyun.com/ubuntu/`、中科大 `http://mirrors.ustc.edu.cn/ubuntu/`。

### 1.2 pip 用清华 PyPI

```bash
pip3 config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple
# 或单次使用：
pip3 install --user --break-system-packages -i https://pypi.tuna.tsinghua.edu.cn/simple aqtinstall
```

### 1.3 aqt 下载 Qt 用清华 Qt 镜像

```bash
~/.local/bin/aqt install-qt --base https://mirrors.tuna.tsinghua.edu.cn/qt \
  windows desktop 6.8.3 win64_mingw -O ~/qtwin
```

（`--base` 指向 Qt 官方仓库的镜像根；aqt 3.x 也支持 `-b`。）

### 1.4 GitHub 下载加速（LLVM-MinGW 等，见附录时用）

```bash
wget https://ghproxy.com/https://github.com/mstorsjo/llvm-mingw/releases/download/<tag>/<file>
# 或 gh-proxy 系镜像：https://mirror.ghproxy.com/、https://ghfast.top/
```

---

## 2. 系统基础依赖（apt）

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build git wget curl unzip file zip \
  python3 python3-pip \
  libgl1-mesa-dev libglu1-mesa-dev libxkbcommon-dev \
  libfontconfig1-dev libfreetype6-dev libwayland-dev \
  libxcb-cursor0 \
  g++-mingw-w64-x86-64-posix mingw-w64-x86-64-dev binutils-mingw-w64-x86-64 \
  nsis wine64 p7zip-full
```

说明：
- `ninja-build`：Windows 交叉构建的推荐生成器。
- `libgl*/libxkbcommon/libfontconfig/libfreetype/libwayland`：Qt6 Gui/Quick 的构建与运行依赖。
- `libxcb-cursor0`：Qt6 xcb 平台插件运行时依赖（系统默认不装）。
- **`g++-mingw-w64-x86-64-posix` + `mingw-w64-x86-64-dev` + `binutils-mingw-w64-x86-64`**：Windows 交叉编译器与运行库（**本机实际使用的工具链**，非 LLVM-MinGW）。
- `nsis`：备选生成 Windows NSIS 安装包（本机 zip 分发用不到，CI 备选）。
- `wine64`：仅在「附录 A 的 windeployqt 路线」备选时使用（本机实际不用）。

### 安装后验证

```bash
x86_64-w64-mingw32-g++-posix --version    # 应输出 GCC 13-posix
x86_64-w64-mingw32-windres --version     # 资源编译器
x86_64-w64-mingw32-objdump --version     # 依赖闭包分析工具
cmake --version && ninja --version
python3 --version && pip3 --version
```

---

## 3. 安装 Qt

### 3.1 Linux 本机 Qt（主环境 + 交叉 host）

**方式 A（本机实际）**：Qt 官方在线安装器（`~/Qt/MaintenanceTool`）
- 安装器图形界面安装 `6.12.0 gcc_64`（主）与 `6.8.3 gcc_64`（交叉 host，需与目标同版本）。
- 结果：`~/Qt/6.12.0/gcc_64`、`~/Qt/6.8.3/gcc_64`。

**方式 B（命令行，无图形界面）**：aqtinstall（与 3.2 同一套工具）

```bash
# 安装 aqt（已装：~/.local/bin/aqt，v3.3.0）
pip3 install --user --break-system-packages -i https://pypi.tuna.tsinghua.edu.cn/simple aqtinstall

# 安装 Linux 版（用户级，无需 sudo）
~/.local/bin/aqt install-qt --base https://mirrors.tuna.tsinghua.edu.cn/qt \
  linux desktop 6.8.3 linux_gcc_64 -O ~/Qt
```

### 3.2 Windows 目标 Qt（交叉编译用，aqt 安装）

```bash
~/.local/bin/aqt install-qt --base https://mirrors.tuna.tsinghua.edu.cn/qt \
  windows desktop 6.8.3 win64_mingw -O ~/qtwin
```

- 架构名必须为 **`win64_mingw`**（MinGW/UCRT ABI，与系统 mingw-posix 匹配）。
- 安装结果：`~/qtwin/6.8.3/mingw_64`（qml/plugins/lib/cmake 全套，含 `lib/libQt6EntryPoint.a`）。

### 安装后验证

```bash
~/Qt/6.8.3/gcc_64/bin/qmake -query QT_VERSION        # 6.8.3
~/Qt/6.12.0/gcc_64/bin/qmake -query QT_VERSION       # 6.12.0
ls ~/qtwin/6.8.3/mingw_64/bin/Qt6Core.dll            # Windows SDK 存在
ls ~/qtwin/6.8.3/mingw_64/lib/cmake/Qt6Core/         # CMake 配置存在
```

> **QT_HOST_PATH 的作用**：交叉配置时 Qt CMake 要在 configure 阶段运行 `qmlimportscanner`（**主机**可执行程序）。目标版 Qt（Windows）里的扫描器是 `.exe`，Linux 跑不了，必须提供同版本 Linux Qt 作 host。本机 host = `~/Qt/6.8.3/gcc_64`（toolchain 里 `QT_HOST_PATH` 已写死）。

---

## 4. 交叉工具链

### 4.1 编译器（系统 apt mingw-posix）

工具链文件：`cmake/mingw-system-toolchain.cmake`（本机实际使用）：

```cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

set(QT_MINGW_ROOT "$ENV{HOME}/qtwin/6.8.3/mingw_64")
set(QT_HOST_PATH "$ENV{HOME}/Qt/6.8.3/gcc_64")

set(CMAKE_FIND_ROOT_PATH
    "${QT_MINGW_ROOT}"
    "/usr/x86_64-w64-mingw32"
    "/usr/lib/gcc/x86_64-w64-mingw32/13-posix")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

要点：
- `QT_MINGW_ROOT`：Windows 版 Qt SDK（find_package 命中它）；`QT_HOST_PATH`：同版本 Linux Qt。
- `FIND_ROOT_PATH_MODE_PACKAGE ONLY`：保证交叉包不被宿主机 Qt 污染。
- MinGW 运行库（部署用）：`/usr/lib/gcc/x86_64-w64-mingw32/13-posix/{libgcc_s_seh-1.dll, libstdc++-6.dll}`、`/usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll`。

### 4.2 验证

```bash
cmake -B build-win -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$HOME/proj/xiangqi-qt/cmake/mingw-system-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release
# 无报错、输出 "Build files have been written to ..." 即工具链正常
```

---

## 5. Linux 本机构建与运行（WSL）

```bash
cd ~/proj/xiangqi-qt
cmake -B build-linux -DCMAKE_BUILD_TYPE=Release      # 本机构建（Makefiles）
cmake --build build-linux -j 8
```

- 产物：`build-linux/xiangqi-qt`（ELF x86-64，开发调试用，可直接运行）。
- 单元测试（7 项规则测试，CTest 集成）：

```bash
ctest --test-dir build-linux --output-on-failure
# 或：./build-linux/test_rules
```

- **运行（关键！必须软件渲染）**：

```bash
cd ~/proj/xiangqi-qt
QT_QUICK_BACKEND=software ./build-linux/xiangqi-qt
```

> 不加 `QT_QUICK_BACKEND=software` 时，默认 D3D12 后端在 WSLg 下会 `D3D12: Removing Device.` 后**段错误（exit 139）闪退**。加该变量走软件渲染即可稳定运行。

- 正式安装到自定义 prefix：

```bash
cmake --install build-linux --prefix ~/xiangqi-install
# 安装结果：~/xiangqi-install/bin/xiangqi-qt
QT_QUICK_BACKEND=software ~/xiangqi-install/bin/xiangqi-qt
```

---

## 6. Windows 交叉编译与分发（zip 免安装）

### 6.1 交叉编译

```bash
cd ~/proj/xiangqi-qt
cmake -B build-win -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-system-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-win -j 8
```

- 产物：`build-win/xiangqi-qt.exe`（PE32+ **GUI** 子系统，x86-64）。
- 验证子系统（应为 GUI，非 console）：

```bash
file build-win/xiangqi-qt.exe
# PE32+ executable for MS Windows 5.02 (GUI), x86-64
```

> GUI 子系统由 `CMakeLists.txt` 的 `qt_add_executable(xiangqi-qt WIN32 ...)` 实现；qtmain（`Qt6::EntryPointPrivate`）由 Qt6::Core 按 `WIN32_EXECUTABLE` 属性自动链接，**无需手动链接**。

### 6.2 准备部署目录（deploy，可独立运行）

交叉环境下 windeployqt 不可用，手工收集依赖：

```bash
cd ~/proj/xiangqi-qt
D=build-win/deploy
rm -rf $D && mkdir -p $D/{plugins/platforms,plugins/styles,qml}

# 1) exe
cp build-win/xiangqi-qt.exe $D/

# 2) Qt DLL：全量拷贝（61 个，含传递依赖如 Qt6OpenGL/Qt6Sql）
cp ~/qtwin/6.8.3/mingw_64/bin/Qt6*.dll $D/

# 3) MinGW 运行库（GCC 13-posix）
cp /usr/lib/gcc/x86_64-w64-mingw32/13-posix/libgcc_s_seh-1.dll $D/
cp /usr/lib/gcc/x86_64-w64-mingw32/13-posix/libstdc++-6.dll $D/
cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll $D/

# 4) QML 模块（QtQuick 全风格、QtQml、Qt、QtCore、Assets、builtins.qmltypes 等）
cp -r ~/qtwin/6.8.3/mingw_64/qml/* $D/qml/

# 5) 平台插件 + 样式插件
cp ~/qtwin/6.8.3/mingw_64/plugins/platforms/qwindows.dll $D/plugins/platforms/
cp ~/qtwin/6.8.3/mingw_64/plugins/styles/qmodernwindowsstyle.dll $D/plugins/styles/

# 6) qt.conf（插件与 QML 搜索路径）
cat > $D/qt.conf <<'EOF'
[Paths]
Plugins=plugins
QmlImports=qml
Qml2Imports=qml
EOF
```

**依赖闭包自检**（应只剩 Windows 系统自带库，如 `KERNEL32/USER32/GDI32/ole32/d3d11/dxgi/opengl32/winspool.drv` 等）：

```bash
cd build-win/deploy
for dll in $(x86_64-w64-mingw32-objdump -p xiangqi-qt.exe | grep "DLL Name" | awk '{print $3}' \
  | grep -vE "^(KERNEL32|msvcrt|USER32|GDI32|ADVAPI32|SHELL32|ole32|oleaut32|comdlg32|winmm|ws2_32|version|dwmapi|shcore|uxtheme|imm32|d3d11|d3d12|dxgi|opengl32|dnsapi|winspool.drv|api-ms-win-|ext-ms-win-)"); do
  [ ! -f "$dll" ] && echo "MISSING: $dll"
done
```

> 全量拷贝 DLL 的原因（FAQ P4）：第一次只挑"直接依赖"导致缺 `Qt6OpenGL.dll`（Qt6Quick 渲染依赖）等传递依赖，实机双击闪退；全量拷贝后闭包验证 MISSING=0。

### 6.3 打包与交付

```bash
cd build-win/deploy && zip -qr ../xiangqi-qt-win-x64.zip .
cp ../xiangqi-qt-win-x64.zip /mnt/c/Users/ysm/Downloads/
```

Windows 侧：**删除旧解压目录 → 解压 zip → 双击 `xiangqi-qt.exe`**（免安装，GUI 子系统不弹控制台）。

---

## 7. 常见问题速查

| 现象 | 原因 | 解决 |
|---|---|---|
| Linux 运行段错误 exit 139 | WSLg D3D12 渲染崩溃 | `QT_QUICK_BACKEND=software` |
| Windows 双击闪退 | 缺 Qt6OpenGL.dll 等传递依赖 | 全量拷贝 `bin/Qt6*.dll` + 闭包验证 |
| `Module "Xiangqi" contains no type named "Main"` | 交叉版 loadFromModule 不可靠 | main.cpp 直接 `engine.load(QUrl("qrc:/qml/Main.qml"))` |
| 双击先弹控制台黑框 | exe 是 console 子系统 | `qt_add_executable(... WIN32 ...)`；勿手动链 `Qt6::EntryPoint` |
| `Qt6::EntryPoint ... target was not found` | 该目标不存在 | 目标名是 `Qt6::EntryPointPrivate`，由 Qt6::Core 自动注入，删掉手动链接 |
| `CMake Error: generator Ninja does not match ...` | 对已有目录换生成器 | 已存在的目录用缓存生成器重新配置 |
| `timeout` 导致脚本误判失败 | `timeout` 正常结束返回 124 | 脚本中先记录 `EXIT=$?` 再判断 |

详细排查过程见 [cross_compiling_win_on_linux_faq.md](./cross_compiling_win_on_linux_faq.md)。

---

## 8. 磁盘占用参考

| 组件 | 约占用 |
|---|---|
| Qt 6.12.0 Linux（~/Qt/6.12.0/gcc_64） | 1.2 GB |
| Qt 6.8.3 Linux（~/Qt/6.8.3/gcc_64，交叉 host） | 1.1 GB |
| Qt 6.8.3 Windows mingw（~/qtwin/6.8.3/mingw_64） | 1.1 GB |
| 系统 mingw-w64（apt） | 数百 MB |
| deploy 部署目录 | 约 116 MB |
| 发布 zip | 约 43 MB |

---

## 附录 A（备选路线，本机环境不适用）：LLVM-MinGW + winlibs 交叉工具链

> 保留自旧版指南（Ubuntu 22.04 + /opt 环境）。本机已改用系统 mingw-posix（第 4 节），以下仅供无系统 mingw 或需 clang 编译器的场景参考。对应 toolchain 文件：`cmake/mingw-llvm-toolchain.cmake`。

1. 下载 LLVM-MinGW（Linux 宿主 x86_64，UCRT 版）：<https://github.com/mstorsjo/llvm-mingw/releases>，解压到 `/opt/llvm-mingw`；
2. 下载 winlibs（仅取 libstdc++ 头库与运行时 DLL）：<https://github.com/brechtsanders/winlibs_mingw/releases>（UCRT + POSIX + SEH + x86_64），解压到 `/opt/winlibs`，将 `lib/libstdc++.dll.a` 复制到 `/opt/stdcpp/lib`；
3. toolchain 关键点：编译器 `x86_64-w64-mingw32-clang`，`CMAKE_CXX_FLAGS` 加 `-stdlib=libstdc++` + winlibs 头路径（`-isystem /opt/winlibs/mingw64/include/c++/<ver>/...`），链接器 `-L/opt/stdcpp/lib`；
4. 部署 DLL：`libstdc++-6.dll`（winlibs，GCC 16）+ `libunwind.dll`、`libwinpthread-1.dll`（LLVM-MinGW）。

> 若走该路线且用 wine 跑 `windeployqt.exe`（旧版指南 6.2）：`wine /opt/qtwin/6.8.3/mingw_64/bin/windeployqt.exe --release --no-translations --dir win-dist win-dist/xiangqi-qt.exe`（需 `WINEPREFIX=$HOME/.wine-xq WINEARCH=win64`，首次 `wineboot -i`）。本机实际**不用** wine（直接全量拷贝 DLL，见 6.2）。

## 附录 B（备选路线，记录备查）：MSVC ABI 交叉编译

若必须链接 `win64_msvc2022_64`（MSVC ABI），Linux 上用 clang-cl + lld-link + xwin SDK：

```bash
# LLVM 19+（xwin 新版 MSVC STL 头要求 Clang 19+）
wget https://apt.llvm.org/llvm.sh && sudo ./llvm.sh 19
sudo ln -sf /usr/lib/llvm-19/bin/clang /usr/lib/llvm-19/bin/clang-cl
cargo install xwin
xwin --accept-license splat --output $HOME/.xwin-cache
```

toolchain 要点（工程内 `cmake/msvc-toolchain.cmake`）：`clang-cl` + `lld-link` + `llvm-rc`；头路径 `-imsvc <xwin>/crt/include`、`-imsvc <xwin>/sdk/include/<ver>/{ucrt,um,shared}`；库路径 `/LIBPATH:<xwin>/crt/lib/x86_64` 等。最终还需微软 VC 运行时 DLL（vcruntime140/msvcp140），纯 Linux 下获取繁琐，**默认推荐 MinGW 路线**。

## 附录 C（备选，记录备查）：Linux AppImage / deb 打包

保留自旧版指南，供需要 Linux 单文件/安装包分发时参考：

```bash
# linuxdeploy 三件套（Qt 插件 + AppImage 插件）
cd tools
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage -O linuxdeploy
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage -O linuxdeploy-plugin-qt
wget https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage -O linuxdeploy-plugin-appimage
chmod +x linuxdeploy* && cd ..

export PATH=~/Qt/6.8.3/gcc_64/bin:$PATH
export QML_SOURCES_PATHS=$PWD/qml
export APPIMAGE_EXTRACT_AND_RUN=1
./tools/linuxdeploy --appdir AppDir --executable build-linux/xiangqi-qt \
  --desktop-file tools/xiangqi-qt.desktop --icon-file tools/xiangqi-qt.png --plugin qt
cp ~/Qt/6.8.3/gcc_64/plugins/platforms/libqoffscreen.so AppDir/usr/plugins/platforms/
OUTPUT=xiangqi-qt_0.1.0_amd64.AppImage ./tools/linuxdeploy-plugin-appimage --appdir AppDir
```

deb：将 `AppDir/usr` 复制到 `debroot/usr`，写 `debroot/DEBIAN/control`（Package/Version/Architecture: amd64/Depends: libc6 (>= 2.34), libxcb-cursor0），`dpkg-deb --build debroot xiangqi-qt_0.1.0_amd64.deb`。

## 附录 D（记录备查）：GitHub Actions 三平台 CI

`.github/workflows/build-installers.yml`：打 `v*` tag 或手动触发，Linux（ubuntu-22.04，AppImage+deb）、Windows（windows-latest，MSVC 原生 + windeployqt + NSIS）、macOS（macos-14，dmg）。CI 上 Windows 用 runner 自带 MSVC + aqt 装 `win64_msvc2022_64`，无需交叉编译；Linux 的交叉工具链用于本地产出 Windows 包。

## 附录 E：Git 小步提交示例

```
chore: 项目骨架（CMake + Qt6 Quick/QML + gitignore）
feat(board): 规则引擎（走法生成/将军/九宫/蹩马腿/塞象眼/炮隔子/将帅对脸/FEN）
feat(engine): AI（minimax + alpha-beta + 走法排序）
feat(game): 对局控制器 + 入口（双人/人机/悔棋）
feat(ui): QML 棋盘界面
fix(board): 规则 bug 修复 + 单元测试（CTest，7 项全过）
feat: 支持 Windows 交叉编译并沉淀 FAQ
docs: README + 指南文档按本机实际环境修订
```

规则测试也可不依赖 Qt 直接编译运行：

```bash
g++ -std=c++17 -I src tests/test_rules.cpp src/board.cpp src/engine.cpp -o /tmp/test_rules
/tmp/test_rules
```
