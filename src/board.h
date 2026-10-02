// board.h — 中国象棋棋盘与规则引擎
#pragma once

#include <array>
#include <string>
#include <vector>

namespace xiangqi {

enum class PieceType : int { None = 0, King, Advisor, Elephant, Horse, Rook, Cannon, Soldier };
enum class Color : int { Red = 0, Black = 1 };

struct Piece {
    PieceType type = PieceType::None;
    Color color = Color::Red;
    bool empty() const { return type == PieceType::None; }
};

struct Move {
    int fromRow = -1, fromCol = -1, toRow = -1, toCol = -1;
    bool valid() const { return fromRow >= 0 && fromCol >= 0 && toRow >= 0 && toCol >= 0; }
    bool operator==(const Move& o) const {
        return fromRow == o.fromRow && fromCol == o.fromCol && toRow == o.toRow && toCol == o.toCol;
    }
};

class Board {
public:
    static constexpr int ROWS = 10;
    static constexpr int COLS = 9;

    Board() { reset(); }

    void reset();                       // 标准起始布局
    bool loadFen(const std::string& fen); // 载入 FEN，返回是否成功
    std::string fen() const;            // 导出 FEN

    Piece pieceAt(int row, int col) const { return board_[row][col]; }
    bool isRedTurn() const { return redTurn_; }

    // 走法生成（返回走完后不造成己方被将军的合法走法）
    std::vector<Move> generateMoves(Color color) const;

    // 判断 color 方当前是否被将军
    bool isInCheck(Color color) const;

    // 执行/撤销走子（撤销需保存被吃的子）
    bool makeMove(const Move& m, Color color);   // 返回是否合法
    void undoMove(const Move& m, Piece captured);

    Piece lastCaptured() const { return lastCaptured_; }

    // 胜负判定：true 表示 color 已无合法走法（被将军=将死；否则=困毙）
    bool hasNoLegalMoves(Color color) const;

private:
    bool isInBoard(int r, int c) const { return r >= 0 && r < ROWS && c >= 0 && c < COLS; }
    // 单步走子合法性（不含"走后被将军"检查）
    bool stepLegal(const Move& m, Color color) const;
    // 模拟走后检查将军（make/undo 或拷贝方式）
    bool moveLeavesKingInCheck(const Move& m, Color color) const;
    std::pair<int, int> kingPos(Color color) const;

    std::array<std::array<Piece, COLS>, ROWS> board_{};
    bool redTurn_ = true;
    Piece lastCaptured_{};
};

} // namespace xiangqi
