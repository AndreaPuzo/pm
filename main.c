#include "pm.h"
#include "dev/tmr.h"
#include "dev/scr.h"
#include "dev/kbr.h"
#include "dev/dsk.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <SDL2/SDL.h>

struct dev_fnt8x16_t {
  struct pm_dev_t dev ;
  u_byte_t        buf [ 0x1000 ] ;
} ;

void     dev_fnt8x16_rst (struct dev_fnt8x16_t * fnt) ;
void     dev_fnt8x16_stb (struct dev_fnt8x16_t * fnt, u_word_t adr, u_byte_t dat) ;
u_byte_t dev_fnt8x16_ldb (struct dev_fnt8x16_t * fnt, u_word_t adr) ;

struct sdl_trm_t {
  SDL_Window *   window    ;
  SDL_Renderer * renderer  ;
  int            status    ;
  u_byte_t       rows      ;
  u_byte_t       cols      ;

  /* devices */

  struct pm_dev_scr_t  scr ;
  struct pm_dev_kbr_t  kbr ;
  struct dev_fnt8x16_t fnt ;
} ;

static int _sdl_trm_put_col_chr (struct sdl_trm_t * trm, int fg, int bg, char chr) ;
static int _sdl_trm_put_col_str (struct sdl_trm_t * trm, int fg, int bg, const char * str) ;
static int _sdl_trm_put_chr (struct sdl_trm_t * trm, char chr) ;
static int _sdl_trm_put_str (struct sdl_trm_t * trm, const char * str) ;
static void _sdl_trm_render_chr (struct sdl_trm_t * trm, u_byte_t ofs_x, u_byte_t ofs_y, u_half_t chr) ;

int  sdl_trm_ctor (struct sdl_trm_t * trm) ;
void sdl_trm_dtor (struct sdl_trm_t * trm) ;
void sdl_trm_clk  (struct sdl_trm_t * trm, struct pm_bus_t * bus) ;

static void _dump_dec (struct sdl_trm_t * trm, int fg, int bg, int val)
{
  int tmp = val ;
  int dgs = 0 ;

  do {
    tmp /= 10 ;
    ++dgs ;
  } while (0 != tmp) ;

  const int siz = dgs ;
  char      str [siz + 1] ;

  str[dgs] = '\0' ;

  do {
    str[--dgs] = '0' + (val % 10) ;
    val /= 10 ;
  } while (0 != val) ;

  _sdl_trm_put_col_str(trm, fg, bg, str + dgs) ;
}

static void _dump_hex (struct sdl_trm_t * trm, int fg, int bg, u_word_t val)
{
  static char hex [] = "0123456789ABCDEF" ;
  
  char str [] = {
    '0', 'X',
    hex[(val >> 28) & 0xF] ,
    hex[(val >> 24) & 0xF] ,
    hex[(val >> 20) & 0xF] ,
    hex[(val >> 16) & 0xF] ,
    hex[(val >> 12) & 0xF] ,
    hex[(val >>  8) & 0xF] ,
    hex[(val >>  4) & 0xF] ,
    hex[(val >>  0) & 0xF] ,
    '\0'
  } ;

  _sdl_trm_put_col_str(trm, fg, bg, str) ;
}

static int _boot (struct pm_bus_t * bus, struct pm_bus_rst_t * rst)
{
  struct sdl_trm_t * trm = rst->ptr ;

  _sdl_trm_put_str(trm, "\r[") ;
  _sdl_trm_put_col_str(trm, 0xB, 0x0, " ------ ") ;
  _sdl_trm_put_str(trm, "] BOOTING ") ;
  
  if (rst->adr_max <= rst->adr_cur) {
    _sdl_trm_put_col_str(trm, 0x9, 0x0, "\rNO BOOTLOADER HAS BEEN FOUND") ;
    bus->hlt = 1 ;
    return -1 ;
  }

  u_word_t adr = rst->adr_cur ;

  _sdl_trm_put_str(trm, "AT ") ;
  _dump_hex(trm, 0xF, 0x0, adr) ;
  _sdl_trm_put_str(trm, " ") ;

  // fprintf(stderr, "[ BOOTING AT 0x%08X ", adr) ;
  u_word_t mag = pm_bus_ldw(bus, adr) ;
  // fprintf(stderr, "0x%08X ]\n", mag) ;

  _dump_hex(trm, 0xB, 0x0, mag) ;

  if (rst->mag == mag) {
    _sdl_trm_put_str(trm, "\r[") ;
    _sdl_trm_put_col_str(trm, 0xA, 0x0, " PASSED ") ;
    _sdl_trm_put_str(trm, "]\n\r") ;

    for (int idx = 0 ; idx < rst->dst_siz ; ++idx) {
      u_byte_t dat = pm_bus_ldb(bus, adr + idx) ;
      pm_bus_stb(bus, rst->dst_adr + idx, dat) ;
    }

    pm_bus_stw(bus, rst->dst_adr, 0x00000000) ;

    adr = rst->adr_min ;
  } else {
    _sdl_trm_put_str(trm, "\r[") ;
    _sdl_trm_put_col_str(trm, 0x9, 0x0, " FAILED ") ;
    _sdl_trm_put_str(trm, "]\n\r") ;

    adr += rst->dst_siz ;
  }

  rst->adr_cur = adr ;

  return rst->mag != mag ;
}

