#include "movegen.h"
#include <vector>
#include <algorithm>


// Magic numbers for rooks and bishops
const uint64_t rook_magics[64] = {
    0x2480001240002088ULL,
    0x3402004b0004000ULL,
    0x23000900106000c0ULL,
    0x8100060850002100ULL,
    0x200102008240201ULL,
    0x1000300181c0002ULL,
    0x2100428100020004ULL,
    0x4480010008204080ULL,
    0xd0008002c0026280ULL,
    0xc0010040008102a0ULL,
    0x2005020420180ULL,
    0x800800809000ULL,
    0xc01000803000c10ULL,
    0x452001002000408ULL,
    0x8041000b00020004ULL,
    0x4001000440822100ULL,
    0x808000314000ULL,
    0x12008802081c000ULL,
    0x21010010200040ULL,
    0x420a020008306044ULL,
    0x810808004010800ULL,
    0x2021010006040028ULL,
    0x8340010210208ULL,
    0x204020000610394ULL,
    0x208080024010ULL,
    0x60028480400661ULL,
    0x8910d0c200208200ULL,
    0x20210100081000ULL,
    0x128000880800400ULL,
    0x24000202001008ULL,
    0x1040100400821918ULL,
    0x3c00102000080c4ULL,
    0x400088418000a0ULL,
    0x404000b000200800ULL,
    0x210a046042001281ULL,
    0x40801000801800ULL,
    0x41510005000801ULL,
    0x200100200480cULL,
    0x4800500814000102ULL,
    0x200004406000085ULL,
    0x904000288000ULL,
    0x2000402010044008ULL,
    0x2410020010012ULL,
    0x201880010008080ULL,
    0x20450008010010ULL,
    0x89000c00190002ULL,
    0x118100208040001ULL,
    0x2008241020004ULL,
    0x5800800460510100ULL,
    0x1022008041023200ULL,
    0x4000a28042081200ULL,
    0x2010900108028380ULL,
    0x2588040008018080ULL,
    0x14960080040080ULL,
    0xb0c8800a00010080ULL,
    0x48401004600ULL,
    0x4c30c080082101ULL,
    0x120400100211081ULL,
    0x240200100a85041ULL,
    0x75001000620805ULL,
    0x5200302008040aULL,
    0x8001000400680a81ULL,
    0x800f8210010804ULL,
    0x800122402810042ULL,
};

const uint64_t bishop_magics[64] = {
    0x9081010420022ULL,
    0x484080a082140ULL,
    0x1008028102088210ULL,
    0x914040480086001ULL,
    0x4030800400000ULL,
    0x11040240002000ULL,
    0x8485404208280ULL,
    0x8053008800880c88ULL,
    0x10020a8208420400ULL,
    0x20401060204ULL,
    0x20a8081100ea80ULL,
    0x82080200000ULL,
    0x8100a0210088800ULL,
    0x1000019010082002ULL,
    0x420401480a1001ULL,
    0x160020a0a020a84ULL,
    0x20804808108080ULL,
    0x2020008ac2820200ULL,
    0x5010200100420040ULL,
    0x204060802102001ULL,
    0x3010090400800ULL,
    0xc004408200422000ULL,
    0x924100904825012ULL,
    0x2500181c41010ULL,
    0x10106040028200ULL,
    0x8060004548800ULL,
    0x200044002802ac00ULL,
    0xc104004800a0a0ULL,
    0x880840000802005ULL,
    0x2081000080e024ULL,
    0x1408013001808804ULL,
    0x8003408a01108804ULL,
    0x130210c084100210ULL,
    0x1100220080800ULL,
    0x304002400020402ULL,
    0x3040c800008200ULL,
    0x89080200042200ULL,
    0x2008301020041ULL,
    0xe0020803046a0088ULL,
    0x26801002080ca02ULL,
    0x800900821008805ULL,
    0x800540208c0200aULL,
    0x2100340208000100ULL,
    0x20840c208000080ULL,
    0x14001c0192000400ULL,
    0x801126080a020440ULL,
    0x8c20040322420200ULL,
    0x1042102001040ULL,
    0x100c00b424200510ULL,
    0xd41820490042000ULL,
    0xe00060046281022ULL,
    0x20a80804ULL,
    0x84002822000ULL,
    0x1400810500a2000ULL,
    0x2161028408008900ULL,
    0x4040c4806002022ULL,
    0x2026820800840500ULL,
    0x8088042284042018ULL,
    0x24a821048441001ULL,
    0x120020000208808ULL,
    0x1080048930400ULL,
    0x8200021020080920ULL,
    0x1400408008900ULL,
    0x9020812080200acULL,
};

