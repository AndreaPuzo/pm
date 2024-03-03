#include "pm.h"
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#define DEBUG_0(msg)      fprintf(stderr, msg)
#define DEBUG_1(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)


/* Central Processing Unit */

static void     _cpu_psb   (struct pm_cpu_t * cpu, u_word_t * sp, u_byte_t dat) ;
static void     _cpu_psh   (struct pm_cpu_t * cpu, u_word_t * sp, u_half_t dat) ;
static void     _cpu_psw   (struct pm_cpu_t * cpu, u_word_t * sp, u_word_t dat) ;
static u_byte_t _cpu_plb   (struct pm_cpu_t * cpu, u_word_t * sp) ;
static u_half_t _cpu_plh   (struct pm_cpu_t * cpu, u_word_t * sp) ;
static u_word_t _cpu_plw   (struct pm_cpu_t * cpu, u_word_t * sp) ;
static u_word_t _cpu_fch   (struct pm_cpu_t * cpu) ;
static void     _cpu_stcsr (struct pm_cpu_t * cpu, int idx, u_word_t dat) ;
static u_word_t _cpu_ldcsr (struct pm_cpu_t * cpu, int idx) ;
static void     _cpu_secsr (struct pm_cpu_t * cpu, int idx, u_word_t dat, u_word_t msk, int shf) ;
static u_word_t _cpu_gecsr (struct pm_cpu_t * cpu, int idx, u_word_t msk, int shf) ;
static u_word_t _cpu_trs   (struct pm_cpu_t * cpu, u_word_t adr, u_word_t xwr) ;
static int      _cpu_xhgcc (struct pm_cpu_t * cpu, int src_pl, int dst_pl) ;
static int      _cpu_int   (struct pm_cpu_t * cpu, int fun, u_word_t num) ;

#define _cpu_pl_set(dat) _cpu_secsr(cpu, PM_CSR_SR, PM_SRM_PL, PM_SRS_PL, dat)
#define _cpu_pl_get()    _cpu_gecsr(cpu, PM_CSR_SR, PM_SRM_PL, PM_SRS_PL)

int pm_cpu_ctor (struct pm_cpu_t * cpu, struct pm_bus_t * bus)
{
  cpu->bus = bus ;
  cpu->pc0 = 0   ;
  cpu->pc1 = 0   ;
  cpu->ck0 = 0   ;
  cpu->ck1 = 0   ;

  for (int idx = 0 ; idx < 0x20 ; ++idx) {
    cpu->csr[idx] = 0 ;
    cpu->xpr[idx] = 0 ;
  }

  return 0 ;
}

void pm_cpu_dtor (struct pm_cpu_t * cpu)
{
}

void pm_cpu_int (struct pm_cpu_t * cpu, u_word_t num)
{
  /* try to delegate interrupts to higher privilege levels */

  u_word_t id0 = _cpu_ldcsr(cpu, PM_CSR(0, PM_CSR_ID0)) ;
  u_word_t id1 = _cpu_ldcsr(cpu, PM_CSR(0, PM_CSR_ID1)) ;

  if (0 != ((id0 >> num) & 0x1)) {
    u_word_t irr = _cpu_ldcsr(cpu, PM_CSR(1, PM_CSR_IRR)) ;
    irr |= 1 << num ;
    _cpu_stcsr(cpu, PM_CSR(1, PM_CSR_IRR), irr) ;
    return ;
  }

  if (0 != ((id1 >> num) & 0x1)) {
    u_word_t irr = _cpu_ldcsr(cpu, PM_CSR(2, PM_CSR_IRR)) ;
    irr |= 1 << num ;
    _cpu_stcsr(cpu, PM_CSR(2, PM_CSR_IRR), irr) ;
    return ;
  }

  /* interrupt the machine privilege level */

  u_word_t irr =_cpu_ldcsr(cpu, PM_CSR(0, PM_CSR_IRR)) ;
  irr |= 1 << num ;
  _cpu_stcsr(cpu, PM_CSR(0, PM_CSR_IRR), irr) ;
}

void pm_cpu_rst (struct pm_cpu_t * cpu)
{
  cpu->pc0 = 0x80000000 ;
  cpu->ck0 = 0          ;
  cpu->ck1 = 0          ;

  for (int idx = 0 ; idx < 0x20 ; ++idx) {
    cpu->csr[idx] = 0 ;
    cpu->xpr[idx] = 0 ;
  }

  _cpu_stcsr(cpu, PM_CSR(0, PM_CSR_SR ), PM_SRF_RN ) ;
  _cpu_stcsr(cpu, PM_CSR(0, PM_CSR_IMR), 0xFFFFFFFC) ;
}