static void _dump_cpu (struct pm_cpu_t * cpu, struct sdl_trm_t * trm)
{
  static struct pm_cpu_t lst ;
  static int init = 0 ;

  if (init == 0) {
    lst = *cpu ;
    init = 1 ;
  }

#define DEF 0x8
#define CHG 0xB

  pm_dev_scr_stb(&trm->scr, 0xF4, 0x00) ;

  _sdl_trm_put_col_str(trm, 0xF, 0x0, " PC0   : ") ;
  _dump_hex(trm, cpu->pc0 == lst.pc0 ? DEF : CHG, 0x0, cpu->pc0) ;
  _sdl_trm_put_col_str(trm, 0xF, 0x0, "  PC1   : ") ;
  _dump_hex(trm, cpu->pc1 == lst.pc1 ? DEF : CHG, 0x0, cpu->pc1) ;
  _sdl_trm_put_col_str(trm, 0xF, 0x0, "  INS   : ") ;
  _dump_hex(trm, cpu->ins == lst.ins ? DEF : CHG, 0x0, cpu->ins) ;
  _sdl_trm_put_str(trm, "\n\r") ;
  _sdl_trm_put_col_str(trm, 0xF, 0x0, " CK0   : ") ;
  _dump_hex(trm, cpu->ck0 == lst.ck0 ? DEF : CHG, 0x0, cpu->ck0) ;
  _sdl_trm_put_col_str(trm, 0xF, 0x0, "  CK1   : ") ;
  _dump_hex(trm, cpu->ck1 == lst.ck1 ? DEF : CHG, 0x0, cpu->ck1) ;
  _sdl_trm_put_str(trm, "\n\r") ;

  for (int idx = 0 ; idx < 0x10 ; ++idx) {
    _sdl_trm_put_col_str(trm, 0xF, 0x0, " XPR") ;
    _dump_dec(trm, 0xF, 0x0, idx) ;
    if (idx <= 0x9) {
      _sdl_trm_put_col_str(trm, 0xF, 0x0, "  : ") ;
    } else {
      _sdl_trm_put_col_str(trm, 0xF, 0x0, " : ") ;
    }
    _dump_hex(trm, cpu->xpr[idx] == lst.xpr[idx] ? DEF : CHG, 0x0, cpu->xpr[idx]) ;

    _sdl_trm_put_col_str(trm, 0xF, 0x0, "  XPR") ;
    _dump_dec(trm, 0xF, 0x0, 0x10 + idx) ;
    _sdl_trm_put_col_str(trm, 0xF, 0x0, " : ") ;
    _dump_hex(trm, cpu->xpr[0x10 + idx] == lst.xpr[0x10 + idx] ? DEF : CHG, 0x0, cpu->xpr[0x10 + idx]) ;

    _sdl_trm_put_col_str(trm, 0xF, 0x0, "  CSR") ;
    _dump_dec(trm, 0xF, 0x0, idx) ;
    if (idx <= 0x9) {
      _sdl_trm_put_col_str(trm, 0xF, 0x0, "  : ") ;
    } else {
      _sdl_trm_put_col_str(trm, 0xF, 0x0, " : ") ;
    }
    _dump_hex(trm, cpu->csr[idx] == lst.csr[idx] ? DEF : CHG, 0x0, cpu->csr[idx]) ;

    _sdl_trm_put_col_str(trm, 0xF, 0x0, "  CSR") ;
    _dump_dec(trm, 0xF, 0x0, 0x10 + idx) ;
    _sdl_trm_put_col_str(trm, 0xF, 0x0, " : ") ;
    _dump_hex(trm, cpu->csr[0x10 + idx] == lst.csr[0x10 + idx] ? DEF : CHG, 0x0, cpu->csr[0x10 + idx]) ;

    _sdl_trm_put_str(trm, "\n\r") ;
  }

  lst = *cpu ;
}

