# Windows 交叉编译（Linux 主机 → Windows）FAQ

> 项目：xiangqi-qt（Qt6 + QML 中国象棋）
> 场景：WSL Ubuntu 24.04（发行版 `ubuntu2604`）下用 MinGW 交叉编译 Qt6 应用为 Windows x64 版
> 本文档汇总本轮（2026-10）交叉编译过程中遇到的全部问题与已验证的解决方案，均可复用。

---

## 0. 环境概览

| 组件 | 版本 / 路径 | 说明 |
| --- | --- | --- |
| WSL 发行版 | `ubuntu2604`，用户 `ssy` | 调用惯例：`wsl -d ubuntu2604 -u ssy -- bash -c '...'` |
| 主机 Qt（Linux） | `~/Qt/6.12.0/gcc_64`、`~/Qt/6.8.3/gcc_64` | Linux 构建/运行用 |
| 目标 Qt（Windows） | `~/qtwin/6.8.3/mingw_64` | aqt 安装的 Windows 交叉 SDK |
| 编译器 | `/usr/bin/x86_64-w64-mingw32-g++-posix`（GCC 13.2-posix） | 系统 mingw，含 `windres` |
| 构建工具 | cmake 4.4.3、ninja 1.13.2、make、makensis(NSIS) | 系统自带 |
| 交叉工具链文件 | `cmake/mingw-system-toolchain.cmake` | 本机自建（项目原 toolchain 引用不存在的 /opt 路径） |
| 构建目录 | `build-win/`（Ninja + 交叉工具链） | 产物 `xiangqi-qt.exe` |
| 部署目录 | `build-win/deploy/` | 可独立运行目录（约 116MB） |
| 交付包 | `build-win/xiangqi-qt-win-x64.zip` → `~/Downloads/`（Windows 侧） | 约 43MB |

---

## 一、环境准备：Qt Windows SDK 安装

### P1. aqtinstall 安装与调用

**问题**：系统无 Qt Windows 包，需用 aqt 安装；`pip3 install` 提示 externally-managed-environment。

**解决**：

```bash
pip3 install --user --break-system-packages aqtinstall
```

- 装好后 aqt 位于 `~/.local/bin/aqt`，**不在 PATH**，需全路径调用（或 `export PATH="$HOME/.local/bin:$PATH"`）。
- 本环境 python3 venv/ensurepip 缺失不可用，无法用虚拟环境，只能用 `--user --break-system-packages`。

### P2. Qt 版本选择：6.12.0 无 Windows 包，改用 6.8.3

**问题**：`aqt install-qt` 查询 6.12.0 的 `win64_mingw` 架构时持续报 `Failed to download checksum for Updates.xml`，且该版本在 aqt 源中**没有 `win64_mingw` 架构**。

**解决**：选用 **Qt 6.8.3 + win64_mingw**（与项目原 toolchain 文件引用一致）：

```bash
~/.local/bin/aqt install-qt windows desktop 6.8.3 win64_mingw -O ~/qtwin
```

约 47 秒完成，SDK 位于 `~/qtwin/6.8.3/mingw_64`，包含 bin/qml/plugins/lib/cmake 全套。

> 注意：**不要重试 6.12.0 的 Windows 包**（checksum 下载失败为已知现象）。

---

## 二、交叉工具链

### P3. 项目自带 toolchain 文件引用不存在的 /opt 路径

**问题**：项目 `cmake/` 下原有 3 个 toolchain 文件均引用 `/opt/...`（如 `/opt/qt-windows`、`/opt/mingw`），在 WSL 下全部不存在，无法直接使用。

**解决**：新建本机 toolchain `cmake/mingw-system-toolchain.cmake`（关键内容）：

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
- `QT_MINGW_ROOT`：Windows 版 Qt SDK（find_package 找到它）；`QT_HOST_PATH`：Linux 版 Qt（工具运行所需）。
- `FIND_ROOT_PATH_MODE_PACKAGE ONLY` 保证交叉包不被宿主机 Qt 污染。

### 构建命令

```bash
cmake -B build-win -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$HOME/proj/xiangqi-qt/cmake/mingw-system-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-win -j 8
```

---

## 三、实机运行问题

### P4. 双击闪退（exit 非 0 或黑屏闪退）

**问题**：首次部署后 Windows 实机双击闪退。

