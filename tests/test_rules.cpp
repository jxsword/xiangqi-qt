// test_rules.cpp — 规则引擎核心逻辑单元测试（独立编译运行）
#include <cassert>
#include <cstdio>

#include "board.h"
#include "engine.h"

using namespace xiangqi;

#define CHECK(cond, msg) do { \
    if (!(cond)) { std::printf("FAIL: %s (line %d)\n", msg, __LINE__); return 1; } \
} while (0)

int main() {
    // 1. 起始布局与 FEN
    Board b;
    CHECK(b.pieceAt(0, 0).type == PieceType::Rook && b.pieceAt(0, 0).color == Color::Black, "黑车位置");
    CHECK(b.pieceAt(9, 4).type == PieceType::King && b.pieceAt(9, 4).color == Color::Red, "红帅位置");
    CHECK(b.pieceAt(9, 0).type == PieceType::Rook && b.pieceAt(9, 0).color == Color::Red, "红车位置");
    CHECK(b.fen().find("rheakaehr") == 0, "FEN 起始段");
    std::printf("1. 起始布局/FEN: OK\n");

    // 2. 起始走法数（标准中国象棋红方首步合法走法 44 种）
    auto m0 = b.generateMoves(Color::Red);
    CHECK(m0.size() == 44, "红方首步走法数应为 44");
    std::printf("2. 首步走法数 %zu (期望44): OK\n", m0.size());

    // 3. 蹩马腿：马 (7,1)? 起始红马在 (9,1)。马前进到 (7,2)/(7,0) 需要 (8,1) 空——起始 (8,1) 空 ✓
    //    验证马不能越兵：红马 (9,1)→(7,0)：马腿 (8,1) 空，日字 ✓ 应合法；→(7,2) 同理
    //    炮 (7,1)→(7,0)? 车/炮起始 (9,0)
    bool horseLegal = false;
    for (const auto& m : m0)
        if (m.fromRow == 9 && m.fromCol == 1 && m.toRow == 7 && m.toCol == 0) horseLegal = true;
    CHECK(horseLegal, "红马(9,1)->(7,0) 应合法");
    std::printf("3. 马跳日: OK\n");

    // 4. 蹩马腿：模拟挡腿后马不能跳
    //    FEN: 马在 (5,4)，(4,4) 有子挡腿 → 不能跳 (3,5)
    Board b2;
    CHECK(b2.loadFen("4k4/9/9/9/4n4/9/9/9/9/4K4 w - - 0 1"), "FEN 载入");
    // 黑马 (4,4)→(2,3) 马腿 (3,4) 空 ✓；→(2,5) 马腿 (3,4) 空 ✓；(4,4)→(3,2) 马腿 (4,3) 空 ✓
    // 放置红子挡 (3,4)：改为 FEN 有红兵在 (3,4)
    Board b3;
    CHECK(b3.loadFen("4k4/9/9/4P4/4n4/9/9/9/9/4K4 w - - 0 1"), "FEN2 载入");
    bool blocked = false;
    for (const auto& m : b3.generateMoves(Color::Black))
        if (m.fromRow == 4 && m.fromCol == 4 && (m.toRow == 2 && m.toCol == 3)) blocked = true;
    CHECK(!blocked, "蹩马腿：马(4,4)->(2,3) 应被挡");
    bool unblocked = false;
    for (const auto& m : b3.generateMoves(Color::Black))
        if (m.fromRow == 4 && m.fromCol == 4 && (m.toRow == 2 && m.toCol == 5)) unblocked = true;
    CHECK(!unblocked, "马(4,4)->(2,5) 马腿(3,4)被挡也应非法");
    std::printf("4. 蹩马腿: OK\n");

    // 5. 将帅对脸：红帅 (9,4)、黑将 (0,4)、中间无子 → 黑将不能走到 (1,4)（对脸非法）
    Board b4;
    CHECK(b4.loadFen("4k4/9/9/9/9/9/9/9/9/4K4 w - - 0 1"), "FEN3 载入");
    bool faceOff = false;
    for (const auto& m : b4.generateMoves(Color::Black))
        if (m.fromRow == 0 && m.fromCol == 4 && m.toRow == 1 && m.toCol == 4) faceOff = true;
    CHECK(!faceOff, "将帅对脸：黑将(0,4)->(1,4) 应非法");
    CHECK(b4.isInCheck(Color::Black), "对脸时应视为黑方被将军");
    std::printf("5. 将帅对脸: OK\n");

    // 6. 炮隔子打：黑炮 (2,2) 隔红兵 (6,2) 吃红马 (8,2)；无隔子时不能吃
    Board b5;
    CHECK(b5.loadFen("4k4/9/2c6/9/9/9/2P6/9/2N6/5K3 w - - 0 1"), "FEN5 载入");
    bool cannonCapture = false;
    for (const auto& m : b5.generateMoves(Color::Black))
        if (m.fromRow == 2 && m.fromCol == 2 && m.toRow == 8 && m.toCol == 2) cannonCapture = true;
    CHECK(cannonCapture, "炮隔红兵吃 (2,2)->(8,2) 应合法");
    Board b6;
    CHECK(b6.loadFen("4k4/9/2c6/9/9/9/9/9/2N6/5K3 w - - 0 1"), "FEN6 载入");
    bool cannonNoCap = false;
    for (const auto& m : b6.generateMoves(Color::Black))
        if (m.fromRow == 2 && m.fromCol == 2 && m.toRow == 8 && m.toCol == 2) cannonNoCap = true;
    CHECK(!cannonNoCap, "炮无隔子不能吃 (2,2)->(8,2) 应非法");
    std::printf("6. 炮隔子打: OK\n");

    // 7. AI 引擎能给出合法走法
    Engine eng;
    Move best = eng.bestMove(b, Color::Red, 2);
    CHECK(best.valid(), "AI 应返回合法走法");
    std::printf("7. AI bestMove(红,depth2): (%d,%d)->(%d,%d) OK\n",
                best.fromRow, best.fromCol, best.toRow, best.toCol);

    std::printf("\n全部规则测试通过 ✓\n");
    return 0;
}
