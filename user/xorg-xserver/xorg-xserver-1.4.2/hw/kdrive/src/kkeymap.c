/*
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "kdrive.h"
#include <X11/keysym.h>

/*
 * Map scan codes (both regular and synthesized from extended keys)
 * to X keysyms
 */

const KeySym kdDefaultKeymap[KD_MAX_LENGTH * KD_MAX_WIDTH] = {
/*      0       */	 NoSymbol,	NoSymbol,
/* These are directly mapped from DOS scanset 0 */
/*      1     8 */	 XK_Escape, NoSymbol,
/*      2     9 */	 XK_1,	XK_exclam,
/*      3    10 */	 XK_2,	XK_at,
/*      4    11 */	 XK_3,	XK_numbersign,
/*      5    12 */	 XK_4,	XK_dollar,
/*      6    13 */	 XK_5,	XK_percent,
/*      7    14 */	 XK_6,	XK_asciicircum,
/*      8    15 */	 XK_7,	XK_ampersand,
/*      9    16 */	 XK_8,	XK_asterisk,
/*     10    17 */	 XK_9,	XK_parenleft,
/*     11    18 */	 XK_0,	XK_parenright,
/*     12    19 */	 XK_minus,	XK_underscore,
/*     13    20 */	 XK_equal,	XK_plus,
/*     14    21 */	 XK_BackSpace,	NoSymbol,
/*     15    22 */	 XK_Tab,	NoSymbol,
/*     16    23 */	 XK_Q,	NoSymbol,
/*     17    24 */	 XK_W,	NoSymbol,
/*     18    25 */	 XK_E,	NoSymbol,
/*     19    26 */	 XK_R,	NoSymbol,
/*     20    27 */	 XK_T,	NoSymbol,
/*     21    28 */	 XK_Y,	NoSymbol,
/*     22    29 */	 XK_U,	NoSymbol,
/*     23    30 */	 XK_I,	NoSymbol,
/*     24    31 */	 XK_O,	NoSymbol,
/*     25    32 */	 XK_P,	NoSymbol,
/*     26    33 */	 XK_bracketleft,	XK_braceleft,
/*     27    34 */	 XK_bracketright,	XK_braceright,
/*     28    35 */	 XK_Return,	NoSymbol,
/*     29    36 */	 XK_Control_L,	NoSymbol,
/*     30    37 */	 XK_A,	NoSymbol,
/*     31    38 */	 XK_S,	NoSymbol,
/*     32    39 */	 XK_D,	NoSymbol,
/*     33    40 */	 XK_F,	NoSymbol,
/*     34    41 */	 XK_G,	NoSymbol,
/*     35    42 */	 XK_H,	NoSymbol,
/*     36    43 */	 XK_J,	NoSymbol,
/*     37    44 */	 XK_K,	NoSymbol,
/*     38    45 */	 XK_L,	NoSymbol,
/*     39    46 */	 XK_semicolon,	XK_colon,
/*     40    47 */	 XK_apostrophe,	XK_quotedbl,
/*     41    48 */	 XK_grave,	XK_asciitilde,
/*     42    49 */	 XK_Shift_L,	NoSymbol,
/*     43    50 */	 XK_backslash,	XK_bar,
/*     44    51 */	 XK_Z,	NoSymbol,
/*     45    52 */	 XK_X,	NoSymbol,
/*     46    53 */	 XK_C,	NoSymbol,
/*     47    54 */	 XK_V,	NoSymbol,
/*     48    55 */	 XK_B,	NoSymbol,
/*     49    56 */	 XK_N,	NoSymbol,
/*     50    57 */	 XK_M,	NoSymbol,
/*     51    58 */	 XK_comma,	XK_less,
/*     52    59 */	 XK_period,	XK_greater,
/*     53    60 */	 XK_slash,	XK_question,
/*     54    61 */	 XK_Shift_R,	NoSymbol,
/*     55    62 */	 XK_KP_Multiply,	NoSymbol,
/*     56    63 */	 XK_Alt_L,	XK_Meta_L,
/*     57    64 */	 XK_space,	NoSymbol,
/*     58    65 */	 XK_Caps_Lock,	NoSymbol,
/*     59    66 */	 XK_F1,	NoSymbol,
/*     60    67 */	 XK_F2,	NoSymbol,
/*     61    68 */	 XK_F3,	NoSymbol,
/*     62    69 */	 XK_F4,	NoSymbol,
/*     63    70 */	 XK_F5,	NoSymbol,
/*     64    71 */	 XK_F6,	NoSymbol,
/*     65    72 */	 XK_F7,	NoSymbol,
/*     66    73 */	 XK_F8,	NoSymbol,
/*     67    74 */	 XK_F9,	NoSymbol,
/*     68    75 */	 XK_F10,	NoSymbol,
/*     69    76 */	 XK_Break,	XK_Pause,
/*     70    77 */	 XK_Scroll_Lock,	NoSymbol,
/*     71    78 */	 XK_KP_Home,	XK_KP_7,
/*     72    79 */	 XK_KP_Up,	XK_KP_8,
/*     73    80 */	 XK_KP_Page_Up,	XK_KP_9,
/*     74    81 */	 XK_KP_Subtract,	NoSymbol,
/*     75    82 */	 XK_KP_Left,	XK_KP_4,
/*     76    83 */	 XK_KP_5,	NoSymbol,
/*     77    84 */	 XK_KP_Right,	XK_KP_6,
/*     78    85 */	 XK_KP_Add,	NoSymbol,
/*     79    86 */	 XK_KP_End,	XK_KP_1,
/*     80    87 */	 XK_KP_Down,	XK_KP_2,
/*     81    88 */	 XK_KP_Page_Down,	XK_KP_3,
/*     82    89 */	 XK_KP_Insert,	XK_KP_0,
/*     83    90 */	 XK_KP_Delete,	XK_KP_Decimal,
/*     84    91 */     NoSymbol,	NoSymbol,
/*     85    92 */     NoSymbol,	NoSymbol,
/*     86    93 */     NoSymbol,	NoSymbol,
/*     87    94 */	 XK_F11,	NoSymbol,
/*     88    95 */	 XK_F12,	NoSymbol,
    
/* These are remapped from the extended set (using ExtendMap) */
    
/*     89    96 */	 XK_Control_R,	NoSymbol,
/*     90    97 */	 XK_KP_Enter,	NoSymbol,
/*     91    98 */	 XK_KP_Divide,	NoSymbol,
/*     92    99 */	 XK_Sys_Req,	XK_Print,
/*     93   100 */	 XK_Alt_R,	XK_Meta_R,
/*     94   101 */	 XK_Num_Lock,	NoSymbol,
/*     95   102 */	 XK_Home,	NoSymbol,
/*     96   103 */	 XK_Up,		NoSymbol,
/*     97   104 */	 XK_Page_Up,	NoSymbol,
/*     98   105 */	 XK_Left,	NoSymbol,
/*     99   106 */	 XK_Right,	NoSymbol,
/*    100   107 */	 XK_End,	NoSymbol,
/*    101   108 */	 XK_Down,	NoSymbol,
/*    102   109 */	 XK_Page_Down,	NoSymbol,
/*    103   110 */	 XK_Insert,	NoSymbol,
/*    104   111 */	 XK_Delete,	NoSymbol,
/*    105   112 */	 XK_Super_L,	NoSymbol,
/*    106   113 */	 XK_Super_R,	NoSymbol,
/*    107   114 */	 XK_End,	NoSymbol,
/*    108   115 */	 XK_Down,	NoSymbol,
/*    109   116 */	 NoSymbol,	NoSymbol,
/*    110   117 */	 NoSymbol,	NoSymbol,
/*    111   118 */	 NoSymbol,	NoSymbol,
/*    112   119 */	 NoSymbol,	NoSymbol,
/*    113   120 */	 NoSymbol,	NoSymbol,
/*    114   121 */	 NoSymbol,	NoSymbol,
/*    115   122 */	 NoSymbol,	NoSymbol,
/*    116   123 */	 NoSymbol,	NoSymbol,
/*    117   124 */	 NoSymbol,	NoSymbol,
/*    118   125 */	 NoSymbol,	NoSymbol,
/*    119   126 */	 NoSymbol,	NoSymbol,
/*    120   127 */	 NoSymbol,	NoSymbol,
/*    121   128 */	 NoSymbol,	NoSymbol,
/*    122   129 */	 NoSymbol,	NoSymbol,
/*    123   130 */	 NoSymbol,	NoSymbol,
/*    124   131 */	 NoSymbol,	NoSymbol,
/*    125   132 */	 NoSymbol,	NoSymbol,
/*    126   133 */	 NoSymbol,	NoSymbol,
/*    127   134 */	 NoSymbol,	NoSymbol,
/*    128   135 */	 NoSymbol,	NoSymbol,
/*    129   136 */	 NoSymbol,	NoSymbol,
/*    130   137 */	 NoSymbol,	NoSymbol,
/*    131   138 */	 NoSymbol,	NoSymbol,
/*    132   139 */	 NoSymbol,	NoSymbol,
/*    133   140 */	 NoSymbol,	NoSymbol,
/*    134   141 */	 NoSymbol,	NoSymbol,
/*    135   142 */	 NoSymbol,	NoSymbol,
/*    136   143 */	 NoSymbol,	NoSymbol,
/*    137   144 */	 NoSymbol,	NoSymbol,
/*    138   145 */	 NoSymbol,	NoSymbol,
/*    139   146 */	 XK_Menu,	NoSymbol,
/*    140   147 */	 NoSymbol,	NoSymbol,
/*    141   148 */	 NoSymbol,	NoSymbol,
/*    142   149 */	 NoSymbol,	NoSymbol,
/*    143   150 */	 NoSymbol,	NoSymbol,
/*    144   151 */	 NoSymbol,	NoSymbol,
/*    145   152 */	 NoSymbol,	NoSymbol,
/*    146   153 */	 NoSymbol,	NoSymbol,
/*    147   154 */	 NoSymbol,	NoSymbol,
/*    148   155 */	 NoSymbol,	NoSymbol,
/*    149   156 */	 NoSymbol,	NoSymbol,
/*    150   157 */	 NoSymbol,	NoSymbol,
/*    151   158 */	 NoSymbol,	NoSymbol,
/*    152   159 */	 NoSymbol,	NoSymbol,
/*    153   160 */	 NoSymbol,	NoSymbol,
/*    154   161 */	 NoSymbol,	NoSymbol,
/*    155   162 */	 NoSymbol,	NoSymbol,
/*    156   163 */	 NoSymbol,	NoSymbol,
/*    157   164 */	 NoSymbol,	NoSymbol,
/*    158   165 */	 XK_Begin,	NoSymbol,
/*    159   166 */	 NoSymbol,	NoSymbol,
/*    160   167 */	 NoSymbol,	NoSymbol,
/*    161   168 */	 NoSymbol,	NoSymbol,
/*    162   169 */	 NoSymbol,	NoSymbol,
/*    163   170 */	 NoSymbol,	NoSymbol,
/*    164   171 */	 NoSymbol,	NoSymbol,
/*    165   172 */	 NoSymbol,	NoSymbol,
/*    166   173 */	 NoSymbol,	NoSymbol,
/*    167   174 */	 NoSymbol,	NoSymbol,
/*    168   175 */	 NoSymbol,	NoSymbol,
/*    169   176 */	 NoSymbol,	NoSymbol,
/*    170   177 */	 NoSymbol,	NoSymbol,
/*    171   178 */	 NoSymbol,	NoSymbol,
/*    172   179 */	 NoSymbol,	NoSymbol,
/*    173   180 */	 NoSymbol,	NoSymbol,
/*    174   181 */	 NoSymbol,	NoSymbol,
/*    175   182 */	 NoSymbol,	NoSymbol,
/*    176   183 */	 NoSymbol,	NoSymbol,
/*    177   184 */	 NoSymbol,	NoSymbol,
/*    178   185 */	 NoSymbol,	NoSymbol,
/*    179   186 */	 NoSymbol,	NoSymbol,
/*    180   187 */	 NoSymbol,	NoSymbol,
/*    181   188 */	 NoSymbol,	NoSymbol,
/*    182   189 */	 NoSymbol,	NoSymbol,
/*    183   190 */	 NoSymbol,	NoSymbol,
/*    184   191 */	 NoSymbol,	NoSymbol,
/*    185   192 */	 NoSymbol,	NoSymbol,
/*    186   193 */	 NoSymbol,	NoSymbol,
/*    187   194 */	 NoSymbol,	NoSymbol,
/*    188   195 */	 NoSymbol,	NoSymbol,
/*    189   196 */	 NoSymbol,	NoSymbol,
/*    190   197 */	 NoSymbol,	NoSymbol,
/*    191   198 */	 NoSymbol,	NoSymbol,
/*    192   199 */	 NoSymbol,	NoSymbol,
/*    193   200 */	 NoSymbol,	NoSymbol,
/*    194   201 */	 NoSymbol,	NoSymbol,
/*    195   202 */	 NoSymbol,	NoSymbol,
/*    196   203 */	 NoSymbol,	NoSymbol,
/*    197   204 */	 NoSymbol,	NoSymbol,
/*    198   205 */	 NoSymbol,	NoSymbol,
/*    199   206 */	 NoSymbol,	NoSymbol,
/*    200   207 */	 NoSymbol,	NoSymbol,
/*    201   208 */	 NoSymbol,	NoSymbol,
/*    202   209 */	 NoSymbol,	NoSymbol,
/*    203   210 */	 NoSymbol,	NoSymbol,
/*    204   211 */	 NoSymbol,	NoSymbol,
/*    205   212 */	 NoSymbol,	NoSymbol,
/*    206   213 */	 NoSymbol,	NoSymbol,
/*    207   214 */	 NoSymbol,	NoSymbol,
/*    208   215 */	 NoSymbol,	NoSymbol,
/*    209   216 */	 NoSymbol,	NoSymbol,
/*    210   217 */	 NoSymbol,	NoSymbol,
/*    211   218 */	 NoSymbol,	NoSymbol,
/*    212   219 */	 NoSymbol,	NoSymbol,
/*    213   220 */	 NoSymbol,	NoSymbol,
/*    214   221 */	 NoSymbol,	NoSymbol,
/*    215   222 */	 NoSymbol,	NoSymbol,
/*    216   223 */	 NoSymbol,	NoSymbol,
/*    217   224 */	 NoSymbol,	NoSymbol,
/*    218   225 */	 NoSymbol,	NoSymbol,
/*    219   226 */	 NoSymbol,	NoSymbol,
/*    220   227 */	 NoSymbol,	NoSymbol,
/*    221   228 */	 NoSymbol,	NoSymbol,
/*    222   229 */	 NoSymbol,	NoSymbol,
/*    223   230 */	 NoSymbol,	NoSymbol,
/*    224   231 */	 NoSymbol,	NoSymbol,
/*    225   232 */	 NoSymbol,	NoSymbol,
/*    226   233 */	 NoSymbol,	NoSymbol,
/*    227   234 */	 NoSymbol,	NoSymbol,
/*    228   235 */	 NoSymbol,	NoSymbol,
/*    229   236 */	 NoSymbol,	NoSymbol,
/*    230   237 */	 NoSymbol,	NoSymbol,
/*    231   238 */	 XK_Execute,	NoSymbol,
};

/*
 * List of locking key codes
 */

CARD8 kdLockMap[] = {
    65,
    101,
    77,
};

#define NUM_LOCK (sizeof (kdLockMap) / sizeof (kdLockMap[0]))

int kdNumLock = NUM_LOCK;

/*
 * Map containing list of keys which the X server makes locking when
 * the KEYMAP_LOCKING_ALTGR flag is set in CEKeymapFlags
 */

CARD8 kdOptionalLockMap[] = {
    100,
};

#define NUM_OPTIONAL_LOCK  (sizeof (kdOptionalLockMap) / sizeof (kdOptionalLockMap[0]))

int kdNumOptionalLock = NUM_OPTIONAL_LOCK;

const CARD8 kdDefaultModMap[MAP_LENGTH];

unsigned long kdDefaultKeymapFlags = 0;

const KeySymsRec kdDefaultKeySyms = {
    kdDefaultKeymap,
    KD_MIN_KEYCODE,
    KD_MAX_KEYCODE,
    KD_MAX_WIDTH
};
