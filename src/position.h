
#ifndef POSITION_H
#define POSITION_H

#include <string>
#include <cstring>

#include "bitboard.h"
#include "types.h"

#define bb(p) bitboard<p>()

struct StateInfo
{
    Piece   captured;
    Square  ep_sq;
    uint8_t castling_rights;
    Color   side_to_move;
};

extern Bitboard bitboards[16];
extern Piece board[SQUARE_NB];

extern StateInfo state_stack[MAX_PLY], *state_ptr;

template<Piece P>
inline Bitboard bitboard() { return bitboards[P]; }

inline Piece piece_on(Square s) { return board[s]; }

namespace Position
{    
    void set(const std::string& fen);
    void commit_move(Move m);
    std::string fen();
    std::string to_string();

    inline bool white_to_move() { return state_ptr->side_to_move == WHITE; }

    inline Bitboard occupied() { return bitboards[WHITE] | bitboards[BLACK]; }
    
    inline Bitboard ep_bb() { return square_bb(state_ptr->ep_sq); }
}

template<Color JustMoved>
void update_castling_rights()
{
    constexpr Bitboard mask = JustMoved == WHITE ? square_bb(A1, E1, H1, A8, H8) : square_bb(A8, E8, H8, A1, H1);
    state_ptr->castling_rights &= castle_masks[JustMoved][pext(bitboards[JustMoved], mask)];
}

template<Color Us>
void do_move(Move m)
{
    constexpr Color Them  = !Us;

    constexpr Piece Pawn  = make_piece(Us, PAWN);
    constexpr Piece Rook  = make_piece(Us, ROOK);
    constexpr Piece Queen = make_piece(Us, QUEEN);
    constexpr Piece King  = make_piece(Us, KING);

    constexpr Direction Up  = Us == WHITE ? NORTH : SOUTH;
    constexpr Direction Up2 = Up * 2;

    Square from = from_sq(m);
    Square to   = to_sq(m);

    memcpy(state_ptr + 1, state_ptr, sizeof(StateInfo));
    state_ptr++;
    state_ptr->captured = piece_on(to);
    state_ptr->ep_sq = (from + Up) * !(to - from ^ Up2 | piece_on(from) ^ Pawn);

    Bitboard zero_to = ~square_bb(to);
    Bitboard from_to =  square_bb(from, to);

    switch (type_of(m))
    {
    case NORMAL:
        bitboards[board[to]] &= zero_to;
        bitboards[Them] &= zero_to;
        bitboards[board[from]] ^= from_to;
        bitboards[Us] ^= from_to;

        board[to] = board[from];
        board[from] = NO_PIECE;

        update_castling_rights<Us>();
        
        return;
    case PROMOTION:
    {
        Piece promotion = make_piece(Us, promotion_type(m));
        
        bitboards[board[to]] &= zero_to;
        bitboards[Them] &= zero_to;
        bitboards[Pawn] ^= square_bb(from);
        bitboards[promotion] ^= ~zero_to;
        bitboards[Us] ^= from_to;

        board[to] = promotion;
        board[from] = NO_PIECE;
        
        update_castling_rights<Us>();
        
        return;
    }
    case CASTLING:
    {
        Move rook_move = Us == WHITE ? to == G1 ? make_move(H1, F1)
                                                : make_move(A1, D1)
                                     : to == G8 ? make_move(H8, F8)
                                                : make_move(A8, D8);
                
        Square rook_from = from_sq(rook_move), rook_to = to_sq(rook_move);        
        Bitboard rook_from_to = square_bb(rook_from, rook_to);

        bitboards[King] ^= from_to;
        bitboards[Rook] ^= rook_from_to;
        bitboards[Us] ^= from_to ^ rook_from_to;

        board[from] = NO_PIECE;
        board[rook_from] = NO_PIECE;
        board[to] = King;
        board[rook_to] = Rook;

        update_castling_rights<Us>();

        return;
    }
    case ENPASSANT:
        constexpr Piece EnemyPawn = make_piece(Them, PAWN);

        Square capsq = to + (Us == WHITE ? SOUTH : NORTH);

        bitboards[Pawn] ^= from_to;
        bitboards[EnemyPawn] ^= square_bb(capsq);
        bitboards[Us] ^= from_to;
        bitboards[Them] ^= square_bb(capsq);

        board[from] = NO_PIECE;
        board[to] = Pawn;
        board[capsq] = NO_PIECE;

        return;
    }
}

template<Color Us>
void undo_move(Move m)
{
    constexpr Color Them  = !Us;

    constexpr Piece Pawn  = make_piece(Us, PAWN);
    constexpr Piece Rook  = make_piece(Us, ROOK);
    constexpr Piece Queen = make_piece(Us, QUEEN);
    constexpr Piece King  = make_piece(Us, KING);

    Piece captured = state_ptr->captured;

    state_ptr--;

    Square from = from_sq(m);
    Square to   = to_sq(m);

    Bitboard to_bb      = square_bb(to);
    Bitboard from_to    = square_bb(from, to);
    Bitboard capture_bb = to_bb * bool(captured);

    switch (type_of(m))
    {
    case NORMAL:
        bitboards[board[to]] ^= from_to;
        bitboards[Us] ^= from_to;
        bitboards[captured] ^= capture_bb;
        bitboards[Them] ^= capture_bb;
        
        board[from] = board[to];
        board[to] = captured;
        
        return;
    case PROMOTION:
        bitboards[board[to]] ^= to_bb;
        bitboards[Pawn] ^= square_bb(from);
        bitboards[Us] ^= from_to;
        bitboards[captured] ^= capture_bb;
        bitboards[Them] ^= capture_bb;
        
        board[to] = captured;
        board[from] = Pawn;
        
        return;
    case CASTLING:
    {
        Move rook_move = Us == WHITE ? to == G1 ? make_move(H1, F1)
                                                : make_move(A1, D1)
                                     : to == G8 ? make_move(H8, F8)
                                                : make_move(A8, D8);
        
        Square rook_from = from_sq(rook_move), rook_to = to_sq(rook_move);        
        Bitboard rook_from_to = square_bb(rook_from, rook_to);

        bitboards[King] ^= from_to;
        bitboards[Rook] ^= rook_from_to;
        bitboards[Us] ^= from_to ^ rook_from_to;
        
        board[to] = NO_PIECE;
        board[rook_to] = NO_PIECE;
        board[from] = King;
        board[rook_from] = Rook;
        
        return;
    }
    case ENPASSANT:
        constexpr Piece EnemyPawn = make_piece(Them, PAWN);

        Square capsq = to + (Us == WHITE ? SOUTH : NORTH);

        bitboards[Pawn] ^= from_to;
        bitboards[Us] ^= from_to;
        bitboards[EnemyPawn] ^= square_bb(capsq);
        bitboards[Them] ^= square_bb(capsq);
        
        board[to] = NO_PIECE;
        board[from] = Pawn;
        board[capsq] = EnemyPawn;
        
        return;
    }
}

#endif

