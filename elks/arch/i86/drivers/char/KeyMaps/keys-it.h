/* Keymap:IT:Italiano:Italy      */

#ifndef __KEYMAP_IT__
#define __KEYMAP_IT__

#if defined(CONFIG_KEYMAP_IT)

/*
    Italian keyboard

    2004.09.20  Daniele Giacomini, daniele@swlibero.org
                Added italian keyboard using CP 437 encoding.
                Numeric keypad is always active.
                Every key array has a descriptive map shown with ISO
                8859-1 encoding to explain key locations. Please note
                that "<" and ">" are made with [Ctrl]+[Alt]+[1] and
                [Ctrl]+[Alt]+[2], and there are more extentions, like
                the italian keyboard map used with GNU/Linux
*/

/*
    \ 1 2 3 4 5 6 7 8 9 0 ' ì
       q w e r t y u i o p è +
        a s d f g h j k l ò à ù
         z x c v b n m , . -
*/

static unsigned char xtkb_scan[] = {
  0, 033, '1', '2', '3', '4', '5', '6',
  '7', '8', '9', '0', '\'', 0215, '\b', '\t',
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
  'o', 'p', 0212, '+', 015, 0202, 'a', 's',
  'd', 'f', 'g', 'h', 'j', 'k', 'l', 0225,
  0205, '\\', 0200, 0227, 'z', 'x', 'c', 'v',
  'b', 'n', 'm', ',', '.', '-', 0201, '*',
  0203, ' ', 0204, 0241, 0242, 0243, 0244,
  0245, 0246, 0247, 0250, 0251, 0252, 0205,
  '?', '7', '8', '9', '-', '4', '5', '6',
  '+', '1', '2', '3', '0', '.'
};

/*
    | ! " £ $ % & / ( ) = ? ^
       Q W E R T Y U I O P é *
        A S D F G H J K L ç ° §
         Z X C V B N M ; : _
*/

static unsigned char xtkb_scan_shifted[] = {
  0, 033, '!', '"', 0234, '$', '%', '&', '/', '(',
  ')', '=', '?', '^', '\b', '\t', 'Q', 'W', 'E',
  'R', 'T', 'Y', 'U', 'I', 'O', 'P', 0202, '*', '\r',
  0202, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K',
  'L', 0207, 0370, '|', 0200, 025, 'Z', 'X', 'C',
  'V', 'B', 'N', 'M', ';', ':', '_', 0201, '*',
  0203, ' ', 0204, 0221, 0222, 0223, 0224, 0225,
  0226, 0227, 0230, 0231, 0232, 0204, 0213, '7',
  '8', '9', '-', '4', '5', '6', '+', '1', '2', '3',
  '0', '.'
};

/*
    \ < > 3 4 5 6 { [ ] } ` ~
       q w e r t y u i o p [ ]
        a s d f g h j k l @ # ù
         « » c v b n m , . -
*/

static unsigned char xtkb_scan_ctrl_alt[] = {
  0, 033, '<', '>', '3', '4', '5', '6', '{', '[',
  ']', '}', '`', '~', '\b', '\t', 'q', 'w', 'e',
  'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\r',
  0202, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k',
  'l', '@', '#', '\\', 0200, 0227, 0256, 0257,
  'c', 'v', 'b', 'n', 'm', ',', '.', '-', 0201,
  '*', 0203, ' ', 0204, 0241, 0242, 0243, 0244,
  0245, 0246, 0247, 0250, 0251, 0252, 0205, '?',
  '7', '8','9', '-', '4', '5', '6', '+', '1',
  '2', '3', '0', '.'
};

/*
    \ 1 2 3 4 5 6 7 8 9 0 ' ì
       Q W E R T Y U I O P è +
        A S D F G H J K L ò à ù
         Z X C V B N M , . -
*/

static unsigned char xtkb_scan_caps[] = {
  0, 033, '1', '2', '3', '4', '5', '6', '7', '8',
  '9', '0', '\'', 0215, '\b', '\t', 'Q', 'W', 'E',
  'R', 'T', 'Y', 'U', 'I', 'O', 'P', 0212, '+', '\r',
  0202, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K',
  'L', 0225, 0205, '\\', 0200, 0227, 'Z', 'X',
  'C', 'V', 'B', 'N', 'M', ',', '.', '-', 0201,
  '*', 0203, ' ', 0204, 0221, 0222, 0223, 0224,
  0225, 0226, 0227, 0230, 0231, 0232, 0204, 0213,
  '7', '8', '9', '-', '4', '5', '6', '+', '1',
  '2', '3', '0', '.'
};

#endif

#endif
