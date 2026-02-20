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

    bool is_stack_block;
    bool is_boolean_block;
    bool is_c_shape;       // <--- ADD THIS
    bool is_e_shape;       // <--- ADD THIS (for if/else)

    BlockKind kind; 
    int subtype;    

    BlockDef() : label(""), color({128,128,128}), width(0), height(0),
                 is_stack_block(false), is_boolean_block(false), 
                 is_c_shape(false), is_e_shape(false), // <--- ADD INITIALIZERS
                 kind(BK_MOTION), subtype(0) {}
};

/* Get blocks for a category index (0-8). Returns count. */
int blocks_get_for_category(int category, BlockDef *out, int max_out);

/* Get category name */
const char* blocks_category_name(int category);

/* Get category dot color */
Color blocks_category_color(int category);

#endif