// How many bits are needed for each square
const int bishop_table_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

const int rook_table_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};


Magic RookDatabase[64];
Magic BishopDatabase[64];

// Das finale, flache Array für alle Züge (107'648 Einträge)
uint64_t GiantAttackStorage[107648];

uint64_t mask_rook(int square) {
    uint64_t mask = 0ULL;
    int target_r = square / 8;
    int target_c = square % 8;

    // North (Up) - Stop before rank 7
    for (int r = target_r + 1; r < 7; r++) mask |= (1ULL << (r * 8 + target_c));
    
    // South (Down) - Stop before rank 0
    for (int r = target_r - 1; r > 0; r--) mask |= (1ULL << (r * 8 + target_c));
    
    // East (Right) - Stop before file 7
    for (int c = target_c + 1; c < 7; c++) mask |= (1ULL << (target_r * 8 + c));
    
    // West (Left) - Stop before file 0
    for (int c = target_c - 1; c > 0; c--) mask |= (1ULL << (target_r * 8 + c));

    return mask;
}


uint64_t mask_bishop(int square) {
    uint64_t mask = 0ULL;
    int target_r = square / 8;
    int target_c = square % 8;

    // Northeast
    for (int r = target_r + 1, c = target_c + 1; r < 7 && c < 7; r++, c++) 
        mask |= (1ULL << (r * 8 + c));
        
    // Southeast
    for (int r = target_r - 1, c = target_c + 1; r > 0 && c < 7; r--, c++) 
        mask |= (1ULL << (r * 8 + c));
        
    // Northwest
    for (int r = target_r + 1, c = target_c - 1; r < 7 && c > 0; r++, c--) 
        mask |= (1ULL << (r * 8 + c));
        
    // Southwest
    for (int r = target_r - 1, c = target_c - 1; r > 0 && c > 0; r--, c--) 
        mask |= (1ULL << (r * 8 + c));

    return mask;
}


int count_bits(uint64_t bitboard) {
#if defined(_MSC_VER)
    return __popcnt64(bitboard); // Microsoft-Variante
#else
    return __builtin_popcountll(bitboard); // GCC/Clang-Variante
#endif
}


uint64_t generate_blockers(int index, uint64_t mask) {
    uint64_t blockers = 0ULL;
    int bit_count = 0;
    
    for (int sq = 0; sq < 64; ++sq) {
        if (mask & (1ULL << sq)) {
            // If the bit_count-th bit of our index is 1, set that square as occupied
            if (index & (1 << bit_count)) {
                blockers |= (1ULL << sq);
            }
            bit_count++;
        }
    }
    return blockers;
}


uint64_t calculate_rook_attacks_on_the_fly(int square, uint64_t blockers) {
    uint64_t attacks = 0ULL;
    int tr = square / 8;
    int tc = square % 8;

    // Up
    for (int r = tr + 1; r <= 7; ++r) {
        attacks |= (1ULL << (r * 8 + tc));
        if (blockers & (1ULL << (r * 8 + tc))) break;
    }
    // Down
    for (int r = tr - 1; r >= 0; --r) {
        attacks |= (1ULL << (r * 8 + tc));
        if (blockers & (1ULL << (r * 8 + tc))) break;
    }
    // Right
    for (int c = tc + 1; c <= 7; ++c) {
        attacks |= (1ULL << (tr * 8 + c));
        if (blockers & (1ULL << (tr * 8 + c))) break;
    }
    // Left
    for (int c = tc - 1; c >= 0; --c) {
        attacks |= (1ULL << (tr * 8 + c));
        if (blockers & (1ULL << (tr * 8 + c))) break;
    }
    return attacks;
}