void pm_cpu_clk (struct pm_cpu_t * cpu)
{
  if (0 < cpu->ck1) {
    --cpu->ck1 ;
    ++cpu->ck0 ;
    return ;
  }

  cpu->xpr[0] = 0 ;

  /* handle interrupts */

  int pl = _cpu_pl_get() ;
  int wait_for_interrupt = 0 ;

  for (int idx = 0 ; idx < 4 ; ++idx) {
    u_word_t sr = _cpu_ldcsr(cpu, PM_CSR(idx, PM_CSR_SR)) ;

    if (0 == (sr & PM_SRF_EI)) {
      if (0 != (sr & PM_SRF_WI) && pl == idx) {
        wait_for_interrupt = 1 ;
      }

      continue ;
    }

    do {
      u_word_t irr = _cpu_ldcsr(cpu, PM_CSR(idx, PM_CSR_IRR)) ;
      u_word_t imr = _cpu_ldcsr(cpu, PM_CSR(idx, PM_CSR_IMR)) ;

      irr &= ~imr ;

      if (0 == irr) {
        if (0 != (sr & PM_SRF_WI) && pl == idx) {
          wait_for_interrupt = 1 ;
        }
                                                     
        break ;
      }

      int num ;

      for (num = 0 ; num < 0x20 ; ++num) {
        if (0 != ((irr >> num) & 0x1))
          break ;
      }

      irr &= ~(1 << num) ;
      _cpu_stcsr(cpu, PM_CSR(idx, PM_CSR_IRR), irr) ;

      if (0 == _cpu_int(cpu, 0, num)) {
        if (0 != (sr & PM_SRF_WI) && pl == idx) {
          sr &= ~PM_SRF_WI ;
          _cpu_stcsr(cpu, PM_CSR(idx, PM_CSR_SR), sr) ;
          wait_for_interrupt = 0 ;
        }

        break ;
      }
    } while (1) ;
  }

  if (0 != wait_for_interrupt)
    return ;

  /* fetch the next instruction */

  s_word_t ins = _cpu_fch(cpu) ;
  cpu->ins = ins ;

  /* decode the instruction */

  u_word_t o = (ins >>  0) & 0x01F ;
  u_word_t a = (ins >>  5) & 0x01F ;
  u_word_t b = (ins >> 10) & 0x01F ;
  u_word_t c = (ins >> 15) & 0x01F ;
  u_word_t u = (ins >> 20) & 0xFFF ;
  u_word_t s = (ins >> 20) ;

  /* execute the instruction */

  switch (o) {
  case 0x00 : {
    cpu->xpr[a] = (u_word_t)(
      (u_word_t)cpu->xpr[b] + (u_word_t)(cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x01 : {
    cpu->xpr[a] = (u_word_t)(
      (u_word_t)cpu->xpr[b] - (u_word_t)(cpu->xpr[c] + s)
    ) ;  
  } break ;

  case 0x02 : {
    cpu->xpr[a] = (u_word_t)(
      (u_word_t)cpu->xpr[b] * (u_word_t)(cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x03 : {
    u_word_t div = cpu->xpr[c] + s ;

    if (0 == div) {
      pm_cpu_int(cpu, PM_INT_DZ) ;
      break ;
    }

    cpu->xpr[a] = (u_word_t)(
      (u_word_t)cpu->xpr[b] / (u_word_t)div
    ) ;
  } break ;
  
  case 0x04 : {
     u_word_t div = cpu->xpr[c] + s ;

     if (0 == div) {
       pm_cpu_int(cpu, PM_INT_DZ) ;
       break ;
     }

     cpu->xpr[a] = (u_word_t)(
       (u_word_t)cpu->xpr[b] % (u_word_t)div
     ) ;
  } break ;

  case 0x05 : {
    cpu->xpr[a] = (u_word_t)(
      (s_word_t)cpu->xpr[b] * (s_word_t)(cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x06 : {
    u_word_t div = cpu->xpr[c] + s ;

    if (0 == div) {
      pm_cpu_int(cpu, PM_INT_DZ) ;
      break ;
    }

    cpu->xpr[a] = (u_word_t)(
      (s_word_t)cpu->xpr[b] / (s_word_t)div
    ) ;
  } break ;

  case 0x07 : {
    u_word_t div = cpu->xpr[c] + s ;

    if (0 == div) {
      pm_cpu_int(cpu, PM_INT_DZ) ;
      break ;
    }

    cpu->xpr[a] = (u_word_t)(
      (s_word_t)cpu->xpr[b] % (s_word_t)div
    ) ;
  } break ;

  case 0x08 : {
    cpu->xpr[a] = (u_word_t)(
      (u_word_t)cpu->xpr[b] & (u_word_t)(cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x09 : {
    cpu->xpr[a] = (u_word_t)(
      (u_word_t)cpu->xpr[b] | (u_word_t)(cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x0A : {
    cpu->xpr[a] = (u_word_t)(
      (u_word_t)cpu->xpr[b] ^ (u_word_t)(cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x0B : {
    cpu->xpr[a] = (u_word_t)(
      (u_word_t)cpu->xpr[b] << (u_word_t)(cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x0C : {
    cpu->xpr[a] = (u_word_t)(
      (u_word_t)cpu->xpr[b] >> (u_word_t)(cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x0D : {
    cpu->xpr[a] = (u_word_t)(
      (s_word_t)cpu->xpr[b] >> (u_word_t)(cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x0E : {
    int rshf = ( c & 0x1) * 16 ;
    int ishf = (~c & 0x1) * 16 ;
    u = (u << 4) | (c >> 1) ;

    cpu->xpr[a] = (cpu->xpr[b] & (0xFFFF << ishf)) | (u << rshf) ;
  } break ;

  case 0x0F : {
    s = (s << 4) | (c >> 1) ;

    u_word_t la = cpu->pc0 ;
    
    if (0 == (c & 0x1)) {
      cpu->pc0  = cpu->xpr[b] + (s * sizeof(u_word_t)) ;
    } else {
      cpu->pc0 += (cpu->xpr[b] + s) * sizeof(u_word_t) ;
    }

    cpu->xpr[a] = la ;
  } break ;

  case 0x10 : {
    if ((u_word_t)cpu->xpr[a] != (u_word_t)cpu->xpr[b]) {
      cpu->pc0 += (cpu->xpr[c] + s) * sizeof(u_word_t) ;
    }
  } break ;

  case 0x11 : {
    if ((u_word_t)cpu->xpr[a] == (u_word_t)cpu->xpr[b]) {
      cpu->pc0 += (cpu->xpr[c] + s) * sizeof(u_word_t) ;
    }
  } break ;

  case 0x12 : {
    if ((u_word_t)cpu->xpr[a] <  (u_word_t)cpu->xpr[b]) {
      cpu->pc0 += (cpu->xpr[c] + s) * sizeof(u_word_t) ;
    }
  } break ;

  case 0x13 : {
    if ((u_word_t)cpu->xpr[a] <= (u_word_t)cpu->xpr[b]) {
      cpu->pc0 += (cpu->xpr[c] + s) * sizeof(u_word_t) ;
    }
  } break ;

  case 0x14 : {
    if ((s_word_t)cpu->xpr[a] <  (s_word_t)cpu->xpr[b]) {
      cpu->pc0 += (cpu->xpr[c] + s) * sizeof(u_word_t) ;
    }
  } break ;

  case 0x15 : {
    if ((s_word_t)cpu->xpr[a] <= (s_word_t)cpu->xpr[b]) {
      cpu->pc0 += (cpu->xpr[c] + s) * sizeof(u_word_t) ;
    }
  } break ;

  case 0x16 : {
    cpu->xpr[a] = (u_word_t)(
      (s_byte_t)pm_cpu_ldb(cpu, cpu->xpr[b] + cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x17 : {
    cpu->xpr[a] = (u_word_t)(
      (s_half_t)pm_cpu_ldh(cpu, cpu->xpr[b] + cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x18 : {
    cpu->xpr[a] = (u_word_t)(
      (u_byte_t)pm_cpu_ldb(cpu, cpu->xpr[b] + cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x19 : {
    cpu->xpr[a] = (u_word_t)(
      (u_half_t)pm_cpu_ldh(cpu, cpu->xpr[b] + cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x1A : {
    cpu->xpr[a] = (u_word_t)(
      (u_word_t)pm_cpu_ldw(cpu, cpu->xpr[b] + cpu->xpr[c] + s)
    ) ;
  } break ;

  case 0x1B : {
    pm_cpu_stb(cpu, cpu->xpr[b] + cpu->xpr[c] + s, (u_byte_t)cpu->xpr[a]) ;
  } break ;

  case 0x1C : {
    pm_cpu_sth(cpu, cpu->xpr[b] + cpu->xpr[c] + s, (u_half_t)cpu->xpr[a]) ;
  } break ;

  case 0x1D : {
    pm_cpu_stw(cpu, cpu->xpr[b] + cpu->xpr[c] + s, (u_word_t)cpu->xpr[a]) ;
  } break ;

  case 0x1E : {
    switch (c) {
    case 0x00 : {
      _cpu_psb(cpu, cpu->xpr + a, (u_byte_t)(cpu->xpr[b] + s)) ;
    } break ;

    case 0x01 : {
      _cpu_psh(cpu, cpu->xpr + a, (u_half_t)(cpu->xpr[b] + s)) ;
    } break ;

    case 0x02 : {
      _cpu_psw(cpu, cpu->xpr + a, (u_word_t)(cpu->xpr[b] + s)) ;
    } break ;

    case 0x03 : {
      cpu->xpr[b] = (u_word_t)(
        (u_byte_t)_cpu_plb(cpu, cpu->xpr + a)
      ) ;
    } break ;

    case 0x04 : {
      cpu->xpr[b] = (u_word_t)(
        (u_half_t)_cpu_plh(cpu, cpu->xpr + a)
      ) ;
    } break ;

    case 0x05 : {
      cpu->xpr[b] = (u_word_t)(
        (u_word_t)_cpu_plw(cpu, cpu->xpr + a)
      ) ;
    } break ;

    case 0x06 : {
      cpu->xpr[b] = (u_word_t)(
        (s_byte_t)_cpu_plb(cpu, cpu->xpr + a)
      ) ;                                                    
    } break ;

    case 0x07 : {
      cpu->xpr[b] = (u_word_t)(
        (s_half_t)_cpu_plh(cpu, cpu->xpr + a)
      ) ;
    } break ;

    case 0x08 : {
      cpu->xpr[a] = ~cpu->xpr[b] + s ;
    } break ;

    case 0x09 : {
      u_word_t tmp = cpu->xpr[a] ;
      cpu->xpr[a]  = cpu->xpr[b] ;
      cpu->xpr[b]  = tmp         ;
    } break ;

    case 0x0A : {
      _cpu_psw(cpu, cpu->xpr + a, cpu->xpr[b]) ;
      cpu->xpr[b] = cpu->xpr[a] ;

      if (
        0 == _cpu_gecsr(
          cpu, PM_CSR(pl, PM_CSR_SR), PM_SRM_SD, PM_SRS_SD
        )
      ) {
        cpu->xpr[a] -= u ;
      } else {
        cpu->xpr[a] += u ;
      }
    } break ;

    case 0x0B : {
      cpu->xpr[a] = cpu->xpr[b] ;
      cpu->xpr[b] = _cpu_plw(cpu, cpu->xpr + a) ;
    } break ;

    case 0x0C :
    case 0x0D :
    case 0x0E :
    case 0x0F : {
      pm_cpu_int(cpu, PM_INT_UD) ;
    } break ;

    case 0x10 : {
      cpu->xpr[b] = _cpu_ldcsr(cpu, a) & s ;
    } break ;

    case 0x11 : {
      int pl_csr = (a >> 3) & 0x3 ;

      if (0x3 == pl || pl_csr < pl) {
        pm_cpu_int(cpu, PM_INT_PP) ;
        break ;
      }

      _cpu_stcsr(cpu, a, cpu->xpr[b] | s) ;
    } break ;

    case 0x12 : {
      u_word_t msk = (u >> 4) & 0xF ; 
      u_word_t shf = (u >> 0) & 0x7 ;
      cpu->xpr[b]  = _cpu_gecsr(cpu, a, msk, 4 * shf) ;
    } break ;

    case 0x13 : {
      u_word_t val = (u >> 8) & 0xF ;
      u_word_t msk = (u >> 4) & 0xF ;
      u_word_t shf = (u >> 0) & 0x7 ;

      int pl_csr = (a >> 3) & 0x3 ;

      if (0x3 == pl || pl_csr < pl) {
        pm_cpu_int(cpu, PM_INT_PP) ;
        break ;
      }

      _cpu_secsr(cpu, a, cpu->xpr[b] | val, msk, 4 * shf) ;
    } break ;

    case 0x14 : {
      _cpu_int(cpu, 0, a) ;
    } break ;

    case 0x15 : {
      _cpu_int(cpu, 1, 0) ;
    } break ;

    case 0x16 : {
      if (a <= 0xF) {
        pm_bus_rst(cpu->bus, a) ;
      } else {
        pm_bus_rst(cpu->bus, -1) ;
      }
    } break ;
    
    case 0x17 :
    case 0x18 :
    case 0x19 :
    case 0x1A :
    case 0x1B :
    case 0x1C :
    case 0x1D :
    case 0x1E :
    case 0x1F : {
      pm_cpu_int(cpu, PM_INT_UD) ;
    } break ;
    }
  } break ;

  case 0x1F : {
    pm_cpu_int(cpu, PM_INT_UD) ;
  } break ;
  }

  pl = _cpu_pl_get() ;
    
  if (
    0 != _cpu_gecsr(
      cpu, PM_CSR(pl, PM_CSR_SR), PM_SRM_SS, PM_SRS_SS
    )
  ) {
    pm_cpu_int(cpu, PM_INT_SS) ;
  }

  if (
    0 == _cpu_gecsr(
      cpu, PM_CSR(pl, PM_CSR_SR), PM_SRM_RN, PM_SRS_RN
    )
  ) {
    if (0 != pl) {
      _cpu_xhgcc(cpu, pl, 0) ;
    } else {
      cpu->bus->hlt = 1 ;
    }
  }

  --cpu->ck1 ;
  ++cpu->ck0 ;
}

#define _st_fun(typ)                     \
  do {                                   \
    cpu->ck1 += 2 ;                      \
    adr = _cpu_trs(cpu, adr, 0x2) ;      \
    pm_bus_st##typ(cpu->bus, adr, dat) ; \
  } while (0)

void pm_cpu_stb (struct pm_cpu_t * cpu, u_word_t adr, u_byte_t dat)
{
  _st_fun(b) ;
}

void pm_cpu_sth (struct pm_cpu_t * cpu, u_word_t adr, u_half_t dat)
{
  _st_fun(h) ;
}

void pm_cpu_stw (struct pm_cpu_t * cpu, u_word_t adr, u_word_t dat)
{
  _st_fun(w) ;
}

#define _inc_sp(siz)                                     \
  do {                                                   \
    int pl = _cpu_pl_get() ;                             \
                                                         \
    if (                                                 \
      0 == _cpu_gecsr(                                   \
        cpu, PM_CSR(pl, PM_CSR_SR), PM_SRM_SD, PM_SRS_SD \
      )                                                  \
    ) {                                                  \
      *sp -= siz ;                                       \
      adr = *sp ;                                        \
    } else {                                             \
      adr = *sp ;                                        \
      *sp += siz ;                                       \
    }                                                    \
  } while(0)

static void _cpu_psb (struct pm_cpu_t * cpu, u_word_t * sp, u_byte_t dat)
{
  u_word_t adr ;
  _inc_sp(sizeof(dat)) ;
  _st_fun(b) ;
}

static void _cpu_psh (struct pm_cpu_t * cpu, u_word_t * sp, u_half_t dat)
{
  u_word_t adr ;
  _inc_sp(sizeof(dat)) ;
  _st_fun(h) ;
}

static void _cpu_psw (struct pm_cpu_t * cpu, u_word_t * sp, u_word_t dat)
{
  u_word_t adr ;
  _inc_sp(sizeof(dat)) ;
  _st_fun(w) ;
}

#undef _inc_sp
#undef _st_fun

#define _ld_fun(typ, x)                        \
  do {                                         \
    cpu->ck1 += 2 ;                            \
    adr = _cpu_trs(cpu, adr, 0x1 | (x << 2)) ; \
    return pm_bus_ld##typ(cpu->bus, adr) ;     \
  } while (0)

u_byte_t pm_cpu_ldb (struct pm_cpu_t * cpu, u_word_t adr)
{
  _ld_fun(b, 0) ;
}

u_half_t pm_cpu_ldh (struct pm_cpu_t * cpu, u_word_t adr)
{
  _ld_fun(h, 0) ;
}

u_word_t pm_cpu_ldw (struct pm_cpu_t * cpu, u_word_t adr)
{
  _ld_fun(w, 0) ;
}

static u_word_t _cpu_fch (struct pm_cpu_t * cpu)
{
  cpu->pc1 = cpu->pc0 ;
  cpu->pc0 += sizeof(u_word_t) ;

  u_word_t adr = cpu->pc1 ;
  _ld_fun(w, 1) ;
}

#define _dec_sp(siz)                                     \
  do {                                                   \
    int pl = _cpu_pl_get() ;                             \
                                                         \
    if (                                                 \
      0 == _cpu_gecsr(                                   \
        cpu, PM_CSR(pl, PM_CSR_SR), PM_SRM_SD, PM_SRS_SD \
      )                                                  \
    ) {                                                  \
      adr = *sp ;                                        \
      *sp += siz ;                                       \
    } else {                                             \
      *sp -= siz ;                                       \
      adr = *sp ;                                        \
    }                                                    \
  } while(0)

static u_byte_t _cpu_plb (struct pm_cpu_t * cpu, u_word_t * sp)
{
  u_word_t adr ;
  _dec_sp(sizeof(u_byte_t)) ;
  _ld_fun(b, 0) ;
}

static u_half_t _cpu_plh (struct pm_cpu_t * cpu, u_word_t * sp)
{
  u_word_t adr ;
  _dec_sp(sizeof(u_half_t)) ;
  _ld_fun(h, 0) ;
}

static u_word_t _cpu_plw (struct pm_cpu_t * cpu, u_word_t * sp)
{
  u_word_t adr ;
  _dec_sp(sizeof(u_word_t)) ;
  _ld_fun(w, 0) ;
}

#undef _dec_sp
#undef _ld_fun

static void _cpu_stcsr (struct pm_cpu_t * cpu, int idx, u_word_t dat)
{
  cpu->csr[idx & 0x1F] = dat ;
}

static u_word_t _cpu_ldcsr (struct pm_cpu_t * cpu, int idx)
{
  return cpu->csr[idx & 0x1F] ;
}

static void _cpu_secsr (struct pm_cpu_t * cpu, int idx, u_word_t dat, u_word_t msk, int shf)
{
  u_word_t csr = _cpu_ldcsr(cpu, idx) ;
  csr &= ~(msk << shf) ;
  csr |= (dat & msk) << shf ;
  _cpu_stcsr(cpu, idx, csr) ;
}

static u_word_t _cpu_gecsr (struct pm_cpu_t * cpu, int idx, u_word_t msk, int shf)
{
  return (_cpu_ldcsr(cpu, idx) >> shf) & msk ;
}

static u_word_t _cpu_trs (struct pm_cpu_t * cpu, u_word_t adr, u_word_t xwr)
{
  int pl = _cpu_pl_get() ;

  if (
    0 == _cpu_gecsr(
      cpu, PM_CSR(pl, PM_CSR_SR), PM_SRM_PM, PM_SRS_PM
    )
  )
    return adr ;

  u_word_t ofs = (adr >>  0) & 0xFFF ;
  u_word_t pg1 = (adr >> 12) & 0x3FF ;
  u_word_t pg0 = (adr >> 22) & 0x3FF ;

  adr = _cpu_ldcsr(cpu, PM_CSR(pl, PM_CSR_PDT)) ;

  _cpu_secsr(cpu, PM_CSR(pl, PM_CSR_SR), PM_SRM_PM, PM_SRS_PM, 0) ;
  adr = pm_cpu_ldw(cpu, adr + pg0 * sizeof(adr)) ;
  _cpu_secsr(cpu, PM_CSR(pl, PM_CSR_SR), PM_SRM_PM, PM_SRS_PM, 1) ;

  if (0 == (adr & 0x1)) {
    pm_cpu_int(cpu, PM_INT_PF) ;
    return 0 ;
  }

  if (((adr >> 1) & 0x3) < pl) {
    pm_cpu_int(cpu, PM_INT_PP) ;
    return 0 ;
  }

  adr &= ~0xFFF ;

  _cpu_secsr(cpu, PM_CSR(pl, PM_CSR_SR), PM_SRM_PM, PM_SRS_PM, 0) ;
  adr = pm_cpu_ldw(cpu, adr + pg1 * sizeof(adr)) ;
  _cpu_secsr(cpu, PM_CSR(pl, PM_CSR_SR), PM_SRM_PM, PM_SRS_PM, 1) ;

  if (0 == (adr & 0x1)) {
    pm_cpu_int(cpu, PM_INT_PF) ;
    return 0 ;
  }

  if (((adr >> 1) & 0x3) < pl) {
    pm_cpu_int(cpu, PM_INT_PP) ;
    return 0 ;
  }

  if (xwr != (((adr >> 3) & 0x7) & xwr)) {
    pm_cpu_int(cpu, PM_INT_AP) ;
    return 0 ;
  }
  
  return (adr & ~0xFFF) | ofs ;
}

static int _cpu_xhgcc (struct pm_cpu_t * cpu, int src_pl, int dst_pl)
{
  u_word_t src_isp = _cpu_ldcsr(cpu, PM_CSR(src_pl, PM_CSR_ISP)) ;
  u_word_t dst_isp = _cpu_ldcsr(cpu, PM_CSR(dst_pl, PM_CSR_ISP)) ;

  if (0 != (src_isp & 0xFF) || 0 != (dst_isp & 0xFF)) {
    pm_cpu_int(cpu, PM_INT_AP) ;
    return -1 ;
  }

  u_word_t src_pc0 = cpu->pc0 ;
  u_word_t src_csr [ 8 ] ;
  u_word_t src_xpr [ 0x20 ] ;

  for (int idx = 0 ; idx < 0x08 ; ++idx) {
    src_csr[idx] = cpu->csr[PM_CSR(src_pl, idx)] ;
  }

  for (int idx = 0 ; idx < 0x20 ; ++idx) {
    src_xpr[idx] = cpu->xpr[idx] ;
  }

  /* load the destination context */

  /* 1. switch to the destination PDT */

  _cpu_pl_set(dst_pl) ;

  /* 2. load the context */

  cpu->pc0 = pm_cpu_ldw(cpu, dst_isp) ;

  for (int idx = 0 ; idx < 8 ; ++idx) {
    cpu->csr[PM_CSR(dst_pl, idx)] = pm_cpu_ldw(cpu, dst_isp + (1 + idx) * sizeof(u_word_t)) ;
  }

  for (int idx = 0 ; idx < 0x20 ; ++idx) {
    cpu->xpr[idx] = pm_cpu_ldw(cpu, dst_isp + (9 + idx) * sizeof(u_word_t)) ;
  }

  /* 3. link the destination context with the source */

  pm_cpu_stw(cpu, dst_isp, src_pl) ;

  /* store the source context */

  /* 1. switch to the source PDT */

  _cpu_pl_set(src_pl) ;

  /* 2. store the context */

  pm_cpu_stw(cpu, src_isp, src_pc0) ;

  for (int idx = 0 ; idx < 8 ; ++idx) {
    pm_cpu_stw(cpu, src_isp + (1 + idx) * sizeof(u_word_t), cpu->csr[PM_CSR(dst_pl, idx)]) ;
  }

  for (int idx = 0 ; idx < 0x20 ; ++idx) {
    pm_cpu_stw(cpu, src_isp + (9 + idx) * sizeof(u_word_t), cpu->xpr[idx]) ;
  }

  /* switch to the destination privilege level */

  _cpu_pl_set(dst_pl) ;
  
  return 0 ;
}

static int _cpu_int (struct pm_cpu_t * cpu, int fun, u_word_t num)
{
  int      src_pl = _cpu_pl_get() ;
  u_word_t src_sr = _cpu_ldcsr(cpu, PM_CSR(src_pl, PM_CSR_SR)) ;

  /* invoke an interrupt subroutine */

  if (0 == fun) {
    if (0 == (src_sr & PM_SRF_EI))
      return -1 ;

    u_word_t ivt = _cpu_ldcsr(cpu, PM_CSR(src_pl, PM_CSR_IVT)) ;
    u_word_t atr = pm_cpu_ldw(cpu, ivt + num * 2 * sizeof(u_word_t) + 0) ;
    u_word_t isr = pm_cpu_ldw(cpu, ivt + num * 2 * sizeof(u_word_t) + 1) ;

    if (0 == (atr & 0x1)) {
      pm_cpu_int(cpu, PM_INT_IF) ;
      return -1 ;
    }

    int typ    = (atr >> 1) & 0x7 ;
    int min_pl = (atr >> 4) & 0x3 ; 
    int dst_pl = (atr >> 6) & 0x3 ;

    if (min_pl < src_pl || 0x3 == dst_pl) {
      pm_cpu_int(cpu, PM_INT_PP) ;
      return -1 ;
    }

    /* exchange the context */
    
    switch ((typ >> 1) & 0x3) {
    case 0x0 : /* abort     */ {
      pm_cpu_int(cpu, PM_INT_MC) ;
    } return -1 ;

    case 0x1 : /* exception */ {
    } break ;

    case 0x2 : /* fault     */ {
      cpu->pc0 = cpu->pc1 ;
    } break ;

    case 0x3 : /* trap      */ {
    } break ;
    }

    if (0 != _cpu_xhgcc(cpu, src_pl, dst_pl))
      return -1 ;

    cpu->pc0 = isr ;

    /* disable interrupts and set the serving flag */

    int      pl = _cpu_pl_get() ;
    u_word_t sr = _cpu_ldcsr(cpu, PM_CSR(pl, PM_CSR_SR)) ;
    sr &= PM_SRF_EI | PM_SRF_WI ;
    sr |= PM_SRF_SI ;
    _cpu_stcsr(cpu, PM_CSR(pl, PM_CSR_SR), sr) ;

    return 0 ;
  }
  
  /* return from an interrupt subroutine */

  if (1 == fun) {
    if (0x3 == src_pl) {
      pm_cpu_int(cpu, PM_INT_PP) ;
      return -1 ;
    }

    if (0 == (src_sr & PM_SRF_SI)) {
      pm_cpu_int(cpu, PM_INT_RF) ;
      return -1 ;
    }

    /* exchange the context */

    u_word_t src_isp = _cpu_ldcsr(cpu, PM_CSR(src_pl, PM_CSR_ISP)) ;
    int      dst_pl  = pm_cpu_ldw(cpu, src_isp) ;

    if (0 != _cpu_xhgcc(cpu, src_pl, dst_pl))
      return -1 ;

    return 0 ;
  }
  
  return -1 ;
}

/* Random Access Memory */

int pm_ram_ctor (struct pm_ram_t * ram, struct pm_bus_t * bus, u_word_t len)
{
  if (0 != (len & ((4 << 10) - 1)) || 0x80000000 < len)
    return -1 ;

  u_byte_t * buf = (u_byte_t *)calloc(len, sizeof(u_byte_t)) ;

  if (NULL == buf)
    return -1 ;

  ram->bus = bus ;
  ram->len = len ;
  ram->buf = buf ;

  return 0 ;
}

void pm_ram_dtor (struct pm_ram_t * ram)
{
  if (NULL == ram->buf)
    return ;

  free(ram->buf) ;
}

void pm_ram_int (struct pm_ram_t * ram)
{
  pm_bus_int(ram->bus, PM_INT_MR) ;
}

void pm_ram_rst (struct pm_ram_t * ram)
{
  if (NULL == ram->buf)
    return ;

  memset(ram->buf, 0, ram->len * sizeof(u_byte_t)) ; 
}

void pm_ram_clk (struct pm_ram_t * ram)
{
}

void pm_ram_stb (struct pm_ram_t * ram, u_word_t adr, u_byte_t dat)
{
  ram->buf[adr] = dat ;
}

void pm_ram_sth (struct pm_ram_t * ram, u_word_t adr, u_half_t dat)
{
#if __PM_ENDIAN_LE == __PM_ENDIAN
  ram->buf[adr + 1] = (dat >> 8) & 0xFF ;
  ram->buf[adr + 0] = (dat >> 0) & 0xFF ;
#else
  ram->buf[adr + 0] = (dat >> 8) & 0xFF ;
  ram->buf[adr + 1] = (dat >> 0) & 0xFF ;
#endif
}

void pm_ram_stw (struct pm_ram_t * ram, u_word_t adr, u_word_t dat)
{
#if __PM_ENDIAN_LE == __PM_ENDIAN
  ram->buf[adr + 3] = (dat >> 24) & 0xFF ;
  ram->buf[adr + 2] = (dat >> 16) & 0xFF ;
  ram->buf[adr + 1] = (dat >>  8) & 0xFF ;
  ram->buf[adr + 0] = (dat >>  0) & 0xFF ;
#else
  ram->buf[adr + 0] = (dat >> 24) & 0xFF ;
  ram->buf[adr + 1] = (dat >> 16) & 0xFF ;
  ram->buf[adr + 2] = (dat >>  8) & 0xFF ;
  ram->buf[adr + 3] = (dat >>  0) & 0xFF ;
#endif
}

u_byte_t pm_ram_ldb (struct pm_ram_t * ram, u_word_t adr)
{
  return ram->buf[adr] ;
}

u_half_t pm_ram_ldh (struct pm_ram_t * ram, u_word_t adr)
{
  u_half_t dat = 0 ;

#if __PM_ENDIAN_LE == __PM_ENDIAN
  dat |= ram->buf[adr + 1] ; dat <<= 8 ;
  dat |= ram->buf[adr + 0] ;
#else
  dat |= ram->buf[adr + 0] ; dat <<= 8 ;
  dat |= ram->buf[adr + 1] ;
#endif

  return dat ;
}

u_word_t pm_ram_ldw (struct pm_ram_t * ram, u_word_t adr)
{
  u_word_t dat = 0 ;

#if __PM_ENDIAN_LE == __PM_ENDIAN
  dat |= ram->buf[adr + 3] ; dat <<= 8 ;
  dat |= ram->buf[adr + 2] ; dat <<= 8 ;
  dat |= ram->buf[adr + 1] ; dat <<= 8 ;
  dat |= ram->buf[adr + 0] ;
#else
  dat |= ram->buf[adr + 0] ; dat <<= 8 ;
  dat |= ram->buf[adr + 1] ; dat <<= 8 ;
  dat |= ram->buf[adr + 2] ; dat <<= 8 ;
  dat |= ram->buf[adr + 3] ;
#endif

  return dat ;
}

/* Input/Output Memory */

int pm_iom_ctor (struct pm_iom_t * iom, struct pm_bus_t * bus, u_word_t adr, u_word_t len)
{
  iom->bus = bus ;
  iom->adr = adr ;
  iom->len = len ;
  
  for (int idx = 0 ; idx < 0x10 ; ++idx) {
    iom->dev[idx] = NULL ;
  }

  return 0 ;
}

void pm_iom_dtor (struct pm_iom_t * iom)
{
  for (int idx = 0 ; idx < 0x10 ; ++idx) {
    (void) pm_iom_umn(iom, iom->dev[idx]) ;
  }
}

void pm_iom_int (struct pm_iom_t * iom, struct pm_dev_t * dev)
{
  if (NULL == dev)
    return ;

  pm_bus_int(iom->bus, PM_INT_HI(dev->id)) ;
}

void pm_iom_rst (struct pm_iom_t * iom, int id)
{
  if (-1 == id) {
    for (int idx = 0 ; idx < 0x10 ; ++idx) {
      struct pm_dev_t * dev = iom->dev[idx] ;

      if (NULL == dev || NULL == dev->rst)
        continue ;

      dev->rst(dev) ;
    }
  } else {
    for (int idx = 0 ; idx < 0x10 ; ++idx) {
      struct pm_dev_t * dev = iom->dev[idx] ;
      
      if (dev->id != id)
      	continue ;

      if (NULL == dev || NULL == dev->rst)
        continue ;

      dev->rst(dev) ;
    }
  }
}

void pm_iom_clk (struct pm_iom_t * iom)
{
  for (int idx = 0 ; idx < 0x10 ; ++idx) {
    struct pm_dev_t * dev = iom->dev[idx] ;

    if (NULL == dev || NULL == dev->clk)
      continue ;

    dev->clk(dev) ;
  }
}

#define _st_fun(typ)                           \
  do {                                         \
    u_word_t ofs ;                             \
                                               \
    for (int idx = 0 ; idx < 0x10 ; ++idx) {   \
      struct pm_dev_t * dev = iom->dev[idx] ;  \
                                               \
      if (NULL == dev || NULL == dev->st##typ) \
        continue ;                             \
                                               \
      ofs = adr - dev->adr ;                   \
      if (ofs < dev->len) {                    \
        dev->st##typ(dev, ofs, dat) ;          \
        return ;                               \
      }                                        \
    }                                          \
  } while (0)

void pm_iom_stb (struct pm_iom_t * iom, u_word_t adr, u_byte_t dat)
{
  _st_fun(b) ;
}

void pm_iom_sth (struct pm_iom_t * iom, u_word_t adr, u_half_t dat)
{
  _st_fun(h) ;
}

void pm_iom_stw (struct pm_iom_t * iom, u_word_t adr, u_word_t dat)
{
  _st_fun(w) ;
}

#undef _st_fun

#define _ld_fun(typ, typid)                    \
  do {                                         \
    u_##typid##_t dat = 0 ;                    \
    u_word_t ofs ;                             \
                                               \
    for (int idx = 0 ; idx < 0x10 ; ++idx) {   \
      struct pm_dev_t * dev = iom->dev[idx] ;  \
                                               \
      if (NULL == dev || NULL == dev->ld##typ) \
        continue ;                             \
                                               \
      ofs = adr - dev->adr ;                   \
      /* DEBUG_1("\naddress: 0x%08X, offset: 0x%08X, length: 0x%08X, id: %u\n", adr, ofs, dev->len, dev->id) ; */ \
      if (ofs < dev->len) {                    \
        dat = dev->ld##typ(dev, ofs) ;         \
        return dat ;                           \
      }                                        \
    }                                          \
                                               \
    return dat ;                               \
  } while (0)

u_byte_t pm_iom_ldb (struct pm_iom_t * iom, u_word_t adr)
{
  _ld_fun(b, byte) ;
}

u_half_t pm_iom_ldh (struct pm_iom_t * iom, u_word_t adr)
{
  _ld_fun(h, half) ;
}

u_word_t pm_iom_ldw (struct pm_iom_t * iom, u_word_t adr)
{
  _ld_fun(w, word) ;
}

#undef _ld_fun

#define _overlapping(a, b, c, d) (0 == (b < c || a > d))

int pm_iom_mnt (struct pm_iom_t * iom, struct pm_dev_t * dev)
{
  if (NULL == dev || 0 == dev->len)
    return -1 ;

  int idx ;

  for (idx = 0 ; idx < 0x10 ; ++idx) {
    if (NULL == iom->dev[idx])
      break ;
  }

  if (0x10 == idx)
    return -1 ;

  u_word_t start = dev->adr ;
  u_word_t end   = start + dev->len - 1 ;

  for (int jdx = 0 ; jdx < 0x10 ; ++jdx) {
    if (NULL == iom->dev[jdx])
      continue ;

    u_word_t dev_start = iom->dev[jdx]->adr ;
    u_word_t dev_end   = dev_start + iom->dev[jdx]->len - 1 ;

    if (_overlapping(dev_start, dev_end, start, end))
      return -1 ;
  }

  dev->bus      = iom ;
  dev->id       = idx ;
  iom->dev[idx] = dev ;

  return 0 ;
}

#undef _overlapping

int pm_iom_umn (struct pm_iom_t * iom, struct pm_dev_t * dev)
{
  if (NULL == dev)
    return -1 ;

  for (int idx = 0 ; idx < 0x10 ; ++idx) {
    if (dev == iom->dev[idx]) {
      iom->dev[idx] = NULL ;
      return 0 ;
    }
  }

  return -1 ;
}

/* omniBUS */

int pm_bus_ctor (struct pm_bus_t * bus, u_word_t len)
{
  int res ;

  res = pm_cpu_ctor(&bus->cpu, bus) ;
  if (0 != res)
    return res ;

  res = pm_ram_ctor(&bus->ram, bus, len) ;
  if (0 != res)
    return res ;

  res = pm_iom_ctor(&bus->iom, bus, 0x00000000, 0x80000000) ;
  if (0 != res)
    return res ;

  return 0 ;
}

void pm_bus_dtor (struct pm_bus_t * bus)
{
  pm_cpu_dtor(&bus->cpu) ;
  pm_ram_dtor(&bus->ram) ;
  pm_iom_dtor(&bus->iom) ;
}

void pm_bus_int (struct pm_bus_t * bus, u_word_t irq)
{
  pm_cpu_int(&bus->cpu, irq) ;
}

void pm_bus_rst (struct pm_bus_t * bus, int id)
{
  bus->hlt = 0 ;
  
  if (-1 == id) {
    pm_cpu_rst(&bus->cpu) ;
    pm_ram_rst(&bus->ram) ; 
  }
  
  pm_iom_rst(&bus->iom, id) ;
}

void pm_bus_clk (struct pm_bus_t * bus)
{
  pm_cpu_clk(&bus->cpu) ;
  pm_ram_clk(&bus->ram) ;
  pm_iom_clk(&bus->iom) ;
}

#define _st_fun(typ)                        \
  do {                                      \
    u_word_t ofs ;                          \
                                            \
    ofs = adr - 0x80000000 ;                \
    if (                                    \
      ofs               <  bus->ram.len &&  \
      ofs + sizeof(dat) <= bus->ram.len     \
    ) {                                     \
      pm_ram_st##typ(&bus->ram, ofs, dat) ; \
      return ;                              \
    }                                       \
                                            \
    ofs = adr - bus->iom.adr ;              \
    if (ofs < bus->iom.len) {               \
      pm_iom_st##typ(&bus->iom, ofs, dat) ; \
      return ;                              \
    }                                       \
                                            \
    pm_bus_int(bus, PM_INT_BF) ;            \
  } while (0)

void pm_bus_stb (struct pm_bus_t * bus, u_word_t adr, u_byte_t dat)
{
  _st_fun(b) ;
}

void pm_bus_sth (struct pm_bus_t * bus, u_word_t adr, u_half_t dat)
{
  _st_fun(h) ;
}

void pm_bus_stw (struct pm_bus_t * bus, u_word_t adr, u_word_t dat)
{
  _st_fun(w) ;
}

#undef _st_fun

#define _ld_fun(typ, typid)                  \
  do {                                       \
    u_##typid##_t dat = 0 ;                  \
    u_word_t ofs ;                           \
                                             \
    ofs = adr - 0x80000000 ;                 \
    if (                                     \
      ofs               <  bus->ram.len &&   \
      ofs + sizeof(dat) <= bus->ram.len      \
    ) {                                      \
      dat = pm_ram_ld##typ(&bus->ram, ofs) ; \
      return dat ;                           \
    }                                        \
                                             \
    ofs = adr - bus->iom.adr ;               \
    if (ofs < bus->iom.len) {                \
      dat = pm_iom_ld##typ(&bus->iom, ofs) ; \
      return dat ;                           \
    }                                        \
                                             \
    pm_bus_int(bus, PM_INT_BF) ;             \
    return dat ;                             \
  } while (0)

u_byte_t pm_bus_ldb (struct pm_bus_t * bus, u_word_t adr)
{
  _ld_fun(b, byte) ;
}

u_half_t pm_bus_ldh (struct pm_bus_t * bus, u_word_t adr)
{
  _ld_fun(h, half) ;
}

u_word_t pm_bus_ldw (struct pm_bus_t * bus, u_word_t adr)
{
  _ld_fun(w, word) ;
}

#undef _ld_fun

int pm_bus_mnt (struct pm_bus_t * bus, struct pm_dev_t * dev)
{
  return pm_iom_mnt(&bus->iom, dev) ;
}

int pm_bus_umn (struct pm_bus_t * bus, struct pm_dev_t * dev)
{
  return pm_iom_umn(&bus->iom, dev) ;
}
