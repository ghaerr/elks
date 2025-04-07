/*
 * BMP Decoder/encoder from Microwindows
 *
 * Copyright (c) 2000, 2001, 2003, 2005, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2000 Martin Jolicoeur <martinj@visuaide.com>
 *
 * Image decode routine for BMP files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graphics.h"
#include "app.h"

#define USE_DRAWSCANLINE    1       /* =1 to use vga_drawscanline instead of drawpixel */

#define MWPACKED
#define abs(x)  ((x < 0)? -x : x)

/* BMP stuff*/
#define BI_RGB      0L
#define BI_RLE8     1L
#define BI_RLE4     2L
#define BI_BITFIELDS    3L

typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;
typedef long            LONG;

typedef struct {
    /* BITMAPFILEHEADER*/
    BYTE    bfType[2];
    DWORD   bfSize;
    WORD    bfReserved1;
    WORD    bfReserved2;
    DWORD   bfOffBits;
} MWPACKED BMPFILEHEAD;

/* windows style*/
typedef struct {
    /* BITMAPINFOHEADER*/
    DWORD   BiSize;
    LONG    BiWidth;
    LONG    BiHeight;
    WORD    BiPlanes;
    WORD    BiBitCount;
    DWORD   BiCompression;
    DWORD   BiSizeImage;
    LONG    BiXpelsPerMeter;
    LONG    BiYpelsPerMeter;
    DWORD   BiClrUsed;
    DWORD   BiClrImportant;
} MWPACKED BMPINFOHEAD;

/* os/2 style*/
typedef struct {
    /* BITMAPCOREHEADER*/
    DWORD   bcSize;
    WORD    bcWidth;
    WORD    bcHeight;
    WORD    bcPlanes;
    WORD    bcBitCount;
} MWPACKED BMPCOREHEAD;

static int  DecodeRLE8(unsigned char *buf, FILE *src);
static int  DecodeRLE4(unsigned char *buf, FILE *src);
static void put4(int b);

struct palette {
    unsigned char r, g, b, a;
};
#define RGBDEF(r,g,b)   {r, g, b, 0}

#if 1
/*
 * CGA palette for 16 color systems.
 */
struct palette palette[16] = {
    RGBDEF( 0   , 0   , 0    ), /* black*/
    RGBDEF( 0   , 0   , 0xAA ), /* blue*/
    RGBDEF( 0   , 0xAA, 0    ), /* green*/
    RGBDEF( 0   , 0xAA, 0xAA ), /* cyan*/
    RGBDEF( 0xAA, 0   , 0    ), /* red*/
    RGBDEF( 0xAA, 0   , 0xAA ), /* magenta*/
    RGBDEF( 0xAA, 0x55, 0    ), /* brown*/
    RGBDEF( 0xAA, 0xAA, 0xAA ), /* ltgray*/
    RGBDEF( 0x55, 0x55, 0x55 ), /* gray*/
    RGBDEF( 0x55, 0x55, 0xFF ), /* ltblue*/
    RGBDEF( 0x55, 0xFF, 0x55 ), /* ltgreen*/
    RGBDEF( 0x55, 0xFF, 0xFF ), /* ltcyan*/
    RGBDEF( 0xFF, 0x55, 0x55 ), /* ltred*/
    RGBDEF( 0xFF, 0x55, 0xFF ), /* ltmagenta*/
    RGBDEF( 0xFF, 0xFF, 0x55 ), /* yellow*/
    RGBDEF( 0xFF, 0xFF, 0xFF ), /* white*/
};

#else

/*
 * Standard palette for 16 color systems.
 */
struct palette palette[16] = {
    /* 16 EGA colors, arranged in VGA standard palette order*/
    RGBDEF( 0  , 0  , 0   ),    /* black*/
    RGBDEF( 0  , 0  , 128 ),    /* blue*/
    RGBDEF( 0  , 128, 0   ),    /* green*/
    RGBDEF( 0  , 128, 128 ),    /* cyan*/
    RGBDEF( 128, 0  , 0   ),    /* red*/
    RGBDEF( 128, 0  , 128 ),    /* magenta*/
    RGBDEF( 128, 64 , 0   ),    /* adjusted brown*/
    RGBDEF( 192, 192, 192 ),    /* ltgray*/
    RGBDEF( 128, 128, 128 ),    /* gray*/
    RGBDEF( 0  , 0  , 255 ),    /* ltblue*/
    RGBDEF( 0  , 255, 0   ),    /* ltgreen*/
    RGBDEF( 0  , 255, 255 ),    /* ltcyan*/
    RGBDEF( 255, 0  , 0   ),    /* ltred*/
    RGBDEF( 255, 0  , 255 ),    /* ltmagenta*/
    RGBDEF( 255, 255, 0   ),    /* yellow*/
    RGBDEF( 255, 255, 255 ),    /* white*/
};
#endif

