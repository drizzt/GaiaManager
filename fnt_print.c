// fnt_print.c

#include <stddef.h>
#include "fnt_print.h"
//#include "graphics.h"

#define MAX_STR (2048)

static int nextx;
static int nexty;

#if 0
static void *fnt_malloc(int size);
static int fnt_mfree(void *ptr);
#endif

static void fnt_read_header(const uint8_t* fnt_ptr, fnt_t *font);


#if 0
/*---------------------------------------------------------------------------
  フォントをファイルからロード
    *path: フォントファイルのパス
    *font: font_tへのポインタ
    return: ファイルサイズ
            -1 指定ファイル無し / -2 メモリ確保失敗 / -3 読込失敗
---------------------------------------------------------------------------*/
int fnt_load_file(const void* path, fnt_t *font)
{
  SceIoStat stat;
  SceUID fp;
  int size;

  if(sceIoGetstat(path, &stat) < 0)
    return -1;

  font->fnt_ptr = fnt_malloc(stat.st_size);
  if(font->fnt_ptr <0)
    return -2;

  font->file_flag = 1;

  fp = sceIoOpen(path, PSP_O_RDONLY, 0777);
  if(fp < 0)
    return -3;

  size = sceIoRead(fp, font->fnt_ptr, stat.st_size);
  sceIoClose(fp);

  if(size != stat.st_size)
    return -3;

  fnt_read_header(font->fnt_ptr, font);
  return stat.st_size;

}
#endif

/*---------------------------------------------------------------------------
  メモリ上のフォントをロード
    *ptr: フォントデータへのポインタ
    *font: fnt_tへのポインタ
---------------------------------------------------------------------------*/
void fnt_load_mem(const void* ptr, fnt_t *font)
{
  font->file_flag = 0;

  font->fnt_ptr = (void *)ptr;

  fnt_read_header(font->fnt_ptr, font);
}

#if 0
/*---------------------------------------------------------------------------
  フォントの解放
  ファイルからロードした場合は、メモリも解放する
    *font: fnt_tへのポインタ
    return: エラーの場合は負を返す
---------------------------------------------------------------------------*/
int fnt_free(fnt_t *font)
{
  if(font->file_flag == 1)
    return fnt_mfree(font->fnt_ptr);

  return 0;
}
#endif

#if 0
/*---------------------------------------------------------------------------
  メモリ確保
    size: 利用メモリサイズ
    return: 確保したメモリへのポインタ
            エラーの場合はNULLを返す
---------------------------------------------------------------------------*/
static void *fnt_malloc(int size)
{
  int *p;
  int h_block;

  if(size == 0) return NULL;

  h_block = sceKernelAllocPartitionMemory(2, "block", 0, size + sizeof(h_block), NULL);

  if(h_block < 0) return NULL;

  p = (int *)sceKernelGetBlockHeadAddr(h_block);
  *p = h_block;

  return (void *)(p + 1);
}
#endif

#if 0
/*---------------------------------------------------------------------------
  メモリ解放
    *ptr: 確保したメモリへのポインタ
    return: エラーの場合は負の値を返す
---------------------------------------------------------------------------*/
static int fnt_mfree(void *ptr)
{
  return sceKernelFreePartitionMemory((SceUID)*((int *)ptr - 1));
}
#endif

/*---------------------------------------------------------------------------
  フォント ヘッダ読込み
    *fnt_ptr: フォントデーターへのポインタ
    *font: fnt_tへのポインタ
---------------------------------------------------------------------------*/
static void fnt_read_header(const uint8_t* fnt_ptr, fnt_t *font)
{
  uint32_t pad;
  uint32_t shift;

  font->maxwidth = (fnt_ptr[5] << 8) + fnt_ptr[4];
  font->height   = (fnt_ptr[7] << 8) + fnt_ptr[6];
  font->ascent   = (fnt_ptr[9] << 8) + fnt_ptr[8];

  font->firstchar   = (fnt_ptr[15] << 24) + (fnt_ptr[14] << 16) + (fnt_ptr[13] << 8) + fnt_ptr[12];
  font->defaultchar = (fnt_ptr[19] << 24) + (fnt_ptr[18] << 16) + (fnt_ptr[17] << 8) + fnt_ptr[16];
  font->size        = (fnt_ptr[23] << 24) + (fnt_ptr[22] << 16) + (fnt_ptr[21] << 8) + fnt_ptr[20];
  font->nbits       = (fnt_ptr[27] << 24) + (fnt_ptr[26] << 16) + (fnt_ptr[25] << 8) + fnt_ptr[24];
  font->noffset     = (fnt_ptr[31] << 24) + (fnt_ptr[30] << 16) + (fnt_ptr[29] << 8) + fnt_ptr[28];
  font->nwidth      = (fnt_ptr[35] << 24) + (fnt_ptr[34] << 16) + (fnt_ptr[33] << 8) + fnt_ptr[32];

  font->bits = (const uint8_t *)(&fnt_ptr[36]);

  if(font->nbits < 0xFFDB)
  {
    pad = 1;
    font->long_offset = 0;
    shift = 1;
  }
  else
  {
    pad = 3;
    font->long_offset = 1;
    shift = 2;
  }

  if(font->noffset != 0)
    font->offset = (uint32_t *)(((uint32_t)font->bits + font->nbits + pad) & ~pad);
  else
    font->offset = NULL;

  if(font->nwidth != 0)
    font->width = (uint8_t *)((uint32_t)font->offset + (font->noffset << shift));
  else
    font->width = NULL;
}

