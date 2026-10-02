// main.cpp — 程序入口
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "gamecontroller.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("中国象棋"));
    QGuiApplication::setOrganizationName(QStringLiteral("xiangqi"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QQmlApplicationEngine engine;

    GameController controller;
    engine.rootContext()->setContextProperty(QStringLiteral("controller"), &controller);

    engine.loadFromModule("Xiangqi", "Main");
    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
