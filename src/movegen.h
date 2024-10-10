
#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "bitboard.h"
#include "position.h"
#include "types.h"

template<Color Us>
class MoveList {
 public:
    MoveList();
    Move* begin() { return moves; }
    Move* end() { return last; }
    size_t size() const { return last - moves; }    
 private:
    Move moves[128], *last = moves;
};

template<MoveType Type, Direction D>
Move* make_pawn_moves(Move* list, Bitboard attacks) {

    if constexpr (Type == NORMAL)
    {
        for (;attacks; clear_lsb(attacks))
        {
            Square to = lsb(attacks);
            *list++ = make_move(to - D, to);
        }
    }
    else if constexpr (Type == PROMOTION)
    {
        for (;attacks; clear_lsb(attacks))
        {
            Square to = lsb(attacks);
            *list++ = make_move<KNIGHT_PROMOTION>(to - D, to);
            *list++ = make_move<BISHOP_PROMOTION>(to - D, to);
            *list++ = make_move<ROOK_PROMOTION>(to - D, to);
            *list++ = make_move<QUEEN_PROMOTION>(to - D, to);
        }
    }

    return list;
}

inline Move* make_moves(Move* list, Square from, Bitboard to) {

    for (;to; clear_lsb(to))
        *list++ = make_move(from, lsb(to));

    return list;
}

