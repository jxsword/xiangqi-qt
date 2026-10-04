// main.cpp — 程序入口
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqml.h>

#include "gamecontroller.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("中国象棋"));
    QGuiApplication::setOrganizationName(QStringLiteral("xiangqi"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    // 注册为 QML 类型，由 QML 在 Window 内创建实例，避免 context property 注入时序导致 null
    qmlRegisterType<GameController>("Xiangqi", 1, 0, "GameController");

    QQmlApplicationEngine engine;
    engine.loadFromModule("Xiangqi", "Main");
    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