uint64_t calculate_bishop_attacks_on_the_fly(int square, uint64_t blockers) {
    uint64_t attacks = 0ULL;
    int tr = square / 8;
    int tc = square % 8;

    // Northeast
    for (int r = tr + 1, c = tc + 1; r <= 7 && c <= 7; ++r, ++c) {
        attacks |= (1ULL << (r * 8 + c));
        if (blockers & (1ULL << (r * 8 + c))) break;
    }
    // Southeast
    for (int r = tr - 1, c = tc + 1; r >= 0 && c <= 7; --r, ++c) {
        attacks |= (1ULL << (r * 8 + c));
        if (blockers & (1ULL << (r * 8 + c))) break;
    }
    // Northwest
    for (int r = tr + 1, c = tc - 1; r <= 7 && c >= 0; ++r, --c) {
        attacks |= (1ULL << (r * 8 + c));
        if (blockers & (1ULL << (r * 8 + c))) break;
    }
    // Southwest
    for (int r = tr - 1, c = tc - 1; r >= 0 && c >= 0; --r, --c) {
        attacks |= (1ULL << (r * 8 + c));
        if (blockers & (1ULL << (r * 8 + c))) break;
    }
    return attacks;
}


// Stores the attack masks for the every rook and bishop square
uint64_t rook_masks[64];
uint64_t bishop_masks[64];

void initialize_masks() {
    for (int sq = 0; sq < 64; sq++) {
        rook_masks[sq] = mask_rook(sq);
        bishop_masks[sq] = mask_bishop(sq);
    }
}


void init_sliders_database() {
    initialize_masks();

    // Pointer, der trackt, an welcher Stelle im Riesen-Array wir gerade schreiben
    uint64_t* storage_pointer = GiantAttackStorage;

    // --- ROOKS FÜLLEN ---
    for (int sq = 0; sq < 64; ++sq) {
        int bits = rook_table_bits[sq];
        
        // Datenbank-Eintrag für dieses Feld vorbereiten
        RookDatabase[sq].mask = rook_masks[sq];
        RookDatabase[sq].magic = rook_magics[sq];
        RookDatabase[sq].shift = 64 - bits;
        RookDatabase[sq].attacks = storage_pointer; // Zeiger auf den freien Speicherplatz

        // Alle möglichen Blocker-Kombinationen für dieses Feld durchlaufen
        int num_permutations = 1 << count_bits(rook_masks[sq]);
        for (int i = 0; i < num_permutations; ++i) {
            uint64_t blockers = generate_blockers(i, rook_masks[sq]);
            uint64_t true_attack = calculate_rook_attacks_on_the_fly(sq, blockers);
            
            // Per Magic-Zahl den Index im Speicher berechnen
            int index = (blockers * RookDatabase[sq].magic) >> RookDatabase[sq].shift;
            
            // Den echten Zug an diesem Index speichern
            RookDatabase[sq].attacks[index] = true_attack;
        }
        
        // Den Pointer für das nächste Feld vorschieben
        storage_pointer += (1 << bits);
    }

    // --- BISHOPS FÜLLEN ---
    for (int sq = 0; sq < 64; ++sq) {
        int bits = bishop_table_bits[sq];
        
        BishopDatabase[sq].mask = bishop_masks[sq];
        BishopDatabase[sq].magic = bishop_magics[sq];
        BishopDatabase[sq].shift = 64 - bits;
        BishopDatabase[sq].attacks = storage_pointer;

        int num_permutations = 1 << count_bits(bishop_masks[sq]);
        for (int i = 0; i < num_permutations; ++i) {
            uint64_t blockers = generate_blockers(i, bishop_masks[sq]);
            uint64_t true_attack = calculate_bishop_attacks_on_the_fly(sq, blockers);
            
            int index = (blockers * BishopDatabase[sq].magic) >> BishopDatabase[sq].shift;
            BishopDatabase[sq].attacks[index] = true_attack;
        }
        
        storage_pointer += (1 << bits);
    }
}