template<Color Us>
MoveList<Us>::MoveList()
{
    constexpr Color Them           = !Us;
    constexpr Piece FriendlyPawn   = make_piece(Us,   PAWN);
    constexpr Piece EnemyPawn      = make_piece(Them, PAWN);
    constexpr Piece FriendlyKnight = make_piece(Us,   KNIGHT);
    constexpr Piece EnemyKnight    = make_piece(Them, KNIGHT);
    constexpr Piece FriendlyBishop = make_piece(Us,   BISHOP);
    constexpr Piece EnemyBishop    = make_piece(Them, BISHOP);
    constexpr Piece FriendlyRook   = make_piece(Us,   ROOK);
    constexpr Piece EnemyRook      = make_piece(Them, ROOK);
    constexpr Piece FriendlyQueen  = make_piece(Us,   QUEEN);
    constexpr Piece EnemyQueen     = make_piece(Them, QUEEN);
    constexpr Piece FriendlyKing   = make_piece(Us,   KING);
    constexpr Piece EnemyKing      = make_piece(Them, KING);

    Bitboard seen_by_enemy      = pawn_attacks<Them>(bb(EnemyPawn)) | king_attacks(lsb(bb(EnemyKing)));
    Bitboard enemy_rook_queen   = bb(EnemyQueen) | bb(EnemyRook);
    Bitboard enemy_bishop_queen = bb(EnemyQueen) | bb(EnemyBishop);
    Square   ksq                = lsb(bb(FriendlyKing));
    Bitboard occupied           = occupied_bb() ^ square_bb(ksq);

    for (Bitboard b = bb(EnemyKnight);    b; clear_lsb(b)) seen_by_enemy |= knight_attacks(lsb(b));
    for (Bitboard b = enemy_bishop_queen; b; clear_lsb(b)) seen_by_enemy |= bishop_attacks(lsb(b), occupied);
    for (Bitboard b = enemy_rook_queen;   b; clear_lsb(b)) seen_by_enemy |= rook_attacks  (lsb(b), occupied);

    toggle_square(occupied, ksq);

    Bitboard checkmask = knight_attacks(ksq) & bb(EnemyKnight) | pawn_attacks<Us>(ksq) & bb(EnemyPawn);

    for (Bitboard checkers = bishop_attacks(ksq, occupied) & enemy_bishop_queen | rook_attacks(ksq, occupied) & enemy_rook_queen; checkers; clear_lsb(checkers))
        checkmask |= check_ray(ksq, lsb(checkers));

    if (more_than_one(checkmask & double_check(ksq)))
    {
        last = make_moves(last, ksq, king_attacks(ksq) & ~(seen_by_enemy | bb(Us)));
        return;
    }

    checkmask = (checkmask | -!checkmask) & ~bb(Us);

    Bitboard pinned = 0;

    for (Bitboard pinners = bishop_xray(ksq, occupied) & enemy_bishop_queen | rook_xray(ksq, occupied) & enemy_rook_queen; pinners; clear_lsb(pinners))
        pinned |= check_ray(ksq, lsb(pinners));

    constexpr Direction Up      = Us == WHITE ? NORTH      : SOUTH;
    constexpr Direction Up2     = Us == WHITE ? NORTH * 2  : SOUTH * 2;
    constexpr Direction UpRight = Us == WHITE ? NORTH_EAST : SOUTH_WEST;
    constexpr Direction UpLeft  = Us == WHITE ? NORTH_WEST : SOUTH_EAST;
    constexpr Bitboard  Rank3   = Us == WHITE ? RANK_3     : RANK_6;
    constexpr Bitboard  Rank6   = Us == WHITE ? RANK_6     : RANK_3;
    constexpr Bitboard  Rank7   = Us == WHITE ? RANK_7     : RANK_2;

    Bitboard pawns = bb(FriendlyPawn) & ~Rank7;
    Bitboard empty = ~occupied;
    Bitboard e     = shift<Up>(Rank3 & empty) & empty;

    last = make_pawn_moves<NORMAL, UpRight>(last, shift<UpRight>(pawns & (~pinned | anti_diag(ksq))) & bb(Them) & checkmask);
    last = make_pawn_moves<NORMAL, UpLeft >(last, shift<UpLeft >(pawns & (~pinned | main_diag(ksq))) & bb(Them) & checkmask);
    last = make_pawn_moves<NORMAL, Up     >(last, shift<Up     >(pawns & (~pinned | file_bb  (ksq))) & empty    & checkmask);
    last = make_pawn_moves<NORMAL, Up2    >(last, shift<Up2    >(pawns & (~pinned | file_bb  (ksq))) & e        & checkmask);

    if (Bitboard promotable = bb(FriendlyPawn) & Rank7)
    {
        last = make_pawn_moves<PROMOTION, UpRight>(last, shift<UpRight>(promotable & (~pinned | anti_diag(ksq))) & bb(Them) & checkmask);
        last = make_pawn_moves<PROMOTION, UpLeft >(last, shift<UpLeft >(promotable & (~pinned | main_diag(ksq))) & bb(Them) & checkmask);
        last = make_pawn_moves<PROMOTION, Up     >(last, shift<Up     >(promotable &  ~pinned                  ) & empty    & checkmask);
    }
 
    if (shift<UpRight>(bb(FriendlyPawn)) & Position::ep_bb() & Rank6)
    {
        *last = make_move<ENPASSANT>(state_ptr->ep_sq - UpRight, state_ptr->ep_sq);
        Bitboard after_ep = occupied ^ square_bb(state_ptr->ep_sq - UpRight, state_ptr->ep_sq - Up, state_ptr->ep_sq);
        last += !(bishop_attacks(ksq, after_ep) & enemy_bishop_queen | rook_attacks(ksq, after_ep) & enemy_rook_queen);
    }
    if (shift<UpLeft>(bb(FriendlyPawn)) & Position::ep_bb() & Rank6)
    {
        *last = make_move<ENPASSANT>(state_ptr->ep_sq - UpLeft, state_ptr->ep_sq);
        Bitboard after_ep = occupied ^ square_bb(state_ptr->ep_sq - UpLeft, state_ptr->ep_sq - Up, state_ptr->ep_sq);
        last += !(bishop_attacks(ksq, after_ep) & enemy_bishop_queen | rook_attacks(ksq, after_ep) & enemy_rook_queen);
    }

    for (Bitboard b = bb(FriendlyKnight) & ~pinned; b; clear_lsb(b))
    {
        Square from = lsb(b);
        last = make_moves(last, from, knight_attacks(from) & checkmask);
    }

    Bitboard bishop_queen = bb(FriendlyBishop) | bb(FriendlyQueen);
    Bitboard rook_queen   = bb(FriendlyRook)   | bb(FriendlyQueen);

    for (Bitboard b = bishop_queen & ~pinned; b; clear_lsb(b))
    {
        Square from = lsb(b);
        last = make_moves(last, from, bishop_attacks(from, occupied) & checkmask);
    }
    for (Bitboard b = bishop_queen & pinned; b; clear_lsb(b))
    {
        Square from = lsb(b);
        last = make_moves(last, from, bishop_attacks(from, occupied) & checkmask & align_mask(ksq, from));
    }
    for (Bitboard b = rook_queen & ~pinned; b; clear_lsb(b))
    {
        Square from = lsb(b);
        last = make_moves(last, from, rook_attacks(from, occupied) & checkmask);
    }
    for (Bitboard b = rook_queen & pinned; b; clear_lsb(b))
    {
        Square from = lsb(b);
        last = make_moves(last, from, rook_attacks(from, occupied) & checkmask & align_mask(ksq, from));
    }

    last = make_moves(last, ksq, king_attacks(ksq) & ~(seen_by_enemy | bb(Us)));

    constexpr Bitboard k_no_atk = Us == WHITE ? square_bb(E1, F1, G1) : square_bb(E8, F8, G8);
    constexpr Bitboard k_no_occ = Us == WHITE ? square_bb(F1, G1)     : square_bb(F8, G8);
    constexpr Bitboard q_no_atk = Us == WHITE ? square_bb(C1, D1, E1) : square_bb(C8, D8, E8);
    constexpr Bitboard q_no_occ = Us == WHITE ? square_bb(B1, C1, D1) : square_bb(B8, C8, D8);

    constexpr Move SCASTLE = Us == WHITE ? make_move<CASTLING>(E1, G1) : make_move<CASTLING>(E8, G8);
    constexpr Move LCASTLE = Us == WHITE ? make_move<CASTLING>(E1, C1) : make_move<CASTLING>(E8, C8);

    *last = SCASTLE;
    last += !((seen_by_enemy & k_no_atk | occupied & k_no_occ | !Position::kingside_rights <Us>()));

    *last = LCASTLE;
    last += !((seen_by_enemy & q_no_atk | occupied & q_no_occ | !Position::queenside_rights<Us>()));
}

#endif

