#ifndef DATA_TYPES_H_INCLUDED
#define DATA_TYPES_H_INCLUDED

// Boolean Data Type
typedef enum boolean_e
{
    false = 0,
    true = 1
} boolean_t;

// Represents the colors in HSV (for Color Picker)
typedef struct ColorHSV_s
{
    int h, s, v;
} ColorHSV_t;

// Represents the colors in RGB (for Color Picker)
typedef struct ColorRGB_s
{
    int r,g,b,a;
} ColorRGB_t;

typedef struct rect
{
    int x, y;
    int w, h;
} rect;

// GUI Button Data Type
typedef struct button_s
{
    char* name;
    rect box;
    int (*OnClick)(struct button_s* btn);
    int data1;

    boolean_t render;
    char* fileName;
} button_t;

// Represents a pixel
typedef struct transform2d_x
{
    int x,y;
} transform2d_t;

#endif
