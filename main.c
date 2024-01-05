#include "pm.h"
#include "dev/tmr.h"
#include "dev/scr.h"
#include "dev/kbr.h"

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
  int            quit      ;

  /* devices */

  struct pm_dev_scr_t  scr ;
  struct pm_dev_kbr_t  kbr ;
  struct dev_fnt8x16_t fnt ;
} ;

static void _sdl_trm_put_chr (struct sdl_trm_t * trm, u_byte_t ofs_x, u_byte_t ofs_y, u_half_t chr) ;

int  sdl_trm_ctor (struct sdl_trm_t * trm) ;
void sdl_trm_dtor (struct sdl_trm_t * trm) ;
void sdl_trm_clk  (struct sdl_trm_t * trm) ;

int main (int argc, char ** argv)
{
  char *   img_fn  = NULL      ;
  u_word_t ram_len = 0x2000000 ;
  int      arg_ofs = -1        ;

  for (int idx = 1 ; idx < argc ; ++idx) {
    char * args = argv[idx] ;

    if (0 == strcmp(args, "--help") || 0 == strcmp(args, "-h")) {
      fprintf(
        stdout ,
        "usage: %s [options] IMG\n"
        "usage: %s --help|-h\n"
        "options:\n"
        "  -h, --help    : print this message.\n"
        "  -v, --ver     : print the version.\n"
        "  -m, --mem NUM : set NUM as the memory length.\n"
        "  -a, --arg ... : ... are passed as arguments to\n"
        "                  the machine through a special\n"
        "                  device.\n" ,
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

_read_ram_len :
      char * str    = args ;
      char * endptr = NULL ;

      ram_len = (u_word_t)strtoul(str, &endptr, 0) ;

      if (NULL != endptr) {
        if (0 == strcmp(endptr, "K")) {
          ram_len <<= 10 ;
        } else if (0 == strcmp(endptr, "M")) {
          ram_len <<= 20 ;
        } else if (0 == strcmp(endptr, "G")) {
          ram_len <<= 30 ;
        }
      }

      if (0x80000000 < ram_len) {
        fprintf(stderr, "error: too much memory for working memory\n") ;
        return EXIT_FAILURE ;
      }
    } else if (0 == strncmp(args, "--mem=", 6)) {
      args += 6 ;
      goto _read_ram_len ;
    } else {
      img_fn = args ;
    }
  }

  static struct pm_bus_t     bus ;
  static struct pm_dev_tmr_t tmr [ 4 ] ;
  static struct pm_dev_dsk_t dsk [ 4 ] ;
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

    dsk[idx].dev.adr = 0x20000000 + (idx * 0x400) ;
    dsk[idx].dev.len = 0x400 ;
    dsk[idx].dev.rst = (pm_dev_rst_t)pm_dev_dsk_rst ;
    dsk[idx].dev.clk = (pm_dev_clk_t)pm_dev_dsk_clk ;
    dsk[idx].dev.stb = (pm_dev_stb_t)pm_dev_dsk_stb ;
    dsk[idx].dev.sth = (pm_dev_sth_t)pm_dev_dsk_sth ;
    dsk[idx].dev.stw = (pm_dev_stw_t)pm_dev_dsk_stw ;
    dsk[idx].dev.ldb = (pm_dev_ldb_t)pm_dev_dsk_ldb ;
    dsk[idx].dev.ldh = (pm_dev_ldh_t)pm_dev_dsk_ldh ;
    dsk[idx].dev.ldw = (pm_dev_ldw_t)pm_dev_tmr_ldw ;
  }

  if (0 != pm_bus_ctor(&bus, ram_len)) {
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

  pm_bus_rst(&bus) ;

  for (;;) {
    pm_bus_clk(&bus) ;
    sdl_trm_clk(&trm) ;

    if (0 != trm.quit || 0 != bus.hlt)
      break ;
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

  memset(fnt->buf, 0, sizeof(fnt->buf)) ;
  memcpy(fnt->buf + 0x10 * 0x20, fnt8x16, sizeof(fnt8x16)) ;

/*
  for (int i = 0 ; i < 0x100 ; ++i) {
    fprintf(stderr, "%c |", (' ' <= i && i <= '~') ? i : '?') ;
    for (int j = 0 ; j < 0x10 ; ++j) {
      fprintf(stderr, " %02X", fnt->buf[i * 0x10 + j]) ;
    }
    fprintf(stderr, "\n") ;
  }
*/
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

  trm->window = SDL_CreateWindow(
    "PocketMachine"        ,
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

void sdl_trm_clk (struct sdl_trm_t * trm)
{
  SDL_Event event ;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT : {
      trm->quit = 1 ;
    } break ;

    case SDL_KEYDOWN : {
      SDL_Keycode cod = event.key.keysym.sym ;

      if (cod == SDLK_UP) {
        if (0 != trm->scr.cur_y) {
          --trm->scr.cur_y ;
          trm->scr.edit = 1 ;
        }
      }

      if (cod == SDLK_DOWN) {
        if (trm->scr.cur_y < trm->scr.len_y - 1) {
          ++trm->scr.cur_y ;
          trm->scr.edit = 1 ;
        }
      }

      if (cod == SDLK_LEFT) {
        if (0 != trm->scr.cur_x) {
          --trm->scr.cur_x ;
          trm->scr.edit = 1 ;
        }
      }

      if (cod == SDLK_RIGHT) {
        if (trm->scr.cur_x < trm->scr.len_x - 1) {
          ++trm->scr.cur_x ;
          trm->scr.edit = 1 ;
        }
      }






      SDL_Keymod  mod = SDL_GetModState() ;

      u_byte_t ctrl = 0, alt = 0, shift = 0 ;

      if (0 != (mod & KMOD_LCTRL)) {
        ++ctrl ;
      }
      
      if (0 != (mod & KMOD_RCTRL)) {
        ++ctrl ;
      }

      if (0 != (mod & KMOD_LALT)) { 
        ++alt ;
      }

      if (0 != (mod & KMOD_RALT)) {
        ++alt ;
      }

      if (0 != (mod & KMOD_LSHIFT)) {
        ++shift ;
      }

      if (0 != (mod & KMOD_RSHIFT)) {
        ++shift ;
      }

      if (0 != ctrl) {
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)0x1D) ;
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)ctrl) ;
      }

      if (0 != alt) {
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)0x1E) ;
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)alt) ;
      }

      if (0 != shift) {
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)0x1F) ;
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)shift) ;
      }

      const char * nam = SDL_GetKeyName(cod) ;

      if (1 == strlen(nam)) {
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)nam[0]) ;
      } else if (SDLK_ESCAPE == cod) {
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)0x1B) ;
      } else if (SDLK_TAB == cod) {
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)0x09) ;
      } else if (SDLK_RETURN == cod) {
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)0x0A) ;
      } else if (SDLK_DELETE == cod) {
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)0x7F) ;
      } else if (SDLK_BACKSPACE == cod) {
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)0x07) ;
      } else {
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)0xFF) ;
      }
    } break ;

    case SDL_TEXTINPUT : {
      const char * txt = event.text.text ;

      for (int idx = 0 ; 0 != txt[idx] ; ++idx) {
        pm_dev_kbr_enq(&trm->kbr, (u_byte_t)txt[idx]) ;
      }
    } break ;
    }
  }
  
  if (0 == trm->scr.edit)
    return ;

  SDL_RenderClear(trm->renderer) ;

  for (int i = 0 ; i < trm->scr.len_y ; ++i) {
    for (int j = 0 ; j < trm->scr.len_x ; ++j) {
      u_half_t chr = trm->scr.buf[i * trm->scr.len_y + j] ;
      
//    fprintf(stderr, "( % 3u,% 3u ) 0x%04X\t", i, j, chr) ;
      if (i == trm->scr.cur_y && j == trm->scr.cur_x) {
        chr =
          (((chr >> 12) & 0x0F) <<  8) |
          (((chr >>  8) & 0x0F) << 12) |
          (((chr >>  0) & 0xFF) <<  0)
        ;
      }      
//    fprintf(stderr, "0x%04X\n", chr) ;

      _sdl_trm_put_chr(trm, j, i, chr) ;
    }
  }

  SDL_RenderPresent(trm->renderer) ;

  trm->scr.edit = 0 ;
}

static void _sdl_trm_put_chr (struct sdl_trm_t * trm, u_byte_t ofs_x, u_byte_t ofs_y, u_half_t chr)
{
  int fgc = (chr >> 12) & 0xF  ;
  int bgc = (chr >>  8) & 0xF  ;
  int cod = (chr >>  0) & 0xFF ;

  u_word_t glyph_adr = trm->scr.fnt + cod * 0x10 ;

  for (int row = 0 ; row < 0x10 ; ++row) {
    u_byte_t dat = pm_bus_ldb(trm->scr.dev.bus->bus, glyph_adr) ;

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
