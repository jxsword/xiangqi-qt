// engine.cpp — AI 引擎实现：简化但可用的评估 + alpha-beta 搜索
#include "engine.h"

#include <algorithm>
#include <cstdlib>

namespace xiangqi {

namespace {
// 子力基础分值（相对值）
int pieceValue(PieceType t) {
    switch (t) {
        case PieceType::King: return 100000;
        case PieceType::Rook: return 900;
        case PieceType::Cannon: return 450;
        case PieceType::Horse: return 400;
        case PieceType::Elephant: return 200;
        case PieceType::Advisor: return 200;
        case PieceType::Soldier: return 100;
        default: return 0;
    }
}

// 简单位置加成：兵过河 +60，兵到达底线附近 +40，马/炮中位 +10
int positionBonus(PieceType t, Color c, int row, int col) {
    int bonus = 0;
    switch (t) {
        case PieceType::Soldier:
            if (c == Color::Red) {
                if (row < 5) bonus += 60;          // 过河
                if (row <= 2) bonus += 40;         // 深入
            } else {
                if (row > 4) bonus += 60;
                if (row >= 7) bonus += 40;
            }
            break;
        case PieceType::Horse:
        case PieceType::Cannon:
            if (row >= 3 && row <= 6 && col >= 2 && col <= 6) bonus += 10;
            break;
        default: break;
    }
    return bonus;
}
} // namespace

int Engine::evaluate(const Board& board, Color color) {
    int score = 0;
    for (int r = 0; r < Board::ROWS; ++r) {
        for (int c = 0; c < Board::COLS; ++c) {
            Piece p = board.pieceAt(r, c);
            if (p.empty()) continue;
            int v = pieceValue(p.type) + positionBonus(p.type, p.color, r, c);
            score += (p.color == Color::Red) ? v : -v;
        }
    }
    return (color == Color::Red) ? score : -score;
}

void Engine::sortMoves(Board& board, std::vector<Move>& moves, Color color) {
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        auto score = [&](const Move& m) {
            Piece dst = board.pieceAt(m.toRow, m.toCol);
            if (dst.empty()) return 0;
            // MVV-LVA：吃高价值子优先，被吃的低价值子优先（先吃）
            return pieceValue(dst.type) * 10 - pieceValue(board.pieceAt(m.fromRow, m.fromCol).type);
        };
        return score(a) > score(b);
    });
}

int Engine::search(Board& board, int depth, int alpha, int beta, Color color, int ply) {
    if (depth == 0) {
        return evaluate(board, color);
    }
    std::vector<Move> moves = board.generateMoves(color);
    if (moves.empty()) {
        // 无合法走法：被将军=将死（极差），否则=困毙（也判负）
        return -100000 + ply;
    }
    sortMoves(board, moves, color);

    int best = -1000000;
    for (const Move& m : moves) {
        Piece captured = board.pieceAt(m.toRow, m.toCol);
        if (!board.makeMove(m, color)) continue;
        int val = -search(board, depth - 1, -beta, -alpha, color == Color::Red ? Color::Black : Color::Red, ply + 1);
        board.undoMove(m, captured);
        if (val > best) best = val;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break; // 剪枝
    }
    return best;
}

Move Engine::bestMove(const Board& board, Color color, int depth) {
    Board work = board;
    std::vector<Move> moves = work.generateMoves(color);
    if (moves.empty()) return {};
    sortMoves(work, moves, color);

    Move best{};
    int bestVal = -1000000;
    int alpha = -1000000;
    const int beta = 1000000;
    for (const Move& m : moves) {
        Piece captured = work.pieceAt(m.toRow, m.toCol);
        if (!work.makeMove(m, color)) continue;
        int val = -search(work, depth - 1, -beta, -alpha, color == Color::Red ? Color::Black : Color::Red, 1);
        work.undoMove(m, captured);
        if (val > bestVal) {
            bestVal = val;
            best = m;
        }
        if (val > alpha) alpha = val;
    }
    return best;
}

} // namespace xiangqi
