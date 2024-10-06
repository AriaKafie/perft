
#include "bitboard.h"

#include <algorithm>

void init_magics();

static Bitboard attacks_bb(PieceType pt, Square sq, Bitboard occupied)
{
    Direction rook_directions[4] = { NORTH, EAST, SOUTH, WEST };
    Direction bishop_directions[4] = { NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST };

    Bitboard atk = 0;

    for (Direction d : pt == ROOK ? rook_directions : bishop_directions)
        for (Square s = sq; safe_step(s, d) && !(square_bb(s) & occupied); atk |= square_bb(s += d));
    
    return atk;
}

void Bitboards::init()
{
    for (Square s1 = H1; s1 <= A8; s1++)
    {
        FileBB[s1] = FILE_H << (s1 % 8);

        for (Square s2 = H1; s2 <= A8; s2++)
            SquareDistance[s1][s2] = std::max(file_distance(s1, s2), rank_distance(s1, s2));
    }

    init_magics();
    
    Bitboard* table_ptr = xray_table;

    for (PieceType pt : { BISHOP, ROOK })
    {
        Bitboard* masks = pt == BISHOP ? bishop_masks : rook_masks;

        for (Square s = H1; s <= A8; s++)
            for (Bitboard occupied = 0, i = 0; i < 1 << popcount(masks[s]); occupied = generate_occupancy(masks[s], ++i))
                *table_ptr++ = attacks_bb(pt, s, occupied ^ (attacks_bb(pt, s, occupied) & occupied));
    }

    for (Square s1 = H1; s1 <= A8; s1++)
    {
        MainDiag[s1] = bishop_attacks(s1, 0) & (mask(s1, NORTH_WEST) | mask(s1, SOUTH_EAST)) | square_bb(s1);
        AntiDiag[s1] = bishop_attacks(s1, 0) & (mask(s1, NORTH_EAST) | mask(s1, SOUTH_WEST)) | square_bb(s1);
        
        CenterDistance[s1] = std::min({ rank_distance(s1, E4) + file_distance(s1, E4),
                                        rank_distance(s1, E5) + file_distance(s1, E5),
                                        rank_distance(s1, D4) + file_distance(s1, D4),
                                        rank_distance(s1, D5) + file_distance(s1, D5) });

        for (Square s2 = H1; s2 <= A8; s2++)
            if (PieceType pt; attacks_bb(pt=BISHOP, s1, 0) & square_bb(s2) || attacks_bb(pt=ROOK, s1, 0) & square_bb(s2))
                AlignMask[s1][s2] = attacks_bb(pt, s1, 0) & attacks_bb(pt, s2, 0) | square_bb(s1, s2);
                    
        for (Square ksq = s1, checker = H1; checker <= A8; checker++)
        {
            for (Direction d : { NORTH, NORTH_EAST, EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, WEST, NORTH_WEST })
            {
                Bitboard ray = 0;

                for (Square s = ksq; safe_step(s, d) && !(square_bb(s) & square_bb(checker)); ray |= square_bb(s += d));

                if (ray & square_bb(checker))
                    CheckRay[ksq][checker] = ray;
            }
        }

        for (Direction d : { NORTH, NORTH_EAST, EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, WEST, NORTH_WEST })
            KingAttacks[s1] |= safe_step(s1, d);

        for (Direction d : { NORTH+NORTH_EAST, NORTH_EAST+EAST, SOUTH_EAST+EAST, SOUTH+SOUTH_EAST,
                             SOUTH+SOUTH_WEST, SOUTH_WEST+WEST, NORTH_WEST+WEST, NORTH+NORTH_WEST })
            KnightAttacks[s1] |= safe_step(s1, d);

        DoubleCheck[s1] = KingAttacks[s1] | KnightAttacks[s1];

        PawnAttacks[WHITE][s1] = pawn_attacks<WHITE>(square_bb(s1));
        PawnAttacks[BLACK][s1] = pawn_attacks<BLACK>(square_bb(s1));
    }
    
    uint8_t clearK = 0b0111;
    uint8_t clearQ = 0b1011;
    uint8_t cleark = 0b1101;
    uint8_t clearq = 0b1110;

    for (int i = 0; i < 1 << 5; i++)
    {
        Bitboard w_occ = generate_occupancy(square_bb(A1, E1, H1, A8, H8), i);
        Bitboard b_occ = generate_occupancy(square_bb(A8, E8, H8, A1, H1), i);

        uint8_t w_rights = 0b1111;
        uint8_t b_rights = 0b1111;

        if ((w_occ & square_bb(A1)) == 0) w_rights &= clearQ;
        if ((w_occ & square_bb(E1)) == 0) w_rights &= clearK & clearQ;
        if ((w_occ & square_bb(H1)) == 0) w_rights &= clearK;
        if ((w_occ & square_bb(A8)) != 0) w_rights &= clearq;
        if ((w_occ & square_bb(H8)) != 0) w_rights &= cleark;

        if ((b_occ & square_bb(A8)) == 0) b_rights &= clearq;
        if ((b_occ & square_bb(E8)) == 0) b_rights &= cleark & clearq;
        if ((b_occ & square_bb(H8)) == 0) b_rights &= cleark;
        if ((b_occ & square_bb(A1)) != 0) b_rights &= clearQ;
        if ((b_occ & square_bb(H1)) != 0) b_rights &= clearK;

        castle_masks[WHITE][pext(w_occ, square_bb(A1, E1, H1, A8, H8))] = w_rights;
        castle_masks[BLACK][pext(b_occ, square_bb(A8, E8, H8, A1, H1))] = b_rights;
    }
}

void init_magics() {

    Bitboard* table_ptr = pext_table;

    for (PieceType pt : { BISHOP, ROOK })
    {
        int*      base = pt == BISHOP ? bishop_base  : rook_base;
        Bitboard* mask = pt == BISHOP ? bishop_masks : rook_masks;

        for (Square s = H1; s <= A8; s++)
        {
            base[s] = table_ptr - pext_table;
            mask[s] = attacks_bb(pt, s, 0) & ~((FILE_A | FILE_H) & ~file_bb(s) | (RANK_1 | RANK_8) & ~rank_bb(s));

            for (int i = 0; i < 1 << popcount(mask[s]); i++)
                *table_ptr++ = attacks_bb(pt, s, generate_occupancy(mask[s], i));
        }
    }
}


