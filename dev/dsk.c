#include "dsk.h"

static void _sth_le (u_byte_t * dst, u_half_t src)
{
  dat[0] = (dat >> 0) & 0xFF ;
  dat[1] = (dat >> 8) & 0xFF ;
}

static void _sth_be (u_byte_t * dst, u_half_t src)
{
  dat[1] = (dat >> 0) & 0xFF ;
  dat[0] = (dat >> 8) & 0xFF ;
}

static void _stw_le (u_byte_t * dst, u_word_t src)
{
  dat[0] = (dat >>  0) & 0xFF ;
  dat[1] = (dat >>  8) & 0xFF ;
  dat[2] = (dat >> 12) & 0xFF ;
  dat[3] = (dat >> 24) & 0xFF ;
}

static void _stw_be (u_byte_t * dst, u_word_t src)
{
  dat[3] = (dat >>  0) & 0xFF ;
  dat[2] = (dat >>  8) & 0xFF ;
  dat[1] = (dat >> 12) & 0xFF ;
  dat[0] = (dat >> 24) & 0xFF ;
}

static u_half_t _ldh_le (u_byte_t * src)
{
  u_half_t dat = 0 ;

  dat |= src[1] ; dat <<= 8 ;
  dat |= src[0] ;

  return dat ;
}

static u_half_t _ldh_be (u_byte_t * src)
{
  u_half_t dat = 0 ;

  dat |= src[0] ; dat <<= 8 ;
  dat |= src[1] ;

  return dat ;
}

static u_word_t _ldw_le (u_byte_t * src)
{
  u_word_t dat = 0 ;

  dat |= src[3] ; dat <<= 8 ;
  dat |= src[2] ; dat <<= 8 ;
  dat |= src[1] ; dat <<= 8 ;
  dat |= src[0] ;

  return dat ;
}

static u_word_t _ldw_be (u_byte_t * src)
{
  u_word_t dat = 0 ;

  dat |= src[0] ; dat <<= 8 ;
  dat |= src[1] ; dat <<= 8 ;
  dat |= src[2] ; dat <<= 8 ;
  dat |= src[3] ;

  return dat ;
}

static void _dev_dsk_stsec (struct pm_dev_dsk_t * dsk) ;
static void _dev_dsk_ldsec (struct pm_dev_dsk_t * dsk) ;

void pm_dev_dsk_rst (struct pm_dev_dsk_t * dsk)
{
  if (NULL != dsk->fp) {
    fclose(dsk->fp) ;
    dsk->fp = NULL  ;

    if (NULL == dsk->fn)
      return ;
  }

  dsk->fp = fopen(dsk->fn, "r+") ;

  if (NULL == dsk->fp) {
    dsk->err = PM_DEV_DSK_ERR_NF ;
    return ;
  }

  if (fseek(dsk->fp, 0, SEEK_END) < 0) {
    fclose(dsk->fp) ;
    dsk->fp  = NULL ;
    dsk->err = PM_DEV_DSK_ERR_SK ;
    return ;
  }

  long len = ftell(dsk->fp) ;

  if (len < 0) {
    fclose(dsk->fp) ;
    dsk->fp  = NULL ;
    dsk->err = PM_DEV_DSK_ERR_RL ;
    return ;
  }

  if (fseek(dsk->fp, 0, SEEK_SET) < 0) {
    fclose(dsk->fp) ;
    dsk->fp  = NULL ;
    dsk->err = PM_DEV_DSK_ERR_SK ;
    return ;
  }

  if (0 == dsk->len || 0 != (dsk->len & 0x1FF)) {
    fclose(dsk->fp) ;
    dsk->fp  = NULL ;
    dsk->err = PM_DEV_DSK_ERR_IL ;
    return ;
  }

  dsk->err = PM_DEV_DSK_NO_ERR ;
  dsk->sec = 0 ;
  _dev_dsk_ldsec(dsk) ;
}

void pm_dev_dsk_clk (struct pm_dev_dsk_t * dsk)
{
}

void pm_dev_dsk_stb (struct pm_dev_dsk_t * dsk, u_word_t adr, u_byte_t dat)
{
  
}

void pm_dev_dsk_sth (struct pm_dev_dsk_t * dsk, u_word_t adr, u_half_t dat)
{

}

void pm_dev_dsk_stw (struct pm_dev_dsk_t * dsk, u_word_t adr, u_word_t dat)
{
  if (0 != (adr & 0x3))
    return ;

  switch (adr) {
  case 0x00 : {
    dsk->err = dat ;
  } break ;

  case 0x04 : {
    /* ignored */
  } break ;

  case 0x08 : {
    if ((dsk->len / 0x200) <= dat) {
      dsk->err = PM_DEV_DSK_ERR_IS ;
    } else {
      _dev_dsk_stsec(dsk) ;
      dsk->sec = dat ;
      _dev_dsk_ldsec(dsk) ;
    }
  } break ;

  case 0x0C : {
    _dev_dsk_stsec(dsk) ;
  } break ;

  default : {
    if (0x100 < adr)
      break ;

    adr &= 0x1FF ;
    


  } break ;
  }
}

u_byte_t pm_dev_dsk_ldb (struct pm_dev_dsk_t * dsk, u_word_t adr)
{

}

u_half_t pm_dev_dsk_ldh (struct pm_dev_dsk_t * dsk, u_word_t adr)
{

}

u_word_t pm_dev_dsk_ldw (struct pm_dev_dsk_t * dsk, u_word_t adr)
{

}

static void _dev_dsk_stsec (struct pm_dev_dsk_t * dsk)
{
  if (NULL == dsk->fp)
    return ;

  long ofs = dsk->sec * 0x200 ;

  if (fseek(dsk->fp, ofs, SEEK_SET) < 0) {
    dsk->err = PM_DEV_DSK_ERR_SK ;
    return ;
  }

  if (0x200 != fwrite(dsk->buf, sizeof(u_byte_t), 0x200, dsk->fp)) {
    dsk->err = PM_DEV_DSK_ERR_WS ;
    return ;
  }
}

static void _dev_dsk_ldsec (struct pm_dev_dsk_t * dsk)
{
  if (NULL == dsk->fp)
    return ;

  long ofs = dsk->sec * 0x200 ;

  if (fseek(dsk->fp, ofs, SEEK_SET) < 0) {
    dsk->err = PM_DEV_DSK_ERR_SK ;
    return ;
  }

  if (0x200 != fread(dsk->buf, sizeof(u_byte_t), 0x200, dsk->fp)) {
    dsk->err = PM_DEV_DSK_ERR_RS ;
    return ;
  }
}
