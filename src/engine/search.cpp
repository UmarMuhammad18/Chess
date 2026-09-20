#include "search.hpp"
#include "eval.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace engine {

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

    void Search::clear_killers() {
        for (auto& ply : killers) {
            ply[0] = Move{};
            ply[1] = Move{};
        }
    }

    void Search::clear_history() {
        for (auto& c : history)
            for (auto& f : c)
                f.fill(0);
    }

    int Search::score_move(const Board& board, const Move& move, Move tt_move, int ply) {
        if (move.data == tt_move.data) return 1000000;

        if (move.is_capture() || move.is_promotion()) {
            Piece victim = Piece::NONE;
            Color them = opposite(board.get_side_to_move());
            for (int i = 0; i < 6; i++) {
                if (get_bit(board.get_pieces(static_cast<Piece>(i), them), move.get_to())) {
                    victim = static_cast<Piece>(i);
                    break;
                }
            }
            if (move.get_flag() == MoveFlag::EnPassant) victim = Piece::PAWN;

            Piece attacker = board.piece_on(move.get_from());
            int v = (victim == Piece::NONE) ? 0 : (static_cast<int>(victim) + 1) * 100;
            int a = static_cast<int>(attacker) + 1;
            int promo = move.is_promotion() ? 900 : 0;
            return 500000 + v - a + promo;
        }

        if (ply < MAX_PLY) {
            if (move.data == killers[ply][0].data) return 400000;
            if (move.data == killers[ply][1].data) return 350000;
        }

        Color us = board.get_side_to_move();
        int from = static_cast<int>(move.get_from());
        int to   = static_cast<int>(move.get_to());
        return history[static_cast<int>(us)][from][to];
    }

    int Search::quiesce(Board& board, int alpha, int beta, int ply) {
        nodes++;
        if (ply > 24 || time_up()) return Evaluator::evaluate(board);

        if (board.is_fifty_move_draw() || board.repetition_count() >= 2)
            return 0;

        bool in_chk = MoveGen::in_check(board);
        int stand_pat = Evaluator::evaluate(board);

        if (!in_chk) {
            if (stand_pat >= beta) return stand_pat;
            if (stand_pat > alpha) alpha = stand_pat;
        }

        std::vector<Move> moves = in_chk ? MoveGen::generate_legal_moves(board)
                                         : MoveGen::generate_legal_noisy(board);
        if (in_chk && moves.empty()) return -MATE + ply;

        std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
            return score_move(board, a, Move{}, ply) > score_move(board, b, Move{}, ply);
        });

        for (const Move& move : moves) {
            if (!in_chk && move.is_capture() && !move.is_promotion()) {
                Piece victim = board.piece_on(move.get_to());
                Piece attacker = board.piece_on(move.get_from());
                if (victim != Piece::NONE &&
                    static_cast<int>(attacker) > static_cast<int>(victim) &&
                    stand_pat + 100 < alpha) {
                    continue;
                }
            }

            board.make_move(move);
            int score = -quiesce(board, -beta, -alpha, ply + 1);
            board.unmake_move(move);

            if (time_up()) return alpha;
            if (score >= beta) return score;
            if (score > alpha) alpha = score;
        }
        return alpha;
    }

    int Search::negamax(Board& board, int depth, int alpha, int beta, int ply, bool do_null) {
        nodes++;
        if (time_up()) return alpha;

        if (ply > 0 && (board.is_fifty_move_draw() || board.repetition_count() >= 2))
            return 0;

        int mate_val = MATE - ply;
        if (alpha >= mate_val) return alpha;
        if (beta <= -mate_val) return beta;

        int orig_alpha = alpha;
        Move tt_move{};
        int tt_score = 0;

        if (ply > 0 && tt.probe(board.hash_key, depth, alpha, beta, tt_score, tt_move)) {
            return tt_score;
        }

        bool in_chk = MoveGen::in_check(board);
        if (in_chk) depth++;

        if (depth <= 0) {
            return quiesce(board, alpha, beta, ply);
        }

        if (do_null && !in_chk && depth >= 3 && ply > 0) {
            int non_pawn = popcount(board.get_pieces(Color::WHITE) | board.get_pieces(Color::BLACK))
                         - popcount(board.get_pieces(Piece::PAWN));
            if (non_pawn > 2) {
                board.make_null_move();
                int R = (depth > 6) ? 3 : 2;
                int score = -negamax(board, depth - 1 - R, -beta, -beta + 1, ply + 1, false);
                board.unmake_null_move();
                if (time_up()) return alpha;
                if (score >= beta) return beta;
            }
        }

        std::vector<Move> moves = MoveGen::generate_legal_moves(board);
        if (moves.empty()) {
            if (in_chk) return -MATE + ply;
            return 0;
        }

        std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
            return score_move(board, a, tt_move, ply) > score_move(board, b, tt_move, ply);
        });

        int max_val = -INF;
        Move best_move = moves[0];
        int moves_searched = 0;

        for (const Move& move : moves) {
            bool is_quiet = !move.is_capture() && !move.is_promotion() && !in_chk;

            board.make_move(move);

            int new_depth = depth - 1;
            int score;

            if (moves_searched >= 3 && depth >= 3 && is_quiet) {
                int reduction = (moves_searched >= 6) ? 2 : 1;
                score = -negamax(board, new_depth - reduction, -alpha - 1, -alpha, ply + 1, true);
                if (score > alpha) {
                    score = -negamax(board, new_depth, -beta, -alpha, ply + 1, true);
                }
            } else {
                if (moves_searched == 0) {
                    score = -negamax(board, new_depth, -beta, -alpha, ply + 1, true);
                } else {
                    score = -negamax(board, new_depth, -alpha - 1, -alpha, ply + 1, true);
                    if (score > alpha && score < beta) {
                        score = -negamax(board, new_depth, -beta, -alpha, ply + 1, true);
                    }
                }
            }

            board.unmake_move(move);
            moves_searched++;

            if (time_up()) return max_val;

            if (score > max_val) {
                max_val = score;
                best_move = move;
            }
            if (max_val > alpha) alpha = max_val;

            if (alpha >= beta) {
                if (is_quiet && ply < MAX_PLY) {
                    if (killers[ply][0].data != move.data) {
                        killers[ply][1] = killers[ply][0];
                        killers[ply][0] = move;
                    }
                    Color us = board.get_side_to_move();
                    int from = static_cast<int>(move.get_from());
                    int to   = static_cast<int>(move.get_to());
                    history[static_cast<int>(us)][from][to] += depth * depth;
                    if (history[static_cast<int>(us)][from][to] > 100000)
                        history[static_cast<int>(us)][from][to] = 100000;
                }
                break;
            }
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
        clear_killers();

        int max_depth = std::max(1, std::min(limits.max_depth, 64));
        timed = limits.use_clock;
        if (timed) {
            int think_ms = limits.movetime_ms;
            if (think_ms <= 0) {
                int remain = (board.get_side_to_move() == Color::WHITE) ? limits.wtime_ms : limits.btime_ms;
                int inc    = (board.get_side_to_move() == Color::WHITE) ? limits.winc_ms  : limits.binc_ms;
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
            int alpha = -INF;
            int beta  =  INF;
            if (depth >= 5) {
                alpha = result.score - 50;
                beta  = result.score + 50;
            }

            std::sort(root_moves.begin(), root_moves.end(), [&](const Move& a, const Move& b) {
                if (a.data == result.best.data) return true;
                if (b.data == result.best.data) return false;
                return score_move(board, a, tt_hint, 0) > score_move(board, b, tt_hint, 0);
            });

            int best_score = -INF;
            Move best_move = root_moves[0];
            bool completed = true;
            bool re_search = true;

            while (re_search) {
                re_search = false;
                best_score = -INF;

                for (const Move& move : root_moves) {
                    board.make_move(move);
                    int score = -negamax(board, depth - 1, -beta, -alpha, 1, true);
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

                if (best_score <= alpha || best_score >= beta) {
                    alpha = -INF;
                    beta  =  INF;
                    re_search = true;
                }
            }

            if (!completed) break;

            result.best  = best_move;
            result.score = best_score;
            result.depth = depth;
            result.nodes = nodes;
            result.pv = extract_pv(board, best_move, depth);

            if (std::abs(best_score) > MATE - 128) break;
            if (time_up()) break;
        }

        result.nodes = nodes;
        return result;
    }

    std::vector<Move> Search::extract_pv(Board& board, Move first, int max_len) {
        std::vector<Move> pv;
        if (first.data == 0) return pv;
        pv.push_back(first);

        std::vector<Move> played;
        board.make_move(first);
        played.push_back(first);

        for (int i = 1; i < max_len && i < 32; i++) {
            Move next{};
            if (!tt.probe_move(board.hash_key, next) || next.data == 0) break;

            auto legal = MoveGen::generate_legal_moves(board);
            bool ok = false;
            for (const auto& m : legal) {
                if (m.data == next.data) { ok = true; break; }
            }
            if (!ok) break;

            pv.push_back(next);
            board.make_move(next);
            played.push_back(next);
        }

        for (auto it = played.rbegin(); it != played.rend(); ++it)
            board.unmake_move(*it);

        return pv;
    }

    Move Search::search_best_move(Board& board, int depth) {
        SearchLimits limits;
        limits.max_depth = depth;
        limits.use_clock = false;
        auto r = search(board, limits);
        return r.best;
    }
}
