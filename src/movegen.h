
#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "bitboard.h"
#include "position.h"
#include "types.h"

inline Move data[COLOR_NB][5] =
{
    { make_move<CASTLING>(E1, G1), make_move<CASTLING>(E1, C1), NULLMOVE, make_move<CASTLING>(E1, G1), NULLMOVE },
    { make_move<CASTLING>(E8, G8), make_move<CASTLING>(E8, C8), NULLMOVE, make_move<CASTLING>(E8, G8), NULLMOVE }
};

inline Move *table[COLOR_NB][1 << 4][1 << 6];

namespace MoveGen { void init(); };

inline void MoveGen::init()
{
    for (Color c : { WHITE, BLACK })
        for (int rights = 0; rights <= 0xf; rights++)
            for (Bitboard hash = 0; hash <= 0b111111; hash++)
            {
                Move *kcastle   = &data[c][3];
                Move *qcastle   = &data[c][1];
                Move *both      = &data[c][0];
                Move *no_castle = &data[c][2];
                
                Move *src = no_castle;

                bool rights_k = rights & (c == WHITE ? 0b1000 : 0b0010);
                bool rights_q = rights & (c == WHITE ? 0b0100 : 0b0001);
                bool rights_kq = rights_k && rights_q;

                if (rights_k || rights_q)
                {
                    if      (hash == 0)              src = rights_kq ? both : rights_k ? kcastle : qcastle;
                    else if ((hash & 0b000111) == 0) src = rights_k ? kcastle : no_castle;
                    else if ((hash & 0b111100) == 0) src = rights_q ? qcastle : no_castle;
                }

                table[c][rights][hash] = src;
            }
}

template<MoveType Type, Direction D>
Move *make_pawn_moves(Move *list, Bitboard attacks)
{
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
            *list++ = make_move<ROOK_PROMOTION  >(to - D, to);
            *list++ = make_move<QUEEN_PROMOTION >(to - D, to);
        }
    }

    return list;
}

inline Move *make_moves(Move *list, Square from, Bitboard to)
{
    for (;to; clear_lsb(to))
        *list++ = make_move(from, lsb(to));

    return list;
}

