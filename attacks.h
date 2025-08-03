#ifndef ATTACKS_H
#define ATTACKS_H

#include "utility.h"

// Does not include pawns
typedef enum NonSlider {
    KNIGHT_NON_SLIDER, KING_NON_SLIDER, NON_SLIDERS
} NonSlider;

// Does not include the queen
typedef enum Slider {
    BISHOP_SLIDER, ROOK_SLIDER, SLIDERS
} Slider;

// https://www.chessprogramming.org/Magic_Bitboards
typedef struct SliderAttacks {
    Bitboard *attacks;
    Bitboard mask;
} SliderAttacks;


extern Bitboard pawnAttacks[COLOURS][SQUARES];
extern Bitboard nonSliderAttacks[NON_SLIDERS][SQUARES]; // Does not include pawns
extern SliderAttacks sliderAttacks[SLIDERS][SQUARES]; // Does not include the queen

static inline Bitboard getPawnAttacks(Colour c, Square sq) {
    return pawnAttacks[c][sq];
}

// Does not include pawn attacks
static inline Bitboard getNonSliderAttacks(NonSlider nonSlider, Square sq) {
    return nonSliderAttacks[nonSlider][sq];
}

// Does not include queen attacks
static inline Bitboard getSliderAttacks(Slider slider, Bitboard occupied, Square sq) {
    const SliderAttacks *sa = &sliderAttacks[slider][sq];
    return sa->attacks[pext(occupied, sa->mask)];
}

void initializeAttacks();

#endif
