#include "dsk.h"

static void _sth_le (u_byte_t * dst, u_half_t src)
{
  dst[0] = (src >> 0) & 0xFF ;
  dst[1] = (src >> 8) & 0xFF ;
}

static void _sth_be (u_byte_t * dst, u_half_t src)
{
  dst[1] = (src >> 0) & 0xFF ;
  dst[0] = (src >> 8) & 0xFF ;
}

static void _stw_le (u_byte_t * dst, u_word_t src)
{
  dst[0] = (src >>  0) & 0xFF ;
  dst[1] = (src >>  8) & 0xFF ;
  dst[2] = (src >> 12) & 0xFF ;
  dst[3] = (src >> 24) & 0xFF ;
}

static void _stw_be (u_byte_t * dst, u_word_t src)
{
  dst[3] = (src >>  0) & 0xFF ;
  dst[2] = (src >>  8) & 0xFF ;
  dst[1] = (src >> 12) & 0xFF ;
  dst[0] = (src >> 24) & 0xFF ;
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

  if (0 == len || 0 != (len & 0x1FF)) {
    fclose(dsk->fp) ;
    dsk->fp  = NULL ;
    dsk->err = PM_DEV_DSK_ERR_IL ;
    return ;
  }

  dsk->len = len ;
  dsk->err = PM_DEV_DSK_NO_ERR ;
  dsk->sec = 0 ;
/* dsk->bo  = __PM_ENDIAN ; */
  _dev_dsk_ldsec(dsk) ;
}

void pm_dev_dsk_clk (struct pm_dev_dsk_t * dsk)
{
}

void pm_dev_dsk_stb (struct pm_dev_dsk_t * dsk, u_word_t adr, u_byte_t dat)
{
  switch (adr) {
  case 0x00 :
    /* fallthrough */
  case 0x04 :
    /* fallthrough */
  case 0x08 :
    /* fallthrough */
  case 0x0C :
    /* fallthrough */
  case 0x10 : {
    /* ignored */
  } break ;

  default : {
    if (adr < 0x200)
      break ;

    dsk->buf[adr & 0x1FF] = dat ;
  } break ;
  }
}

void pm_dev_dsk_sth (struct pm_dev_dsk_t * dsk, u_word_t adr, u_half_t dat)
{
  if (0 != (adr & 0x1))
    return ;

  switch (adr) {
  case 0x00 :
    /* fallthrough */
  case 0x04 :
    /* fallthrough */
  case 0x08 :
    /* fallthrough */
  case 0x0C :
    /* fallthrough */
  case 0x10 : {
    /* ignored */
  } break ;

  default : {
    if (adr < 0x200)
      break ;

    adr &= 0x1FF ;
    
    if (__PM_ENDIAN_LE == dsk->bo) {
      _sth_le(dsk->buf + adr, dat) ;
    } else {
      _sth_be(dsk->buf + adr, dat) ;
    }
  } break ;
  }
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
    dsk->bo = (0 == dat) ? __PM_ENDIAN_LE : __PM_ENDIAN_BE ;
  } break ;

  case 0x10 : {
    _dev_dsk_stsec(dsk) ;
  } break ;

  default : {
    if (adr < 0x200)
      break ;

    adr &= 0x1FF ;
    
    if (__PM_ENDIAN_LE == dsk->bo) {
      _stw_le(dsk->buf + adr, dat) ;
    } else {
      _stw_be(dsk->buf + adr, dat) ;
    }
  } break ;
  }
}

u_byte_t pm_dev_dsk_ldb (struct pm_dev_dsk_t * dsk, u_word_t adr)
{
  u_half_t dat = 0 ;

  switch (adr) {
  case 0x00 :
    /* fallthrough */
  case 0x04 :
    /* fallthrough */
  case 0x08 :
    /* fallthrough */
  case 0x0C :
    /* fallthrough */
  case 0x10 : {
    /* ignored */
  } break ;

  default : {
    if (adr < 0x200)
      break ;

    dat = dsk->buf[adr & 0x1FF] ;
  } break ;
  }

  return dat ;
}

u_half_t pm_dev_dsk_ldh (struct pm_dev_dsk_t * dsk, u_word_t adr)
{
  u_half_t dat = 0 ;

  if (0 != (adr & 0x1))
    return dat ;

  switch (adr) {
  case 0x00 :
    /* fallthrough */
  case 0x04 :
    /* fallthrough */
  case 0x08 :
    /* fallthrough */
  case 0x0C :
    /* fallthrough */
  case 0x10 : {
    /* ignored */
  } break ;

  default : {
    if (adr < 0x200)
      break ;

    adr &= 0x1FF ;
    
    if (__PM_ENDIAN_LE == dsk->bo) {
      dat = _ldh_le(dsk->buf + adr) ;
    } else {
      dat = _ldh_be(dsk->buf + adr) ;
    }
  } break ;
  }

  return dat ;
}

u_word_t pm_dev_dsk_ldw (struct pm_dev_dsk_t * dsk, u_word_t adr)
{
  u_word_t dat = 0 ;

  if (0 != (adr & 0x3))
    return dat ;

  switch (adr) {
  case 0x00 : {
    dat = dsk->err ;
  } break ;

  case 0x04 : {
    dat = dsk->len ;
  } break ;

  case 0x08 : {
    dat = dsk->sec ;
  } break ;

  case 0x0C : {
    dat = dsk->bo ;
  } break ;

  case 0x10 : {
    _dev_dsk_ldsec(dsk) ;
  } break ;

  default : {
    if (adr < 0x200)
      break ;

    adr &= 0x1FF ;
    
    if (__PM_ENDIAN_LE == dsk->bo) {
      dat = _ldw_le(dsk->buf + adr) ;
    } else {
      dat = _ldw_be(dsk->buf + adr) ;
    }
  } break ;
  }

  return dat ;
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
