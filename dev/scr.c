#include "scr.h"
#include <string.h>

static void _dev_scr_clr          (struct pm_dev_scr_t * scr, u_half_t chr) ;
static void _dev_scr_ver_scroll_u (struct pm_dev_scr_t * scr, u_byte_t n) ;
static void _dev_scr_ver_scroll_d (struct pm_dev_scr_t * scr, u_byte_t n) ;
static void _dev_scr_hor_scroll_l (struct pm_dev_scr_t * scr, u_byte_t n) ;
static void _dev_scr_hor_scroll_r (struct pm_dev_scr_t * scr, u_byte_t n) ;

void pm_dev_scr_rst (struct pm_dev_scr_t * scr)
{
  scr->edit  = 0 ;
  scr->len_x = 80 ;
  scr->len_y = 25 ;
  scr->cur_x = 0 ;
  scr->cur_y = 0 ;
  scr->fnt   = 0x50000000 ;

  scr->pal[0x0] = 0x000000FF ;
  scr->pal[0x1] = 0xCC0000FF ;
  scr->pal[0x2] = 0x4E9A06FF ;
  scr->pal[0x3] = 0xC4A000FF ;
  scr->pal[0x4] = 0x729FCFFF ;
  scr->pal[0x5] = 0x75507BFF ;
  scr->pal[0x6] = 0x06989AFF ;
  scr->pal[0x7] = 0xD3D7CFFF ;
  scr->pal[0x8] = 0x555753FF ;
  scr->pal[0x9] = 0xEF2929FF ;
  scr->pal[0xA] = 0x8AE234FF ;
  scr->pal[0xB] = 0xFCE94FFF ;
  scr->pal[0xC] = 0x32AFFFFF ;
  scr->pal[0xD] = 0xAD7FA8FF ;
  scr->pal[0xE] = 0x34E2E2FF ;
  scr->pal[0xF] = 0xFFFFFFFF ;

  _dev_scr_clr(scr, 0xF020) ;

  pm_iom_int(scr->dev.bus, (struct pm_dev_t *)scr) ;
}

void pm_dev_scr_clk (struct pm_dev_scr_t * scr)
{
}

void pm_dev_scr_stb (struct pm_dev_scr_t * scr, u_word_t adr, u_byte_t dat)
{
  switch (adr) {
  case 0x00 : {
    scr->len_x = dat ;
    scr->edit  = 1 ;
  } break ;
  
  case 0x01 : {
    scr->len_y = dat ;
    scr->edit  = 1 ;
  } break ;

  case 0x02 : {
    scr->cur_x = dat ;
    scr->edit  = 1 ;
  } break ;

  case 0x03 : {
    scr->cur_y = dat ;
    scr->edit  = 1 ;
  } break ;

  case 0x04 : case 0x05 : case 0x06 : case 0x07 : {
    /* ignored */
  } break ;

  case 0x08 : case 0x18 : case 0x28 : case 0x38 :
  case 0x48 : case 0x58 : case 0x68 : case 0x78 :
  case 0x88 : case 0x98 : case 0xA8 : case 0xB8 :
  case 0xC8 : case 0xD8 : case 0xE8 : case 0xF8 :
  case 0x09 : case 0x19 : case 0x29 : case 0x39 :
  case 0x49 : case 0x59 : case 0x69 : case 0x79 :
  case 0x89 : case 0x99 : case 0xA9 : case 0xB9 :
  case 0xC9 : case 0xD9 : case 0xE9 : case 0xF9 :
  case 0x0A : case 0x1A : case 0x2A : case 0x3A :
  case 0x4A : case 0x5A : case 0x6A : case 0x7A :
  case 0x8A : case 0x9A : case 0xAA : case 0xBA :
  case 0xCA : case 0xDA : case 0xEA : case 0xFA :
  case 0x0B : case 0x1B : case 0x2B : case 0x3B :
  case 0x4B : case 0x5B : case 0x6B : case 0x7B :
  case 0x8B : case 0x9B : case 0xAB : case 0xBB :
  case 0xCB : case 0xDB : case 0xEB : case 0xFB : {
    int col_idx = (adr >> 4) & 0xF ;
    int com_ofs = (adr & 0x3) * 8 ;

    u_word_t * col = scr->pal + col_idx ;
    *col &= ~(0xFF << com_ofs) ;
    *col |=  (dat  << com_ofs) ;

    scr->edit = 1 ;
  } break ;
  
  case 0xF0 : {
    _dev_scr_ver_scroll_u(scr, dat) ;
  } break ;

  case 0xF1 : {
    _dev_scr_ver_scroll_d(scr, dat) ;
  } break ;

  case 0xF2 : {
    _dev_scr_hor_scroll_l(scr, dat) ;
  } break ;

  case 0xF3 : {
    _dev_scr_hor_scroll_r(scr, dat) ;
  } break ;

  case 0xF4 : {
    _dev_scr_clr(scr, 0xF000 | dat) ;
  } break ;

  default : {
    if (0 == (adr & 0x10000))
      break ;
    
    scr->buf[adr & 0xFFFF] &= ~0xFF ;
    scr->buf[adr & 0xFFFF] |= dat   ;
    
    scr->edit = 1 ;
  } break ;
  }
}

