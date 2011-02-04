// fnt_print.h

#ifndef __FNT_PRINT__
#define __FNT_PRINT__

// 標準では強制固定幅モードは利用しない
//#define USE_FIX

/*
 * .fnt loadable font file format definition
 *
 * format               len         description
 * -------------------  ----------- -------------------------- ---------------------------------
 * char x4    version     4           'RB12'                     [0]
 * uint16_t      maxwidth    2           最大幅                     [4]
 * uint16_t      height      2           高さ                       [6]
 * uint16_t      ascent      2           ベースライン               [8]
 * char x2    pad         2           パディング(32bit)          [10]
 * int      firstchar   4           最初のフォント             [12]
 * int      defaultchar 4           デフォルトのフォント       [16]
 * int      size        4           フォント数                 [20]
 * int      nbits       4           イメージデータサイズ       [24]
 * int      noffset     4           オフセットデータサイズ *1  [28]
 * int      nwidth      4           幅データサイズ*2           [32]
 * char*      bits        nbits       イメージデータ             [36]
 * char       pad         0～3        パディング(16/32bit)*3     [36 + nbits]
 * uint16_t/32*  offset      noffsetx2/4 オフセットデータ*1*3       [36 + nbits + pad]
 * char*      width       nwidth      幅データ*2                 [36 + nbits + pad + offset + pad]
 *
 * *1 noffsetが0の場合は固定幅となり、offsetデータは無し
 * *2 nwidthが0の場合は固定幅となり、widthデータは無し
 * *3 nbitsが0xFFDB未満の場合は16bit、0xFFDB以上の場合は32bitとなる
 *
 */

//typedef unsigned int uint32_t;
//typedef unsigned short uint16_t;
//typedef unsigned char uint8_t;
//typedef int int32_t;
//typedef short s16;
//typedef char s8;
typedef uint32_t color4_t;

typedef struct {
  uint16_t maxwidth;       // 最大幅
  uint16_t height;         // 高さ
  uint16_t ascent;         // ベースライン(未使用)
  uint32_t firstchar;      // 最初のフォント
  uint32_t size;           // フォント数
  uint32_t defaultchar;    // デフォルトのフォント
  uint32_t nbits;          // イメージデータサイズ
  uint32_t noffset;        // オフセットデータサイズ
  uint32_t nwidth;         // 幅データサイズ
  const uint8_t *bits;     // イメージデータへのポインタ
  const uint32_t *offset;  // オフセットデータへのポインタ
  const uint8_t *width;    // 幅データへのポインタ
  uint32_t long_offset;    // オフセットデータ幅のフラグ
  uint32_t file_flag;      // ファイルからロードした場合のフラグ
  void *fnt_ptr;      // 確保したメモリへのポインタ
} fnt_t;

#define FNT_FONT_FILL	(0x01) // フォント部分をVRAMに書込む
#define FNT_BACK_FILL	(0x10) // BG部分をVRAMに書込む

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
  フォントをファイルからロード
    *path: フォントファイルのパス
    *font: font_tへのポインタ
    return: ファイルサイズ
            -1 指定ファイル無し / -2 メモリ確保失敗 / -3 読込失敗
---------------------------------------------------------------------------*/
int32_t fnt_load_file(const void* path, fnt_t *font);

/*---------------------------------------------------------------------------
  メモリ上のフォントをロード
    *ptr: フォントデータへのポインタ
    *font: fnt_tへのポインタ
---------------------------------------------------------------------------*/
void fnt_load_mem(const void* ptr, fnt_t *font);

/*---------------------------------------------------------------------------
  フォントの解放
  ファイルからロードした場合は、メモリも解放する
    *font: fnt_tへのポインタ
    return: エラーの場合は負を返す
---------------------------------------------------------------------------*/
int32_t fnt_free(fnt_t *font);

/*---------------------------------------------------------------------------
  文字列の幅を得る
    *font: font_tへのポインタ
    *str: 文字列へのポインタ(UTF-8N)
    return: 文字列の幅
---------------------------------------------------------------------------*/
uint32_t fnt_get_width(const fnt_t* font, const char *str, int mx);

/*---------------------------------------------------------------------------
  文字の幅を得る
    *font: font_tへのポインタ
    ucs2: 文字(UCS2)
    return: 文字の幅
---------------------------------------------------------------------------*/
uint32_t fnt_get_width_ucs2(const fnt_t* font, uint32_t ucs2);

/*---------------------------------------------------------------------------
  グリフデータを得る
    *font: font_tへのポインタ
    ucs2: 文字(UCS2)
    return: グリフデータへのポインタ
---------------------------------------------------------------------------*/
uint8_t* fnt_get_bits(const fnt_t* font, uint32_t ucs2);

#if 0
/*---------------------------------------------------------------------------
  一文字表示
    *font: font_tへのポインタ
    x: 横方向位置
    y: 縦方向位置
    *str: 文字列(UTF8-N)へのポインタ('\n'は改行を行う)
    color: 文字色
    back: 背景色
    fill: 書込フラグ FNT_FONT_FILL(0x01) 文字部, FNT_BACK_FILL(0x10) 背景部
    rate: 混合比(-100～100)
    mx: 横方向倍率
    my: 縦方向倍率
---------------------------------------------------------------------------*/
uint32_t fnt_print_xy(const fnt_t* font, uint16_t x, uint16_t y, void *str, uint32_t color,
    uint32_t back, uint8_t fill, s16 rate, uint16_t mx, uint16_t my, int x_length, int y_length);
#endif

/*---------------------------------------------------------------------------
  文字列表示(表示VRAMへの描画)
    *font: font_tへのポインタ
    x: 横方向位置
    y: 縦方向位置
    *str: 文字列(UTF8-N)へのポインタ('\n'は改行を行う)
    color: 文字色
    back: 背景色
    fill: 書込フラグ FNT_FONT_FILL(0x01) 文字部, FNT_BACK_FILL(0x10) 背景部
    rate: 混合比(-100～100)
    mx: 横方向倍率
    my: 縦方向倍率
---------------------------------------------------------------------------*/
uint32_t fnt_print_vram(const fnt_t* font, uint32_t *vram, uint32_t bufferwidth, uint16_t x, uint16_t y, const void *str, uint32_t color, uint32_t back, uint16_t mx, uint16_t my);

/*---------------------------------------------------------------------------
  UFT-8Nの一文字をUTF-16に変換する
    *utf16: 変換後の文字(UTF-16)へのポインタ
    *utf8: UFT-8N文字へのポインタ
    return: UTF-8Nの次の文字へのポインタ
---------------------------------------------------------------------------*/
char* utf8_utf16(uint16_t *utf16, const char *utf8);

/*---------------------------------------------------------------------------
  UFT-8Nの文字列をUTF-16に変換する
    *utf16: 変換後の文字列(UTF-16)へのポインタ
    *utf8: UFT-8N文字列へのポインタ
---------------------------------------------------------------------------*/
void utf8_to_utf16(uint16_t *utf16, const char *utf8);

/*---------------------------------------------------------------------------
  UTF-16の文字列の長さを得る
    *utf16: 文字列(UTF-16)へのポインタ
    return: 文字数
---------------------------------------------------------------------------*/
uint16_t utf16len(const uint16_t *utf16);

#ifdef __cplusplus
}
#endif

#endif
