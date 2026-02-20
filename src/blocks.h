#ifndef BLOCKS_H
#define BLOCKS_H

#include "config.h"
#include "types.h"
#include <string>

struct BlockDef {
    std::string label;
    Color color;
    int width;
    int height;

    /* Palette items: only stack blocks (Motion/Looks) */
    bool is_stack_block;
    bool is_boolean_block;  // شش‌ضلعی (sensing boolean shape)

    /* For stack blocks */
    BlockKind kind; /* BK_MOTION / BK_LOOKS */
    int subtype;    /* MotionBlockType / LooksBlockType */

    BlockDef()
        : label(""),
          color({128,128,128}),
          width(0),
          height(0),
          is_stack_block(false),
          kind(BK_MOTION),
          subtype(0) {}
};

/* Get blocks for a category index (0-8). Returns count. */
int blocks_get_for_category(int category, BlockDef *out, int max_out);

/* Get category name */
const char* blocks_category_name(int category);

/* Get category dot color */
Color blocks_category_color(int category);

#endif