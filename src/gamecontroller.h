// gamecontroller.h — QML 与 C++ 核心之间的桥接
#pragma once

#include <QObject>
#include <QString>

#include "board.h"
#include "engine.h"

class GameController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool redTurn READ isRedTurn NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(bool gameOver READ isGameOver NOTIFY stateChanged)
    Q_PROPERTY(QString modeName READ modeName NOTIFY modeChanged)
    Q_PROPERTY(int version READ stateVersion NOTIFY stateChanged)

public:
    enum class Mode { HumanVsHuman, HumanVsAI };
    Q_ENUM(Mode)

    explicit GameController(QObject* parent = nullptr);

    bool isRedTurn() const { return board_.isRedTurn(); }
    QString statusText() const { return status_; }
    bool isGameOver() const { return gameOver_; }
    QString modeName() const { return mode_ == Mode::HumanVsAI ? QStringLiteral("人机对战（黑方为 AI）") : QStringLiteral("双人对战"); }
    int stateVersion() const { return stateVersion_; }

    Q_INVOKABLE QString pieceAt(int row, int col) const; // 返回棋子显示字符，空为 ""
    Q_INVOKABLE bool isSelected(int row, int col) const;
    Q_INVOKABLE bool isLegalTarget(int row, int col) const;
    Q_INVOKABLE void onSquareClicked(int row, int col);
    Q_INVOKABLE void newGame(int mode); // 0=双人 1=人机
    Q_INVOKABLE void undoMove();
    Q_INVOKABLE int searchDepth() const { return aiDepth_; }
    Q_INVOKABLE void setSearchDepth(int d);

signals:
    void stateChanged();   // 棋盘/状态变化（QML 刷新）
    void modeChanged();

private:
    void refreshStatus();
    void notifyState();    // 递增 version 并发出 stateChanged
    void tryAiMove();      // 人机模式下 AI 走子
    void doMove(const xiangqi::Move& m);
    QString pieceDisplay(int row, int col) const;

    xiangqi::Board board_;
    xiangqi::Engine engine_;
    Mode mode_ = Mode::HumanVsHuman;
    bool gameOver_ = false;
    QString status_;
    int selRow_ = -1;   // 当前选中棋子行（-1 表示无选中）
    int selCol_ = -1;   // 当前选中棋子列
    std::vector<xiangqi::Move> legalTargets_;
    int aiDepth_ = 3;
    int stateVersion_ = 0;   // 状态版本号，供 QML 绑定刷新
    // 简单悔棋：仅记录最近一步
    struct History { xiangqi::Move move; xiangqi::Piece captured; };
    std::vector<History> history_;
};