**根因**：windeployqt 在交叉环境下不可用，手动挑选 DLL 时**遗漏了传递依赖**——缺 `Qt6OpenGL.dll`（Qt6Quick 渲染依赖，闪退主因）、`Qt6Sql.dll` 等。

**解决**：**全量拷贝 Qt Windows SDK 的 `bin/Qt6*.dll`（61 个）**，并做**依赖闭包验证**：

```bash
cd deploy
for dll in $(x86_64-w64-mingw32-objdump -p xiangqi-qt.exe | grep "DLL Name" | awk '{print $3}' \
  | grep -vE "^(KERNEL32|msvcrt|USER32|GDI32|...)$"); do
  [ ! -f "$dll" ] && echo "MISSING: $dll"
done
```

- 白名单 = Windows 系统自带 DLL（KERNEL32/USER32/GDI32/ole32/shell32/d3d11/dxgi/opengl32 等）。
- 对 deploy 下**每个 exe/dll** 遍历其导入表与本地目录比对，直到无 MISSING（唯一剩 `winspool.drv` 属系统库可忽略）。

### P5. `QQmlApplicationEngine failed to load component: Module "Xiangqi" contains no type named "Main"`

**问题**：Windows 实机启动报 QML 模块找不到 `Main` 类型。

**排查过程**（已做，避免重复）：
- 对比 build-win 与 Qt 6.8.3 Linux 版构建的 qmlcache / 类型注册 / qmldir / qrc / 链接对象，**逐项 IDENTICAL**；
- Linux 6.8.3 带同样的 `qt.conf`（`QmlImports=qml`）offscreen 运行正常 → 排除 Qt 版本与构建产物问题；
- 定位：`engine.loadFromModule("Xiangqi", "Main")` 依赖 **qmldir 模块类型注册**，该机制在 Windows 交叉版 Qt 运行时**不可靠**（空模块注册先命中）。

**解决**：`src/main.cpp` 改为**直接加载资源路径**：

```cpp
engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
```

- `Main.qml` 已由构建期 AOT 编译进 qmlcache（注册键就是 `/qml/Main.qml`），直接命中缓存加载，**不依赖 qmldir 模块查找**；
- `Main.qml` 内 `import Xiangqi 1.0` 所需的 `GameController` 走 **C++ `qmlRegisterType`** 注册，同样不依赖 qmldir；
- 三平台验证：Linux 6.8.3 offscreen、Linux 6.12 offscreen、Windows 交叉编译均正常。

### P6. 运行时先弹控制台黑框，一直两个窗口

**问题**：exe 为 **console 子系统**（CMake 默认），Windows 双击先弹黑色控制台再开 GUI。

**解决**：声明 GUI 子系统：

```cmake
qt_add_executable(xiangqi-qt WIN32 src/main.cpp ...)
```

**踩坑记录（重要）**：
- 曾尝试手动链接 `Qt6::EntryPoint` → 报 `target was not found`。**该目标在 Qt 6.8.3 mingw 包中不存在**。
- 真实目标名是 **`Qt6::EntryPointPrivate`**（`lib/cmake/Qt6EntryPointPrivate/`，库 `lib/libQt6EntryPoint.a`，qtmain 提供 `WinMain → main`）。
- 且 **Qt6::Core 的 `INTERFACE_LINK_LIBRARIES` 已内置条件生成器表达式**（`Qt6CoreTargets.cmake:65`）：

  ```
  $<$<AND:$<NOT:$<BOOL:$<TARGET_PROPERTY:qt_no_entrypoint>>>,
       $<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>,
       $<BOOL:$<TARGET_PROPERTY:WIN32_EXECUTABLE>>>:Qt6::EntryPointPrivate>
  ```

  即：只要 `qt_add_executable(... WIN32 ...)` 设置了 `WIN32_EXECUTABLE` 属性，**Qt6::Core 会自动注入 EntryPointPrivate 链接，无需也不能显式链接**。

**验证**：`file build-win/xiangqi-qt.exe` 输出

```
PE32+ executable for MS Windows 5.02 (GUI), x86-64
```

（console → GUI；exe 从 474KB 增至 505KB，因链接了 qtmain。）

> 副作用：GUI 子系统下从命令行运行 `.\xiangqi-qt.exe` 命令立即返回、不阻塞，程序日志不再打印到终端——Windows GUI 程序标准行为。

---

## 四、部署结构（可复用）