/*---------------------------------------------------------------------------
  文字列の幅を得る
    *font: fnt_tへのポインタ
    *str: 文字列へのポインタ(UTF-8N)
    return: 文字列の幅
---------------------------------------------------------------------------*/
uint32_t fnt_get_width(const fnt_t* font, const char *str, int mx)
{
  uint16_t i;
  uint16_t len;
  uint16_t width = 0;
  uint16_t ucs[MAX_STR];

  utf8_to_utf16(ucs, str);
  len = utf16len(ucs);

  for (i = 0; i < len; i++)
    width += fnt_get_width_ucs2(font ,ucs[i]);

  return width * mx;
}

/*---------------------------------------------------------------------------
  文字の幅を得る
    *font: fnt_tへのポインタ
    ucs2: 文字(UCS2)
    return: 文字の幅
---------------------------------------------------------------------------*/
uint32_t fnt_get_width_ucs2(const fnt_t* font, uint32_t ucs2)
{
  uint16_t width = 0;

  if((ucs2 < font->firstchar) || (ucs2 >= font->firstchar + font->size))
    ucs2 = font->defaultchar;

  if(font->nwidth != 0)
    width = font->width[ucs2 - font->firstchar];
  else
    width = font->maxwidth;

  return width;
}

/*---------------------------------------------------------------------------
  グリフデータを得る
    *font: fnt_tへのポインタ
    ucs2: 文字(UCS2)
    return: グリフデータへのポインタ
---------------------------------------------------------------------------*/
uint8_t* fnt_get_bits(const fnt_t* font, uint32_t ucs2)
{
    uint8_t* bits;
    uint8_t* tmp;

    if((ucs2 < font->firstchar) || (ucs2 >= font->firstchar + font->size))
      ucs2 = font->defaultchar;

    ucs2 -= font->firstchar;

    if(font->long_offset == 0)
    {
      tmp = (uint8_t *)font->offset + ucs2 * 2;
      bits = (uint8_t *)font->bits + (font->offset?
          (tmp[1] << 8) + tmp[0]:
          (uint8_t)(((font->height + 7) / 8) * font->maxwidth * ucs2));
    }
    else
    {
      tmp = (uint8_t *)font->offset + ucs2 * 4;
      bits = (uint8_t *)font->bits + (font->offset?
          (tmp[3] << 24) + (tmp[2] << 16) + (tmp[1] << 8) + tmp[0]:
          (uint8_t)(((font->height + 7) / 8) * font->maxwidth * ucs2));
    }

    return bits;
}

