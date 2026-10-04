# 中国象棋 xiangqi-qt

基于 Qt 6（C++17 + QML/Qt Quick）的中国象棋游戏：纯 C++ 规则引擎 + AI + QML 界面。
支持 Linux（WSL Ubuntu）原生运行，以及 Linux 主机交叉编译 Windows x64 版。

## 功能特性

**完整规则引擎**（`src/board.cpp` / `src/board.h`）
- 全部棋种走法生成：车 / 马 / 炮 / 将 / 士 / 象 / 兵
- 规则细节：蹩马腿、塞象眼、炮隔子打、九宫限制、将帅对脸、将军判定
- 走法合法性过滤（模拟走后不能留下己方被将军）
- FEN 导入 / 导出、胜负判定（将死 / 困毙）、悔棋状态记录

**AI 对弈**（`src/engine.cpp` / `src/engine.h`）
- minimax + alpha-beta 剪枝 + 走法排序（MVV-LVA，提高剪枝效率）
- 搜索深度可调（1~5 层，界面 SpinBox 控制）

**对局控制**（`src/gamecontroller.cpp` / `src/gamecontroller.h`）
- 双人对战 / 人机对战（AI 执黑）两种模式
- 悔棋（多步）、新游戏、AI 深度调节

**QML 界面**（`qml/Main.qml`）
- 木质风格棋盘：河界、九宫斜线、棋子中心对齐交叉点
- 选中高亮（金色圆圈）、合法走法提示（橙点 = 空位，橙圈 = 吃子）
- 顶部工具栏（模式切换 / 悔棋 / AI 深度）+ 底部状态栏（回合与胜负状态）

**单元测试**（`tests/test_rules.cpp`，7 项，CTest 集成）
- 起始布局 / FEN、首步 44 种合法走法、马跳日、蹩马腿、塞象眼 / 将帅对脸、炮隔子打、AI 返回合法走法

**跨平台产物**
- Linux：ELF x86-64（WSL Ubuntu 26.04 实测通过）
- Windows：PE32+ GUI x86-64（Linux 交叉编译，免安装 zip 分发）

## 环境要求

- WSL Ubuntu（本机为 Ubuntu 26.04.1 LTS）
- Qt：`~/Qt/6.12.0/gcc_64`（主环境）与 `~/Qt/6.8.3/gcc_64`（交叉编译 host，与目标同版本）
- 构建：CMake 3.16+、Make / Ninja、GCC（系统自带）
- Windows 交叉编译：系统 mingw-w64（`g++-mingw-w64-x86-64-posix`，GCC 13.2）+ aqt 安装的 `~/qtwin/6.8.3/mingw_64`（Qt 6.8.3 win64_mingw）

开发环境完整搭建（含国内镜像加速）详见：
- [`docs/Qt6开发环境与Windows交叉编译搭建指南.md`](docs/Qt6开发环境与Windows交叉编译搭建指南.md)
- [`docs/cross_compiling_win_on_linux_faq.md`](docs/cross_compiling_win_on_linux_faq.md)（交叉编译问题速查）

## Linux 开发（WSL Ubuntu）

### 构建

```bash
cd ~/proj/xiangqi-qt
cmake -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux -j 8
```

- 构建输出目录 / 文件：`build-linux/xiangqi-qt`（开发调试产物，ELF x86-64）

### 调试运行（不安装，直接运行）

```bash
cd ~/proj/xiangqi-qt
QT_QUICK_BACKEND=software ./build-linux/xiangqi-qt
```

> 必须加 `QT_QUICK_BACKEND=software` 使用软件渲染：默认 D3D12 后端在 WSLg 下会段错误（exit 139）闪退。

### 单元测试

```bash
ctest --test-dir build-linux --output-on-failure
# 或直接运行规则测试：
./build-linux/test_rules
```

### 正式安装（安装到自定义 prefix）

```bash
cmake --install build-linux --prefix ~/xiangqi-install
# 安装结果：~/xiangqi-install/bin/xiangqi-qt
QT_QUICK_BACKEND=software ~/xiangqi-install/bin/xiangqi-qt   # 运行（同样需软件渲染）
```

## Windows 交叉编译（Linux 主机 → Windows x64）

### 构建

```bash
cd ~/proj/xiangqi-qt
cmake -B build-win -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-system-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-win -j 8
```

- 构建输出目录 / 文件：`build-win/xiangqi-qt.exe`（PE32+ GUI，x86-64）

### 打包分发（免安装 zip）