static unsigned int find_nearest_color(int r, int g, int b)
{
    struct palette *rgb;
    int     R, G, B;
    long    diff = 0x7fffffffL;
    long    sq;
    int     best = 0;

    for(rgb=palette; diff && rgb < &palette[16]; ++rgb) {
        R = rgb->r - r;
        G = rgb->g - g;
        B = rgb->b - b;
#if 1
        /* speedy linear distance method*/
        sq = abs(R) + abs(G) + abs(B);
#else
        /* slower distance-cubed with luminance adjustment*/
        /* gray is .30R + .59G + .11B*/
        /* = (R*77 + G*151 + B*28)/256*/
        sq = (long)R*R*30*30 + (long)G*G*59*59 + (long)B*B*11*11;
#endif

        if(sq < diff) {
            best = rgb - palette;
            if((diff = sq) == 0)
                return best;
        }
    }
    return best;
}

/*
 * Decode and display BMP file
 */
int
draw_bmp(char *path, int x, int y)
{
    int         w, h, i, compression, width, height, bpp, palsize, pitch;
    DWORD       hdrsize;
    FILE *src;
    unsigned char *image;
    BMPFILEHEAD bmpf;
    static unsigned char imagebits[640*4];  /* static so no stack usage */
    static struct palette bmppal[256];
    static int cache[256];

    if ((src = fopen(path, "r")) == NULL)
        goto err;

    /* read BMP file header*/
    if (fread(&bmpf, 1, sizeof(bmpf), src) != sizeof(bmpf))
        goto out;

    /* check magic bytes*/
    if (bmpf.bfType[0] != 'B' || bmpf.bfType[1] != 'M')
        goto out;               /* not bmp image*/

    /* Read header size to determine header type*/
    if (fread(&hdrsize, 1, sizeof(hdrsize), src) != sizeof(hdrsize))
        goto out;               /* not bmp image*/

    /* might be windows or os/2 header */
    if(hdrsize == sizeof(BMPCOREHEAD)) {
        BMPCOREHEAD bmpc;

        /* read os/2 header */
        if (fread(&bmpc.bcWidth, 1, sizeof(bmpc)-sizeof(DWORD), src) !=
            sizeof(bmpc)-sizeof(DWORD))
                goto out;       /* not bmp image*/

        compression = BI_RGB;
        width = bmpc.bcWidth;
        height = bmpc.bcHeight;
        bpp = bmpc.bcBitCount;
        if (bpp <= 8)
            palsize = 1 << bpp;
        else palsize = 0;
    } else {
        BMPINFOHEAD bmpi;

        /* read windows header */
        if (fread(&bmpi.BiWidth, 1, sizeof(bmpi)-sizeof(DWORD), src)
            != sizeof(bmpi)-sizeof(DWORD))
                goto out;    /* not bmp image*/

        compression = bmpi.BiCompression;
        width = bmpi.BiWidth;
        height = bmpi.BiHeight;
        bpp = bmpi.BiBitCount;
        palsize = bmpi.BiClrUsed;
        if (palsize > 256) palsize = 0;
        else if (palsize == 0 && bpp <= 8) palsize = 1 << bpp;
    }
    /*__dprintf("'%s' bpp %d comp %d pal %d %dx%d\n",
        path, bpp, compression, palsize, width, height);*/

    if (width > 640)        /* bounds check for later array overflow */
        goto out;

    /* compute pitch: bytes per line*/
    switch(bpp) {
#if UNUSED
    case 1:
        pitch = (width+7)/8;
        break;
    case 4:
        pitch = (width+1)/2;
        break;
    case 16:
        pitch = width * 2;
        break;
    case 32:
        pitch = width * 4;
        break;
#endif
    case 8:
        pitch = width;
        memset(cache, 0xff, 256*sizeof(int));
        break;
    case 24:
        pitch = width * 3;
        break;
    default:
        __dprintf("%s: image bpp not 8 or 24\n", path);
        goto out;
    }
    pitch = (pitch + 3) & ~3;

    /* get colormap*/
    if (bpp <= 8) {
        for (i=0; i<palsize; i++) {
            bmppal[i].b = fgetc(src);
            bmppal[i].g = fgetc(src);
            bmppal[i].r = fgetc(src);
            if (hdrsize != sizeof(BMPCOREHEAD))
                fgetc(src);
        }
    }
#if UNUSED
    /* determine 16bpp 5/5/5 or 5/6/5 format*/
    if (bpp == 16) {
        DWORD format = 0x7c00;      /* default is 5/5/5*/

        if (compression == BI_BITFIELDS) {
            if (fread(&format, 1, sizeof(format), src) != sizeof(format))
                goto out;
        }
        if (format == 0x7c00)
            data_format = MWIF_RGB555;
        /* else it's 5/6/5 format, no flag required*/
    }
#endif

    /* decode image data*/
    fseek(src, bmpf.bfOffBits, SEEK_SET);

    h = height;
    /* For every row, draw image from bottom up */
    while (--h >= 0) {
        /* Get row data from file, images are DWORD right aligned */
        if(compression == BI_RLE8) {
            if(!DecodeRLE8(imagebits, src))
                break;
        } else if(compression == BI_RLE4) {
            if(!DecodeRLE4(imagebits, src))
                break;
        } else {
            if(fread(imagebits, 1, pitch, src) != pitch)
                goto out;
        }

        image = imagebits;
        switch (bpp) {
        case 8:
            for (w = 0; w < width; w++) {
                unsigned int c;
                struct palette *pal;
                if ((c = cache[image[w]]) == 0xffff) {
                    pal = &bmppal[image[w]];
                    c = find_nearest_color(pal->r, pal->g, pal->b);
                    cache[image[w]] = c;
                }
#if USE_DRAWSCANLINE
                image[w] = c;
#else
                drawpixel(x+w, y+h, c);
#endif
            }
#if USE_DRAWSCANLINE
            vga_drawscanline(image, x, y+h, width);
#endif
            break;
        case 24:
            for (w = 0; w < width; w++) {
                unsigned int c = find_nearest_color(image[2], image[1], image[0]);
                drawpixel(x+w, y+h, c);
                image += 3;
            }
            break;
        }
    }
    fclose(src);
    return 1;

out:
    fclose(src);
err:
    __dprintf("Image load error: %s\n", path);
    return 0;
}

