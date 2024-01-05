#include "tmr.h"

void pm_dev_tmr_rst (struct pm_dev_tmr_t * tmr)
{
  tmr->sr  = PM_DEV_TMR_SRF_RS ;
  tmr->ofs = 0 ;
  tmr->lmt = 0 ;

  pm_iom_int(tmr->dev.bus, (struct pm_dev_t *)tmr) ;
}

void pm_dev_tmr_clk (struct pm_dev_tmr_t * tmr)
{
  if (0 == (tmr->sr & PM_DEV_TMR_SRF_RN))
    return ;

  /* TODO (#1): rewrite it pretty */
  u_word_t ck = tmr->dev.bus->bus->cpu.ck0 ;

  if (0 != (tmr->sr & PM_DEV_TMR_SRF_RS)) {
    tmr->ofs = ck ;
    tmr->sr &= ~PM_DEV_TMR_SRF_RS ;
  } else if (ck < tmr->ofs) {
    tmr->sr &= ~PM_DEV_TMR_SRF_RN ;
    tmr->sr |=  PM_DEV_TMR_SRF_OV ;
    pm_iom_int(tmr->dev.bus, (struct pm_dev_t *)tmr) ;
    return ;
  }

  if (tmr->lmt <= (ck - tmr->ofs)) {
    tmr->sr &= ~PM_DEV_TMR_SRF_RN ;
    pm_iom_int(tmr->dev.bus, (struct pm_dev_t *)tmr) ;
  }
}

void pm_dev_tmr_stw (struct pm_dev_tmr_t * tmr, u_word_t adr, u_word_t dat)
{
  switch (adr) {
  case 0x0 : {
    tmr->sr &= ~PM_DEV_TMR_SRF_RN ;
  } break ;

  case 0x1 : {
    tmr->sr |=  PM_DEV_TMR_SRF_RN ;
  } break ;

  case 0x2 : {
    tmr->sr &= ~PM_DEV_TMR_SRF_OV ;
  } break ;

  case 0x3 : {
    tmr->sr |=  PM_DEV_TMR_SRF_OV ;
  } break ;

  case 0x4 : {
    tmr->sr &= ~PM_DEV_TMR_SRF_RS ;
  } break ;

  case 0x5 : {
    tmr->sr |=  PM_DEV_TMR_SRF_RS ;
  } break ;

  case 0x6 : {
    tmr->sr = dat ;
  } break ;

  case 0x7 : {} break ;

  case 0x8 : {
    tmr->lmt = dat ;
  } break ;
  }
}

u_word_t pm_dev_tmr_ldw (struct pm_dev_tmr_t * tmr, u_word_t adr)
{
  u_word_t dat = 0 ;

  switch (adr) {
  case 0x0 : {
    dat = 0 == (tmr->sr & PM_DEV_TMR_SRF_RN) ;
  } break ;

  case 0x1 : {
    dat = 0 != (tmr->sr & PM_DEV_TMR_SRF_RN) ;
  } break ;

  case 0x2 : {
    dat = 0 == (tmr->sr & PM_DEV_TMR_SRF_OV) ;
  } break ;

  case 0x3 : {
    dat = 0 != (tmr->sr & PM_DEV_TMR_SRF_OV) ;
  } break ;

  case 0x4 : {
    dat = 0 == (tmr->sr & PM_DEV_TMR_SRF_RS) ;
  } break ;

  case 0x5 : {
    dat = 0 != (tmr->sr & PM_DEV_TMR_SRF_RS) ;
  } break ;

  case 0x6 : {
    dat = tmr->sr ;
  } break ;

  case 0x7 : {
    dat = tmr->ofs ;
  } break ;

  case 0x8 : {
    dat = tmr->lmt ;
  } break ;
  }

  return dat ;
}