void pm_dev_scr_sth (struct pm_dev_scr_t * scr, u_word_t adr, u_half_t dat)
{
  if (0 != (adr & 0x1) && adr < 0x10000)
    return ;

  switch (adr) {
  case 0x0 : {
    scr->len_x = (dat >> 0) & 0xFF ;
    scr->len_y = (dat >> 8) & 0xFF ;
    scr->edit  = 1 ;
  } break ;

  case 0x2 : {
    scr->cur_x = (dat >> 0) & 0xFF ;
    scr->cur_y = (dat >> 8) & 0xFF ;
    scr->edit  = 1 ;
  } break ;

  case 0x04 : case 0x06 :
    /* fallthrough */
  case 0x08 : case 0x18 : case 0x28 : case 0x38 :
  case 0x48 : case 0x58 : case 0x68 : case 0x78 :
  case 0x88 : case 0x98 : case 0xA8 : case 0xB8 :
  case 0xC8 : case 0xD8 : case 0xE8 : case 0xF8 :
  case 0x0A : case 0x1A : case 0x2A : case 0x3A :
  case 0x4A : case 0x5A : case 0x6A : case 0x7A :
  case 0x8A : case 0x9A : case 0xAA : case 0xBA :
  case 0xCA : case 0xDA : case 0xEA : case 0xFA : {
    /* ignore */
  } break ;

  case 0xF4 : {
    _dev_scr_clr(scr, dat) ;
  } break ;

  default : {
    if (0 == (adr & 0x10000))
      break ;

    scr->buf[adr & 0xFFFF] = dat ;
    
    scr->edit = 1 ;
  } break ;
  }
}

void pm_dev_scr_stw (struct pm_dev_scr_t * scr, u_word_t adr, u_word_t dat)
{
  if (0 != (adr & 0x3))
    return ;

  switch (adr) {
  case 0x00 : {
    scr->len_x = (dat >>  0) & 0xFF ;
    scr->len_y = (dat >>  8) & 0xFF ;
    scr->cur_x = (dat >> 16) & 0xFF ;
    scr->cur_y = (dat >> 24) & 0xFF ;
    scr->edit  = 1 ;
  } break ;

  case 0x04 : {
    scr->fnt  = dat ;
    scr->edit = 1 ;
  } break ;

  case 0x08 : case 0x18 : case 0x28 : case 0x38 :
  case 0x48 : case 0x58 : case 0x68 : case 0x78 :
  case 0x88 : case 0x98 : case 0xA8 : case 0xB8 :
  case 0xC8 : case 0xD8 : case 0xE8 : case 0xF8 : {
    scr->pal[adr >> 4] = dat ;
    scr->edit = 1 ;
  } break ;
  }
}