int main (int argc, char ** argv)
{
  char *   cdroms [ 2 ] = { NULL, NULL } ;
  int      cdromc       = 0         ;
  char *   hdds   [ 4 ] = { NULL, NULL, NULL, NULL } ;
  int      hddc         = 0         ;
  u_word_t cdromo [ 2 ] = { __PM_ENDIAN } ;
  u_word_t hddo   [ 4 ] = { __PM_ENDIAN } ;
  u_word_t mem_len      = 0x2000000 ;
  int      arg_ofs      = -1        ;

  for (int idx = 1 ; idx < argc ; ++idx) {
    char * args = argv[idx] ;

    if (0 == strcmp(args, "--help") || 0 == strcmp(args, "-h")) {
      fprintf(
        stdout ,
        "usage: %s [options]\n"
        "usage: %s --help|-h\n"
        "options:\n"
        "  -h, --help          : print this message.\n"
        "  -v, --ver           : print the version.\n"
        "  -m, --mem NUM       : set NUM as the memory length.\n"
        "  -a, --arg ...       : ... are passed as arguments to\n"
        "                        the machine through a special\n"
        "                        device.\n"
        "  --cdrom=PATH:ENDIAN : attach a read-only memory.\n"
        "  --hdd=PATH:ENDIAN   : attach a read-write memory.\n" ,
        argv[0] ,
        argv[0]
      ) ;
      
      return EXIT_SUCCESS ;
    }

    if (0 == strcmp(args, "--ver") || 0 == strcmp(args, "-v")) {
      fprintf(
        stdout ,
        "PocketMachine (PM) %u.%u.%u\n"
        "Built on %s at %s\n"
        "Copyright (c) 2024, Andrea Puzo.\n" ,
        __PM_VERSION_MAJOR ,
        __PM_VERSION_MINOR ,
        __PM_VERSION_PATCH ,
        __DATE__           ,
        __TIME__
      ) ;

      return EXIT_SUCCESS ;
    }

    if (0 == strcmp(args, "--arg") || 0 == strcmp(args, "-a")) {
      arg_ofs = idx + 1 ;
      break ;
    }

    if (0 == strcmp(args, "--mem") || 0 == strcmp(args, "-m")) {
      if (argc - 1 == idx) {
        fprintf(stderr, "error: option `%s` requires 1 argument\n", args) ;
        continue ;
      }

      args = argv[++idx] ;

_read_mem_len :
      char * str    = args ;
      char * endptr = NULL ;

      mem_len = (u_word_t)strtoul(str, &endptr, 0) ;

      if (NULL != endptr) {
        if (0 == strcmp(endptr, "K")) {
          mem_len <<= 10 ;
        } else if (0 == strcmp(endptr, "M")) {
          mem_len <<= 20 ;
        } else if (0 == strcmp(endptr, "G")) {
          mem_len <<= 30 ;
        }
      }

      if (0x80000000 < mem_len) {
        fprintf(stderr, "error: too much memory for working memory\n") ;
        return EXIT_FAILURE ;
      }
    } else if (0 == strncmp(args, "--mem=", 6)) {
      args += 6 ;
      goto _read_mem_len ;
    } else if (0 == strncmp(args, "--cdrom=", 8) || 0 == strncmp(args, "--hdd=", 6)) {
      int is_cdrom = 0 ;

      if ('c' == args[2]) {
        args += 8 ;
        is_cdrom = 1 ;
      } else {
        args += 6 ;
      }

      char quote = '\0' ;

      if ('\'' == args[0] || '\"' == args[0]) {
        quote = *args++ ;
      }

      if (0 != is_cdrom) {
        if (cdromc == sizeof(cdroms) / sizeof(cdroms[0])) {
          fprintf(stderr, "error: too many cdroms has been attached\n") ;
          return EXIT_FAILURE ;
        }

        cdroms[cdromc] = args ;
      } else {
        if (hddc == sizeof(hdds) / sizeof(hdds[0])) {
          fprintf(stderr, "error: too many hard-disks has been attached\n") ;
          return EXIT_FAILURE ;
        }

        hdds[hddc] = args ;
      }

      int order = __PM_ENDIAN ;

      if (quote != '\0') {
        while (quote != *args && '\0' != *args) {
          ++args ;
        }

        if (':' == *args) {
          if (0 == strcmp(args + 1, "be")) {
            order = __PM_ENDIAN_BE ;
          } else if (0 == strcmp(args + 1, "le")) {
            order = __PM_ENDIAN_LE ;
          }
        }
      } else {
        while (':' != *args && '\0' != *args) {
          ++args ;
        }

        if (':' == *args) {
          if (0 == strcmp(args + 1, "be")) {
            order = __PM_ENDIAN_BE ;
          } else if (0 == strcmp(args + 1, "le")) {
            order = __PM_ENDIAN_LE ;
          }
        }
      }

      *args = '\0' ;

      if (0 != is_cdrom) {
        cdromo[cdromc++] = order ;
      } else {
        hddo[hddc++] = order ;
      }
    }
  }

  if (0 < arg_ofs) {
    FILE * fp = fopen("args", "w") ;

    if (NULL == fp) {
      fprintf(stderr, "error: cannot create the disk to store the arguments\n") ;
      return EXIT_FAILURE ;
    }

    for (int idx = 0 ; idx < 0x200 ; ++idx) {
      fputc(0x00, fp) ;
    }

    fseek(fp, 0, SEEK_SET) ;

    for (int idx = arg_ofs ; idx < argc ; ++idx) {
      char * arg = argv[idx] ;
      size_t len = strlen(arg) + 1 ;

      if (len != fwrite(arg, sizeof(char), len, fp)) {
        fclose(fp) ;
        fprintf(stderr, "error: cannot write the %u-th argument\n", idx - arg_ofs + 1) ;
        return EXIT_FAILURE ;
      }
    }

    fclose(fp) ;
  }

  static struct pm_bus_t     bus ;
  static struct pm_dev_tmr_t tmr [ 4 ] ;
  static struct pm_dev_dsk_t dsk [ 6 + 1 ] ;
  static struct sdl_trm_t    trm ;

  for (int idx = 0 ; idx < 4 ; ++idx) {
    tmr[idx].dev.adr = 0x40000000 + (idx * 0x100) ;
    tmr[idx].dev.len = 0x100 ;
    tmr[idx].dev.rst = (pm_dev_rst_t)pm_dev_tmr_rst ;
    tmr[idx].dev.clk = (pm_dev_clk_t)pm_dev_tmr_clk ;
    tmr[idx].dev.stb = (pm_dev_stb_t)NULL ;
    tmr[idx].dev.sth = (pm_dev_sth_t)NULL ;
    tmr[idx].dev.stw = (pm_dev_stw_t)pm_dev_tmr_stw ;
    tmr[idx].dev.ldb = (pm_dev_ldb_t)NULL ;
    tmr[idx].dev.ldh = (pm_dev_ldh_t)NULL ;
    tmr[idx].dev.ldw = (pm_dev_ldw_t)pm_dev_tmr_ldw ;
  }

  for (int idx = 0 ; idx < 6 + 1 ; ++idx) {
    dsk[idx].dev.adr = 0x20000000 + (idx * 0x400) ;
    dsk[idx].dev.len = 0x400 ;
    dsk[idx].dev.rst = (pm_dev_rst_t)pm_dev_dsk_rst ;
    dsk[idx].dev.clk = (pm_dev_clk_t)pm_dev_dsk_clk ;
    dsk[idx].dev.stb = (pm_dev_stb_t)pm_dev_dsk_stb ;
    dsk[idx].dev.sth = (pm_dev_sth_t)pm_dev_dsk_sth ;
    dsk[idx].dev.stw = (pm_dev_stw_t)pm_dev_dsk_stw ;
    dsk[idx].dev.ldb = (pm_dev_ldb_t)pm_dev_dsk_ldb ;
    dsk[idx].dev.ldh = (pm_dev_ldh_t)pm_dev_dsk_ldh ;
    dsk[idx].dev.ldw = (pm_dev_ldw_t)pm_dev_dsk_ldw ;

    dsk[idx].fp = NULL ;

    if (1 <= idx && idx <= 2) {
      dsk[idx].fn      = cdroms[idx - 1] ;
      dsk[idx].dev.stb = NULL ;
      dsk[idx].dev.sth = NULL ;
      dsk[idx].bo      = cdromo[idx - 1] ;
    } else if (3 <= idx && idx <= 6) {
      dsk[idx].fn = hdds[idx - 3] ;
      dsk[idx].bo = hddo[idx - 3] ;
    } else {
      dsk[idx].fn = "args" ;
      dsk[idx].bo = __PM_ENDIAN ;
    }
  }

  if (0 != pm_bus_ctor(&bus, mem_len)) {
    fprintf(stderr, "Ops... Something has gone wrong during bus configuration.\n") ;
    return EXIT_FAILURE ;
  }

  if (0 != sdl_trm_ctor(&trm)) {
    pm_bus_dtor(&bus) ;
    fprintf(stderr, "Ops... Something has gone wrong during terminal configuration.\n") ;
    return EXIT_FAILURE ;
  }

  for (int idx = 0 ; idx < 4 ; ++idx) {
    if (0 != pm_bus_mnt(&bus, (struct pm_dev_t *)(tmr + idx))) {
      fprintf(stderr, "error: cannot mount timer %u\n", idx) ;
      goto failure ;
    }
  }

  for (int idx = 0 ; idx < 6 + 1 ; ++idx) {
    if (0 != pm_bus_mnt(&bus, (struct pm_dev_t *)(dsk + idx))) {
      fprintf(stderr, "error: cannot mount disk %u\n", idx) ;
      goto failure ;
    }
  }

  if (0 != pm_bus_mnt(&bus, (struct pm_dev_t *)&trm.fnt)) {
    fprintf(stderr, "error: cannot mount the terminal font\n") ;
    goto failure ;
  }

  if (0 != pm_bus_mnt(&bus, (struct pm_dev_t *)&trm.kbr)) {
    fprintf(stderr, "error: cannot mount the terminal keyboard\n") ;
    goto failure ;
  }

  if (0 != pm_bus_mnt(&bus, (struct pm_dev_t *)&trm.scr)) {
    fprintf(stderr, "error: cannot mount the terminal screen\n") ;
    goto failure ;
  }

  /*
  for (int err = 1 ; err > 0 ;) {
    trm.rows = trm.scr.len_y ;
    trm.cols = trm.scr.len_x ;

    err = _boot(&bus, &trm, 0x4570FEED) ;

    sdl_trm_clk(&trm, &bus) ;

    if (0 == trm.status)
      break ;
  }
  */

  bus.rst.adr_min = 0x20000000           ;
  bus.rst.adr_max = 0x40000000           ;
  bus.rst.adr_eps = 0x200                ;
  bus.rst.dst_adr = 0x80000000           ;
  bus.rst.dst_siz = bus.rst.adr_eps      ;
  bus.rst.mag     = 0x4570FEED           ;
  bus.rst.ptr     = (void *)&trm         ;
  bus.boot        = (pm_bus_boot_t)_boot ;

  pm_bus_rst(&bus, -1) ;

  int halt_i = 0 ;
  for (int i = 1 ; i > 0 ; ++i) {
    trm.rows = trm.scr.len_y ;
    trm.cols = trm.scr.len_x ;

    if (trm.status & (1 << 2)) {
      pm_bus_clk(&bus) ;

      if (0 != bus.rst.rdy) {
        _dump_cpu(&bus.cpu, &trm) ;
      }

      if (trm.status & (1 << 1)) {
        trm.status ^= (1 << 2) ;
      }
    }

    sdl_trm_clk(&trm, &bus) ;

    if (0 == trm.status)
      break ;

    if (0 != bus.hlt) {
      trm.status = 1 ;

      if (halt_i == 0) {
        halt_i = i ;
      }

      pm_dev_scr_stb(&trm.scr, 0xF4, 0x00) ;
      _sdl_trm_put_str(&trm, "[") ;
      _sdl_trm_put_col_str(&trm, 0x6, 0x0, " HALTED ") ;
      _sdl_trm_put_str(&trm, "] ") ;
      _dump_dec(&trm, 0x6, 0x0, i - halt_i) ;
    }
  }

success:
  pm_bus_dtor(&bus) ;
  sdl_trm_dtor(&trm) ;
  return EXIT_SUCCESS ;

failure:
  pm_bus_dtor(&bus) ;
  sdl_trm_dtor(&trm) ;
  return EXIT_FAILURE ;
}

