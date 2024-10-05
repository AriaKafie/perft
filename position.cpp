
#include "position.h"

#include <cstring>
#include <sstream>

#include "bitboard.h"
#include "uci.h"

std::string piece_to_char = "  PNBRQK  pnbrqk";

void Position::set(const std::string& fen) {

rft[G1] = make_move(H1, F1);
rft[C1] = make_move(A1, D1);
rft[G8] = make_move(H8, F8);
rft[C8] = make_move(A8, D8);
    
    memset(board, NO_PIECE, sizeof(board));
    memset(bitboards, 0ull, sizeof(bitboards));

    Square             sq = A8;
    std::istringstream is(fen);
    std::string        pieces, color, castling, enpassant;

    is >> pieces >> color >> castling >> enpassant;

    for (char token : pieces)
    {
        if (std::isdigit(token))
            sq -= token - '0'; 
        else if (size_t piece = piece_to_char.find(token); piece != std::string::npos)
        {
            board[sq] = piece;
            bitboards[piece] |= square_bb(sq);
            bitboards[color_of(piece)] |= square_bb(sq);
            sq--;
        }
    }

    side_to_move = color == "w" ? WHITE : BLACK;

    state_ptr->castling_rights = state_ptr->ep_sq = 0;

    for (char token : castling)
        if (size_t idx = std::string("qkQK").find(token); idx != std::string::npos)
            state_ptr->castling_rights |= 1 << idx;

    if (enpassant != "-")
        state_ptr->ep_sq = uci_to_square(enpassant);
}

std::string Position::to_string() {

    std::stringstream ss;

    ss << "\n+---+---+---+---+---+---+---+---+\n";

    for (Square sq = A8; sq >= H1; sq--)
    {
        ss << "| " << piece_to_char[board[sq]] << " ";

        if (sq % 8 == 0)
            ss << "| " << (sq / 8 + 1) << "\n+---+---+---+---+---+---+---+---+\n";
    }

    return ss.str() + "  a   b   c   d   e   f   g   h\n\n" + fen() + "\n";
}

std::string Position::fen() {

    std::stringstream fen;

    for (int rank = 7; rank >= 0; rank--)
    {
        for (int file = 7; file >= 0; file--)
        {
            if (Piece pc = piece_on(rank * 8 + file))
                fen << piece_to_char[pc];
            else
            {
                int empty = 0, f;

                for (f = file; f >= 0 && !piece_on(rank * 8 + f); f--)
                    empty++;

                fen << empty;

                file = f + 1;
            }
        }

        if (rank)
            fen << "/";
    }

    fen << " " << "wb"[side_to_move] << " ";

    if (!state_ptr->castling_rights)
        fen << "-";
    else
    {
        if (kingside_rights <WHITE>()) fen << "K";
        if (queenside_rights<WHITE>()) fen << "Q";
        if (kingside_rights <BLACK>()) fen << "k";
        if (queenside_rights<BLACK>()) fen << "q";
    }
  
    fen << " " << (state_ptr->ep_sq ? square_to_uci(state_ptr->ep_sq) : "-");

    return fen.str();
}
