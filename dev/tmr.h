#ifndef __PM_DEV_TMR_H
# define __PM_DEV_TMR_H

# include "../pm.h"

# define PM_DEV_TMR_SRS_RN 0
# define PM_DEV_TMR_SRS_OV 1
# define PM_DEV_TMR_SRS_RS 2

# define PM_DEV_TMR_SRM_RN 1
# define PM_DEV_TMR_SRM_OV 1
# define PM_DEV_TMR_SRM_RS 1

# define PM_DEV_TMR_SRF_RN (PM_DEV_TMR_SRM_RN << PM_DEV_TMR_SRS_RN)
# define PM_DEV_TMR_SRF_OV (PM_DEV_TMR_SRM_OV << PM_DEV_TMR_SRS_OV)
# define PM_DEV_TMR_SRF_RS (PM_DEV_TMR_SRM_RS << PM_DEV_TMR_SRS_RS)

struct pm_dev_tmr_t {
  struct pm_dev_t dev ;
  u_word_t        sr  ;
  u_word_t        ofs ;
  u_word_t        lmt ;
} ;

void     pm_dev_tmr_rst (struct pm_dev_tmr_t * tmr) ;
void     pm_dev_tmr_clk (struct pm_dev_tmr_t * tmr) ;
void     pm_dev_tmr_stw (struct pm_dev_tmr_t * tmr, u_word_t adr, u_word_t dat) ;
u_word_t pm_dev_tmr_ldw (struct pm_dev_tmr_t * tmr, u_word_t adr) ;

#endif
