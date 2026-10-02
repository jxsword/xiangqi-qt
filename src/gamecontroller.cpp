// gamecontroller.cpp — 对局控制实现
#include "gamecontroller.h"

#include <QTimer>

using namespace xiangqi;

namespace {
// 棋子显示字符：红方用现代汉字，黑方用传统汉字（象棋惯例）
QString pieceText(PieceType t, Color c) {
    switch (t) {
        case PieceType::King:    return c == Color::Red ? QStringLiteral("帅") : QStringLiteral("将");
        case PieceType::Advisor: return c == Color::Red ? QStringLiteral("仕") : QStringLiteral("士");
        case PieceType::Elephant:return c == Color::Red ? QStringLiteral("相") : QStringLiteral("象");
        case PieceType::Horse:   return c == Color::Red ? QStringLiteral("马") : QStringLiteral("馬");
        case PieceType::Rook:    return c == Color::Red ? QStringLiteral("车") : QStringLiteral("車");
        case PieceType::Cannon:  return c == Color::Red ? QStringLiteral("炮") : QStringLiteral("砲");
        case PieceType::Soldier: return c == Color::Red ? QStringLiteral("兵") : QStringLiteral("卒");
        default: return QString();
    }
}
} // namespace

GameController::GameController(QObject* parent) : QObject(parent) {
    refreshStatus();
}

QString GameController::pieceAt(int row, int col) const {
    if (row < 0 || row >= Board::ROWS || col < 0 || col >= Board::COLS) return QString();
    return pieceDisplay(row, col);
}

QString GameController::pieceDisplay(int row, int col) const {
    Piece p = board_.pieceAt(row, col);
    if (p.empty()) return QString();
    return pieceText(p.type, p.color);
}

bool GameController::isSelected(int row, int col) const {
    return selected_.valid() && selected_.fromRow == row && selected_.fromCol == col;
}

bool GameController::isLegalTarget(int row, int col) const {
    for (const auto& m : legalTargets_)
        if (m.toRow == row && m.toCol == col) return true;
    return false;
}

void GameController::onSquareClicked(int row, int col) {
    if (gameOver_) return;

    // 已有选中：尝试走子
    if (selected_.valid()) {
        for (const auto& m : legalTargets_) {
            if (m.toRow == row && m.toCol == col) {
                doMove(m);
                return;
            }
        }
        // 点了另一个己方棋子：重新选中
        Piece p = board_.pieceAt(row, col);
        if (!p.empty() && (p.color == Color::Red) == board_.isRedTurn()) {
            selected_ = {row, col, -1, -1};
            legalTargets_ = board_.generateMoves(board_.isRedTurn() ? Color::Red : Color::Black);
            QVector<Move> filtered;
            for (const auto& t : legalTargets_) if (t.fromRow == row && t.fromCol == col) filtered.push_back(t);
            legalTargets_ = std::vector<Move>(filtered.begin(), filtered.end());
            emit stateChanged();
        } else {
            selected_ = {};
            legalTargets_.clear();
            emit stateChanged();
        }
        return;
    }

    // 无选中：选择己方棋子
    Piece p = board_.pieceAt(row, col);
    Color turn = board_.isRedTurn() ? Color::Red : Color::Black;
    if (!p.empty() && p.color == turn) {
        selected_ = {row, col, -1, -1};
        legalTargets_.clear();
        for (const auto& m : board_.generateMoves(turn))
            if (m.fromRow == row && m.fromCol == col) legalTargets_.push_back(m);
        emit stateChanged();
    }
}

void GameController::doMove(const Move& m) {
    Piece captured = board_.pieceAt(m.toRow, m.toCol);
    if (board_.makeMove(m, board_.isRedTurn() ? Color::Red : Color::Black)) {
        history_.push_back({m, captured});
        selected_ = {};
        legalTargets_.clear();
        // 胜负判定
        Color next = board_.isRedTurn() ? Color::Red : Color::Black;
        if (board_.hasNoLegalMoves(next)) {
            gameOver_ = true;
            status_ = (next == Color::Red) ? QStringLiteral("黑方胜（红方无子可动）") : QStringLiteral("红方胜（黑方无子可动）");
            emit stateChanged();
            return;
        }
        refreshStatus();
        emit stateChanged();
        tryAiMove();
    }
}

void GameController::tryAiMove() {
    if (gameOver_) return;
    if (mode_ != Mode::HumanVsAI) return;
    Color turn = board_.isRedTurn() ? Color::Red : Color::Black;
    if (turn != Color::Black) return; // AI 执黑

    // 异步（延迟）让 AI 走子，避免阻塞 UI
    QTimer::singleShot(120, this, [this] {
        if (gameOver_) return;
        Move m = engine_.bestMove(board_, Color::Black, aiDepth_);
        if (m.valid()) doMove(m);
    });
}

void GameController::newGame(int mode) {
    board_.reset();
    gameOver_ = false;
    selected_ = {};
    legalTargets_.clear();
    history_.clear();
    mode_ = (mode == 1) ? Mode::HumanVsAI : Mode::HumanVsHuman;
    refreshStatus();
    emit stateChanged();
    emit modeChanged();
}

void GameController::undoMove() {
    if (gameOver_) return;
    if (history_.empty()) return;
    int steps = (mode_ == Mode::HumanVsAI) ? 2 : 1; // 人机模式撤销红方与 AI 两步
    while (!history_.empty() && steps-- > 0) {
        const auto& h = history_.back();
        board_.undoMove(h.move, h.captured);
        history_.pop_back();
    }
    gameOver_ = false;
    selected_ = {};
    legalTargets_.clear();
    refreshStatus();
    emit stateChanged();
}

void GameController::setSearchDepth(int d) {
    aiDepth_ = qBound(1, d, 5);
}

void GameController::refreshStatus() {
    Color turn = board_.isRedTurn() ? Color::Red : Color::Black;
    QString side = (turn == Color::Red) ? QStringLiteral("红方") : QStringLiteral("黑方");
    if (board_.isInCheck(turn))
        status_ = QStringLiteral("轮到%1（被将军！）").arg(side);
    else
        status_ = QStringLiteral("轮到%1走棋").arg(side);
}
