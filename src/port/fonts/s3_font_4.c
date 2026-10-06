/*******************************************************************************
 * Size: 4 px
 * Bpp: 2
 * Opts: --size 4 --bpp 2 --format lvgl --no-compress --no-prefilter --font lvgl/scripts/generators/built_in_font/Montserrat-Medium.ttf -r 0x20-0x17F,0x1E00-0x1EFF,0x2022,0x2026,0x2190-0x2193 --font lvgl/scripts/generators/built_in_font/FontAwesome5-Solid+Brands+Regular.woff -r 61441,61448,61451,61452,61453,61457,61459,61461,61465,61468,61473,61478,61479,61480,61502,61507,61512,61516,61517,61521,61522,61523,61524,61543,61544,61550,61552,61553,61556,61559,61560,61561,61563,61587,61589,61636,61637,61639,61641,61664,61671,61674,61683,61724,61732,61787,61931,62016,62017,62018,62019,62020,62087,62099,62212,62189,62810,63426,63650 --lv-include lvgl/lvgl.h --lv-font-name s3_font_4 -o ports/esp32/fonts/s3_font_4.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef S3_FONT_4
#define S3_FONT_4 1
#endif

#if S3_FONT_4

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0021 "!" */
    0x50,

    /* U+0022 "\"" */
    0x50,

    /* U+0023 "#" */
    0x1, 0x45, 0x0,

    /* U+0024 "$" */
    0x61, 0x86, 0x0,

    /* U+0025 "%" */
    0x44, 0x14, 0x4,

    /* U+0026 "&" */
    0x51, 0x85, 0x40,

    /* U+0027 "'" */
    0x40,

    /* U+0028 "(" */
    0x44, 0x44,

    /* U+0029 ")" */
    0x55,

    /* U+002A "*" */
    0x80,

    /* U+002B "+" */
    0x0, 0x80,

    /* U+002C "," */
    0x40,

    /* U+002D "-" */
    0x0,

    /* U+002E "." */
    0x0,

    /* U+002F "/" */
    0x0, 0x1, 0x4,

    /* U+0030 "0" */
    0x51, 0x15, 0x0,

    /* U+0031 "1" */
    0x84, 0x40,

    /* U+0032 "2" */
    0x50, 0x49, 0x0,

    /* U+0033 "3" */
    0x60, 0x45, 0x0,

    /* U+0034 "4" */
    0x11, 0x46, 0x40,

    /* U+0035 "5" */
    0x51, 0x45, 0x0,

    /* U+0036 "6" */
    0x52, 0x45, 0x0,

    /* U+0037 "7" */
    0x60, 0x41, 0x0,

    /* U+0038 "8" */
    0x51, 0x85, 0x0,

    /* U+0039 "9" */
    0x51, 0x45, 0x0,

    /* U+003A ":" */
    0x0,

    /* U+003B ";" */
    0x10,

    /* U+003C "<" */
    0x1, 0x41, 0x0,

    /* U+003D "=" */
    0x10, 0x40,

    /* U+003E ">" */
    0x0, 0x40, 0x0,

    /* U+003F "?" */
    0x50, 0x40, 0x0,

    /* U+0040 "@" */
    0x14, 0x55, 0x55, 0x14,

    /* U+0041 "A" */
    0x21, 0x59, 0x80,

    /* U+0042 "B" */
    0x55, 0x55, 0x40,

    /* U+0043 "C" */
    0x51, 0x5, 0x0,

    /* U+0044 "D" */
    0x54, 0x44, 0x54,

    /* U+0045 "E" */
    0x51, 0x45, 0x0,

    /* U+0046 "F" */
    0x51, 0x44, 0x0,

    /* U+0047 "G" */
    0x51, 0x5, 0x40,

    /* U+0048 "H" */
    0x45, 0x64, 0x40,

    /* U+0049 "I" */
    0x54,

    /* U+004A "J" */
    0x4, 0x11, 0x40,

    /* U+004B "K" */
    0x45, 0x84, 0x40,

    /* U+004C "L" */
    0x41, 0x5, 0x0,

    /* U+004D "M" */
    0x81, 0x55, 0x41,

    /* U+004E "N" */
    0x85, 0x54, 0x80,

    /* U+004F "O" */
    0x54, 0x44, 0x54,

    /* U+0050 "P" */
    0x55, 0x15, 0x0,

    /* U+0051 "Q" */
    0x54, 0x44, 0x54, 0x4,

    /* U+0052 "R" */
    0x55, 0x15, 0x40,

    /* U+0053 "S" */
    0x51, 0x45, 0x0,

    /* U+0054 "T" */
    0x60, 0x41, 0x0,

    /* U+0055 "U" */
    0x45, 0x15, 0x40,

    /* U+0056 "V" */
    0x45, 0x2, 0x0,

    /* U+0057 "W" */
    0x48, 0x15, 0x42, 0x20,

    /* U+0058 "X" */
    0x50, 0x84, 0x40,

    /* U+0059 "Y" */
    0x11, 0x8, 0x4,

    /* U+005A "Z" */
    0x24, 0x49, 0x0,

    /* U+005B "[" */
    0x44, 0x44,

    /* U+005C "\\" */
    0x0, 0x41, 0x1,

    /* U+005D "]" */
    0x96,

    /* U+005E "^" */
    0x1, 0x40,

    /* U+005F "_" */
    0x0,

    /* U+0060 "`" */
    0x0,

    /* U+0061 "a" */
    0x61, 0x80,

    /* U+0062 "b" */
    0x41, 0x55, 0x40,

    /* U+0063 "c" */
    0x51, 0x40,

    /* U+0064 "d" */
    0x5, 0x55, 0x40,

    /* U+0065 "e" */
    0x61, 0x40,

    /* U+0066 "f" */
    0x48, 0x40,

    /* U+0067 "g" */
    0x55, 0x51, 0x0,

    /* U+0068 "h" */
    0x41, 0x44, 0x40,

    /* U+0069 "i" */
    0x14,

    /* U+006A "j" */
    0x1, 0x11,

    /* U+006B "k" */
    0x41, 0x45, 0x0,

    /* U+006C "l" */
    0x54,

    /* U+006D "m" */
    0x55, 0x45,

    /* U+006E "n" */
    0x51, 0x10,

    /* U+006F "o" */
    0x51, 0x40,

    /* U+0070 "p" */
    0x55, 0x54, 0x0,

    /* U+0071 "q" */
    0x55, 0x50, 0x40,

    /* U+0072 "r" */
    0x84,

    /* U+0073 "s" */
    0x56,

    /* U+0074 "t" */
    0x8, 0x40,

    /* U+0075 "u" */
    0x45, 0x50,

    /* U+0076 "v" */
    0x14, 0x14,

    /* U+0077 "w" */
    0x51, 0x58,

    /* U+0078 "x" */
    0x51, 0x40,

    /* U+0079 "y" */
    0x14, 0x14, 0x10,

    /* U+007A "z" */
    0x69,

    /* U+007B "{" */
    0x44, 0x44,

    /* U+007C "|" */
    0x55,

    /* U+007D "}" */
    0x44, 0x44,

    /* U+007E "~" */
    0x0,

    /* U+00A0 " " */

    /* U+00A1 "¡" */
    0x14,

    /* U+00A2 "¢" */
    0x11, 0x42, 0x0,

    /* U+00A3 "£" */
    0x12, 0x49, 0x0,

    /* U+00A4 "¤" */
    0x55, 0x15, 0x40,

    /* U+00A5 "¥" */
    0x11, 0x8, 0x8,

    /* U+00A6 "¦" */
    0x41,

    /* U+00A7 "§" */
    0x55, 0x10,

    /* U+00A8 "¨" */
    0x0,

    /* U+00A9 "©" */
    0x10, 0x10, 0x0,

    /* U+00AA "ª" */
    0x40,

    /* U+00AB "«" */
    0x5, 0x0,

    /* U+00AC "¬" */
    0x10, 0x0,

    /* U+00AD "­" */
    0x0,

    /* U+00AE "®" */
    0x10, 0x14, 0x0,

    /* U+00AF "¯" */
    0x0,

    /* U+00B0 "°" */
    0x0,

    /* U+00B1 "±" */
    0x20, 0x40,

    /* U+00B2 "²" */
    0x14,

    /* U+00B3 "³" */
    0x51,

    /* U+00B4 "´" */
    0x0,

    /* U+00B5 "µ" */
    0x45, 0x54, 0x0,

    /* U+00B6 "¶" */
    0xa0, 0x41, 0x0,

    /* U+00B7 "·" */
    0x0,

    /* U+00B8 "¸" */
    0x0,

    /* U+00B9 "¹" */
    0x44,

    /* U+00BA "º" */
    0x50,

    /* U+00BB "»" */
    0x5, 0x0,

    /* U+00BC "¼" */
    0x0, 0x1, 0x1, 0x10,

    /* U+00BD "½" */
    0x0, 0x11, 0x1,

    /* U+00BE "¾" */
    0x0, 0x5, 0x1, 0x10,

    /* U+00BF "¿" */
    0x1, 0x5, 0x0,

    /* U+00C0 "À" */
    0x0, 0x85, 0x66,

    /* U+00C1 "Á" */
    0x0, 0x85, 0x66,

    /* U+00C2 "Â" */
    0x10, 0x85, 0x66,

    /* U+00C3 "Ã" */
    0x0, 0x85, 0x66,

    /* U+00C4 "Ä" */
    0x0, 0x85, 0x66,

    /* U+00C5 "Å" */
    0x0, 0x85, 0x66,

    /* U+00C6 "Æ" */
    0x19, 0x18, 0x99,

    /* U+00C7 "Ç" */
    0x51, 0x5, 0x0,

    /* U+00C8 "È" */
    0x1, 0x45, 0x14,

    /* U+00C9 "É" */
    0x11, 0x45, 0x14,

    /* U+00CA "Ê" */
    0x11, 0x45, 0x14,

    /* U+00CB "Ë" */
    0x1, 0x45, 0x14,

    /* U+00CC "Ì" */
    0x1, 0x11,

    /* U+00CD "Í" */
    0x4, 0x44,

    /* U+00CE "Î" */
    0x10, 0x41, 0x4,

    /* U+00CF "Ï" */
    0x4, 0x44,

    /* U+00D0 "Ð" */
    0x54, 0x84, 0x54,

    /* U+00D1 "Ñ" */
    0x2, 0x15, 0x52,

    /* U+00D2 "Ò" */
    0x10, 0x54, 0x44, 0x54,

    /* U+00D3 "Ó" */
    0x0, 0x54, 0x44, 0x54,

    /* U+00D4 "Ô" */
    0x10, 0x54, 0x44, 0x54,

    /* U+00D5 "Õ" */
    0x0, 0x54, 0x44, 0x54,

    /* U+00D6 "Ö" */
    0x0, 0x54, 0x44, 0x54,

    /* U+00D7 "×" */
    0x5, 0x0,

    /* U+00D8 "Ø" */
    0x58, 0x54, 0x94,

    /* U+00D9 "Ù" */
    0x1, 0x14, 0x55,

    /* U+00DA "Ú" */
    0x1, 0x14, 0x55,

    /* U+00DB "Û" */
    0x11, 0x14, 0x55,

    /* U+00DC "Ü" */
    0x1, 0x14, 0x55,

    /* U+00DD "Ý" */
    0x0, 0x11, 0x8, 0x4,

    /* U+00DE "Þ" */
    0x51, 0x15, 0x0,

    /* U+00DF "ß" */
    0x51, 0x45, 0x40,

    /* U+00E0 "à" */
    0x1, 0x86, 0x0,

    /* U+00E1 "á" */
    0x11, 0x86, 0x0,

    /* U+00E2 "â" */
    0x11, 0x86, 0x0,

    /* U+00E3 "ã" */
    0x1, 0x86, 0x0,

    /* U+00E4 "ä" */
    0x1, 0x86, 0x0,

    /* U+00E5 "å" */
    0x1, 0x86, 0x0,

    /* U+00E6 "æ" */
    0x69, 0x54,

    /* U+00E7 "ç" */
    0x51, 0x40, 0x0,

    /* U+00E8 "è" */
    0x1, 0x85, 0x0,

    /* U+00E9 "é" */
    0x11, 0x85, 0x0,

    /* U+00EA "ê" */
    0x11, 0x85, 0x0,

    /* U+00EB "ë" */
    0x1, 0x85, 0x0,

    /* U+00EC "ì" */
    0x1, 0x10,

    /* U+00ED "í" */
    0x4, 0x40,

    /* U+00EE "î" */
    0x10, 0x41, 0x0,

    /* U+00EF "ï" */
    0x4, 0x40,

    /* U+00F0 "ð" */
    0x61, 0x55, 0x0,

    /* U+00F1 "ñ" */
    0x11, 0x44, 0x40,

    /* U+00F2 "ò" */
    0x1, 0x45, 0x0,

    /* U+00F3 "ó" */
    0x11, 0x45, 0x0,

    /* U+00F4 "ô" */
    0x11, 0x45, 0x0,

    /* U+00F5 "õ" */
    0x1, 0x45, 0x0,

    /* U+00F6 "ö" */
    0x1, 0x45, 0x0,

    /* U+00F7 "÷" */
    0x10, 0x0,

    /* U+00F8 "ø" */
    0x1, 0x89, 0x0,

    /* U+00F9 "ù" */
    0x1, 0x15, 0x40,

    /* U+00FA "ú" */
    0x11, 0x15, 0x40,

    /* U+00FB "û" */
    0x11, 0x15, 0x40,

    /* U+00FC "ü" */
    0x1, 0x15, 0x40,

    /* U+00FD "ý" */
    0x0, 0x14, 0x14, 0x10,

    /* U+00FE "þ" */
    0x42, 0x55, 0x50,

    /* U+00FF "ÿ" */
    0x0, 0x14, 0x14, 0x10,

    /* U+0100 "Ā" */
    0x0, 0x85, 0x66,

    /* U+0101 "ā" */
    0x1, 0x86, 0x0,

    /* U+0102 "Ă" */
    0x0, 0x85, 0x66,

    /* U+0103 "ă" */
    0x1, 0x86, 0x0,

    /* U+0104 "Ą" */
    0x20, 0x54, 0x98, 0x0,

    /* U+0105 "ą" */
    0x61, 0x80, 0x0,

    /* U+0106 "Ć" */
    0x1, 0x44, 0x14,

    /* U+0107 "ć" */
    0x11, 0x45, 0x0,

    /* U+0108 "Ĉ" */
    0x11, 0x44, 0x14,

    /* U+0109 "ĉ" */
    0x11, 0x45, 0x0,

    /* U+010A "Ċ" */
    0x1, 0x44, 0x14,

    /* U+010B "ċ" */
    0x1, 0x45, 0x0,

    /* U+010C "Č" */
    0x11, 0x44, 0x14,

    /* U+010D "č" */
    0x11, 0x45, 0x0,

    /* U+010E "Ď" */
    0x10, 0x54, 0x44, 0x54,

    /* U+010F "ď" */
    0x8, 0x54, 0x54,

    /* U+0110 "Đ" */
    0x54, 0x84, 0x54,

    /* U+0111 "đ" */
    0x9, 0x55, 0x40,

    /* U+0112 "Ē" */
    0x1, 0x45, 0x14,

    /* U+0113 "ē" */
    0x1, 0x85, 0x0,

    /* U+0114 "Ĕ" */
    0x1, 0x5, 0x14,

    /* U+0115 "ĕ" */
    0x1, 0x85, 0x0,

    /* U+0116 "Ė" */
    0x1, 0x45, 0x14,

    /* U+0117 "ė" */
    0x1, 0x85, 0x0,

    /* U+0118 "Ę" */
    0x51, 0x45, 0x0,

    /* U+0119 "ę" */
    0x61, 0x40, 0x0,

    /* U+011A "Ě" */
    0x11, 0x45, 0x14,

    /* U+011B "ě" */
    0x11, 0x85, 0x0,

    /* U+011C "Ĝ" */
    0x11, 0x44, 0x15,

    /* U+011D "ĝ" */
    0x11, 0x55, 0x44,

    /* U+011E "Ğ" */
    0x1, 0x4, 0x15,

    /* U+011F "ğ" */
    0x1, 0x55, 0x44,

    /* U+0120 "Ġ" */
    0x1, 0x44, 0x15,

    /* U+0121 "ġ" */
    0x1, 0x55, 0x44,

    /* U+0122 "Ģ" */
    0x51, 0x5, 0x40,

    /* U+0123 "ģ" */
    0x1, 0x55, 0x44,

    /* U+0124 "Ĥ" */
    0x11, 0x15, 0x91,

    /* U+0125 "ĥ" */
    0x10, 0x10, 0x14, 0x11,

    /* U+0126 "Ħ" */
    0x98, 0x58, 0x44,

    /* U+0127 "ħ" */
    0x81, 0x44, 0x40,

    /* U+0128 "Ĩ" */
    0x0, 0x41, 0x4,

    /* U+0129 "ĩ" */
    0x10, 0x41, 0x0,

    /* U+012A "Ī" */
    0x4, 0x44,

    /* U+012B "ī" */
    0x4, 0x40,

    /* U+012C "Ĭ" */
    0x0, 0x1, 0x4,

    /* U+012D "ĭ" */
    0x44, 0x40,

    /* U+012E "Į" */
    0x54,

    /* U+012F "į" */
    0x14,

    /* U+0130 "İ" */
    0x15,

    /* U+0131 "ı" */
    0x50,

    /* U+0132 "Ĳ" */
    0x44, 0x11, 0x0,

    /* U+0133 "ĳ" */
    0x5, 0x51,

    /* U+0134 "Ĵ" */
    0x10, 0x10, 0x45,

    /* U+0135 "ĵ" */
    0x10, 0x41, 0x4,

    /* U+0136 "Ķ" */
    0x45, 0x84, 0x40,

    /* U+0137 "ķ" */
    0x41, 0x45, 0x0,

    /* U+0138 "ĸ" */
    0x51, 0x40,

    /* U+0139 "Ĺ" */
    0x1, 0x4, 0x14,

    /* U+013A "ĺ" */
    0x4, 0x44,

    /* U+013B "Ļ" */
    0x41, 0x5, 0x0,

    /* U+013C "ļ" */
    0x54,

    /* U+013D "Ľ" */
    0x41, 0x5, 0x0,

    /* U+013E "ľ" */
    0x44, 0x40,

    /* U+013F "Ŀ" */
    0x41, 0x5, 0x0,

    /* U+0140 "ŀ" */
    0x44, 0x40,

    /* U+0141 "Ł" */
    0x42, 0x5, 0x0,

    /* U+0142 "ł" */
    0x10, 0x81, 0x0,

    /* U+0143 "Ń" */
    0x2, 0x15, 0x52,

    /* U+0144 "ń" */
    0x11, 0x44, 0x40,

    /* U+0145 "Ņ" */
    0x85, 0x54, 0x80,

    /* U+0146 "ņ" */
    0x51, 0x10, 0x0,

    /* U+0147 "Ň" */
    0x12, 0x15, 0x52,

    /* U+0148 "ň" */
    0x11, 0x44, 0x40,

    /* U+0149 "ŉ" */
    0x1, 0x14, 0x40,

    /* U+014A "Ŋ" */
    0x85, 0x54, 0x85,

    /* U+014B "ŋ" */
    0x51, 0x11, 0x40,

    /* U+014C "Ō" */
    0x0, 0x54, 0x44, 0x54,

    /* U+014D "ō" */
    0x1, 0x45, 0x0,

    /* U+014E "Ŏ" */
    0x0, 0x44, 0x44, 0x54,

    /* U+014F "ŏ" */
    0x1, 0x45, 0x0,

    /* U+0150 "Ő" */
    0x0, 0x54, 0x44, 0x54,

    /* U+0151 "ő" */
    0x11, 0x45, 0x0,

    /* U+0152 "Œ" */
    0x59, 0x12, 0x45, 0x90,

    /* U+0153 "œ" */
    0x59, 0x16, 0x40,

    /* U+0154 "Ŕ" */
    0x11, 0x54, 0x55,

    /* U+0155 "ŕ" */
    0x8, 0x40,

    /* U+0156 "Ŗ" */
    0x55, 0x15, 0x40,

    /* U+0157 "ŗ" */
    0x84, 0x0,

    /* U+0158 "Ř" */
    0x11, 0x54, 0x55,

    /* U+0159 "ř" */
    0x8, 0x40,

    /* U+015A "Ś" */
    0x1, 0x45, 0x14,

    /* U+015B "ś" */
    0x5, 0x60,

    /* U+015C "Ŝ" */
    0x11, 0x45, 0x14,

    /* U+015D "ŝ" */
    0x5, 0x60,

    /* U+015E "Ş" */
    0x51, 0x45, 0x0,

    /* U+015F "ş" */
    0x56, 0x0,

    /* U+0160 "Š" */
    0x11, 0x45, 0x14,

    /* U+0161 "š" */
    0x5, 0x60,

    /* U+0162 "Ţ" */
    0x60, 0x41, 0x0,

    /* U+0163 "ţ" */
    0x8, 0x40,

    /* U+0164 "Ť" */
    0x11, 0x81, 0x4,

    /* U+0165 "ť" */
    0x8, 0x40,

    /* U+0166 "Ŧ" */
    0x60, 0x81, 0x0,

    /* U+0167 "ŧ" */
    0x4c, 0x40,

    /* U+0168 "Ũ" */
    0x1, 0x14, 0x55,

    /* U+0169 "ũ" */
    0x11, 0x15, 0x40,

    /* U+016A "Ū" */
    0x1, 0x14, 0x55,

    /* U+016B "ū" */
    0x1, 0x15, 0x40,

    /* U+016C "Ŭ" */
    0x1, 0x14, 0x55,

    /* U+016D "ŭ" */
    0x1, 0x15, 0x40,

    /* U+016E "Ů" */
    0x1, 0x14, 0x55,

    /* U+016F "ů" */
    0x1, 0x15, 0x40,

    /* U+0170 "Ű" */
    0x1, 0x14, 0x55,

    /* U+0171 "ű" */
    0x11, 0x15, 0x40,

    /* U+0172 "Ų" */
    0x45, 0x15, 0x0,

    /* U+0173 "ų" */
    0x45, 0x50, 0x0,

    /* U+0174 "Ŵ" */
    0x0, 0x12, 0x5, 0x50, 0x88,

    /* U+0175 "ŵ" */
    0x10, 0x51, 0x58,

    /* U+0176 "Ŷ" */
    0x0, 0x11, 0x8, 0x4,

    /* U+0177 "ŷ" */
    0x0, 0x14, 0x14, 0x10,

    /* U+0178 "Ÿ" */
    0x0, 0x11, 0x8, 0x4,

    /* U+0179 "Ź" */
    0x10, 0x91, 0x24,

    /* U+017A "ź" */
    0x6, 0x90,

    /* U+017B "Ż" */
    0x0, 0x91, 0x24,

    /* U+017C "ż" */
    0x6, 0x90,

    /* U+017D "Ž" */
    0x10, 0x91, 0x24,

    /* U+017E "ž" */
    0x6, 0x90,

    /* U+017F "ſ" */
    0x44, 0x40,

    /* U+1E08 "Ḉ" */
    0x1, 0x44, 0x14, 0x0,

    /* U+1E09 "ḉ" */
    0x11, 0x45, 0x0,

    /* U+1E0C "Ḍ" */
    0x54, 0x44, 0x54, 0x0,

    /* U+1E0D "ḍ" */
    0x5, 0x55, 0x40,

    /* U+1E0E "Ḏ" */
    0x54, 0x44, 0x54, 0x0,

    /* U+1E0F "ḏ" */
    0x5, 0x55, 0x40,

    /* U+1E14 "Ḕ" */
    0x0, 0x45, 0x14, 0x50,

    /* U+1E15 "ḕ" */
    0x0, 0x6, 0x14,

    /* U+1E16 "Ḗ" */
    0x0, 0x45, 0x14, 0x50,

    /* U+1E17 "ḗ" */
    0x0, 0x6, 0x14,

    /* U+1E1C "Ḝ" */
    0x1, 0x5, 0x14, 0x10,

    /* U+1E1D "ḝ" */
    0x1, 0x85, 0x0,

    /* U+1E20 "Ḡ" */
    0x1, 0x44, 0x15,

    /* U+1E21 "ḡ" */
    0x1, 0x55, 0x44,

    /* U+1E24 "Ḥ" */
    0x45, 0x64, 0x40,

    /* U+1E25 "ḥ" */
    0x41, 0x44, 0x40,

    /* U+1E2A "Ḫ" */
    0x45, 0x64, 0x40,

    /* U+1E2B "ḫ" */
    0x41, 0x44, 0x40,

    /* U+1E2E "Ḯ" */
    0x4, 0x44, 0x40,

    /* U+1E2F "ḯ" */
    0x0, 0x44,

    /* U+1E36 "Ḷ" */
    0x41, 0x5, 0x0,

    /* U+1E37 "ḷ" */
    0x54,

    /* U+1E3A "Ḻ" */
    0x41, 0x5, 0x0,

    /* U+1E3B "ḻ" */
    0x10, 0x41, 0x0,

    /* U+1E42 "Ṃ" */
    0x81, 0x55, 0x41, 0x0,

    /* U+1E43 "ṃ" */
    0x55, 0x45, 0x0,

    /* U+1E44 "Ṅ" */
    0x2, 0x15, 0x52,

    /* U+1E45 "ṅ" */
    0x1, 0x44, 0x40,

    /* U+1E46 "Ṇ" */
    0x85, 0x54, 0x80,

    /* U+1E47 "ṇ" */
    0x51, 0x10, 0x0,

    /* U+1E48 "Ṉ" */
    0x85, 0x54, 0x80,

    /* U+1E49 "ṉ" */
    0x51, 0x10, 0x0,

    /* U+1E4C "Ṍ" */
    0x0, 0x54, 0x44, 0x54,

    /* U+1E4D "ṍ" */
    0x0, 0x5, 0x14,

    /* U+1E4E "Ṏ" */
    0x10, 0x54, 0x44, 0x54,

    /* U+1E4F "ṏ" */
    0x0, 0x5, 0x14,

    /* U+1E50 "Ṑ" */
    0x0, 0x10, 0x54, 0x44, 0x54,

    /* U+1E51 "ṑ" */
    0x0, 0x5, 0x14,

    /* U+1E52 "Ṓ" */
    0x0, 0x10, 0x54, 0x44, 0x54,

    /* U+1E53 "ṓ" */
    0x10, 0x5, 0x14,

    /* U+1E5A "Ṛ" */
    0x55, 0x15, 0x40,

    /* U+1E5B "ṛ" */
    0x84, 0x0,

    /* U+1E5E "Ṟ" */
    0x55, 0x15, 0x40,

    /* U+1E5F "ṟ" */
    0x20, 0x40, 0x0,

    /* U+1E60 "Ṡ" */
    0x1, 0x45, 0x14,

    /* U+1E61 "ṡ" */
    0x5, 0x60,

    /* U+1E62 "Ṣ" */
    0x51, 0x45, 0x0,

    /* U+1E63 "ṣ" */
    0x56, 0x0,

    /* U+1E64 "Ṥ" */
    0x11, 0x45, 0x14,

    /* U+1E65 "ṥ" */
    0x0, 0x56,

    /* U+1E66 "Ṧ" */
    0x1, 0x45, 0x14,

    /* U+1E67 "ṧ" */
    0x5, 0x60,

    /* U+1E68 "Ṩ" */
    0x41, 0x45, 0x0,

    /* U+1E69 "ṩ" */
    0x5, 0x60,

    /* U+1E6C "Ṭ" */
    0x60, 0x41, 0x0,

    /* U+1E6D "ṭ" */
    0x84, 0x0,

    /* U+1E6E "Ṯ" */
    0x60, 0x41, 0x0,

    /* U+1E6F "ṯ" */
    0x84, 0x0,

    /* U+1E78 "Ṹ" */
    0x1, 0x14, 0x55,

    /* U+1E79 "ṹ" */
    0x0, 0x44, 0x55,

    /* U+1E7A "Ṻ" */
    0x11, 0x14, 0x55,

    /* U+1E7B "ṻ" */
    0x0, 0x4, 0x55,

    /* U+1E80 "Ẁ" */
    0x0, 0x12, 0x5, 0x50, 0x88,

    /* U+1E81 "ẁ" */
    0x10, 0x51, 0x58,

    /* U+1E82 "Ẃ" */
    0x0, 0x12, 0x5, 0x50, 0x88,

    /* U+1E83 "ẃ" */
    0x0, 0x51, 0x58,

    /* U+1E84 "Ẅ" */
    0x0, 0x12, 0x5, 0x50, 0x88,

    /* U+1E85 "ẅ" */
    0x0, 0x51, 0x58,

    /* U+1E8E "Ẏ" */
    0x0, 0x11, 0x8, 0x4,

    /* U+1E8F "ẏ" */
    0x0, 0x14, 0x14, 0x10,

    /* U+1E92 "Ẓ" */
    0x24, 0x49, 0x0,

    /* U+1E93 "ẓ" */
    0x69, 0x0,

    /* U+1E97 "ẗ" */
    0x8, 0x40,

    /* U+1E9E "ẞ" */
    0x55, 0x54, 0x40,

    /* U+1EA0 "Ạ" */
    0x21, 0x59, 0x80,

    /* U+1EA1 "ạ" */
    0x61, 0x80, 0x0,

    /* U+1EA2 "Ả" */
    0x0, 0x85, 0x66,

    /* U+1EA3 "ả" */
    0x1, 0x86, 0x0,

    /* U+1EA4 "Ấ" */
    0x0, 0x85, 0x66,

    /* U+1EA5 "ấ" */
    0x0, 0x46, 0x18,

    /* U+1EA6 "Ầ" */
    0x10, 0x85, 0x66,

    /* U+1EA7 "ầ" */
    0x11, 0x86, 0x0,

    /* U+1EA8 "Ẩ" */
    0x0, 0xc5, 0x66,

    /* U+1EA9 "ẩ" */
    0x0, 0x6, 0x18,

    /* U+1EAA "Ẫ" */
    0x10, 0x85, 0x66,

    /* U+1EAB "ẫ" */
    0x51, 0x86, 0x0,

    /* U+1EAC "Ậ" */
    0x31, 0x59, 0x80,

    /* U+1EAD "ậ" */
    0x11, 0x86, 0x0,

    /* U+1EAE "Ắ" */
    0x0, 0x45, 0x66,

    /* U+1EAF "ắ" */
    0x0, 0x46, 0x18,

    /* U+1EB0 "Ằ" */
    0x0, 0x45, 0x66,

    /* U+1EB1 "ằ" */
    0x0, 0x46, 0x18,

    /* U+1EB2 "Ẳ" */
    0x0, 0x45, 0x66,

    /* U+1EB3 "ẳ" */
    0x0, 0x6, 0x18,

    /* U+1EB4 "Ẵ" */
    0x0, 0x85, 0x66,

    /* U+1EB5 "ẵ" */
    0x0, 0x6, 0x18,

    /* U+1EB6 "Ặ" */
    0x11, 0x59, 0x80,

    /* U+1EB7 "ặ" */
    0x1, 0x86, 0x0,

    /* U+1EB8 "Ẹ" */
    0x51, 0x45, 0x0,

    /* U+1EB9 "ẹ" */
    0x51, 0x80, 0x0,

    /* U+1EBA "Ẻ" */
    0x1, 0x45, 0x14,

    /* U+1EBB "ẻ" */
    0x1, 0x85, 0x0,

    /* U+1EBC "Ẽ" */
    0x1, 0x5, 0x14,

    /* U+1EBD "ẽ" */
    0x1, 0x85, 0x0,

    /* U+1EBE "Ế" */
    0x25, 0x45, 0x14,

    /* U+1EBF "ế" */
    0x11, 0x85, 0x0,

    /* U+1EC0 "Ề" */
    0x11, 0x45, 0x14,

    /* U+1EC1 "ề" */
    0x11, 0x85, 0x0,

    /* U+1EC2 "Ể" */
    0x12, 0x45, 0x14,

    /* U+1EC3 "ể" */
    0x0, 0x6, 0x14,

    /* U+1EC4 "Ễ" */
    0x21, 0x45, 0x14,

    /* U+1EC5 "ễ" */
    0x11, 0x85, 0x0,

    /* U+1EC6 "Ệ" */
    0x12, 0x85, 0x14, 0x0,

    /* U+1EC7 "ệ" */
    0x11, 0x46, 0x0,

    /* U+1EC8 "Ỉ" */
    0x4, 0x44,

    /* U+1EC9 "ỉ" */
    0x14,

    /* U+1ECA "Ị" */
    0x54,

    /* U+1ECB "ị" */
    0x14,

    /* U+1ECC "Ọ" */
    0x54, 0x44, 0x54, 0x0,

    /* U+1ECD "ọ" */
    0x51, 0x40, 0x0,

    /* U+1ECE "Ỏ" */
    0x0, 0x54, 0x44, 0x54,

    /* U+1ECF "ỏ" */
    0x1, 0x45, 0x0,

    /* U+1ED0 "Ố" */
    0x14, 0x54, 0x44, 0x54,

    /* U+1ED1 "ố" */
    0x11, 0x45, 0x0,

    /* U+1ED2 "Ồ" */
    0x10, 0x54, 0x44, 0x54,

    /* U+1ED3 "ồ" */
    0x11, 0x45, 0x0,

    /* U+1ED4 "Ổ" */
    0x10, 0x54, 0x44, 0x54,

    /* U+1ED5 "ổ" */
    0x0, 0x5, 0x14,

    /* U+1ED6 "Ỗ" */
    0x20, 0x54, 0x44, 0x54,

    /* U+1ED7 "ỗ" */
    0x11, 0x45, 0x0,

    /* U+1ED8 "Ộ" */
    0x10, 0x68, 0x44, 0x54, 0x0,

    /* U+1ED9 "ộ" */
    0x11, 0x45, 0x0,

    /* U+1EDA "Ớ" */
    0x0, 0x58, 0x44, 0x54,

    /* U+1EDB "ớ" */
    0x11, 0x45, 0x0,

    /* U+1EDC "Ờ" */
    0x10, 0x58, 0x44, 0x54,

    /* U+1EDD "ờ" */
    0x1, 0x45, 0x0,

    /* U+1EDE "Ở" */
    0x0, 0x58, 0x44, 0x54,

    /* U+1EDF "ở" */
    0x1, 0x45, 0x0,

    /* U+1EE0 "Ỡ" */
    0x0, 0x58, 0x44, 0x54,

    /* U+1EE1 "ỡ" */
    0x1, 0x45, 0x0,

    /* U+1EE2 "Ợ" */
    0x0, 0x58, 0x44, 0x54, 0x0,

    /* U+1EE3 "ợ" */
    0x1, 0x45, 0x0,

    /* U+1EE4 "Ụ" */
    0x45, 0x15, 0x40,

    /* U+1EE5 "ụ" */
    0x45, 0x50, 0x0,

    /* U+1EE6 "Ủ" */
    0x1, 0x14, 0x55,

    /* U+1EE7 "ủ" */
    0x1, 0x15, 0x40,

    /* U+1EE8 "Ứ" */
    0x0, 0x44, 0x44, 0x54,

    /* U+1EE9 "ứ" */
    0x11, 0x15, 0x40,

    /* U+1EEA "Ừ" */
    0x0, 0x44, 0x44, 0x54,

    /* U+1EEB "ừ" */
    0x1, 0x15, 0x40,

    /* U+1EEC "Ử" */
    0x0, 0x44, 0x44, 0x54,

    /* U+1EED "ử" */
    0x1, 0x15, 0x40,

    /* U+1EEE "Ữ" */
    0x0, 0x44, 0x44, 0x54,

    /* U+1EEF "ữ" */
    0x11, 0x15, 0x40,

    /* U+1EF0 "Ự" */
    0x0, 0x44, 0x44, 0x54, 0x0,

    /* U+1EF1 "ự" */
    0x1, 0x15, 0x40,

    /* U+1EF2 "Ỳ" */
    0x0, 0x11, 0x8, 0x4,

    /* U+1EF3 "ỳ" */
    0x0, 0x14, 0x14, 0x10,

    /* U+1EF4 "Ỵ" */
    0x11, 0x8, 0x4, 0x0,

    /* U+1EF5 "ỵ" */
    0x14, 0x14, 0x14,

    /* U+1EF6 "Ỷ" */
    0x0, 0x11, 0x8, 0x4,

    /* U+1EF7 "ỷ" */
    0x0, 0x14, 0x14, 0x10,

    /* U+1EF8 "Ỹ" */
    0x0, 0x11, 0x8, 0x4,

    /* U+1EF9 "ỹ" */
    0x0, 0x14, 0x14, 0x10,

    /* U+2022 "•" */
    0x40,

    /* U+2026 "…" */
    0x0,

    /* U+2190 "←" */
    0x50, 0x0,

    /* U+2191 "↑" */
    0x20, 0x0,

    /* U+2192 "→" */
    0x20, 0x0,

    /* U+2193 "↓" */
    0x10, 0x40,

    /* U+F001 "" */
    0x0, 0x2f, 0x22, 0x27, 0x90,

    /* U+F008 "" */
    0x96, 0x35, 0xc9, 0x60,

    /* U+F00B "" */
    0x0, 0xef, 0xaa, 0xef, 0x0,

    /* U+F00C "" */
    0x2, 0x9c, 0x30,

    /* U+F00D "" */
    0x99, 0xc9, 0x80,

    /* U+F011 "" */
    0x0, 0x12, 0x88, 0x56, 0x8, 0x18, 0x0,

    /* U+F013 "" */
    0x0, 0xb, 0xd2, 0x64, 0xbd, 0x0, 0x0,

    /* U+F015 "" */
    0x5, 0xa, 0xcb, 0xe0, 0xd8, 0x0, 0x0,

    /* U+F019 "" */
    0x4, 0x2, 0x41, 0xf0, 0xea, 0x29, 0x40,

    /* U+F01C "" */
    0x6a, 0x34, 0x9f, 0xf4,

    /* U+F021 "" */
    0x29, 0x87, 0x50, 0xd2, 0x54,

    /* U+F026 "" */
    0x7, 0xf2,

    /* U+F027 "" */
    0x1, 0xcf, 0x48,

    /* U+F028 "" */
    0x0, 0x1c, 0x4f, 0x54, 0x84, 0x0, 0x0,

    /* U+F03E "" */
    0xbf, 0xe2, 0xaa,

    /* U+F043 "" */
    0x0, 0xcf, 0xae, 0x10,

    /* U+F048 "" */
    0x2, 0x7b, 0xe3, 0x0,

    /* U+F04C "" */
    0x44, 0xee, 0xee, 0xdd, 0x0,

    /* U+F04D "" */
    0x54, 0xfe, 0xfe, 0xfd, 0x0,

    /* U+F051 "" */
    0x2, 0xeb, 0xd6, 0x0,

    /* U+F052 "" */
    0x4, 0x7, 0xc2, 0xf4, 0xfd, 0x0, 0x0,

    /* U+F053 "" */
    0x0, 0x89, 0x8, 0x0,

    /* U+F054 "" */
    0x1, 0x42, 0x14, 0x0,

    /* U+F067 "" */
    0x0, 0x20, 0xbd, 0x20, 0x0,

    /* U+F068 "" */
    0xbd,

    /* U+F06E "" */
    0x6a, 0x37, 0xc6, 0xa0,

    /* U+F070 "" */
    0x10, 0x1, 0xa8, 0x27, 0xa0, 0x99, 0x0, 0x10,

    /* U+F071 "" */
    0x0, 0x0, 0xb0, 0xd, 0x82, 0xec, 0x1a, 0x80,

    /* U+F074 "" */
    0x0, 0x9b, 0x25, 0x9b, 0x0,

    /* U+F077 "" */
    0x10, 0x68, 0x40,

    /* U+F078 "" */
    0x40, 0x68, 0x10,

    /* U+F079 "" */
    0x0, 0x7, 0x64, 0x20, 0x82, 0x5c, 0x0, 0x0,

    /* U+F07B "" */
    0xf9, 0xff, 0xff,

    /* U+F093 "" */
    0x0, 0x3c, 0x28, 0xaa, 0x65,

    /* U+F095 "" */
    0x0, 0x40, 0x70, 0x18, 0xf8, 0x14, 0x0,

    /* U+F0C4 "" */
    0x40, 0xac, 0x74, 0x98, 0x0,

    /* U+F0C5 "" */
    0x29, 0xbd, 0xfe, 0xee, 0x64,

    /* U+F0C7 "" */
    0x54, 0x88, 0xed, 0xed, 0x0,

    /* U+F0C9 "" */
    0xa9, 0xa9, 0xa9,

    /* U+F0E0 "" */
    0xff, 0xff, 0xff,

    /* U+F0E7 "" */
    0x0, 0x38, 0x3d, 0x8, 0x0,

    /* U+F0EA "" */
    0x10, 0xe4, 0xe9, 0xee, 0x19,

    /* U+F0F3 "" */
    0x0, 0x78, 0xbc, 0xbd, 0x10,

    /* U+F11C "" */
    0xba, 0x7a, 0xea, 0xa4,

    /* U+F124 "" */
    0x0, 0x40, 0xb8, 0x2f, 0x40, 0x70, 0x0, 0x0,

    /* U+F15B "" */
    0x43, 0xef, 0xff, 0xa8,

    /* U+F1EB "" */
    0x4, 0x25, 0x62, 0xa0, 0x20, 0x0, 0x0,

    /* U+F240 "" */
    0xaa, 0xbf, 0xea, 0xa8,

    /* U+F241 "" */
    0xaa, 0xbf, 0x6a, 0xa8,

    /* U+F242 "" */
    0xaa, 0xbe, 0x2a, 0xa8,

    /* U+F243 "" */
    0xaa, 0xbc, 0x2a, 0xa8,

    /* U+F244 "" */
    0xaa, 0xa0, 0x2a, 0xa8,

    /* U+F287 "" */
    0x4, 0xd, 0x58, 0x6, 0x0,

    /* U+F293 "" */
    0x0, 0xa8, 0xdc, 0x98, 0x64,

    /* U+F2ED "" */
    0x14, 0xbc, 0xad, 0xad, 0x64,

    /* U+F304 "" */
    0x0, 0x40, 0xb0, 0xf0, 0xf0, 0x10, 0x0,

    /* U+F55A "" */
    0x3f, 0xfd, 0x73, 0xfc,

    /* U+F7C2 "" */
    0x14, 0x98, 0xfc, 0xfc, 0xa8,

    /* U+F8A2 "" */
    0x1, 0xeb, 0x40
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 17, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 17, .box_w = 1, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 25, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 2, .adv_w = 45, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 5, .adv_w = 40, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 8, .adv_w = 54, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 11, .adv_w = 44, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 14, .adv_w = 13, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 15, .adv_w = 22, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 17, .adv_w = 22, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 18, .adv_w = 26, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 19, .adv_w = 37, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 21, .adv_w = 15, .box_w = 1, .box_h = 2, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 22, .adv_w = 25, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 23, .adv_w = 15, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 24, .adv_w = 23, .box_w = 3, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 27, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 24, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 35, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 38, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 41, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 44, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 47, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 50, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 53, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 56, .adv_w = 15, .box_w = 1, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 57, .adv_w = 15, .box_w = 1, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 58, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 61, .adv_w = 37, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 63, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 66, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 69, .adv_w = 66, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 73, .adv_w = 47, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 76, .adv_w = 48, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 79, .adv_w = 46, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 53, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 85, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 88, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 91, .adv_w = 49, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 94, .adv_w = 52, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 20, .box_w = 1, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 98, .adv_w = 33, .box_w = 3, .box_h = 3, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 101, .adv_w = 46, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 104, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 107, .adv_w = 61, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 110, .adv_w = 52, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 113, .adv_w = 54, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 116, .adv_w = 46, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 123, .adv_w = 47, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 126, .adv_w = 40, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 129, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 132, .adv_w = 51, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 135, .adv_w = 46, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 138, .adv_w = 72, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 145, .adv_w = 41, .box_w = 4, .box_h = 3, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 148, .adv_w = 42, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 21, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 153, .adv_w = 23, .box_w = 3, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 21, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 157, .adv_w = 37, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 159, .adv_w = 32, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 160, .adv_w = 38, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 161, .adv_w = 38, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 166, .adv_w = 37, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 168, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 171, .adv_w = 39, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 173, .adv_w = 23, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 175, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 178, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 181, .adv_w = 18, .box_w = 1, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 182, .adv_w = 18, .box_w = 2, .box_h = 4, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 184, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 187, .adv_w = 18, .box_w = 1, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 68, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 190, .adv_w = 44, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 192, .adv_w = 41, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 194, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 197, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 200, .adv_w = 26, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 201, .adv_w = 32, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 202, .adv_w = 26, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 204, .adv_w = 43, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 206, .adv_w = 36, .box_w = 4, .box_h = 2, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 58, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 210, .adv_w = 35, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 212, .adv_w = 36, .box_w = 4, .box_h = 3, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 215, .adv_w = 33, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 216, .adv_w = 22, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 218, .adv_w = 19, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 219, .adv_w = 22, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 221, .adv_w = 37, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 222, .adv_w = 17, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 222, .adv_w = 17, .box_w = 1, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 223, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 226, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 229, .adv_w = 45, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 232, .adv_w = 45, .box_w = 4, .box_h = 3, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 235, .adv_w = 19, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 236, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 238, .adv_w = 38, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 239, .adv_w = 51, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 242, .adv_w = 26, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 243, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 245, .adv_w = 37, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 247, .adv_w = 25, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 248, .adv_w = 51, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 38, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 252, .adv_w = 27, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 253, .adv_w = 37, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 255, .adv_w = 28, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 256, .adv_w = 28, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 257, .adv_w = 38, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 258, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 261, .adv_w = 41, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 264, .adv_w = 17, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 265, .adv_w = 38, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 266, .adv_w = 28, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 267, .adv_w = 27, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 268, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 270, .adv_w = 66, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 274, .adv_w = 66, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 277, .adv_w = 66, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 281, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 284, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 287, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 290, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 293, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 296, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 299, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 302, .adv_w = 67, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 305, .adv_w = 46, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 308, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 311, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 314, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 317, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 320, .adv_w = 20, .box_w = 2, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 322, .adv_w = 20, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 324, .adv_w = 20, .box_w = 3, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 327, .adv_w = 20, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 329, .adv_w = 53, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 332, .adv_w = 52, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 335, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 339, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 343, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 347, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 351, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 355, .adv_w = 37, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 357, .adv_w = 54, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 363, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 366, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 369, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 372, .adv_w = 41, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 376, .adv_w = 46, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 379, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 382, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 385, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 388, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 391, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 394, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 397, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 400, .adv_w = 63, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 402, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 405, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 408, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 411, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 414, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 417, .adv_w = 18, .box_w = 2, .box_h = 3, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 419, .adv_w = 18, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 421, .adv_w = 18, .box_w = 3, .box_h = 3, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 424, .adv_w = 18, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 426, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 429, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 432, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 435, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 438, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 441, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 444, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 447, .adv_w = 37, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 449, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 452, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 455, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 458, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 461, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 464, .adv_w = 36, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 468, .adv_w = 44, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 471, .adv_w = 36, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 475, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 478, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 481, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 487, .adv_w = 47, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 491, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 494, .adv_w = 46, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 497, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 500, .adv_w = 46, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 503, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 506, .adv_w = 46, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 509, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 512, .adv_w = 46, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 515, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 518, .adv_w = 53, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 522, .adv_w = 44, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 525, .adv_w = 53, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 528, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 531, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 534, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 537, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 540, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 543, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 546, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 549, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 552, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 555, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 558, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 561, .adv_w = 49, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 564, .adv_w = 44, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 567, .adv_w = 49, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 570, .adv_w = 44, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 573, .adv_w = 49, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 576, .adv_w = 44, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 579, .adv_w = 49, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 582, .adv_w = 44, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 585, .adv_w = 52, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 588, .adv_w = 44, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 592, .adv_w = 53, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 595, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 598, .adv_w = 20, .box_w = 3, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 601, .adv_w = 18, .box_w = 3, .box_h = 3, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 604, .adv_w = 20, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 606, .adv_w = 18, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 608, .adv_w = 20, .box_w = 3, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 611, .adv_w = 18, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 613, .adv_w = 20, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 614, .adv_w = 18, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 615, .adv_w = 20, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 616, .adv_w = 18, .box_w = 1, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 617, .adv_w = 46, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 620, .adv_w = 36, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 622, .adv_w = 33, .box_w = 3, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 625, .adv_w = 18, .box_w = 3, .box_h = 4, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 628, .adv_w = 46, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 631, .adv_w = 39, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 634, .adv_w = 39, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 636, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 639, .adv_w = 18, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 641, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 644, .adv_w = 18, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 645, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 648, .adv_w = 18, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 650, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 653, .adv_w = 22, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 655, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 658, .adv_w = 19, .box_w = 3, .box_h = 3, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 661, .adv_w = 52, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 664, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 667, .adv_w = 52, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 670, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 673, .adv_w = 52, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 676, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 679, .adv_w = 49, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 682, .adv_w = 52, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 685, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 688, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 692, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 695, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 699, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 702, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 706, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 709, .adv_w = 72, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 713, .adv_w = 68, .box_w = 5, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 716, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 719, .adv_w = 26, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 721, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 724, .adv_w = 26, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 726, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 729, .adv_w = 26, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 731, .adv_w = 40, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 734, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 736, .adv_w = 40, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 739, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 741, .adv_w = 40, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 744, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 746, .adv_w = 40, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 749, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 751, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 754, .adv_w = 26, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 756, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 759, .adv_w = 26, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 761, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 764, .adv_w = 27, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 766, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 769, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 772, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 775, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 778, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 781, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 784, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 787, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 790, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 793, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 796, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 799, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 802, .adv_w = 72, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 807, .adv_w = 58, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 810, .adv_w = 41, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 814, .adv_w = 36, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 818, .adv_w = 41, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 822, .adv_w = 42, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 825, .adv_w = 33, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 827, .adv_w = 42, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 830, .adv_w = 33, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 832, .adv_w = 42, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 835, .adv_w = 33, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 837, .adv_w = 20, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 839, .adv_w = 46, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 843, .adv_w = 37, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 846, .adv_w = 53, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 850, .adv_w = 44, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 853, .adv_w = 53, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 857, .adv_w = 44, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 860, .adv_w = 43, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 864, .adv_w = 39, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 867, .adv_w = 43, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 871, .adv_w = 39, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 874, .adv_w = 43, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 878, .adv_w = 39, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 881, .adv_w = 49, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 884, .adv_w = 44, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 887, .adv_w = 52, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 890, .adv_w = 44, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 893, .adv_w = 52, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 896, .adv_w = 44, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 899, .adv_w = 20, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 902, .adv_w = 18, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 904, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 907, .adv_w = 18, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 908, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 911, .adv_w = 18, .box_w = 3, .box_h = 4, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 914, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 918, .adv_w = 68, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 921, .adv_w = 52, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 924, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 927, .adv_w = 52, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 930, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 933, .adv_w = 52, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 936, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 939, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 943, .adv_w = 41, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 946, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 950, .adv_w = 41, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 953, .adv_w = 54, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 958, .adv_w = 41, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 961, .adv_w = 54, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 966, .adv_w = 41, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 969, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 972, .adv_w = 26, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 974, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 977, .adv_w = 26, .box_w = 3, .box_h = 3, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 980, .adv_w = 40, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 983, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 985, .adv_w = 40, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 988, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 990, .adv_w = 40, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 993, .adv_w = 32, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 995, .adv_w = 40, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 998, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1000, .adv_w = 40, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1003, .adv_w = 32, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1005, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1008, .adv_w = 26, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1010, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1013, .adv_w = 26, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1015, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1018, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1021, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1024, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1027, .adv_w = 72, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1032, .adv_w = 58, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1035, .adv_w = 72, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1040, .adv_w = 58, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1043, .adv_w = 72, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1048, .adv_w = 58, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1051, .adv_w = 41, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1055, .adv_w = 36, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1059, .adv_w = 42, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1062, .adv_w = 33, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1064, .adv_w = 26, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1066, .adv_w = 49, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1069, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1072, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1075, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1078, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1081, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1084, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1087, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1090, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1093, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1096, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1099, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1102, .adv_w = 38, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1105, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1108, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1111, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1114, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1117, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1120, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1123, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1126, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1129, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1132, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1135, .adv_w = 47, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1138, .adv_w = 38, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1141, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1144, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1147, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1150, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1153, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1156, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1159, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1162, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1165, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1168, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1171, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1174, .adv_w = 39, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1177, .adv_w = 43, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1180, .adv_w = 39, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1183, .adv_w = 43, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1187, .adv_w = 39, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1190, .adv_w = 20, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1192, .adv_w = 18, .box_w = 1, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1193, .adv_w = 20, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1194, .adv_w = 18, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1195, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1199, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1202, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1206, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1209, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1213, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1216, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1220, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1223, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1227, .adv_w = 41, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1230, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1234, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1237, .adv_w = 54, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1242, .adv_w = 41, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1245, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1249, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1252, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1256, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1259, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1263, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1266, .adv_w = 54, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1270, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1273, .adv_w = 54, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1278, .adv_w = 41, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1281, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1284, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1287, .adv_w = 51, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1290, .adv_w = 43, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1293, .adv_w = 51, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1297, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1300, .adv_w = 51, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1304, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1307, .adv_w = 51, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1311, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1314, .adv_w = 51, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1318, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1321, .adv_w = 51, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1326, .adv_w = 44, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1329, .adv_w = 41, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1333, .adv_w = 36, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1337, .adv_w = 41, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1341, .adv_w = 36, .box_w = 4, .box_h = 3, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1344, .adv_w = 41, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1348, .adv_w = 36, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1352, .adv_w = 41, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1356, .adv_w = 36, .box_w = 4, .box_h = 4, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1360, .adv_w = 20, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1361, .adv_w = 44, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1362, .adv_w = 38, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1364, .adv_w = 38, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1366, .adv_w = 38, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1368, .adv_w = 38, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1370, .adv_w = 64, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1375, .adv_w = 64, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1379, .adv_w = 64, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1384, .adv_w = 64, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1387, .adv_w = 44, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1390, .adv_w = 64, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1397, .adv_w = 64, .box_w = 5, .box_h = 5, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1404, .adv_w = 72, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1411, .adv_w = 64, .box_w = 5, .box_h = 5, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1418, .adv_w = 72, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1422, .adv_w = 64, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1427, .adv_w = 32, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1429, .adv_w = 48, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1432, .adv_w = 72, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1439, .adv_w = 64, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1442, .adv_w = 44, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1446, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1450, .adv_w = 56, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1455, .adv_w = 56, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1460, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1464, .adv_w = 56, .box_w = 5, .box_h = 5, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1471, .adv_w = 40, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1475, .adv_w = 40, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1479, .adv_w = 56, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1484, .adv_w = 56, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1485, .adv_w = 72, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1489, .adv_w = 80, .box_w = 6, .box_h = 5, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1497, .adv_w = 72, .box_w = 6, .box_h = 5, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1505, .adv_w = 64, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1510, .adv_w = 56, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1513, .adv_w = 56, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1516, .adv_w = 80, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1524, .adv_w = 64, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1527, .adv_w = 64, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1532, .adv_w = 64, .box_w = 5, .box_h = 5, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1539, .adv_w = 56, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1544, .adv_w = 56, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1549, .adv_w = 56, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1554, .adv_w = 56, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1557, .adv_w = 64, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1560, .adv_w = 40, .box_w = 4, .box_h = 5, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1565, .adv_w = 56, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1570, .adv_w = 56, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1575, .adv_w = 72, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1579, .adv_w = 64, .box_w = 6, .box_h = 5, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1587, .adv_w = 48, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1591, .adv_w = 80, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1598, .adv_w = 80, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1602, .adv_w = 80, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1606, .adv_w = 80, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1610, .adv_w = 80, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1614, .adv_w = 80, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1618, .adv_w = 80, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1623, .adv_w = 56, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1628, .adv_w = 56, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1633, .adv_w = 64, .box_w = 5, .box_h = 5, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 1640, .adv_w = 80, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1644, .adv_w = 48, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1649, .adv_w = 64, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_2[] = {
    0x0, 0x1, 0x4, 0x5, 0x6, 0x7, 0xc, 0xd,
    0xe, 0xf, 0x14, 0x15, 0x18, 0x19, 0x1c, 0x1d,
    0x22, 0x23, 0x26, 0x27, 0x2e, 0x2f, 0x32, 0x33,
    0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41,
    0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
    0x52, 0x53, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b,
    0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x64, 0x65,
    0x66, 0x67, 0x70, 0x71, 0x72, 0x73, 0x78, 0x79,
    0x7a, 0x7b, 0x7c, 0x7d, 0x86, 0x87, 0x8a, 0x8b,
    0x8f, 0x96
};