```bash
cd build-win/deploy && zip -qr ../xiangqi-qt-win-x64.zip .
cp ../xiangqi-qt-win-x64.zip /mnt/c/Users/ysm/Downloads/
```

Windows 侧：**解压 zip → 双击 `xiangqi-qt.exe`**（免安装；运行时不弹控制台，GUI 子系统）。

## 开发调试产物 vs 正式构建产物

两者**一般不同**，区分如下：

| 平台 | 开发调试产物 | 正式构建 / 发布产物 |
| --- | --- | --- |
| Linux | `build-linux/xiangqi-qt`（裸 ELF，依赖开发机上的 Qt，可直接运行） | `cmake --install` 安装到 prefix 的 `bin/xiangqi-qt`（安装到目标路径） |
| Windows | `build-win/xiangqi-qt.exe`（裸 exe，缺 DLL / QML / 插件，**不可独立运行**） | `build-win/deploy/`（exe + 61 个 Qt6*.dll + MinGW 运行库 + `qml/` + `plugins/` + `qt.conf`，**可独立运行**）→ `xiangqi-qt-win-x64.zip` |

差异说明：
- **开发调试产物**：追求快速编译与直接运行，不打包任何依赖，只能在开发机（已装 Qt）上使用；
- **正式构建产物**：必须携带全部 Qt DLL、QML 模块、平台插件与 `qt.conf`，否则在目标机器上会闪退（缺 `Qt6OpenGL.dll` 等）或无法加载 QML（缺 `qml/` 模块）；
- Windows 发布前用 `file` 检查子系统应为 `GUI`（非 `console`），否则双击会先弹控制台黑框。

## GitHub Actions 持续集成

仓库内置两个 workflow（`.github/workflows/`），push / PR / tag 自动触发：

| workflow | 触发 | 内容 | 产物 |
| --- | --- | --- | --- |
| `ci` | push master、PR、手动 | Ubuntu 24.04 与 Windows 上 aqtinstall 安装 Qt 6.8.3（`actions/cache` 缓存，命中后跳过安装）→ 构建 → CTest 规则测试 → offscreen 冒烟测试 | `linux-test-bin` / `windows-test-bin`（裸二进制，调试用） |
| `build-installers` | 打 `v*` tag、手动 | 先跑测试 → Linux 用 linuxdeploy 打包 AppImage + deb；Windows 用 windeployqt + NSIS 打包安装程序 | `linux-installers`（AppImage + deb）、`windows-installer`（NSIS .exe） |

- 两 workflow 的 Qt 缓存 key 相同（`<runner.os>-qt-6.8.3-<arch>`），缓存互通，Linux / Windows 各约 1.1 GB，命中后跳过 Qt 安装，构建时间从 5+ 分钟降到 1~2 分钟。
- Windows runner 使用 `ilammy/msvc-dev-cmd` 激活 MSVC 环境 + Ninja 构建（自动适配 VS 2022 / 2026，无需硬编码 vcvars 路径）。
- 手动触发：GitHub Actions 页面 → 对应 workflow → `Run workflow`；发布构建需打 tag：`git tag v0.1.0 && git push origin v0.1.0`。

## 目录结构

```
xiangqi-qt/
├── CMakeLists.txt                  # 构建脚本（qt_add_executable WIN32 + install 规则）
├── src/
│   ├── main.cpp                    # 入口（注册 GameController + 加载 qrc:/qml/Main.qml）
│   ├── board.{h,cpp}               # 棋盘与规则引擎
│   ├── engine.{h,cpp}              # AI（minimax + alpha-beta）
│   └── gamecontroller.{h,cpp}      # QML ↔ C++ 桥接
├── qml/Main.qml                    # QML 界面（棋盘 / 工具栏 / 状态栏）
├── tests/test_rules.cpp            # 规则引擎单元测试
├── cmake/
│   ├── mingw-system-toolchain.cmake    # 本机交叉工具链（系统 mingw-posix）
│   └── mingw-llvm-toolchain.cmake      # 备选：LLVM-MinGW 路线（见指南附录）
├── tools/                          # installer.nsi / desktop / 图标
├── docs/
│   ├── Qt6开发环境与Windows交叉编译搭建指南.md
│   └── cross_compiling_win_on_linux_faq.md
├── build-linux/                    # Linux 构建目录（产物：xiangqi-qt）
└── build-win/                      # Windows 交叉构建目录（产物：xiangqi-qt.exe、deploy/）
```