#if 0
/*---------------------------------------------------------------------------
  文字列表示(表示VRAMへの描画)
    *font: fnt_tへのポインタ
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
int fnt_print_xy(const fnt_t* font, int x, int y, void *str, int color, int back, char fill, int rate, int mx, int my)
{
  void *vram;
  int  bufferwidth;
  int  pixelformat;
  int  pwidth;
  int  pheight;
  int  unk;

  sceDisplayGetMode(&unk, &pwidth, &pheight);
  sceDisplayGetFrameBuf(&vram, &bufferwidth, &pixelformat, unk);

  if(vram == NULL)
    vram = (void*) (0x40000000 | (int) sceGeEdramGetAddr());

  return fnt_print_vram(font, vram, bufferwidth, pixelformat, x, y, str, color, back, fill, rate, mx, my);
}
#endif

/*---------------------------------------------------------------------------
  文字列表示(指定したRAM/VRAMへの描画)
    *font: fnt_tへのポインタ
    *vram: VRAMアドレス
    bufferwidth: VRAM横幅
    pixelformat: ピクセルフォーマット (0=16bit, 1=15bit, 2=12bit, 3=32bit)
    x: 横方向位置
    y: 縦方向位置
    *str: 文字列(UTF8-N)へのポインタ('\n'は改行を行う)
    color: 文字色
    back: 背景色
    fill: 書込フラグ FNT_FONT_FILL(0x01) 文字部, FNT_BACK_FILL(0x10) 背景部
    rate: 混合比(-100～100) ※負の場合は減色
    mx: 横方向倍率
    my: 縦方向倍率
    return: エラー時は-1
---------------------------------------------------------------------------*/
uint32_t fnt_print_vram(const fnt_t* font, uint32_t *vram, uint32_t bufferwidth, uint16_t x, uint16_t y, const void *str, uint32_t color, uint32_t back, uint16_t mx, uint16_t my)
{
  uint16_t i;
  uint16_t len;
  uint16_t ucs[MAX_STR];
  uint32_t dx, dy;
  uint32_t lx, ly;
  uint8_t* index;
  uint8_t* index_tmp;
  uint16_t shift;
  uint8_t pt;
  uint32_t *vptr_tmp;
  uint32_t *vptr;
  uint32_t temp = back;
  uint32_t width;

  if (bufferwidth == 0) return -1;

  nextx = x;
  nexty = y;

  // utf-8nをUCS2に変換
  utf8_to_utf16(ucs, str);
  len = utf16len(ucs);

  for (i = 0; i < len; i++)
  {
//    if (ucs[i] == '\n')
//    {
//      nextx = x;
//      nexty += font->height;
//    }
//    else
    {
      width = fnt_get_width_ucs2(font, ucs[i]);

      index = fnt_get_bits(font, ucs[i]);
      vptr_tmp = &vram[nextx + nexty * bufferwidth];

      for (dx = 0; dx < width; dx++) /* x loop */
      {
        for(lx = 0; lx < mx; lx++) /* mx loop */
        {
          index_tmp = index;
          shift = 0;
          vptr = vptr_tmp;
          pt = *index;

          for(dy = 0; dy < font->height; dy++) /* y loop */
          {
            if(shift >= 8)
            {
              shift = 0;
              index_tmp += width;
              pt = *index_tmp;
            }

            if(pt & 0x01)
                temp = color;
            else
                temp = back;

            for(ly = 0; ly < my; ly++) /* my loop */
            {
              if(temp == color)
                *vptr = temp;
              vptr += bufferwidth;
            } /* my loop */
            shift++;
            pt >>= 1;
          } /* y loop */
          vptr_tmp++;
        } /* mx loop */
        index++;
      } /* x loop */
      nextx = nextx + width * mx;
    }

  }

return 0;
}

/*---------------------------------------------------------------------------
  UFT-8Nの一文字をUTF-16に変換する
    *utf16: 変換後の文字(UTF-16)へのポインタ
    *utf8: UFT-8N文字へのポインタ
    return: UTF-8Nの次の文字へのポインタ
---------------------------------------------------------------------------*/
char* utf8_utf16(uint16_t *utf16, const char *utf8)
{
  uint8_t c = *utf8++;
  uint32_t code;
  int32_t tail = 0;

  if((c <= 0x7f) || (c >= 0xc2))
  {
    /* Start of new character. */
    if(c < 0x80)
    {
      /* U-00000000 - U-0000007F, 1 byte */
      code = c;
    }
    else if(c < 0xe0)   /* U-00000080 - U-000007FF, 2 bytes */
    {
      tail = 1;
      code = c & 0x1f;
    }
    else if(c < 0xf0)   /* U-00000800 - U-0000FFFF, 3 bytes */
    {
      tail = 2;
      code = c & 0x0f;
    }
    else if(c < 0xf5)   /* U-00010000 - U-001FFFFF, 4 bytes */
    {
      tail = 3;
      code = c & 0x07;
    }
    else                /* Invalid size. */
    {
      code = 0xfffd;
    }

    while(tail-- && ((c = *utf8++) != 0))
    {
      if((c & 0xc0) == 0x80)
      {
        /* Valid continuation character. */
        code = (code << 6) | (c & 0x3f);

      }
      else
      {
        /* Invalid continuation char */
        code = 0xfffd;
        utf8--;
        break;
      }
    }
  }
  else
  {
    /* Invalid UTF-8 char */
    code = 0xfffd;
  }
  /* currently we don't support chars above U-FFFF */
  *utf16 = (code < 0x10000) ? code : 0xfffd;
  return (char*)utf8;
}

/*---------------------------------------------------------------------------
  UFT-8Nの文字列をUTF-16に変換する
    *utf16: 変換後の文字列(UTF-16)へのポインタ
    *utf8: UFT-8N文字列へのポインタ
---------------------------------------------------------------------------*/
void utf8_to_utf16(uint16_t *utf16, const char *utf8)
{
  while(*utf8 !='\0')
  {
    utf8 = utf8_utf16(utf16++, utf8);
  }
  *utf16 = '\0';
}

/*---------------------------------------------------------------------------
  UTF-16の文字列の長さを得る
    *utf16: 文字列(UTF-16)へのポインタ
    return: 文字数
---------------------------------------------------------------------------*/
uint16_t utf16len(const uint16_t *utf16)
{
  uint16_t len = 0;
  while(utf16[len] != '\0')
    len++;
  return len;
}

