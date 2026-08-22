#include "search.hpp"
#include "eval.hpp"
#include "utils.hpp"
#include <iostream>
#include <algorithm>

namespace engine {
    int Search::score_move(const Board& board, const Move& move, Move tt_move) {
        if (move.data == tt_move.data) return 20000; // TT move is best
        
        // MVV-LVA (Simplified)
        Piece victim = Piece::NONE;
        Color them = (board.get_side_to_move() == Color::WHITE) ? Color::BLACK : Color::WHITE;
        for(int i=0; i<6; i++) {
            if(get_bit(board.get_pieces(static_cast<Piece>(i), them), move.get_to())) {
                victim = static_cast<Piece>(i);
                break;
            }
        }
        
        if (victim != Piece::NONE) {
            Piece attacker = Piece::PAWN; // simplified for scaffold
            for(int i=0; i<6; i++) {
                if(get_bit(board.get_pieces(static_cast<Piece>(i), board.get_side_to_move()), move.get_from())) {
                    attacker = static_cast<Piece>(i);
                    break;
                }
            }
            int v_score = (static_cast<int>(victim) + 1) * 100;
            int a_score = static_cast<int>(attacker) + 1;
            return 10000 + v_score - a_score;
        }
        
        return 0; // Quiet move
    }

    int Search::negamax(Board& board, int depth, int alpha, int beta) {
        int orig_alpha = alpha;
        
        Move tt_move{0};
        int tt_score;
        if (tt.probe(board.hash_key, depth, alpha, beta, tt_score, tt_move)) {
            return tt_score;
        }

        if (depth == 0) {
            return Evaluator::evaluate(board);
        }
        
        std::vector<Move> moves = MoveGen::generate_legal_moves(board);
        if (moves.empty()) {
            Color us = board.get_side_to_move();
            Color them = (us == Color::WHITE) ? Color::BLACK : Color::WHITE;
            U64 king_bb = board.get_pieces(Piece::KING, us);
            bool in_check = false;
            if (king_bb) {
                Square king_sq = static_cast<Square>(lsb(king_bb));
                in_check = MoveGen::is_square_attacked(board, king_sq, them);
            }
            if (in_check) {
                return -20000 - depth; // checkmate; larger remaining depth = faster mate
            }
            return 0; // stalemate
        }
        
        // Move Ordering
        std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
            return score_move(board, a, tt_move) > score_move(board, b, tt_move);
        });
        
        int max_val = -30000;
        Move best_move = moves[0];
        
        for (const Move& move : moves) {
            board.make_move(move);
            int score = -negamax(board, depth - 1, -beta, -alpha);
            board.unmake_move(move);
            
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

    Move Search::search_best_move(Board& board, int depth) {
        std::vector<Move> moves = MoveGen::generate_legal_moves(board);
        if (moves.empty()) return Move{0};
        
        std::cout << "AI searching depth " << depth << "..." << std::endl;
        
        Move best_move = moves[0];
        int best_score = -30000;
        
        // Simplified root search
        for (const Move& move : moves) {
            board.make_move(move);
            int score = -negamax(board, depth - 1, -30000, 30000);
            board.unmake_move(move);
            
            if (score > best_score) {
                best_score = score;
                best_move = move;
            }
        }
        
        std::cout << "AI chose move from " << (int)best_move.get_from() << " to " << (int)best_move.get_to() << std::endl;
        return best_move;
    }
}