/* Font 8x16 */

void dev_fnt8x16_rst (struct dev_fnt8x16_t * fnt)
{
  static u_byte_t fnt8x16 [] = {
#include "fnt8x16.def"
  } ;

  memcpy(fnt->buf, fnt8x16, sizeof(fnt8x16)) ;
}

void dev_fnt8x16_stb (struct dev_fnt8x16_t * fnt, u_word_t adr, u_byte_t dat)
{
  fnt->buf[adr & 0xFFF] = dat ;
}

u_byte_t dev_fnt8x16_ldb (struct dev_fnt8x16_t * fnt, u_word_t adr)
{
  return fnt->buf[adr & 0xFFF] ;
}

/* Terminal */

int sdl_trm_ctor (struct sdl_trm_t * trm)
{
  if (0 != SDL_Init(SDL_INIT_EVERYTHING)) {
    fprintf(stderr, "error: %s\n", SDL_GetError()) ;
    return -1 ;
  }

  trm->scr.len_x = 80 ;
  trm->scr.len_y = 25 ;
  trm->rows      =  0 ;
  trm->cols      =  0 ;
  trm->status    =  1 ;

  trm->window = SDL_CreateWindow(
    "Pocket Machine"       ,
    SDL_WINDOWPOS_CENTERED ,
    SDL_WINDOWPOS_CENTERED ,
    trm->scr.len_x * 0x08  ,
    trm->scr.len_y * 0x10  ,
    0
  ) ;

  if (NULL == trm->window) {
    fprintf(stderr, "error: %s\n", SDL_GetError()) ;
    return -1 ;
  }

  trm->renderer = SDL_CreateRenderer(
    trm->window              ,
    -1                       ,
    SDL_RENDERER_ACCELERATED
  ) ;

  if (NULL == trm->renderer) {
    fprintf(stderr, "error: %s\n", SDL_GetError()) ;
    return -1 ;
  }

  trm->fnt.dev.adr = 0x50000000 ;
  trm->fnt.dev.len = 0x1000 ;
  trm->fnt.dev.rst = (pm_dev_rst_t)dev_fnt8x16_rst ;
  trm->fnt.dev.clk = (pm_dev_clk_t)NULL ;
  trm->fnt.dev.stb = (pm_dev_stb_t)dev_fnt8x16_stb ;
  trm->fnt.dev.sth = (pm_dev_sth_t)NULL ;
  trm->fnt.dev.stw = (pm_dev_stw_t)NULL ;
  trm->fnt.dev.ldb = (pm_dev_ldb_t)dev_fnt8x16_ldb ;
  trm->fnt.dev.ldh = (pm_dev_ldh_t)NULL ;
  trm->fnt.dev.ldw = (pm_dev_ldw_t)NULL ;

  trm->kbr.dev.adr = 0x50001000 ;
  trm->kbr.dev.len = 0x100 ;
  trm->kbr.dev.rst = (pm_dev_rst_t)pm_dev_kbr_rst ;
  trm->kbr.dev.clk = (pm_dev_clk_t)pm_dev_kbr_clk ;
  trm->kbr.dev.stb = (pm_dev_stb_t)pm_dev_kbr_stb ;
  trm->kbr.dev.sth = (pm_dev_sth_t)NULL ;
  trm->kbr.dev.stw = (pm_dev_stw_t)NULL ;
  trm->kbr.dev.ldb = (pm_dev_ldb_t)pm_dev_kbr_ldb ;
  trm->kbr.dev.ldh = (pm_dev_ldh_t)NULL ;
  trm->kbr.dev.ldw = (pm_dev_ldw_t)NULL ;

  trm->scr.dev.adr = 0x50001100 ;
  trm->scr.dev.len = 0x20000 ;
  trm->scr.dev.rst = (pm_dev_rst_t)pm_dev_scr_rst ;
  trm->scr.dev.clk = (pm_dev_clk_t)pm_dev_scr_clk ;
  trm->scr.dev.stb = (pm_dev_stb_t)pm_dev_scr_stb ;
  trm->scr.dev.sth = (pm_dev_sth_t)pm_dev_scr_sth ;
  trm->scr.dev.stw = (pm_dev_stw_t)pm_dev_scr_stw ;
  trm->scr.dev.ldb = (pm_dev_ldb_t)pm_dev_scr_ldb ;
  trm->scr.dev.ldh = (pm_dev_ldh_t)pm_dev_scr_ldh ;
  trm->scr.dev.ldw = (pm_dev_ldw_t)pm_dev_scr_ldw ;

  return 0 ;
}

