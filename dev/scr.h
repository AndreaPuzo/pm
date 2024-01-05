#ifndef __PM_DEV_SCR_H
# define __PM_DEV_SCR_H

# include "../pm.h"

struct pm_dev_scr_t {
  struct pm_dev_t dev   ;
  int             edit  ;
  u_byte_t        len_x ;
  u_byte_t        len_y ;
  u_byte_t        cur_x ;
  u_byte_t        cur_y ;
  u_word_t        fnt   ;
  u_word_t        pal [ 0x10 ] ;
  u_half_t        buf [ 0x10000 ] ;
} ;

void     pm_dev_scr_rst (struct pm_dev_scr_t * scr) ;
void     pm_dev_scr_clk (struct pm_dev_scr_t * scr) ;
void     pm_dev_scr_stb (struct pm_dev_scr_t * scr, u_word_t adr, u_byte_t dat) ;
void     pm_dev_scr_sth (struct pm_dev_scr_t * scr, u_word_t adr, u_half_t dat) ;
void     pm_dev_scr_stw (struct pm_dev_scr_t * scr, u_word_t adr, u_word_t dat) ;
u_byte_t pm_dev_scr_ldb (struct pm_dev_scr_t * scr, u_word_t adr) ;
u_half_t pm_dev_scr_ldh (struct pm_dev_scr_t * scr, u_word_t adr) ;
u_word_t pm_dev_scr_ldw (struct pm_dev_scr_t * scr, u_word_t adr) ;

#endif