static const uint16_t unicode_list_4[] = {
    0x0, 0x4, 0x16e, 0x16f, 0x170, 0x171, 0xcfdf, 0xcfe6,
    0xcfe9, 0xcfea, 0xcfeb, 0xcfef, 0xcff1, 0xcff3, 0xcff7, 0xcffa,
    0xcfff, 0xd004, 0xd005, 0xd006, 0xd01c, 0xd021, 0xd026, 0xd02a,
    0xd02b, 0xd02f, 0xd030, 0xd031, 0xd032, 0xd045, 0xd046, 0xd04c,
    0xd04e, 0xd04f, 0xd052, 0xd055, 0xd056, 0xd057, 0xd059, 0xd071,
    0xd073, 0xd0a2, 0xd0a3, 0xd0a5, 0xd0a7, 0xd0be, 0xd0c5, 0xd0c8,
    0xd0d1, 0xd0fa, 0xd102, 0xd139, 0xd1c9, 0xd21e, 0xd21f, 0xd220,
    0xd221, 0xd222, 0xd265, 0xd271, 0xd2cb, 0xd2e2, 0xd538, 0xd7a0,
    0xd880
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 160, .range_length = 224, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 7688, .range_length = 151, .glyph_id_start = 320,
        .unicode_list = unicode_list_2, .glyph_id_ofs_list = NULL, .list_length = 74, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    },
    {
        .range_start = 7840, .range_length = 90, .glyph_id_start = 394,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 8226, .range_length = 55425, .glyph_id_start = 484,
        .unicode_list = unicode_list_4, .glyph_id_ofs_list = NULL, .list_length = 65, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 1, 2, 0, 3, 4, 5,
    2, 6, 7, 8, 9, 10, 9, 10,
    11, 12, 0, 13, 14, 15, 16, 17,
    18, 19, 12, 20, 20, 0, 0, 0,
    21, 22, 23, 24, 25, 22, 26, 27,
    28, 29, 29, 30, 31, 32, 29, 29,
    22, 33, 34, 35, 3, 36, 30, 37,
    37, 38, 39, 40, 41, 42, 43, 0,
    44, 0, 45, 46, 47, 48, 49, 50,
    51, 45, 52, 52, 53, 48, 45, 45,
    46, 46, 54, 55, 56, 57, 51, 58,
    58, 59, 58, 60, 41, 0, 0, 9,
    0, 61, 47, 62, 63, 64, 0, 65,
    0, 22, 8, 66, 0, 9, 22, 0,
    67, 0, 0, 0, 0, 51, 29, 9,
    0, 0, 8, 9, 0, 0, 0, 68,
    23, 23, 23, 23, 23, 23, 26, 25,
    26, 26, 26, 26, 29, 29, 29, 29,
    22, 29, 22, 22, 22, 22, 22, 0,
    22, 30, 30, 30, 30, 39, 69, 46,
    45, 45, 45, 45, 45, 45, 49, 47,
    49, 49, 49, 49, 51, 51, 70, 51,
    71, 45, 46, 46, 46, 46, 46, 9,
    46, 51, 51, 51, 51, 58, 46, 58,
    23, 45, 23, 45, 23, 72, 25, 47,
    25, 47, 25, 47, 25, 47, 22, 73,
    22, 48, 26, 49, 26, 49, 26, 49,
    26, 74, 26, 49, 28, 51, 28, 51,
    28, 51, 28, 51, 29, 45, 29, 45,
    29, 51, 29, 51, 29, 51, 29, 75,
    29, 51, 30, 51, 30, 51, 31, 53,
    53, 32, 48, 32, 48, 32, 73, 32,
    48, 32, 48, 29, 45, 29, 45, 29,
    45, 45, 29, 45, 22, 46, 22, 46,
    22, 46, 26, 49, 35, 55, 35, 55,
    35, 55, 3, 56, 3, 56, 3, 56,
    3, 56, 36, 57, 36, 76, 36, 57,
    30, 51, 30, 51, 30, 51, 30, 51,
    30, 51, 30, 77, 37, 58, 39, 58,
    39, 40, 60, 40, 60, 40, 60, 0,
    25, 47, 22, 48, 22, 48, 26, 49,
    26, 49, 26, 49, 28, 51, 29, 45,
    29, 45, 29, 51, 32, 48, 32, 48,
    29, 45, 29, 45, 29, 45, 29, 45,
    22, 46, 22, 46, 22, 46, 22, 46,
    35, 55, 35, 55, 3, 56, 3, 56,
    3, 56, 3, 56, 3, 56, 36, 57,
    36, 57, 30, 51, 30, 51, 37, 58,
    37, 58, 37, 58, 39, 58, 40, 60,
    57, 24, 23, 45, 23, 45, 23, 45,
    23, 45, 23, 45, 23, 45, 23, 45,
    23, 45, 23, 45, 23, 45, 23, 45,
    23, 45, 26, 49, 26, 49, 26, 49,
    26, 49, 26, 49, 26, 49, 26, 49,
    26, 49, 29, 51, 29, 51, 22, 46,
    22, 46, 22, 46, 22, 46, 22, 46,
    22, 46, 22, 46, 22, 78, 22, 78,
    22, 78, 22, 78, 22, 78, 30, 51,
    30, 51, 30, 79, 30, 79, 30, 79,
    30, 79, 30, 79, 39, 58, 39, 58,
    39, 58, 39, 58, 9, 10, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 1, 2, 0, 3, 4, 5,
    2, 6, 7, 8, 9, 10, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 12,
    18, 19, 20, 21, 21, 0, 0, 0,
    22, 23, 24, 25, 23, 25, 25, 25,
    23, 25, 25, 26, 25, 25, 25, 25,
    23, 25, 23, 25, 3, 27, 28, 29,
    29, 30, 31, 32, 33, 34, 35, 0,
    36, 0, 37, 38, 39, 39, 39, 0,
    39, 38, 40, 41, 38, 38, 42, 42,
    39, 42, 39, 42, 43, 44, 45, 46,
    46, 47, 46, 48, 0, 0, 35, 9,
    0, 49, 39, 50, 51, 52, 0, 53,
    0, 23, 8, 9, 9, 9, 23, 0,
    54, 0, 0, 0, 0, 42, 55, 9,
    0, 0, 8, 56, 0, 0, 0, 57,
    24, 24, 24, 24, 24, 24, 24, 23,
    25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 23, 23, 23, 23, 23, 0,
    23, 28, 28, 28, 28, 31, 25, 38,
    37, 37, 37, 37, 37, 37, 37, 39,
    39, 39, 39, 39, 58, 42, 59, 60,
    39, 42, 39, 39, 39, 39, 39, 9,
    39, 45, 45, 45, 45, 46, 38, 46,
    24, 37, 24, 37, 24, 37, 23, 39,
    23, 39, 23, 39, 23, 39, 25, 39,
    25, 39, 25, 39, 25, 39, 25, 39,
    25, 39, 25, 39, 23, 39, 23, 39,
    23, 39, 23, 39, 25, 38, 25, 61,
    25, 62, 25, 63, 25, 64, 25, 65,
    25, 42, 25, 65, 26, 66, 25, 38,
    42, 25, 38, 25, 38, 25, 38, 25,
    38, 25, 38, 25, 42, 25, 42, 25,
    42, 42, 25, 42, 23, 39, 23, 39,
    23, 39, 23, 39, 25, 42, 25, 42,
    25, 42, 3, 43, 3, 43, 3, 43,
    3, 43, 27, 44, 27, 44, 27, 44,
    28, 45, 28, 45, 28, 45, 28, 45,
    28, 45, 28, 45, 29, 46, 31, 46,
    31, 32, 48, 32, 48, 32, 48, 0,
    23, 39, 25, 39, 25, 39, 25, 39,
    25, 39, 25, 39, 23, 39, 25, 38,
    25, 38, 25, 67, 25, 38, 25, 38,
    25, 42, 25, 42, 25, 42, 25, 42,
    23, 39, 23, 39, 23, 39, 23, 39,
    25, 42, 25, 42, 3, 43, 3, 43,
    3, 43, 3, 43, 3, 43, 27, 44,
    27, 44, 28, 45, 28, 45, 29, 46,
    29, 46, 29, 46, 31, 46, 32, 48,
    44, 68, 24, 37, 24, 37, 24, 37,
    24, 37, 24, 37, 24, 37, 24, 37,
    24, 37, 24, 37, 24, 37, 24, 37,
    24, 37, 25, 39, 25, 39, 25, 39,
    25, 39, 25, 39, 25, 39, 25, 39,
    25, 39, 25, 42, 25, 69, 23, 39,
    23, 39, 23, 39, 23, 39, 23, 39,
    23, 39, 23, 39, 23, 39, 23, 39,
    23, 39, 23, 39, 23, 39, 28, 45,
    28, 45, 28, 45, 28, 45, 28, 45,
    28, 45, 28, 45, 31, 46, 31, 46,
    31, 46, 31, 46, 9, 10, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 3, 0,
    2, -1, 0, 0, 0, 0, -4, -4,
    0, 3, 1, 1, -3, 0, 3, 0,
    3, 1, 2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 4, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -1,
    1, 0, 1, 3, 0, -3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -2, 0, 0, 0, 0,
    0, -1, 1, 1, 0, 0, -1, 0,
    0, 1, 0, -1, 0, -1, 0, -1,
    0, 0, 0, 0, -1, 0, 0, -1,
    -1, 0, 0, -1, 0, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -1,
    -1, 0, 0, 0, 0, 0, 0, -1,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, -8, 0, 0, -1, 0, 1,
    2, 0, 0, -1, 1, 1, 2, 1,
    -1, 1, 0, 0, -4, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, -3, 0, -3,
    0, 0, 0, 0, 0, 0, 2, 0,
    -2, -1, 0, 0, 0, -1, 0, 0,
    0, -5, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, -1, 2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, 0,
    0, -3, -3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 2, 0, 1, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 2, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 2, -1,
    0, 0, 1, -1, -2, -9, 0, 2,
    1, 0, -1, 0, 2, 0, 2, 0,
    2, 0, -6, 0, -1, 2, 0, 2,
    -1, 1, 1, 0, 0, 0, -1, 0,
    0, -1, 5, 0, 5, 0, 2, 0,
    3, 1, 1, 0, 0, 0, 0, 0,
    2, 2, 1, -5, 5, 5, 5, 0,
    2, 5, 5, 5, 0, 5, 0, 5,
    0, 0, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, -1, -1, -1, 0,
    0, -1, 0, 0, 0, -3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -4, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, 1, 0, -6, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -4, 0,
    -4, 0, 0, 0, 0, 0, 0, 6,
    -1, -1, 1, 1, -1, 0, -1, 1,
    0, 0, -3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -6, 0,
    1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -1,
    -2, 0, -4, -8, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 4, 0, 0, -2, 0,
    2, 0, -4, -6, -4, -1, 2, 0,
    0, -4, 0, 1, -1, 0, -1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 2, 2, -8, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -4, -1, 0, 3, -1, 3,
    0, -3, -5, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, -1, 0, 0, 0, -1, 0, 0,
    0, 0, 0, 0, -1, 0, -1, 0,
    -1, -1, 0, -2, -2, -2, -1, 0,
    -1, 0, -1, 0, 0, 0, 0, -1,
    0, 0, 1, 0, 0, -1, 0, 0,
    0, 0, -1, 0, 0, 0, 0, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 1, 1, 0,
    0, 0, 0, -1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0,
    -1, 0, -1, 0, 0, 0, 0, 2,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -1, 0, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -1, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -1, -1, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -1,
    0, 0, 0, 0, 0, 0, -1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -2,
    0, -2, 1, 0, 0, -1, 1, 1,
    2, 0, -2, 0, -1, 0, 0, -3,
    1, 0, 0, -3, 1, 0, 0, 0,
    -3, 0, -3, -1, -6, 0, 0, -3,
    0, 1, 2, 0, 1, 0, 0, 0,
    0, 0, 0, -1, -1, 0, 0, 1,
    1, -1, 1, -2, -1, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -1, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, 0,
    0, 0, 0, 0, -1, -1, 0, 0,
    -1, -1, 0, 0, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, -1, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, 1, 0, 0, -1,
    0, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -1, 0, -1, 0, -1, 0,
    0, 0, 0, 0, 0, 0, 1, 0,
    -1, 0, 0, 0, 0, -1, -1, 0,
    0, 0, 1, 0, 1, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -1, 0, 0, 0, 0, 2, 0,
    0, -2, 0, 0, 2, -3, -3, -3,
    -1, 1, 0, -1, -4, -1, 0, -1,
    0, -1, 1, -1, -4, 0, -2, 0,
    0, 0, 0, 1, 0, 0, 1, 0,
    -2, -2, 0, -3, -2, -1, -2, -2,
    -1, -2, 0, -1, -2, -2, -1, -1,
    1, -1, 0, 2, -1, -4, -2, -2,
    -2, 0, -2, -2, -2, -2, -1, -2,
    0, -2, 0, 0, 0, -1, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -1,
    0, 0, 0, 0, -1, 0, -1, -1,
    -1, 0, 0, -2, 0, 0, 0, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -1, 1, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0,
    0, 3, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, -3, -2, 0,
    0, 0, -1, -3, 0, 0, -1, 1,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -1,
    0, 1, 0, 1, -7, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -1, 0, 0, 0, 0, 1,
    0, 0, -1, -1, 0, -1, -1, -1,
    0, 0, 0, 0, 0, 0, -2, 0,
    -1, 0, -1, -1, 0, -1, -2, -2,
    -1, 0, -1, 0, -2, 0, 0, 0,
    0, 5, 0, 0, 0, 0, 0, -1,
    0, 0, 0, 0, 0, 0, 1, 0,
    0, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 5, 0, 0, 0, 0, -3,
    0, 0, 0, 0, 0, -6, -1, 2,
    2, -1, -3, 0, 1, -1, 0, -3,
    0, -1, 1, -4, -1, 1, 0, 1,
    -2, -1, -2, -2, -3, 0, 0, -4,
    0, 4, 0, 0, 0, 0, 0, 0,
    0, 0, -1, -2, -2, 0, 1, 0,
    0, 0, 0, -6, -5, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -1, 0, 0,
    -1, -1, 0, 0, -1, 0, -1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, 1,
    0, 1, 0, -1, 1, 0, 0, -2,
    -1, 0, -1, -1, 0, 0, -1, -1,
    0, 0, -1, 0, 0, -1, -1, 0,
    0, -1, 0, 1, 0, 0, -1, 0,
    0, 0, -1, 0, -1, 0, -1, -1,
    -1, 0, 0, 0, 0, 1, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -1, 1, 0,
    -1, 0, 0, -1, -2, 0, 0, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, -1, -1, -1, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, -1, -1, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 3, 0, 0, -2, 0,
    0, 1, 0, -1, -3, -1, 1, 0,
    0, -3, -1, 1, -1, 0, 0, 0,
    -1, -2, 0, -1, 0, 0, 0, -1,
    0, 0, 0, 1, 1, -1, -1, 0,
    -1, -1, -1, -1, -1, 0, -1, 0,
    -1, -1, -1, 0, 0, 0, 0, 2,
    1, -1, -3, -1, 1, 0, 0, 0,
    0, 0, -1, 0, 0, 0, -1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -1, -1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, -1, 0, 0, -1, -1,
    0, 0, 0, 0, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, -1, 0, 0, 0, -2, 0,
    0, -1, 1, 0, 0, -3, 0, 0,
    -1, -1, 0, -3, -2, -2, 0, 0,
    -3, -1, -3, -2, -3, 0, -2, 0,
    1, 4, -1, 0, -1, -1, 0, -1,
    -1, -2, -1, -2, -3, -1, 0, 0,
    0, 0, 0, -1, -1, 0, 0, -1,
    -1, -1, 0, -1, -1, -1, -1, 0,
    -1, 0, -1, 0, 0, 0, 0, 0,
    0, 0, -4, -1, 2, 1, -1, -2,
    0, 0, -2, 0, -3, 0, -1, 1,
    -6, -1, 0, 0, 0, -4, -1, -3,
    -1, -5, 0, 0, -4, 0, 4, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, 0, 0, 0,
    -4, -5, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -2, 0, -1, 0,
    0, -2, -3, 0, 0, 0, -1, -2,
    -1, 0, 0, 0, 0, 0, 0, -3,
    -1, -2, -2, -1, -1, -2, -1, -1,
    0, -1, -1, -2, -1, 0, -1, -1,
    -1, -1, 0, 0, 0, 0, -2, 0,
    0, 0, 0, 0, 0, 1, 0, 0,
    -3, -1, -1, -1, 0, -1, -1, -1,
    -1, -1, -1, 0, -1, 0, -1, 0,
    0, 0, 0, 1, 0, 0, -1, 3,
    0, -1, -1, -1, 0, 0, 0, 0,
    0, 0, -2, 0, -1, 0, -1, -1,
    0, -1, -2, -2, -1, 0, -1, 1,
    3, 0, 0, 0, 0, 5, 0, 0,
    0, 0, 0, -1, 0, 0, 0, 0,
    0, 0, 1, 0, 0, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, -1, 0, 0, -1,
    -1, 0, 0, -1, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1,
    1, -1, 0, -2, -1, 0, 2, -2,
    -2, -1, -1, 3, 1, 1, -6, 0,
    1, -1, 0, -1, 1, -1, -2, 0,
    -1, 1, -1, -1, -2, -1, 0, 0,
    2, 1, 0, -2, 0, -4, -1, 2,
    -1, -2, 0, -1, -2, -2, -1, -1,
    0, 0, 0, 0, 3, 2, -1, -4,
    1, 2, 1, 0, 2, 2, 2, -1,
    2, 2, 0, -1, 1, 0, -1, 0,
    -2, 0, 1, 2, -1, -2, -3, -2,
    2, 0, 0, -5, -1, 1, -1, 0,
    -1, 0, -1, -2, -1, -1, -1, 0,
    0, -1, -1, -1, 0, 2, 1, -1,
    -4, 0, -4, -1, 0, -2, -4, 0,
    -2, -1, -2, -2, -2, 0, 0, 0,
    0, 2, 1, -1, -5, 1, 2, 1,
    0, 2, 1, 3, -1, 2, 2, -1,
    -1, 0, 0, -1, 0, -1, -1, 0,
    -1, -1, 0, 1, -2, 1, 0, 0,
    -3, 0, -1, -1, -1, 0, -2, -2,
    -2, -1, 0, -2, -1, -1, -1, -2,
    -1, 0, 0, 0, 3, -1, 0, -2,
    -1, 0, -1, -1, -1, -2, -2, -2,
    -1, -1, 0, 0, 0, 0, -1, 0,
    -1, 0, -1, -1, -1, 0, -1, -1,
    -1, -1, 0, -1, 0, -1, 1, 0,
    -1, 0, -3, -1, 0, 1, -2, -2,
    -1, -2, 2, -1, 0, -6, -1, 1,
    -1, -1, -2, 0, -2, -3, -1, -1,
    -1, -1, -1, -2, 0, 0, 0, 2,
    2, 0, -4, 0, -4, -1, 2, -2,
    -4, -1, -2, -3, -3, -2, -3, 0,
    0, 0, -1, 1, 1, -1, -5, 2,
    2, 2, 0, 2, 2, 2, -1, 2,
    -2, 0, -2, 0, 0, 0, 0, -1,
    0, 0, 1, -1, 1, 0, -1, 1,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, -1, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 1, 2, 0,
    0, -1, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 1, 0,
    -1, 0, 2, 0, 1, 0, 0, -1,
    0, 1, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 2, 0, 2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -4, 0,
    -1, 1, 0, 2, 0, 0, 6, 1,
    -1, -1, 1, 1, 0, 0, -3, 0,
    0, 3, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -4, 2,
    9, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 3, 0, 0,
    0, 2, -4, -2, 1, 2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, -1,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -2, 0, 0, 0, 0, 0,
    1, 8, -1, -1, 2, 2, -2, 1,
    0, 0, 1, 1, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -8, 2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, -1, 2, 0, -5, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -2,
    0, 0, 0, -2, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, -1, 0, -1,
    0, -3, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -1, 0, 0, 0, 0, 0, 0,
    0, -1, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -1, 0, -2, 0, 0, 0,
    -1, 1, -1, 0, 0, -2, -1, -1,
    0, 0, -2, 0, -1, 0, -3, 0,
    -1, 0, 0, -5, -1, -3, -1, -2,
    0, 0, -4, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -1, -1,
    -1, 0, 0, 0, 0, 0, -1, -1,
    0, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -1, 0, -1, 1, -1, 1,
    0, 0, -1, 0, -1, -1, 0, -1,
    0, 0, 0, -2, 0, 0, 0, 0,
    -6, -1, -1, 0, -1, 0, 0, -3,
    -1, 0, 0, 0, -1, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, 0,
    0, 0, -1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -1, 0, 0, 0, 0, 0, -1,
    1, 0, 0, 0, -2, -1, -1, 0,
    0, -2, 0, -1, 0, -3, 0, 0,
    0, 0, -6, 0, -1, -2, -3, 0,
    0, -4, 0, 0, -1, 0, 0, 0,
    0, 0, 0, 0, 0, -1, -1, 0,
    0, 0, 0, 0, 0, -1, -1, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, -1, 0, 2, 3, -1, -1, -2,
    1, 3, 1, 1, -2, 1, 3, 1,
    2, 1, 2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 4, 3,
    -1, -1, 0, -1, 5, 3, 5, 0,
    0, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 2, 3, 0, -1, 4, 3,
    4, 2, 4, 3, 5, 5, 3, 4,
    0, 5, 0, 0, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, -5, -1, -1, -3,
    -3, 0, 0, -4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0,
    0, -5, -1, -1, -3, -3, 0, 0,
    -3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, -1, 1, 0, -1,
    1, 1, 1, -2, 0, 0, -1, 1,
    0, 1, 0, 0, 0, 0, -2, 0,
    -1, 0, -1, 0, -1, -3, 0, 4,
    -1, 0, -1, 0, 0, 0, -1, 0,
    -1, -2, -1, -1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, -5, -1, -1, -3, -3,
    0, 0, -4, 0, 0, 0, 0, 0,
    0, 3, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 3, 0, 0, 0, 0, 0,
    -1, 0, -2, -1, -1, 2, -1, -1,
    -3, 0, 0, 0, 0, -2, 0, 1,
    0, 1, 0, 1, -2, -3, -1, 0,
    -2, -1, -2, -3, -2, 0, -1, -1,
    -1, -1, -1, 0, -1, 0, 0, 0,
    0, 1, 0, 1, 0, 0, 0, 0,
    0, 0, 1, 2, 2, 0, -2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, -1, 0, 0, -2, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -4, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, -1, 0, 0, 0, 0, 0, 0,
    -1, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, -1,
    -1, 1, 0, -1, -1, 0, 0, -2,
    0, -1, 0, -1, 0, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -4, 0, 2, 0, 0, -1, 0,
    0, 0, 0, -1, 0, -1, 0, 0,
    0, 0, 0, 0, -1, 0, -1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -1, 0, 0, 3, -1, -2, -2,
    0, 1, 1, 0, -2, 0, 1, 0,
    2, 0, 2, 0, -2, 0, 0, -3,
    0, 0, -2, -2, 0, 0, -1, 0,
    -1, -1, 0, -1, 0, -1, 0, 0,
    1, 0, -1, -2, -1, 0, 0, 0,
    0, 0, 2, 2, 0, -2, 0, 0,
    0, 0, 0, 0, 0, 0, -1, 0,
    0, 0, 0, 0, -1, 0, -1, 0,
    0, 1, -1, 0, 1, -1, 1, 0,
    0, -2, 0, 0, 0, 0, -1, 1,
    -1, 0, 0, 0, -3, -1, -1, 0,
    -2, 0, 0, -3, 0, 2, -1, 0,
    -1, 0, 0, 0, -1, 0, -1, -2,
    0, -1, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, -1,
    0, 0, 0, -1, 0, 0, -1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -4, 0, 1, 0, 0, -1, 0, 0,
    0, 0, 0, 0, -1, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 3, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, 0, -2, 0, 0,
    2, -2, 0, 1, -2, 1, -1, -1,
    -2, -1, 0, -2, -1, -2, 0, -1,
    -3, 0, -1, 0, 0, 0, -1, 1,
    0, 0, 1, 0, 1, -3, 0, -2,
    -1, -2, -1, -3, -1, -2, -1, -2,
    -3, -2, 0, 0, 0, -1, 0, 1,
    -1, -1, -1, 2, 1, 0, 1, 1,
    1, -1, 2, -1, 0, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -1,
    0, 0, 0, -1, 0, 0, 0, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0,
    0, 1, 0, -1, -2, -2, -1, 2,
    0, 1, -1, 0, 2, -1, 0, -3,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 3, 1, -1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, 0, 0,
    0, 0, 0, -3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 1, 0, 0, 0, 0, 1,
    0, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -1, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -4, 0,
    1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 1, 0, 0,
    0, 0, 0, -4, -4, 0, 3, 2,
    1, -3, 0, 3, 0, 2, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, -4, -1, 0, 0, 0, -3,
    0, 3, -3, -3, 0, 0, -3, 0,
    -3, -3, 0, 0, -9, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -6, 1, 3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, -1, -3, 0, -4, -6, -1, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -1, -2, 0,
    -1, -2, -1, 0, -1, 0, 0, 0,
    0, -2, 0, -2, 0, -3, -2, 0,
    -1, -2, -2, -1, 0, -3, 0, -2,
    -1, 0, 0, 0, -1, 0, 0, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, -1, 0, 0,
    0, 0, 0, -1, 0, 0, 0, 0,
    5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 3, 0,
    0, 0, 0, -5, -1, -1, -3, -3,
    0, 0, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -1,
    0, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -2, 0,
    0, 0, -2, 0, 0, 0, 0, -1,
    0, 0, 0, 0, -1, 0, -1, 0,
    -3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -4, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    -1, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 4, 0, 4,
    4, 1, 0, 3, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 4, 1, 1, 1,
    1, 0, 1, 0, 1, 0, -1, 0,
    0, 0, 0, 0, -1, 1, 0, 0,
    0, -2, -1, -1, 0, 0, -2, 0,
    -1, 0, -3, 0, 0, 0, 0, -6,
    0, -1, -2, -3, 0, 0, -4, 0,
    0, -1, 0, 0, 0, 2, 0, 0,
    0, 0, -1, -1, 0, 0, 0, 0,
    0, 0, -1, -1, 0, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 2, 0,
    0, 0, 0, 0, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, -5, -1, -1, -3,
    -3, 0, 0, -4, 0, 0, 0, 0,
    0, 0, 4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 4, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, -1, -1,
    1, 0, -1, -1, 0, 0, -2, 0,
    -1, 0, -1, 0, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -4, 0, 2, 0, 0, -1, 1, 1,
    1, 0, -1, 0, -1, 0, 0, 0,
    0, 0, 0, -1, 0, -1, 0, 0,
    1, 1, 1, 0, 1, 1, 1, 1,
    1, 1, 0, 1, 0, 0, -1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0, -5, -1,
    -1, -3, -3, 0, 0, -4, 0, 0,
    0, 0, 0, 0, 2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    3, 1, -1, 0, 0, 1, 0, -1,
    0, 0, 1, 0, 0, 0, 3, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, -2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -1, -1,
    -1, 0, 0, 0, 0, 0, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 1, 4, -1, -1,
    -1, -1, 2, 2, 1, -1, 0, 3,
    0, 1, 0, 5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 2,
    1, -1, -1, 0, -1, -1, -1, -1,
    -1, 1, -1, -1, -1, -1, 0, 0,
    0, 0, 1, 3, 2, -1, -1, -1,
    -1, -1, 0, -1, -1, -1, -1, -1,
    -1, 0, -1
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 79,
    .right_class_cnt     = 69,
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 5,
    .bpp = 2,
    .kern_classes = 1,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t s3_font_4 = {
#else
lv_font_t s3_font_4 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 6,          /*The maximum line height required by the font*/
    .base_line = 1,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 0,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if S3_FONT_4*/

