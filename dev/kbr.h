#ifndef __PM_DEV_KBR_H
# define __PM_DEV_KBR_H

# include "../pm.h"

struct pm_dev_kbr_t {
  struct pm_dev_t dev   ;
  u_byte_t        front ;
  u_byte_t        rear  ;
  u_byte_t        queue [ 0x20 ] ;
} ;

void     pm_dev_kbr_rst (struct pm_dev_kbr_t * kbr) ;
void     pm_dev_kbr_clk (struct pm_dev_kbr_t * kbr) ;
void     pm_dev_kbr_stb (struct pm_dev_kbr_t * kbr, u_word_t adr, u_byte_t dat) ;
u_byte_t pm_dev_kbr_ldb (struct pm_dev_kbr_t * kbr, u_word_t adr) ;
void     pm_dev_kbr_enq (struct pm_dev_kbr_t * kbr, u_byte_t chr) ;
u_byte_t pm_dev_kbr_deq (struct pm_dev_kbr_t * kbr) ;

#endif
