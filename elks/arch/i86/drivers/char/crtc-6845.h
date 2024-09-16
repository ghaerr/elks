#define OT_MDA 0
#define OT_CGA 1
#define OT_EGA 2
#define OT_VGA 3

#define N_DEVICETYPES 2

struct hw_params {
    int w;
    int h;
    unsigned short crtc_base;
    unsigned short vseg_base;
    int vseg_bytes;
    unsigned short page_words;
    unsigned char max_pages;
    unsigned char n_init_bytes;
    unsigned char init_bytes[16];
};

extern struct hw_params crtc_params[N_DEVICETYPES];
