#define OT_MDA 0
#define OT_CGA 1
#define OT_EGA 2
#define OT_VGA 3

#define N_DEVICETYPES 2

#ifdef DEBUG
static const char *type_string[] = {
    "MDA",
    "CGA",
    "EGA",
    "VGA",
};
#endif

struct hw_params {
    unsigned short w;
    unsigned short h;
    unsigned short crtc_base;
    unsigned vseg_base;
    int vseg_bytes;
    unsigned char max_pages;
    unsigned short page_words;
    unsigned char n_init_bytes;
    unsigned char init_bytes[16];
};

#ifdef CONFIG_CONSOLE_DUAL
static struct hw_params crtc_params[N_DEVICETYPES] = {
    { 80, 25, 0x3B4, 0xB000, 0x1000, 1, 2000, 16,
        {
            0x61, 0x50, 0x52, 0x0F,
            0x19, 0x06, 0x19, 0x19,
            0x02, 0x0D, 0x0B, 0x0C,
            0x00, 0x00, 0x00, 0x00,
        }
    }, /* MDA */
    { 80, 25, 0x3D4, 0xB800, 0x4000, 3, 2000, 16,
        {
            /* CO80 */
            0x71, 0x50, 0x5A, 0x0A,
            0x1F, 0x06, 0x19, 0x1C,
            0x02, 0x07, 0x06, 0x07,
            0x00, 0x00, 0x00, 0x00,
        }
    }, /* CGA */
    // TODO
    //{ 0 },                             /* EGA */
    //{ 0 },                             /* VGA */
};

int INITPROC crtc_probe(unsigned short crtc_base);
int INITPROC crtc_init(unsigned int t);
#endif