template<Color Us>
Move *generate_moves(Move *list)
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
    
    Bitboard enemy_rook_queen   = bb(EnemyQueen) | bb(EnemyRook);
    Bitboard enemy_bishop_queen = bb(EnemyQueen) | bb(EnemyBishop);
    Square   ksq                = lsb(bb(FriendlyKing));
    Bitboard occupied           = Position::occupied() ^ bb(FriendlyKing);
    Bitboard seen_by_enemy      = pawn_attacks<Them>(bb(EnemyPawn)) | king_attacks(lsb(bb(EnemyKing)));

    for (Bitboard b = bb(EnemyKnight);    b; clear_lsb(b)) seen_by_enemy |= knight_attacks(lsb(b));
    for (Bitboard b = enemy_bishop_queen; b; clear_lsb(b)) seen_by_enemy |= bishop_attacks(lsb(b), occupied);
    for (Bitboard b = enemy_rook_queen;   b; clear_lsb(b)) seen_by_enemy |= rook_attacks  (lsb(b), occupied);

    toggle_square(occupied, ksq);

    Bitboard checkmask = knight_attacks(ksq) & bb(EnemyKnight) | pawn_attacks<Us>(ksq) & bb(EnemyPawn);

    for (Bitboard checkers = bishop_attacks(ksq, occupied) & enemy_bishop_queen | rook_attacks(ksq, occupied) & enemy_rook_queen; checkers; clear_lsb(checkers))
        checkmask |= check_ray(ksq, lsb(checkers));

    if (more_than_one(checkmask & double_check(ksq)))
        return make_moves(list, ksq, king_attacks(ksq) & ~(seen_by_enemy | bb(Us)));

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

    list = make_pawn_moves<NORMAL, UpRight>(list, shift<UpRight>(pawns & (~pinned | anti_diag(ksq))) & bb(Them) & checkmask);
    list = make_pawn_moves<NORMAL, UpLeft >(list, shift<UpLeft >(pawns & (~pinned | main_diag(ksq))) & bb(Them) & checkmask);
    list = make_pawn_moves<NORMAL, Up     >(list, shift<Up     >(pawns & (~pinned | file_bb  (ksq))) & empty    & checkmask);
    list = make_pawn_moves<NORMAL, Up2    >(list, shift<Up2    >(pawns & (~pinned | file_bb  (ksq))) & e        & checkmask);

    if (Bitboard promotable = bb(FriendlyPawn) & Rank7)
    {
        list = make_pawn_moves<PROMOTION, UpRight>(list, shift<UpRight>(promotable & (~pinned | anti_diag(ksq))) & bb(Them) & checkmask);
        list = make_pawn_moves<PROMOTION, UpLeft >(list, shift<UpLeft >(promotable & (~pinned | main_diag(ksq))) & bb(Them) & checkmask);
        list = make_pawn_moves<PROMOTION, Up     >(list, shift<Up     >(promotable &  ~pinned                  ) & empty    & checkmask);
    }
 
    if (shift<UpRight>(bb(FriendlyPawn)) & Position::ep_bb() & Rank6)
    {
        *list = make_move<ENPASSANT>(state_ptr->ep_sq - UpRight, state_ptr->ep_sq);
        Bitboard after_ep = occupied ^ square_bb(state_ptr->ep_sq - UpRight, state_ptr->ep_sq - Up, state_ptr->ep_sq);
        list += !(bishop_attacks(ksq, after_ep) & enemy_bishop_queen | rook_attacks(ksq, after_ep) & enemy_rook_queen);
    }
    if (shift<UpLeft>(bb(FriendlyPawn)) & Position::ep_bb() & Rank6)
    {
        *list = make_move<ENPASSANT>(state_ptr->ep_sq - UpLeft, state_ptr->ep_sq);
        Bitboard after_ep = occupied ^ square_bb(state_ptr->ep_sq - UpLeft, state_ptr->ep_sq - Up, state_ptr->ep_sq);
        list += !(bishop_attacks(ksq, after_ep) & enemy_bishop_queen | rook_attacks(ksq, after_ep) & enemy_rook_queen);
    }

    for (Bitboard b = bb(FriendlyKnight) & ~pinned; b; clear_lsb(b))
    {
        Square from = lsb(b);
        list = make_moves(list, from, knight_attacks(from) & checkmask);
    }

    Bitboard bishop_queen = bb(FriendlyBishop) | bb(FriendlyQueen);
    Bitboard rook_queen   = bb(FriendlyRook)   | bb(FriendlyQueen);

    for (Bitboard b = bishop_queen & ~pinned; b; clear_lsb(b))
    {
        Square from = lsb(b);
        list = make_moves(list, from, bishop_attacks(from, occupied) & checkmask);
    }
    for (Bitboard b = bishop_queen & pinned; b; clear_lsb(b))
    {
        Square from = lsb(b);
        list = make_moves(list, from, bishop_attacks(from, occupied) & checkmask & align_mask(ksq, from));
    }
    for (Bitboard b = rook_queen & ~pinned; b; clear_lsb(b))
    {
        Square from = lsb(b);
        list = make_moves(list, from, rook_attacks(from, occupied) & checkmask);
    }
    for (Bitboard b = rook_queen & pinned; b; clear_lsb(b))
    {
        Square from = lsb(b);
        list = make_moves(list, from, rook_attacks(from, occupied) & checkmask & align_mask(ksq, from));
    }

    list = make_moves(list, ksq, king_attacks(ksq) & ~(seen_by_enemy | bb(Us)));

    constexpr int Shift = Us == WHITE ? 1 : 57;

    constexpr Bitboard NoAtk = Us == WHITE ? square_bb(C1, D1, E1, F1, G1) : square_bb(C8, D8, E8, F8, G8);
    constexpr Bitboard NoOcc = Us == WHITE ? square_bb(B1, C1, D1, F1, G1) : square_bb(B8, C8, D8, F8, G8);

    for (Move *src = table[Us][state_ptr->castling_rights][(NoAtk & seen_by_enemy | NoOcc & occupied) >> Shift]; *src; *list++ = *src++);

    return list;
}

#endif