void sdl_trm_dtor (struct sdl_trm_t * trm)
{
  if (NULL != trm->renderer) {
    SDL_DestroyRenderer(trm->renderer) ;
  }

  if (NULL != trm->window) {
    SDL_DestroyWindow(trm->window) ;
  }

  SDL_Quit() ;
}

void sdl_trm_clk (struct sdl_trm_t * trm, struct pm_bus_t * bus)
{
  SDL_Event event ;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT : {
      trm->status = 0 ;
    } break ;

    case SDL_KEYDOWN : {
      if (event.key.keysym.sym == SDLK_SPACE) {
        if (event.key.keysym.mod & KMOD_CTRL) {
          trm->status ^= 1 << 2 ;
          trm->status &= ~(1 << 1) ;
        } else {
          trm->status |= 1 << 1 ;
          trm->status |= 1 << 2 ;
        }
      } else if (event.key.keysym.sym == SDLK_g) {
        u_word_t adr ;
        char     typ ;

        fprintf(stderr, "GET _\b") ;
        fscanf(stdin, "%c AT 0x%x", &typ, &adr) ;
        int c ; while ((c = getchar()) != '\n' && c != EOF) ;
        fprintf(stderr, "[ 0x%08X ] ", adr) ;

        if (typ == 'B') {
          u_byte_t dat = pm_bus_ldb(bus, adr) ;
          fprintf(stderr, " 0x%02X\n", dat) ;
        } else if (typ == 'H') {
          u_half_t dat = pm_bus_ldh(bus, adr) ;
          fprintf(stderr, " 0x%04X\n", dat) ;
        } else {
          u_word_t dat = pm_bus_ldw(bus, adr) ;
          fprintf(stderr, " 0x%08X\n", dat) ;
        }
      } else if (event.key.keysym.sym == SDLK_p) {
        u_word_t adr ;
        u_word_t val ;
        char     typ ;

        fprintf(stderr, "PUT _\b") ;
        fscanf(stdin, "%c AT 0x%x 0x%x", &typ, &adr, &val) ;
        int c ; while ((c = getchar()) != '\n' && c != EOF) ;
        fprintf(stderr, "[ 0x%08X ] %u 0x%08X > ", adr, typ, val) ;

        if (typ == 'B') {
          u_byte_t dat = (u_byte_t)val ;
          pm_bus_stb(bus, adr, dat) ;
          dat = pm_bus_ldb(bus, adr) ;
          fprintf(stderr, " 0x%02X\n", dat) ;
        } else if (typ == 'H') {
          u_half_t dat = (u_half_t)val ;
          pm_bus_sth(bus, adr, dat) ;
          dat = pm_bus_ldh(bus, adr) ;
          fprintf(stderr, " 0x%04X\n", dat) ;
        } else {
          u_word_t dat = (u_word_t)val ;
          pm_bus_stw(bus, adr, dat) ;
          dat = pm_bus_ldw(bus, adr) ;
          fprintf(stderr, " 0x%08X\n", dat) ;
        }
      }

      u_byte_t key = event.key.keysym.scancode ;
      pm_dev_kbr_enq(&trm->kbr, key) ;
    } break ;

    case SDL_KEYUP : {
      u_byte_t key = event.key.keysym.scancode ;
      pm_dev_kbr_enq(&trm->kbr, 0xF0) ;
      pm_dev_kbr_enq(&trm->kbr, key) ;
    } break ;
    }
  }
  
  if (0 == trm->scr.edit)
    return ;

  if (trm->rows != trm->scr.len_y || trm->cols != trm->scr.len_x) {
    if (NULL != trm->renderer) {
      SDL_DestroyRenderer(trm->renderer) ;
    }

    if (NULL != trm->window) {
      SDL_DestroyWindow(trm->window) ;
    }

    trm->window = SDL_CreateWindow(
      "Pocket Machine"       ,
      SDL_WINDOWPOS_CENTERED ,
      SDL_WINDOWPOS_CENTERED ,
      trm->scr.len_x * 0x08  ,
      trm->scr.len_y * 0x10  ,
      0
    ) ;

    if (NULL == trm->window) {
      fprintf(stderr, "error: %s\n", SDL_GetError()) ;
      return ;
    }

    trm->renderer = SDL_CreateRenderer(
      trm->window              ,
      -1                       ,
      SDL_RENDERER_ACCELERATED
    ) ;

    if (NULL == trm->renderer) {
      fprintf(stderr, "error: %s\n", SDL_GetError()) ;
      return ;
    }
  }

  SDL_RenderClear(trm->renderer) ;

  for (int i = 0 ; i < trm->scr.len_y ; ++i) {
    for (int j = 0 ; j < trm->scr.len_x ; ++j) {
      u_half_t chr = trm->scr.buf[i * trm->scr.len_x + j] ;
      
      if (i == trm->scr.cur_y && j == trm->scr.cur_x) {
        chr =
          (((chr >> 12) & 0x0F) <<  8) |
          (((chr >>  8) & 0x0F) << 12) |
          (((chr >>  0) & 0xFF) <<  0)
        ;
      }      

      _sdl_trm_render_chr(trm, j, i, chr) ;
    }
  }

  SDL_RenderPresent(trm->renderer) ;

  trm->scr.edit = 0 ;
}

