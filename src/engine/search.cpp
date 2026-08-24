#include "search.hpp"
#include "eval.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstdlib>

namespace engine {
    static constexpr int MATE = 30000;
    static constexpr int INF = 32000;

    bool Search::time_up() {
        if (stop.load()) return true;
        if (!timed) return false;
        if ((nodes & 2047ULL) == 0) {
            if (std::chrono::steady_clock::now() >= t_end) {
                stop.store(true);
                return true;
            }
        }
        return stop.load();
    }

    int Search::score_move(const Board& board, const Move& move, Move tt_move) {
        if (move.data == tt_move.data) return 20000;

        Piece victim = Piece::NONE;
        Color them = opposite(board.get_side_to_move());
        for (int i = 0; i < 6; i++) {
            if (get_bit(board.get_pieces(static_cast<Piece>(i), them), move.get_to())) {
                victim = static_cast<Piece>(i);
                break;
            }
        }
        if (move.get_flag() == MoveFlag::EnPassant) victim = Piece::PAWN;

        if (victim != Piece::NONE || move.is_promotion()) {
            Piece attacker = board.piece_on(move.get_from());
            int v_score = (static_cast<int>(victim) + 1) * 100;
            int a_score = static_cast<int>(attacker) + 1;
            int promo = move.is_promotion() ? 800 : 0;
            return 10000 + v_score - a_score + promo;
        }
        return 0;
    }

    int Search::quiesce(Board& board, int alpha, int beta, int ply) {
        nodes++;
        if (ply > 12 || time_up()) return Evaluator::evaluate(board);

        if (board.is_fifty_move_draw() || board.repetition_count() >= 2)
            return 0;

        bool chk = MoveGen::in_check(board);
        if (!chk) {
            int stand = Evaluator::evaluate(board);
            if (stand >= beta) return stand;
            if (stand > alpha) alpha = stand;
        }

        std::vector<Move> moves = chk ? MoveGen::generate_legal_moves(board)
                                      : MoveGen::generate_legal_noisy(board);
        if (chk && moves.empty()) return -MATE + ply;

        std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
            return score_move(board, a, Move{}) > score_move(board, b, Move{});
        });

        for (const Move& move : moves) {
            board.make_move(move);
            int score = -quiesce(board, -beta, -alpha, ply + 1);
            board.unmake_move(move);
            if (time_up()) return alpha;
            if (score >= beta) return score;
            if (score > alpha) alpha = score;
        }
        return alpha;
    }

    int Search::negamax(Board& board, int depth, int alpha, int beta, int ply) {
        nodes++;
        if (time_up()) return alpha;

        if (ply > 0 && (board.is_fifty_move_draw() || board.repetition_count() >= 2))
            return 0;

        int orig_alpha = alpha;
        Move tt_move{};
        int tt_score;
        if (ply > 0 && tt.probe(board.hash_key, depth, alpha, beta, tt_score, tt_move)) {
            return tt_score;
        }

        if (depth <= 0) {
            return quiesce(board, alpha, beta, ply);
        }

        std::vector<Move> moves = MoveGen::generate_legal_moves(board);
        if (moves.empty()) {
            if (MoveGen::in_check(board)) return -MATE + ply;
            return 0;
        }

        std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
            return score_move(board, a, tt_move) > score_move(board, b, tt_move);
        });

        int max_val = -INF;
        Move best_move = moves[0];

        for (const Move& move : moves) {
            board.make_move(move);
            int score = -negamax(board, depth - 1, -beta, -alpha, ply + 1);
            board.unmake_move(move);
            if (time_up()) return max_val;

            if (score > max_val) {
                max_val = score;
                best_move = move;
            }
            if (max_val > alpha) alpha = max_val;
            if (alpha >= beta) break;
        }

        HashFlag flag = HashFlag::EXACT;
        if (max_val <= orig_alpha) flag = HashFlag::ALPHA;
        else if (max_val >= beta) flag = HashFlag::BETA;
        tt.store(board.hash_key, depth, max_val, flag, best_move);
        return max_val;
    }

    SearchResult Search::search(Board& board, const SearchLimits& limits) {
        SearchResult result;
        clear_stop();
        nodes = 0;

        int max_depth = std::max(1, std::min(limits.max_depth, 64));
        timed = limits.use_clock;
        if (timed) {
            int think_ms = limits.movetime_ms;
            if (think_ms <= 0) {
                int remain = (board.get_side_to_move() == Color::WHITE) ? limits.wtime_ms : limits.btime_ms;
                int inc = (board.get_side_to_move() == Color::WHITE) ? limits.winc_ms : limits.binc_ms;
                if (remain > 0) think_ms = remain / 30 + inc / 2;
                else think_ms = 1000;
            }
            think_ms = std::max(30, think_ms);
            t_end = std::chrono::steady_clock::now() + std::chrono::milliseconds(think_ms);
        }

        auto root_moves = MoveGen::generate_legal_moves(board);
        if (root_moves.empty()) {
            result.score = MoveGen::in_check(board) ? -MATE : 0;
            return result;
        }

        result.best = root_moves[0];
        Move tt_hint{};
        int dummy;
        tt.probe(board.hash_key, 0, -INF, INF, dummy, tt_hint);

        for (int depth = 1; depth <= max_depth; depth++) {
            std::sort(root_moves.begin(), root_moves.end(), [&](const Move& a, const Move& b) {
                if (a.data == result.best.data) return true;
                if (b.data == result.best.data) return false;
                return score_move(board, a, tt_hint) > score_move(board, b, tt_hint);
            });

            int best_score = -INF;
            Move best_move = root_moves[0];
            bool completed = true;

            for (const Move& move : root_moves) {
                board.make_move(move);
                int score = -negamax(board, depth - 1, -INF, INF, 1);
                board.unmake_move(move);
                if (time_up() && depth > 1) {
                    completed = false;
                    break;
                }
                if (score > best_score) {
                    best_score = score;
                    best_move = move;
                }
            }

            if (!completed) break;

            result.best = best_move;
            result.score = best_score;
            result.depth = depth;
            result.nodes = nodes;
            result.pv.clear();
            result.pv.push_back(best_move);

            if (std::abs(best_score) > MATE - 128) break;
            if (time_up()) break;
        }

        result.nodes = nodes;
        return result;
    }

    Move Search::search_best_move(Board& board, int depth) {
        SearchLimits limits;
        limits.max_depth = depth;
        limits.use_clock = false;
        auto r = search(board, limits);
        return r.best;
    }
}
