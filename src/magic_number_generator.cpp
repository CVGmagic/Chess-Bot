#include <vector>
#include <random>
#include <algorithm>
#include <iostream>
#include <intrin.h>


struct Magic {
    uint64_t mask;      // Isolates relevant blockers
    uint64_t magic;     // The multiplier found via brute force
    int shift;          // Determines how much of the number is cut off (64 - target_bits)
    uint64_t* attacks;  // Pointer to this square's block in the giant array
};

Magic RookDatabase[64];
Magic BishopDatabase[64];


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


// Stores the attack masks for the every rook and bishop square
uint64_t rook_masks[64];
uint64_t bishop_masks[64];

void initialize_masks() {
    for (int sq = 0; sq < 64; sq++) {
        rook_masks[sq] = mask_rook(sq);
        bishop_masks[sq] = mask_bishop(sq);
    }
}


// Generates the ith blocker configuration given a mask
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


// One giant flat array to store all attack permutations
uint64_t GiantAttackStorage[87312];

// Track where we are writing inside the GiantAttackStorage
uint64_t* storage_pointer = GiantAttackStorage;


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


uint64_t random_64bit() {
    // 1. Seed the generator. 
    // We use a FIXED seed (like 12345) instead of a random clock time.
    static std::mt19937_64 generator(123456789ULL);

    // 2. Define the distribution range (the full span of a 64-bit unsigned int)
    static std::uniform_int_distribution<uint64_t> distribution(0, 0xFFFFFFFFFFFFFFFFULL);

    // 3. Generate and return the random number
    return distribution(generator);
}


uint64_t random_uint64_sparse() {
    uint64_t r1 = random_64bit();
    uint64_t r2 = random_64bit();
    uint64_t r3 = random_64bit();
    
    return r1 & r2 & r3; // Make bits more sparse
}

// Uses raycasting logic to determine the attacked squares slowly for magic number generation
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


int count_bits(uint64_t bitboard) {
#if defined(_MSC_VER)
    return __popcnt64(bitboard); // Microsoft-Variante
#else
    return __builtin_popcountll(bitboard); // GCC/Clang-Variante
#endif
}


uint64_t find_minimized_magic(int square, int target_bits, bool is_bishop) {
    uint64_t mask = is_bishop ? bishop_masks[square] : rook_masks[square];
    int mask_bits = count_bits(mask);
    int num_permutations = 1 << mask_bits; 
    int table_size = 1 << target_bits;     

    std::vector<uint64_t> blockers(num_permutations);
    std::vector<uint64_t> true_attacks(num_permutations);

    // 1. Populate the ground truth datasets
    for (int i = 0; i < num_permutations; i++) {
        blockers[i] = generate_blockers(i, mask);
        true_attacks[i] = is_bishop ? calculate_bishop_attacks_on_the_fly(square, blockers[i]) 
                                    : calculate_rook_attacks_on_the_fly(square, blockers[i]);
    }

    // Allocate test tables outside the while loop to keep it lightning fast
    std::vector<uint64_t> test_table(table_size);
    std::vector<bool> index_used(table_size);
    int shift = 64 - target_bits;

    // 2. Start guessing!
    while (true) {
        uint64_t magic = random_uint64_sparse();
        
        std::fill(index_used.begin(), index_used.end(), false);
        bool destructive_collision = false;

        for (int i = 0; i < num_permutations; i++) {
            int index = (blockers[i] * magic) >> shift;

            if (!index_used[index]) {
                test_table[index] = true_attacks[i];
                index_used[index] = true;
            } 
            // If the slot is taken, ensure it's a constructive collision (identical moves)
            else if (test_table[index] != true_attacks[i]) {
                destructive_collision = true;
                break; // Failed candidate, exit permutation loop
            }
        }

        if (!destructive_collision) {
            return magic; // Perfect match found!
        }
    }
}


int main() {
    // 1. Generate your basic masks first
    initialize_masks(); 

    // 2. Print out the discovered Rook Magics formatted as C++ code
    std::cout << "const uint64_t rook_magics[64] = {\n";
    for (int sq = 0; sq < 64; ++sq) {
        uint64_t magic = find_minimized_magic(sq, rook_table_bits[sq], false);
        std::cout << "    0x" << std::hex << magic << "ULL,\n";
    }
    std::cout << "};\n";

    // 3. Print out the discovered Bishop Magics formatted as C++ code
    std::cout << "const uint64_t bishop_magics[64] = {\n";
    for (int sq = 0; sq < 64; ++sq) {
        uint64_t magic = find_minimized_magic(sq, bishop_table_bits[sq], true);
        std::cout << "    0x" << std::hex << magic << "ULL,\n";
    }
    std::cout << "};\n";
    
    return 0;
}