static int _sdl_trm_put_col_chr (struct sdl_trm_t * trm, int fg, int bg, char chr)
{
  if (chr == '\t') {
    int tab0 = _sdl_trm_put_col_chr(trm, fg, bg, ' ') ;
    int tab1 = _sdl_trm_put_col_chr(trm, fg, bg, ' ') ;

    if (tab0 < 0)
      return tab0 ;

    if (tab1 < 0)
      return tab0 ;

    return 2 ;
  }

  if (chr == '\n') {
    if (trm->scr.cur_y < trm->scr.len_y - 1) {
      ++trm->scr.cur_y ;
      trm->scr.edit = 1 ;
    } else {
      pm_dev_scr_stb(&trm->scr, 0xF0, 1) ;
    }

    return 1 ;
  }

  if (chr == '\r') {
    trm->scr.cur_x = 0 ;
    trm->scr.edit  = 1 ;
    return 1 ;
  }

  if (chr == '\b') {
    if (0 < trm->scr.cur_x) {
      --trm->scr.cur_x ;
      trm->scr.edit = 1 ;
    }

    return 1 ;
  }

  u_word_t adr = 0x10000 + (trm->scr.cur_y * trm->scr.len_x + trm->scr.cur_x) ;

  if (fg < 0 || bg < 0) {
    u_half_t dat = pm_dev_scr_ldh(&trm->scr, adr) ;
    
    if (fg < 0) {
      fg = (dat >> 12) & 0xF ;
    }

    if (bg < 0) {
      bg = (dat >>  8) & 0xF ;
    }
  }

  pm_dev_scr_sth(&trm->scr, adr, (fg << 12) | (bg << 8) | (chr & 0xFF)) ;
  ++trm->scr.cur_x ;

  if (trm->scr.cur_x == trm->scr.len_x) {
    if (trm->scr.cur_y < trm->scr.len_y - 1) {
      trm->scr.cur_x = 0 ;
      ++trm->scr.cur_y ;
    } else {
      --trm->scr.cur_x ;
      return -1 ;
    }
  }

  return 1 ;
}

