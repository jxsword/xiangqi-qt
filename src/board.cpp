// board.cpp — 中国象棋规则引擎实现
#include "board.h"

#include <algorithm>

namespace xiangqi {

namespace {
// FEN 字符 → 棋子（小写=黑方，大写=红方）
PieceType fenCharToType(char c) {
    switch (std::tolower(c)) {
        case 'k': return PieceType::King;
        case 'a': return PieceType::Advisor;
        case 'e': return PieceType::Elephant;
        case 'h': return PieceType::Horse;
        case 'r': return PieceType::Rook;
        case 'c': return PieceType::Cannon;
        case 'p': return PieceType::Soldier;
        default: return PieceType::None;
    }
}
char typeToFenChar(PieceType t, Color c) {
    char base = '?';
    switch (t) {
        case PieceType::King: base = 'k'; break;
        case PieceType::Advisor: base = 'a'; break;
        case PieceType::Elephant: base = 'e'; break;
        case PieceType::Horse: base = 'h'; break;
        case PieceType::Rook: base = 'r'; break;
        case PieceType::Cannon: base = 'c'; break;
        case PieceType::Soldier: base = 'p'; break;
        case PieceType::None: return ' ';
    }
    return c == Color::Red ? static_cast<char>(std::toupper(base)) : base;
}

bool inPalace(int row, int col, Color c) {
    // 九宫：红 0..2 行，黑 7..9 行，列 3..5
    if (col < 3 || col > 5) return false;
    if (c == Color::Red) return row >= 0 && row <= 2;
    return row >= 7 && row <= 9;
}

bool crossRiver(int row, Color c) {
    return c == Color::Red ? row > 4 : row < 5;
}
} // namespace

void Board::reset() {
    for (auto& row : board_) row.fill(Piece{});
    // 黑方（上，row 0-4）
    board_[0] = {Piece{PieceType::Rook, Color::Black}, Piece{PieceType::Horse, Color::Black},
                 Piece{PieceType::Elephant, Color::Black}, Piece{PieceType::Advisor, Color::Black},
                 Piece{PieceType::King, Color::Black}, Piece{PieceType::Advisor, Color::Black},
                 Piece{PieceType::Elephant, Color::Black}, Piece{PieceType::Horse, Color::Black},
                 Piece{PieceType::Rook, Color::Black}};
    board_[2][1] = {PieceType::Cannon, Color::Black};
    board_[2][7] = {PieceType::Cannon, Color::Black};
    for (int c = 0; c < COLS; c += 2) board_[3][c] = {PieceType::Soldier, Color::Black};
    // 红方（下，row 5-9）
    board_[9] = {Piece{PieceType::Rook, Color::Red}, Piece{PieceType::Horse, Color::Red},
                 Piece{PieceType::Elephant, Color::Red}, Piece{PieceType::Advisor, Color::Red},
                 Piece{PieceType::King, Color::Red}, Piece{PieceType::Advisor, Color::Red},
                 Piece{PieceType::Elephant, Color::Red}, Piece{PieceType::Horse, Color::Red},
                 Piece{PieceType::Rook, Color::Red}};
    board_[7][1] = {PieceType::Cannon, Color::Red};
    board_[7][7] = {PieceType::Cannon, Color::Red};
    for (int c = 0; c < COLS; c += 2) board_[6][c] = {PieceType::Soldier, Color::Red};
    redTurn_ = true;
    lastCaptured_ = {};
}

bool Board::loadFen(const std::string& fen) {
    // 格式：棋盘 10 段/，红黑方标志；忽略后半段（回合数等）
    std::string boardPart = fen.substr(0, fen.find(' '));
    board_ = {};
    int row = 0, col = 0;
    size_t i = 0;
    while (row < ROWS && i < boardPart.size()) {
        char ch = boardPart[i++];
        if (ch == '/') { ++row; col = 0; continue; }
        if (ch >= '1' && ch <= '9') { col += ch - '0'; continue; }
        if (col < COLS) {
            board_[row][col] = {fenCharToType(ch), std::isupper(ch) ? Color::Red : Color::Black};
            ++col;
        }
    }
    redTurn_ = true;
    return true;
}

std::string Board::fen() const {
    std::string out;
    for (int r = 0; r < ROWS; ++r) {
        int empty = 0;
        for (int c = 0; c < COLS; ++c) {
            const Piece& p = board_[r][c];
            if (p.empty()) { ++empty; continue; }
            if (empty > 0) { out += static_cast<char>('0' + empty); empty = 0; }
            out += typeToFenChar(p.type, p.color);
        }
        if (empty > 0) out += static_cast<char>('0' + empty);
        if (r < ROWS - 1) out += '/';
    }
    out += redTurn_ ? " w" : " b";
    return out;
}

std::pair<int, int> Board::kingPos(Color color) const {
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c)
            if (board_[r][c].type == PieceType::King && board_[r][c].color == color)
                return {r, c};
    return {-1, -1};
}