u_byte_t pm_dev_scr_ldb (struct pm_dev_scr_t * scr, u_word_t adr)
{
  u_byte_t dat = 0 ;

  switch (adr) {
  case 0x00 : {
    dat = scr->len_x ;
  } break ;
  
  case 0x01 : {
    dat = scr->len_y ;
  } break ;

  case 0x02 : {
    dat = scr->cur_x ;
  } break ;

  case 0x03 : {
    dat = scr->cur_y ;
  } break ;

  case 0x04 : case 0x05 : case 0x06 : case 0x07 : {
    /* ignored */
  } break ;

  case 0x08 : case 0x18 : case 0x28 : case 0x38 :
  case 0x48 : case 0x58 : case 0x68 : case 0x78 :
  case 0x88 : case 0x98 : case 0xA8 : case 0xB8 :
  case 0xC8 : case 0xD8 : case 0xE8 : case 0xF8 :
  case 0x09 : case 0x19 : case 0x29 : case 0x39 :
  case 0x49 : case 0x59 : case 0x69 : case 0x79 :
  case 0x89 : case 0x99 : case 0xA9 : case 0xB9 :
  case 0xC9 : case 0xD9 : case 0xE9 : case 0xF9 :
  case 0x0A : case 0x1A : case 0x2A : case 0x3A :
  case 0x4A : case 0x5A : case 0x6A : case 0x7A :
  case 0x8A : case 0x9A : case 0xAA : case 0xBA :
  case 0xCA : case 0xDA : case 0xEA : case 0xFA :
  case 0x0B : case 0x1B : case 0x2B : case 0x3B :
  case 0x4B : case 0x5B : case 0x6B : case 0x7B :
  case 0x8B : case 0x9B : case 0xAB : case 0xBB :
  case 0xCB : case 0xDB : case 0xEB : case 0xFB : {
    int col_idx = (adr >> 4) & 0xF ;
    int com_ofs = (adr & 0x3) * 8 ;

    dat = (scr->pal[col_idx] >> com_ofs) & 0xFF ;
  } break ;

  default : {
    if (0 == (adr & 0x10000))
      break ;
    
    dat = scr->buf[adr & 0xFFFF] & 0xFF ;
  } break ;
  }

  return dat ;
}

u_half_t pm_dev_scr_ldh (struct pm_dev_scr_t * scr, u_word_t adr)
{
  u_half_t dat = 0 ;

  if (0 != (adr & 0x1) && adr < 0x10000)
    return dat ;

  switch (adr) {
  case 0x0 : {
    dat |= (u_half_t)scr->len_x << 0 ;
    dat |= (u_half_t)scr->len_y << 8 ;
  } break ;

  case 0x2 : {
    dat |= (u_half_t)scr->cur_x << 0 ;
    dat |= (u_half_t)scr->cur_y << 8 ;
  } break ;

  case 0x04 : case 0x06 :
    /* fallthrough */
  case 0x08 : case 0x18 : case 0x28 : case 0x38 :
  case 0x48 : case 0x58 : case 0x68 : case 0x78 :
  case 0x88 : case 0x98 : case 0xA8 : case 0xB8 :
  case 0xC8 : case 0xD8 : case 0xE8 : case 0xF8 :
  case 0x0A : case 0x1A : case 0x2A : case 0x3A :
  case 0x4A : case 0x5A : case 0x6A : case 0x7A :
  case 0x8A : case 0x9A : case 0xAA : case 0xBA :
  case 0xCA : case 0xDA : case 0xEA : case 0xFA : {
    /* ignore */
  } break ;

  default : {
    if (0 == (adr & 0x10000))
      break ;

    dat = scr->buf[adr & 0xFFFF] ;
  } break ;
  }

  return dat ;
}

