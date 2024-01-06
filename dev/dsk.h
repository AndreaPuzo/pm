#ifndef __PM_DEV_DSK_H
# define __PM_DEV_DSK_H

# include "../pm.h"
# include <stdio.h>

# define PM_DEV_DSK_NO_ERR 0
# define PM_DEV_DSK_ERR_NF 1
# define PM_DEV_DSK_ERR_RL 2
# define PM_DEV_DSK_ERR_IL 3
# define PM_DEV_DSK_ERR_SK 4
# define PM_DEV_DSK_ERR_WS 5
# define PM_DEV_DSK_ERR_RS 6
# define PM_DEV_DSK_ERR_IS 7

struct pm_dev_dsk_t {
  struct pm_dev_t dev ;
  char *          fn  ;
  FILE *          fp  ;
  u_word_t        bo  ;
  u_word_t        err ;
  u_word_t        len ;
  u_word_t        sec ;
  u_byte_t        buf [ 0x200 ] ;
} ;

void     pm_dev_dsk_rst (struct pm_dev_dsk_t * dsk) ;
void     pm_dev_dsk_clk (struct pm_dev_dsk_t * dsk) ;
void     pm_dev_dsk_stb (struct pm_dev_dsk_t * dsk, u_word_t adr, u_byte_t dat) ;
void     pm_dev_dsk_sth (struct pm_dev_dsk_t * dsk, u_word_t adr, u_half_t dat) ;
void     pm_dev_dsk_stw (struct pm_dev_dsk_t * dsk, u_word_t adr, u_word_t dat) ;
u_byte_t pm_dev_dsk_ldb (struct pm_dev_dsk_t * dsk, u_word_t adr) ;
u_half_t pm_dev_dsk_ldh (struct pm_dev_dsk_t * dsk, u_word_t adr) ;
u_word_t pm_dev_dsk_ldw (struct pm_dev_dsk_t * dsk, u_word_t adr) ;

#endif