static int _sdl_trm_put_col_str (struct sdl_trm_t * trm, int fg, int bg, const char * str)
{
  int len = 0 ;

  while ('\0' != str[len]) {
    int res = _sdl_trm_put_col_chr(trm, fg, bg, str[len]) ;

    if (res < 0)
      break ;

    len += res ;
  }

  return len ;
}


static int _sdl_trm_put_chr (struct sdl_trm_t * trm, char chr)
{
  return _sdl_trm_put_col_chr(trm, -1, -1, chr) ;
}

static int _sdl_trm_put_str (struct sdl_trm_t * trm, const char * str)
{
  int len = 0 ;

  while ('\0' != str[len]) {
    int res = _sdl_trm_put_chr(trm, str[len]) ;

    if (res <= 0)
      break ;

    len += res ;
  }

  return len ;
}

static void _sdl_trm_render_chr (struct sdl_trm_t * trm, u_byte_t ofs_x, u_byte_t ofs_y, u_half_t chr)
{
  int fgc = (chr >> 12) & 0xF  ;
  int bgc = (chr >>  8) & 0xF  ;
  int cod = (chr >>  0) & 0xFF ;

  u_word_t glyph_adr = trm->scr.fnt + cod * 0x10 ;

  for (int row = 0 ; row < 0x10 ; ++row) {
    u_byte_t dat = pm_bus_ldb(trm->scr.dev.bus->bus, glyph_adr + row) ;

    for (int col = 0 ; col < 0x08 ; ++col) {
      int x = (ofs_x * 0x08) + col ;
      int y = (ofs_y * 0x10) + row ;
      int c = 0 ;

      if (0 == ((dat >> (0x07 - col)) & 0x1)) {
        c = bgc ;
      } else {
        c = fgc ;
      }

      u_word_t color = trm->scr.pal[c] ;

      SDL_SetRenderDrawColor(
        trm->renderer        ,
        (color >> 24) & 0xFF ,
        (color >> 16) & 0xFF ,
        (color >>  8) & 0xFF ,
        (color >>  0) & 0xFF
      ) ;

      SDL_RenderDrawPoint(trm->renderer, x, y) ;
    }
  }
}