u_word_t pm_dev_scr_ldw (struct pm_dev_scr_t * scr, u_word_t adr)
{
  u_word_t dat = 0 ;

  if (0 != (adr & 0x3))
    return dat ;

  switch (adr) {
  case 0x00 : {
    dat |= (u_word_t)scr->len_x <<  0;
    dat |= (u_word_t)scr->len_y <<  8 ;
    dat |= (u_word_t)scr->cur_x << 16 ;
    dat |= (u_word_t)scr->cur_y << 24 ;
  } break ;

  case 0x04 : {
    dat = scr->fnt ;
  } break ;

  case 0x08 : case 0x18 : case 0x28 : case 0x38 :
  case 0x48 : case 0x58 : case 0x68 : case 0x78 :
  case 0x88 : case 0x98 : case 0xA8 : case 0xB8 :
  case 0xC8 : case 0xD8 : case 0xE8 : case 0xF8 : {
    dat = scr->pal[adr >> 4] ;
  } break ;
  }

  return dat ;
}

static void _dev_scr_clr (struct pm_dev_scr_t * scr, u_half_t chr)
{
  scr->cur_x = 0 ;
  scr->cur_y = 0 ;

  for (int idx = 0 ; idx < 0x10000 ; ++idx) {
    scr->buf[idx] = chr ;
  }

  scr->edit = 1 ;
}

static void _dev_scr_ver_scroll_u (struct pm_dev_scr_t * scr, u_byte_t n)
{
  if (scr->len_y < n)
    return ;

  if (scr->len_y == n) {
    _dev_scr_clr(scr, 0xF020) ;
    return ;
  }

  for (int i = 0 ; i < scr->len_y - n ; ++i) {
    for (int j = 0 ; j < scr->len_x ; ++j) {
      scr->buf[i * scr->len_x + j] = scr->buf[(i + n) * scr->len_x + j] ;
    }
  }

  for (int i = scr->len_y - n ; i < scr->len_y ; ++i) {
    for (int j = 0 ; j < scr->len_x ; ++j) {
      scr->buf[i * scr->len_x + j] = 0xF020 ;
    }
  }

  scr->edit = 1 ;
}

static void _dev_scr_ver_scroll_d (struct pm_dev_scr_t * scr, u_byte_t n)
{
  if (scr->len_y < n)
    return ;

  if (scr->len_y == n) {
    _dev_scr_clr(scr, 0xF020) ;
    return ;
  }

  for (int i = scr->len_y - 1 ; n <= i ; --i) {
    for (int j = 0 ; j < scr->len_x ; ++j) {
      scr->buf[i * scr->len_x + j] = scr->buf[(i - n) * scr->len_x + j] ;
    }
  }

  for (int i = 0 ; i < n ; ++i) {
    for (int j = 0 ; j < scr->len_x ; ++j) {
      scr->buf[i * scr->len_x + j] = 0xF020 ;
    }
  }

  scr->edit = 1 ;
}

static void _dev_scr_hor_scroll_l (struct pm_dev_scr_t * scr, u_byte_t n)
{
  if (scr->len_x < n)
    return ;

  if (scr->len_x == n) {
    _dev_scr_clr(scr, 0xF020) ;
    return ;
  }

  for (int i = 0 ; i < scr->len_y ; ++i) {
    for (int j = 0 ; j < scr->len_x - n ; ++j) {
      scr->buf[i * scr->len_x + j] = scr->buf[i * scr->len_x + j + n] ;
    }

    for (int j = scr->len_x - n ; j < scr->len_x ; ++j) {
      scr->buf[i * scr->len_x + j] = 0xF020 ;
    }
  }

  scr->edit = 1 ;
}

static void _dev_scr_hor_scroll_r (struct pm_dev_scr_t * scr, u_byte_t n)
{
  if (scr->len_x < n)
    return ;

  if (scr->len_x == n) {
    _dev_scr_clr(scr, 0xF020) ;
    return ;
  }

  for (int i = 0 ; i < scr->len_y ; ++i) {
    for (int j = scr->len_x - 1 ; n <= j ; --j) {
      scr->buf[i * scr->len_x + j] = scr->buf[i * scr->len_x + j - n] ;
    }

    for (int j = 0 ; j < n ; ++j) {
      scr->buf[i * scr->len_x + j] = 0xF020 ;
    }
  }

  scr->edit = 1 ;
}