/*
 * Decode one line of RLE8, return 0 when done with all bitmap data
 */
static int
DecodeRLE8(unsigned char *buf, FILE *src)
{
    int c, n;
    unsigned char *p = buf;

    for (;;) {
        switch (n = fgetc(src)) {
        case EOF:
            return 0;
        case 0: /* 0 = escape */
            switch (n = fgetc(src)) {
            case 0:     /* 0 0 = end of current scan line */
                return 1;
            case 1:     /* 0 1 = end of data */
                return 1;
            case 2:     /* 0 2 xx yy delta mode NOT SUPPORTED */
                (void) fgetc(src);
                (void) fgetc(src);
                continue;
            default:    /* 0 3..255 xx nn uncompressed data */
                for (c = 0; c < n; c++)
                    *p++ = fgetc(src);
                if (n & 1)
                    (void) fgetc(src);
                continue;
            }
        default:
            c = fgetc(src);
            while (n--)
                *p++ = c;
            continue;
        }
    }
}

/*
 * Decode one line of RLE4, return 0 when done with all bitmap data
 */
static unsigned char *p;
static int once;

static void
put4(int b)
{
    static int last;

    last = (last << 4) | b;
    if (++once == 2) {
        *p++ = last;
        once = 0;
    }
}

static int
DecodeRLE4(unsigned char *buf, FILE *src)
{
    int c, n, c1, c2;

    p = buf;
    once = 0;
    c1 = 0;

    for (;;) {
        switch (n = fgetc(src)) {
        case EOF:
            return 0;
        case 0: /* 0 = escape */
            switch (n = fgetc(src)) {
            case 0:     /* 0 0 = end of current scan line */
                if (once)
                    put4(0);
                return 1;
            case 1:     /* 0 1 = end of data */
                if (once)
                    put4(0);
                return 1;
            case 2:     /* 0 2 xx yy delta mode NOT SUPPORTED */
                (void) fgetc(src);
                (void) fgetc(src);
                continue;
            default:    /* 0 3..255 xx nn uncompressed data */
                c2 = (n + 3) & ~3;
                for (c = 0; c < c2; c++) {
                    if ((c & 1) == 0)
                        c1 = fgetc(src);
                    if (c < n)
                        put4((c1 >> 4) & 0x0f);
                    c1 <<= 4;
                }
                continue;
            }
        default:
            c = fgetc(src);
            c1 = (c >> 4) & 0x0f;
            c2 = c & 0x0f;
            for (c = 0; c < n; c++)
                put4((c & 1) ? c2 : c1);
            continue;
        }
    }
}