bool Board::isInCheck(Color color) const {
    auto [kr, kc] = kingPos(color);
    if (kr < 0) return false;
    // 车/炮直线
    const int dr[4] = {-1, 1, 0, 0}, dc[4] = {0, 0, -1, 1};
    for (int d = 0; d < 4; ++d) {
        int r = kr + dr[d], c = kc + dc[d];
        int blockers = 0;
        while (isInBoard(r, c)) {
            const Piece& p = board_[r][c];
            if (!p.empty()) {
                if (blockers == 0) {
                    if (p.color != color && p.type == PieceType::Rook) return true;
                    if (p.color != color && p.type == PieceType::King) return true; // 将帅对脸
                } else if (blockers == 1 && p.color != color && p.type == PieceType::Cannon) {
                    return true;
                }
                ++blockers;
                if (blockers > 1) break;
            }
            r += dr[d]; c += dc[d];
        }
    }
    // 马（8 个日字位 + 蹩腿）
    const int hr[8] = {-2, -2, -1, 1, 2, 2, 1, -1};
    const int hc[8] = {-1, 1, 2, 2, 1, -1, -2, -2};
    const int legR[8] = {-1, -1, 0, 0, 1, 1, 0, 0};
    const int legC[8] = {0, 0, 1, 1, 0, 0, -1, -1};
    for (int i = 0; i < 8; ++i) {
        int r = kr + hr[i], c = kc + hc[i];
        if (!isInBoard(r, c)) continue;
        const Piece& p = board_[r][c];
        if (p.color != color && p.type == PieceType::Horse) {
            if (board_[kr + legR[i]][kc + legC[i]].empty()) return true;
        }
    }
    // 兵/卒（对面）
    int dir = (color == Color::Red) ? -1 : 1; // 红将向上看（黑兵从上方来）：黑兵前进方向是 row+1
    // 黑兵在红将下方（row+1）可吃红将；红兵在红将上方（row-1）？兵只能前进吃，所以：
    // 黑兵攻击：黑兵前进方向 row+1，即黑兵位于 (kr-1, kc±?) 攻击向下 → 黑兵在 (kr-1, kc) 攻击红将
    for (int dc2 : {0}) {
        int r = kr + dir, c = kc + dc2;
        if (isInBoard(r, c)) {
            const Piece& p = board_[r][c];
            if (p.color != color && p.type == PieceType::Soldier) return true;
        }
    }
    // 兵/卒横吃（过河兵左右各一格）
    for (int dc2 : {-1, 1}) {
        int r = kr, c = kc + dc2;
        if (isInBoard(r, c)) {
            const Piece& p = board_[r][c];
            if (p.color != color && p.type == PieceType::Soldier && crossRiver(r, p.color)) return true;
        }
    }
    return false;
}

