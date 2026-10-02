// engine.h — 中国象棋 AI（minimax + alpha-beta）
#pragma once

#include "board.h"

namespace xiangqi {

class Engine {
public:
    Engine() = default;

    // 计算 color 方的最佳走法；depth 为搜索深度（层数）
    Move bestMove(const Board& board, Color color, int depth = 3);

    // 静态评估：>0 红优，<0 黑优（以 color 方视角返回分值）
    static int evaluate(const Board& board, Color color);

private:
    int search(Board& board, int depth, int alpha, int beta, Color color, int ply);
    // 走法排序：先吃子（MVV-LVA），提高剪枝效率
    static void sortMoves(Board& board, std::vector<Move>& moves, Color color);
};

} // namespace xiangqi