/* little-endian storage of longword*/
static void putdw(unsigned long dw, FILE *ofp)
{
    fputc((unsigned char)dw, ofp);
    dw >>= 8;
    fputc((unsigned char)dw, ofp);
    dw >>= 8;
    fputc((unsigned char)dw, ofp);
    dw >>= 8;
    fputc((unsigned char)dw, ofp);
}

/* MWIF_RGB565*/
#define RMASK565    0xf800
#define GMASK565    0x07e0
#define BMASK565    0x001f
#define AMASK565    0x0000

/**
 * Create 8bpp .bmp file from framebuffer data
 *
 * @param path Output file.
 * @return 0 on success, nonzero on error.
 */
int save_bmp(char *pathname)
{
    FILE *ofp;
    int w, h, cx, cy, i, extra, bpp, bytespp, ncolors, sizecolortable;
    int hdrsize, imagesize, filesize, compression, colorsused;
    BMPFILEHEAD bmpf;
    BMPINFOHEAD bmpi;

    ofp = fopen(pathname, "wb");
    if (!ofp)
        return 1;

    cx = CANVAS_WIDTH;
    cy = CANVAS_HEIGHT;
    bpp = 8;                /* write 8bpp for now */
    ncolors = (bpp <= 8)? 16: 0;
    bytespp = (bpp+7)/8;

    /* dword right padded*/
    extra = (cx*bytespp) & 3;
    if (extra)
        extra = 4 - extra;

    /* color table is either palette or 3 longword r/g/b masks*/
    sizecolortable = ncolors? ncolors*4: 3*4;
    if (bpp == 24)
        sizecolortable = 0; /* special case 24bpp has no table*/

    hdrsize = sizeof(bmpf) + sizeof(bmpi) + sizecolortable;
    imagesize = (cx + extra) * cy * bytespp;
    filesize =  hdrsize + imagesize;
    compression = (bpp == 16 || bpp == 32)? BI_BITFIELDS: BI_RGB;
    colorsused = (bpp <= 8)? ncolors: 0;

    /* fill out headers*/
    memset(&bmpf, 0, sizeof(bmpf));
    bmpf.bfType[0] = 'B';
    bmpf.bfType[1] = 'M';
    bmpf.bfSize = filesize;
    bmpf.bfOffBits = hdrsize;

    memset(&bmpi, 0, sizeof(bmpi));
    bmpi.BiSize = sizeof(BMPINFOHEAD);
    bmpi.BiWidth = cx;
    bmpi.BiHeight = cy;
    bmpi.BiPlanes = 1;
    bmpi.BiBitCount = bpp;
    bmpi.BiCompression = compression;
    bmpi.BiSizeImage = imagesize;
    bmpi.BiClrUsed = colorsused;

    /* write headers*/
    fwrite((char *)&bmpf, sizeof(bmpf), 1, ofp);
    fwrite((char *)&bmpi, sizeof(bmpi), 1, ofp);

    /* write colortable*/
    if (sizecolortable) {
        if(bpp <= 8) {
            /* write palette*/
            for(i=0; i<ncolors; i++) {
                fputc(palette[i].b, ofp);
                fputc(palette[i].g, ofp);
                fputc(palette[i].r, ofp);
                fputc(0, ofp);
            }
        } else {
            /* write 3 r/g/b masks*/
            putdw(RMASK565, ofp);
            putdw(GMASK565, ofp);
            putdw(BMASK565, ofp);
        }
    }

    /* write image data, upside down ;)*/
    for(h=cy-1; h>=0; --h) {
        for (int x=0; x<cx; x++) {
            int c = readpixel(x, h);
            fputc(c, ofp);
        }
        for(w=0; w<extra; ++w)
            fputc(0, ofp);      /* DWORD pad each line*/
    }

    fclose(ofp);
    return 0;
}