```
deploy/
├── xiangqi-qt.exe                  # GUI 子系统
├── Qt6*.dll                        # 61 个，来自 ~/qtwin/6.8.3/mingw_64/bin/
├── libgcc_s_seh-1.dll              # /usr/lib/gcc/x86_64-w64-mingw32/13-posix/
├── libstdc++-6.dll                 # 同上
├── libwinpthread-1.dll             # /usr/x86_64-w64-mingw32/lib/
├── qml/                            # QtQuick(含 Controls 全风格 15 子项)、QtQml、Qt、QtCore、
│                                   #   Assets、builtins.qmltypes、jsroot.qmltypes
│                                   #   共 45 个 qmldir + qtquick2plugin.dll
├── plugins/
│   ├── platforms/qwindows.dll
│   └── styles/qmodernwindowsstyle.dll
└── qt.conf                         # [Paths] Plugins=plugins QmlImports=qml Qml2Imports=qml
```

打包交付：

```bash
cd build-win/deploy && zip -qr ../xiangqi-qt-win-x64.zip .
cp ../xiangqi-qt-win-x64.zip /mnt/c/Users/ysm/Downloads/
```

用户侧：删除旧解压目录 → 重新解压 → 双击 `xiangqi-qt.exe`。

---

## 五、脚本/环境坑（WSL + PowerShell 混合操作）

### P7. PowerShell 内联管道转义失败

**问题**：`wsl ... -- bash -c '... | tail ...'` 里含管道、引号时，PowerShell 会转义错乱。

**解决**：把命令写成脚本文件 → `cp` 到 `/tmp` → `bash /tmp/xxx.sh` → 输出再 `cp` 回 `%TEMP%` 由 Windows 侧 Read 读取。

### P8. `set -e` 与 `timeout` 返回值冲突

**问题**：脚本 `set -e` 下，`timeout 8 ./app` 正常结束返回 **124**（非 0），脚本被误判失败提前退出，后续步骤（如 Windows 交叉编译）没执行。

**解决**：`timeout ... ; echo "EXIT=$?"` 记录返回值再判断（124 = 正常运行被 timeout 杀掉），或避免在关键路径前用 `set -e`。

### P9. CMake 生成器冲突

**问题**：`cmake -B build-linux -G Ninja` 报 `generator: Ninja does not match the generator used previously: Unix Makefiles`。

**解决**：对已存在的构建目录不要改生成器；用默认（缓存）生成器重新配置：`cmake -B build-linux -DCMAKE_BUILD_TYPE=Release`。

### P10. 未提交/临时产物注意

- `.gitignore` 已忽略 `build-*/`、`*.exe`、`*.dll` 等构建产物，交叉 SDK（`~/qtwin`）与部署目录无需也不能提交。
- 交叉验证用临时脚本/日志（`%TEMP%/*.txt`、`/tmp/*.sh`）均为一次性，不进入仓库。

---

## 六、交付前验证清单

1. **exe 子系统**：`file build-win/xiangqi-qt.exe` → 必须含 `GUI`（非 console）。
2. **依赖闭包**：遍历 deploy 全部 exe/dll 导入表 vs 本地文件，MISSING = 0（仅允许系统白名单 DLL）。
3. **Linux 回归**（CMakeLists 改动后）：`cmake --build build-linux` + `QT_QUICK_BACKEND=software QT_QPA_PLATFORM=offscreen timeout 8 ./build-linux/xiangqi-qt` → EXIT=124 且无 QML 报错。
4. **Windows 实机**：用户删旧解压目录 → 重新解压 → 双击 → 无黑框、无闪退、功能正常。

---

## 附：关键命令速查

```bash
# 安装 aqt
pip3 install --user --break-system-packages aqtinstall

# 安装 Qt Windows SDK（6.8.3 win64_mingw）
~/.local/bin/aqt install-qt windows desktop 6.8.3 win64_mingw -O ~/qtwin

# 配置 + 构建
cmake -B build-win -G Ninja -DCMAKE_TOOLCHAIN_FILE=$HOME/proj/xiangqi-qt/cmake/mingw-system-toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-win -j 8

# 子系统检查
file build-win/xiangqi-qt.exe

# 部署打包
cd build-win/deploy && zip -qr ../xiangqi-qt-win-x64.zip .
cp ../xiangqi-qt-win-x64.zip /mnt/c/Users/ysm/Downloads/

# Linux 运行（软件渲染，规避 D3D12 段错误）
cd /home/ssy/proj/xiangqi-qt && QT_QUICK_BACKEND=software ./build-linux/xiangqi-qt
```
