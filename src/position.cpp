
#include "position.h"

#include <cstring>
#include <sstream>

#include "bitboard.h"
#include "uci.h"

Bitboard bitboards[16];
Piece board[SQUARE_NB];

StateInfo state_stack[MAX_PLY], *state_ptr = state_stack;

std::string piece_to_char = "  PNBRQK  pnbrqk";

void Position::set(const std::string& fen)
{    
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

    state_ptr->side_to_move = color == "w" ? WHITE : BLACK;

    state_ptr->castling_rights = state_ptr->ep_sq = 0;

    for (char token : castling)
        if (size_t idx = std::string("qkQK").find(token); idx != std::string::npos)
            state_ptr->castling_rights |= 1 << idx;

    if (enpassant != "-")
        state_ptr->ep_sq = uci_to_square(enpassant);
}

std::string Position::to_string()
{
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

std::string Position::fen()
{
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

    fen << " " << "wb"[state_ptr->side_to_move] << " ";

    if (!state_ptr->castling_rights)
        fen << "-";
    else
    {
        if (state_ptr->castling_rights & 0b1000) fen << "K";
        if (state_ptr->castling_rights & 0b0100) fen << "Q";
        if (state_ptr->castling_rights & 0b0010) fen << "k";
        if (state_ptr->castling_rights & 0b0001) fen << "q";
    }
  
    fen << " " << (state_ptr->ep_sq ? square_to_uci(state_ptr->ep_sq) : "-");

    return fen.str();
}

void Position::commit_move(Move m)
{
    if (white_to_move()) do_move<WHITE>(m);
    else                 do_move<BLACK>(m);

    memcpy(state_stack, state_ptr, sizeof(StateInfo));
    state_ptr = state_stack;
    state_ptr->side_to_move = !state_ptr->side_to_move;
}
