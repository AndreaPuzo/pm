#include "kbr.h"

void pm_dev_kbr_rst (struct pm_dev_kbr_t * kbr)
{
  kbr->front = 0 ;
  kbr->rear  = 0 ;

  for (int idx = 0 ; idx < sizeof(kbr->queue) ; ++idx) {
    kbr->queue[idx] = 0xFF ;
  }

  pm_iom_int(kbr->dev.bus, (struct pm_dev_t *)kbr) ;
}

void pm_dev_kbr_clk (struct pm_dev_kbr_t * kbr)
{
}

void pm_dev_kbr_stb (struct pm_dev_kbr_t * kbr, u_word_t adr, u_byte_t dat)
{
  switch (adr) {
  case 0x0 : {
    kbr->front = dat ;
  } break ;

  case 0x1 : {
    kbr->rear = dat ;
  } break ;

  case 0x2 : {
    pm_dev_kbr_enq(kbr, dat) ;
  } break ;
  }
}

u_byte_t pm_dev_kbr_ldb (struct pm_dev_kbr_t * kbr, u_word_t adr)
{
  u_byte_t dat = 0 ;

  switch (adr) {
  case 0x0 : {
    dat = kbr->front ;
  } break ;

  case 0x1 : {
    dat = kbr->rear ;
  } break ;

  case 0x2 : {
    dat = pm_dev_kbr_deq(kbr) ;
  } break ;
  }

  return dat ;
}

void pm_dev_kbr_enq (struct pm_dev_kbr_t * kbr, u_byte_t chr)
{
  u_word_t msk = sizeof(kbr->queue) - 1 ;

  if (kbr->front == ((kbr->rear + 1) & msk)) {
    pm_iom_int(kbr->dev.bus, (struct pm_dev_t *)kbr) ;
    return ;
  }

  kbr->queue[kbr->rear] = chr ;
  kbr->rear = (kbr->rear + 1) & msk ;
}

u_byte_t pm_dev_kbr_deq (struct pm_dev_kbr_t * kbr)
{
  u_word_t msk = sizeof(kbr->queue) - 1 ;

  if (kbr->front == kbr->rear) {
    pm_iom_int(kbr->dev.bus, (struct pm_dev_t *)kbr) ;
    return 0xFF ;
  }

  u_byte_t chr = kbr->queue[kbr->front] ;
  kbr->front = (kbr->front + 1) & msk ;

  return chr ;
}
