#include "search.hpp"
#include "eval.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace engine {
    namespace {
        constexpr int SCORE_TT_MOVE  = 2'000'000;
        constexpr int SCORE_CAPTURE  = 1'000'000;
        constexpr int SCORE_KILLER_1 =   900'000;
        constexpr int SCORE_KILLER_2 =   800'000;
        constexpr int HISTORY_MAX    =   500'000;

        // Late move reduction amounts, indexed by [depth][move number]
        struct LmrTable {
            int r[64][64];
            LmrTable() {
                for (int d = 0; d < 64; d++)
                    for (int m = 0; m < 64; m++)
                        r[d][m] = (d == 0 || m == 0) ? 0 : static_cast<int>(0.75 + std::log(d) * std::log(m) / 2.25);
            }
        };
        const LmrTable LMR;

        // Swap the highest-scored remaining move into slot i (lazy selection sort)
        void pick_move(MoveList& moves, int* scores, int i) {
            int best = i;
            for (int j = i + 1; j < moves.size(); j++)
                if (scores[j] > scores[best]) best = j;
            std::swap(moves[i], moves[best]);
            std::swap(scores[i], scores[best]);
        }
    }

    Search::Search(size_t tt_mb) : tt(tt_mb) {
        new_game();
    }

    void Search::new_game() {
        tt.clear();
        std::memset(history, 0, sizeof(history));
        for (auto& k : killers) k[0] = k[1] = Move{};
    }

    int64_t Search::elapsed_ms() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
    }

    void Search::check_limits() {
        if (hard_limit_ms >= 0 && elapsed_ms() >= hard_limit_ms) stop_flag = true;
        if (node_limit && nodes >= node_limit) stop_flag = true;
    }

    void Search::update_pv(int ply, Move m) {
        pv_table[ply][ply] = m;
        for (int i = ply + 1; i < pv_length[ply + 1]; i++) pv_table[ply][i] = pv_table[ply + 1][i];
        pv_length[ply] = std::max(pv_length[ply + 1], ply + 1);
    }

    void Search::score_moves(const Board& board, const MoveList& moves, int* scores, Move tt_move, int ply) const {
        int us = static_cast<int>(board.get_side_to_move());
        for (int i = 0; i < moves.size(); i++) {
            Move m = moves[i];
            if (m == tt_move) {
                scores[i] = SCORE_TT_MOVE;
            } else if (m.is_capture() || m.is_promotion()) {
                // MVV-LVA: most valuable victim first, least valuable attacker as tiebreak
                Piece victim = m.get_flag() == MoveFlag::EnPassant ? Piece::PAWN : board.piece_on(m.get_to());
                Piece attacker = board.piece_on(m.get_from());
                scores[i] = SCORE_CAPTURE + Evaluator::piece_value(victim) * 10
                          - static_cast<int>(attacker) + Evaluator::piece_value(m.promo_piece());
            } else if (m == killers[ply][0]) {
                scores[i] = SCORE_KILLER_1;
            } else if (m == killers[ply][1]) {
                scores[i] = SCORE_KILLER_2;
            } else {
                scores[i] = history[us][static_cast<int>(m.get_from())][static_cast<int>(m.get_to())];
            }
        }
    }

    int Search::quiescence(Board& board, int ply, int alpha, int beta) {
        pv_length[ply] = ply;
        if ((nodes & 2047) == 0) check_limits();
        if (stop_flag.load(std::memory_order_relaxed)) return 0;

        nodes++;
        seldepth = std::max(seldepth, ply);

        bool in_check = MoveGen::in_check(board);
        if (ply >= MAX_PLY - 1) return in_check ? 0 : Evaluator::evaluate(board);

        // When in check every evasion must be considered and standing pat is not allowed
        int best = -MATE_SCORE + ply;
        int stand_pat = 0;
        MoveList moves;
        if (in_check) {
            MoveGen::generate_pseudo_legal_moves(board, moves);
        } else {
            stand_pat = Evaluator::evaluate(board);
            if (stand_pat >= beta) return stand_pat;
            if (stand_pat > alpha) alpha = stand_pat;
            best = stand_pat;
            MoveGen::generate_captures(board, moves);
        }

        int scores[MAX_MOVES];
        score_moves(board, moves, scores, Move{}, ply);

        Color us = board.get_side_to_move();
        for (int i = 0; i < moves.size(); i++) {
            pick_move(moves, scores, i);
            Move m = moves[i];

            // Delta pruning: skip captures that can't raise alpha even with a margin
            if (!in_check && !m.is_promotion()) {
                Piece victim = m.get_flag() == MoveFlag::EnPassant ? Piece::PAWN : board.piece_on(m.get_to());
                if (stand_pat + Evaluator::piece_value(victim) + 200 <= alpha) continue;
            }

            board.make_move(m);
            if (MoveGen::is_king_attacked(board, us)) { board.unmake_move(m); continue; }
            int score = -quiescence(board, ply + 1, -beta, -alpha);
            board.unmake_move(m);

            if (stop_flag.load(std::memory_order_relaxed)) return 0;
            if (score > best) {
                best = score;
                if (score > alpha) {
                    alpha = score;
                    update_pv(ply, m);
                    if (alpha >= beta) break;
                }
            }
        }
        return best;
    }

    int Search::negamax(Board& board, int depth, int ply, int alpha, int beta, bool allow_null) {
        pv_length[ply] = ply;
        if ((nodes & 2047) == 0) check_limits();
        if (stop_flag.load(std::memory_order_relaxed)) return 0;

        const bool root    = ply == 0;
        const bool pv_node = beta - alpha > 1;

        if (!root) {
            if (board.is_fifty_move_draw() || board.repetition_count() > 0 || board.is_insufficient_material())
                return 0;
            // Mate distance pruning: no line from here can beat a mate already found nearer the root
            alpha = std::max(alpha, -MATE_SCORE + ply);
            beta  = std::min(beta, MATE_SCORE - ply - 1);
            if (alpha >= beta) return alpha;
        }
        if (ply >= MAX_PLY - 1) return Evaluator::evaluate(board);

        const bool in_check = MoveGen::in_check(board);
        if (in_check) depth++; // check extension
        if (depth <= 0) return quiescence(board, ply, alpha, beta);

        nodes++;
        seldepth = std::max(seldepth, ply);

        // Transposition table lookup
        Move tt_move{};
        TTEntry entry;
        if (tt.probe(board.hash_key, entry)) {
            tt_move = entry.best_move;
            if (!pv_node && entry.depth >= depth) {
                int s = score_from_tt(entry.score, ply);
                if (entry.flag == HashFlag::EXACT) return s;
                if (entry.flag == HashFlag::BETA  && s >= beta)  return s;
                if (entry.flag == HashFlag::ALPHA && s <= alpha) return s;
            }
        }

        Color us = board.get_side_to_move();
        int static_eval = in_check ? -INF_SCORE : Evaluator::evaluate(board);

        if (!pv_node && !in_check) {
            // Reverse futility pruning: far above beta at shallow depth, assume it holds
            if (depth <= 6 && std::abs(beta) < MATE_BOUND && static_eval - 80 * depth >= beta)
                return static_eval;

            // Null move pruning: if passing still fails high, a real move almost certainly will.
            // Skipped without non-pawn material to avoid zugzwang blunders.
            if (allow_null && depth >= 3 && static_eval >= beta && board.has_non_pawn_material(us)) {
                int r = 3 + depth / 6;
                board.make_null_move();
                int score = -negamax(board, depth - 1 - r, ply + 1, -beta, -beta + 1, false);
                board.unmake_null_move();
                if (stop_flag.load(std::memory_order_relaxed)) return 0;
                if (score >= beta) return score >= MATE_BOUND ? beta : score;
            }
        }

        MoveList moves;
        MoveGen::generate_pseudo_legal_moves(board, moves);
        int scores[MAX_MOVES];
        score_moves(board, moves, scores, tt_move, ply);

        int best_score = -INF_SCORE;
        Move best_move{};
        HashFlag flag = HashFlag::ALPHA;
        int legal = 0;

        for (int i = 0; i < moves.size(); i++) {
            pick_move(moves, scores, i);
            Move m = moves[i];
            bool quiet = !m.is_capture() && !m.is_promotion();

            board.make_move(m);
            if (MoveGen::is_king_attacked(board, us)) { board.unmake_move(m); continue; }
            legal++;
            bool gives_check = MoveGen::in_check(board);

            int score;
            if (legal == 1) {
                score = -negamax(board, depth - 1, ply + 1, -beta, -alpha, true);
            } else {
                // Late move reductions for quiet moves ordered late
                int r = 0;
                if (depth >= 3 && legal > 3 && quiet && !in_check && !gives_check) {
                    r = LMR.r[std::min(depth, 63)][std::min(legal, 63)];
                    if (pv_node) r--;
                    if (m == killers[ply][0] || m == killers[ply][1]) r--;
                    r = std::clamp(r, 0, depth - 2);
                }
                // Principal variation search: null window first, re-search on surprise
                score = -negamax(board, depth - 1 - r, ply + 1, -alpha - 1, -alpha, true);
                if (score > alpha && r > 0)
                    score = -negamax(board, depth - 1, ply + 1, -alpha - 1, -alpha, true);
                if (score > alpha && score < beta)
                    score = -negamax(board, depth - 1, ply + 1, -beta, -alpha, true);
            }
            board.unmake_move(m);

            if (stop_flag.load(std::memory_order_relaxed)) return 0;

            if (score > best_score) {
                best_score = score;
                best_move = m;
                if (score > alpha) {
                    alpha = score;
                    flag = HashFlag::EXACT;
                    update_pv(ply, m);
                    if (alpha >= beta) {
                        flag = HashFlag::BETA;
                        if (quiet) {
                            if (killers[ply][0] != m) {
                                killers[ply][1] = killers[ply][0];
                                killers[ply][0] = m;
                            }
                            int& h = history[static_cast<int>(us)][static_cast<int>(m.get_from())][static_cast<int>(m.get_to())];
                            h = std::min(h + depth * depth, HISTORY_MAX);
                        }
                        break;
                    }
                }
            }
        }

        if (legal == 0) return in_check ? -MATE_SCORE + ply : 0;

        tt.store(board.hash_key, depth, score_to_tt(best_score, ply), flag, best_move);
        return best_score;
    }

    Move Search::think(Board& board, const SearchLimits& limits, const InfoCallback& on_info) {
        start_time = std::chrono::steady_clock::now();
        stop_flag = false;
        nodes = 0;
        node_limit = limits.nodes;
        info = SearchInfo{};
        tt.new_search();
        for (auto& k : killers) k[0] = k[1] = Move{};
        for (auto& side : history)
            for (auto& from : side)
                for (int& h : from) h /= 2;

        MoveList root_moves = MoveGen::generate_legal_moves(board);
        if (root_moves.empty()) return Move{};

        // Time management: a soft limit decides whether to start another iteration,
        // a hard limit aborts the iteration in progress
        int64_t soft_limit = -1;
        hard_limit_ms = -1;
        int stm = static_cast<int>(board.get_side_to_move());
        if (!limits.infinite) {
            if (limits.movetime > 0) {
                hard_limit_ms = std::max<int64_t>(1, limits.movetime - 10);
            } else if (limits.time[stm] >= 0) {
                int64_t remaining = limits.time[stm];
                int64_t mtg = limits.movestogo > 0 ? std::min(limits.movestogo, 40) : 30;
                int64_t safety = std::min<int64_t>(50, remaining / 10);
                soft_limit = remaining / mtg + limits.inc[stm] * 3 / 4;
                hard_limit_ms = std::min(soft_limit * 4, remaining / 3 + limits.inc[stm]);
                hard_limit_ms = std::max<int64_t>(1, std::min(hard_limit_ms, remaining - safety));
                soft_limit = std::min(soft_limit, hard_limit_ms);
            }
        }

        Move best_move = root_moves[0];
        int max_depth = std::clamp(limits.depth, 1, MAX_PLY - 1);

        for (int depth = 1; depth <= max_depth; depth++) {
            seldepth = 0;
            int score = negamax(board, depth, 0, -INF_SCORE, INF_SCORE, false);

            // The root PV is only written after a root move's subtree finished, so even an
            // aborted iteration's PV move is fully searched (and at least as good as the
            // previous best, which is searched first via the TT move)
            bool aborted = stop_flag.load(std::memory_order_relaxed);
            if (pv_length[0] > 0) best_move = pv_table[0][0];
            if (aborted) break;

            info.depth    = depth;
            info.seldepth = seldepth;
            info.score    = score;
            info.nodes    = nodes;
            info.time_ms  = elapsed_ms();
            info.hashfull = tt.hashfull();
            info.pv.assign(pv_table[0], pv_table[0] + pv_length[0]);
            if (on_info) on_info(info);

            // A forced mate within this depth can't get any better
            if (info.is_mate() && MATE_SCORE - std::abs(score) <= depth) break;
            if (soft_limit >= 0 && info.time_ms >= soft_limit / 2) break;
        }
        return best_move;
    }

    Move Search::search_best_move(Board& board, int depth) {
        SearchLimits limits;
        limits.depth = depth;
        return think(board, limits);
    }
}
