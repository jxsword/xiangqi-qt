cmake_minimum_required(VERSION 3.16)
project(xiangqi-qt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup()

qt_add_executable(xiangqi-qt WIN32
    src/main.cpp
    src/board.h
    src/board.cpp
    src/engine.h
    src/engine.cpp
    src/gamecontroller.h
    src/gamecontroller.cpp
)

qt_add_qml_module(xiangqi-qt
    URI Xiangqi
    VERSION 1.0
    QML_FILES
        qml/Main.qml
    NO_RESOURCE_TARGET_PATH
)

target_link_libraries(xiangqi-qt PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)
# Windows GUI 子系统（qt_add_executable 的 WIN32 参数）下，Qt6::Core 会自动链接
# Qt6::EntryPointPrivate（qtmain: WinMain -> main），无需手动链接，消除控制台黑框

# 规则引擎单元测试（可选 target，CTest 集成）
enable_testing()
add_executable(test_rules
    tests/test_rules.cpp
    src/board.h
    src/board.cpp
    src/engine.h
    src/engine.cpp
)
target_include_directories(test_rules PRIVATE src)
add_test(NAME test_rules COMMAND test_rules)

# 安装规则（Linux：/opt/xiangqi-qt 或 prefix 下 bin）
install(TARGETS xiangqi-qt
    RUNTIME DESTINATION bin
)
