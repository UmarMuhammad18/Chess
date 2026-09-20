import Foundation

/// Thin Swift wrapper around the C API in `chess_c_api.h`.
/// Add `chess_c_api.h` to the app's bridging header, and compile/link the C++ sources
/// or `libchess_core.a`.
final class ChessEngine {
    private var ptr: OpaquePointer?

    init() {
        ptr = chess_create()
    }

    deinit {
        if let ptr { chess_destroy(ptr) }
    }

    func loadFen(_ fen: String) -> Bool {
        guard let ptr else { return false }
        return fen.withCString { chess_load_fen(ptr, $0) != 0 }
    }

    /// 0 white, 1 black
    var sideToMove: Int {
        guard let ptr else { return 0 }
        return Int(chess_side_to_move(ptr))
    }

    func legalUciMoves() -> [String] {
        guard let ptr else { return [] }
        var buf = [CChar](repeating: 0, count: 4096)
        let n = chess_legal_moves(ptr, &buf, Int32(buf.count))
        if n <= 0 { return [] }
        let s = String(cString: buf)
        return s.split(separator: " ").map(String.init)
    }

    func makeUci(_ uci: String) -> Bool {
        guard let ptr else { return false }
        return uci.withCString { chess_make_uci(ptr, $0) != 0 }
    }

    /// Returns (uci, scoreCp).
    func search(movetimeMs: Int = 1000) -> (String, Int)? {
        guard let ptr else { return nil }
        var buf = [CChar](repeating: 0, count: 16)
        let score = chess_search(ptr, Int32(movetimeMs), &buf, Int32(buf.count))
        if score == Int32.min { return nil }
        return (String(cString: buf), Int(score))
    }

    /// 0 ongoing, 1 white mates, 2 black mates, 3 draw/other
    var resultCode: Int {
        guard let ptr else { return 3 }
        return Int(chess_result(ptr))
    }
}