bool Board::stepLegal(const Move& m, Color color) const {
    if (!isInBoard(m.fromRow, m.fromCol) || !isInBoard(m.toRow, m.toCol)) return false;
    const Piece& src = board_[m.fromRow][m.fromCol];
    if (src.empty() || src.color != color) return false;
    const Piece& dst = board_[m.toRow][m.toCol];
    if (!dst.empty() && dst.color == color) return false; // 不能吃己方

    int dr = m.toRow - m.fromRow, dc = m.toCol - m.fromCol;
    switch (src.type) {
        case PieceType::King:
            if (!inPalace(m.toRow, m.toCol, color)) return false;
            if (std::abs(dr) + std::abs(dc) != 1) return false;
            break;
        case PieceType::Advisor:
            if (!inPalace(m.toRow, m.toCol, color)) return false;
            if (std::abs(dr) != 1 || std::abs(dc) != 1) return false;
            break;
        case PieceType::Elephant: {
            if (std::abs(dr) != 2 || std::abs(dc) != 2) return false;
            int midR = (m.fromRow + m.toRow) / 2, midC = (m.fromCol + m.toCol) / 2;
            if (!board_[midR][midC].empty()) return false; // 塞象眼
            if (crossRiver(m.toRow, color)) return false;  // 不能过河
            break;
        }
        case PieceType::Horse: {
            static const int hr[8] = {-2, -2, -1, 1, 2, 2, 1, -1};
            static const int hc[8] = {-1, 1, 2, 2, 1, -1, -2, -2};
            static const int legR[8] = {-1, -1, 0, 0, 1, 1, 0, 0};
            static const int legC[8] = {0, 0, 1, 1, 0, 0, -1, -1};
            int idx = -1;
            for (int i = 0; i < 8; ++i)
                if (dr == hr[i] && dc == hc[i]) { idx = i; break; }
            if (idx < 0) return false;
            if (!board_[m.fromRow + legR[idx]][m.fromCol + legC[idx]].empty()) return false; // 蹩马腿
            break;
        }
        case PieceType::Rook: {
            if (dr != 0 && dc != 0) return false;
            int sr = (dr > 0) - (dr < 0), sc = (dc > 0) - (dc < 0);
            int r = m.fromRow + sr, c = m.fromCol + sc;
            while (r != m.toRow || c != m.toCol) {
                if (!board_[r][c].empty()) return false;
                r += sr; c += sc;
            }
            break;
        }
        case PieceType::Cannon: {
            if (dr != 0 && dc != 0) return false;
            int sr = (dr > 0) - (dr < 0), sc = (dc > 0) - (dc < 0);
            int r = m.fromRow + sr, c = m.fromCol + sc;
            int blockers = 0;
            while (r != m.toRow || c != m.toCol) {
                if (!board_[r][c].empty()) ++blockers;
                r += sr; c += sc;
            }
            if (dst.empty()) return blockers == 0;   // 走：路径无子
            return blockers == 1;                    // 吃：恰好一个炮架
        }
        case PieceType::Soldier: {
            int fwd = (color == Color::Red) ? -1 : 1; // 红向上（row-1），黑向下（row+1）
            if (dr == fwd && dc == 0) break;          // 前进
            if (std::abs(dc) == 1 && dr == 0) {       // 横走（仅过河后）
                if (!crossRiver(m.fromRow, color)) return false;
                break;
            }
            return false;
        }
        default:
            return false;
    }
    return true;
}

bool Board::moveLeavesKingInCheck(const Move& m, Color color) const {
    // 模拟走子，检查走后己方将是否被攻击
    Board copy = *this;
    if (!copy.stepLegal(m, color)) return false;
    Piece captured = copy.board_[m.toRow][m.toCol];
    copy.board_[m.toRow][m.toCol] = copy.board_[m.fromRow][m.fromCol];
    copy.board_[m.fromRow][m.fromCol] = Piece{};
    (void)captured;
    return copy.isInCheck(color);
}

std::vector<Move> Board::generateMoves(Color color) const {
    std::vector<Move> moves;
    moves.reserve(64);
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            const Piece& p = board_[r][c];
            if (p.empty() || p.color != color) continue;
            for (int tr = 0; tr < ROWS; ++tr) {
                for (int tc = 0; tc < COLS; ++tc) {
                    Move m{r, c, tr, tc};
                    if (m.fromRow == m.toRow && m.fromCol == m.toCol) continue;
                    if (!stepLegal(m, color)) continue;
                    if (moveLeavesKingInCheck(m, color)) continue;
                    moves.push_back(m);
                }
            }
        }
    }
    return moves;
}

bool Board::makeMove(const Move& m, Color color) {
    if (!stepLegal(m, color)) return false;
    if (moveLeavesKingInCheck(m, color)) return false;
    lastCaptured_ = board_[m.toRow][m.toCol];
    board_[m.toRow][m.toCol] = board_[m.fromRow][m.fromCol];
    board_[m.fromRow][m.fromCol] = Piece{};
    redTurn_ = !redTurn_;
    return true;
}

void Board::undoMove(const Move& m, Piece captured) {
    board_[m.fromRow][m.fromCol] = board_[m.toRow][m.toCol];
    board_[m.toRow][m.toCol] = captured;
    redTurn_ = !redTurn_;
}

bool Board::hasNoLegalMoves(Color color) const {
    return generateMoves(color).empty();
}

} // namespace xiangqi
