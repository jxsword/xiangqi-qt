// main.cpp — 程序入口
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqml.h>
#include <QUrl>

#include "gamecontroller.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("中国象棋"));
    QGuiApplication::setOrganizationName(QStringLiteral("xiangqi"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    // 注册为 QML 类型，由 QML 在 Window 内创建实例，避免 context property 注入时序导致 null
    qmlRegisterType<GameController>("Xiangqi", 1, 0, "GameController");

    QQmlApplicationEngine engine;
    // 直接加载资源内的 Main.qml（qmlcache 已命中 /qml/Main.qml 的 AOT 缓存）；
    // 不依赖 loadFromModule 的 qmldir 类型注册（Windows 交叉版该机制不可靠）。
    // Main.qml 内 import Xiangqi 1.0 所需类型已由上方 qmlRegisterType 注册。
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
