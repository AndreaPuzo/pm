#ifndef __PASM_H
# define __PASM_H

# include <stdint.h>

typedef uint8_t  u_byte_t ;
typedef uint16_t u_half_t ;
typedef uint32_t u_word_t ;
typedef  int8_t  s_byte_t ;
typedef  int16_t s_half_t ;
typedef  int32_t s_word_t ;

# define PASM_TOK_UNK -1
# define PASM_TOK_EOF  0 /* \0       */
# define PASM_TOK_EOL  1 /* \n       */
# define PASM_TOK_SYM  2 /* SYMBOL   */
# define PASM_TOK_NUM  3 /* NUMBER   */
# define PASM_TOK_STR  4 /* 'STRING' */
# define PASM_TOK_COL  5 /* :        */
# define PASM_TOK_DEL  6 /* ,        */
# define PASM_TOK_COM  7 /* ;COMMENT */

# define PASM_TOK_NUM_BIN (0x20 | PASM_TOK_NUM)
# define PASM_TOK_NUM_OCT (0x80 | PASM_TOK_NUM)
# define PASM_TOK_NUM_DEC (0xA0 | PASM_TOK_NUM)
# define PASM_TOK_NUM_HEX (0xF0 | PASM_TOK_NUM)

struct pasm_tok_t {
  char * buf ;
  long   len ;
  int    typ ;
} ;

void pasm_tok_print (struct pasm_tok_t * tok, FILE * out, long linc, char * linp) ;

struct pasm_lex_t {
  char *            fn   ;
  long              len  ;
  char *            buf  ;
  long              linc ;
  char *            linp ;
  char *            tokp ;
  struct pasm_tok_t tok  ;
} ;

int  pasm_lex_ctor (struct pasm_lex_t * lex, const char * fn) ;
void pasm_lex_dtor (struct pasm_lex_t * lex) ;
void pasm_lex_init (struct pasm_lex_t * lex) ;
int  pasm_lex_next (struct pasm_lex_t * lex) ;

struct pasm_sym_t {
  char *   nam ;
  u_word_t adr ;
} ;

struct pasm_par_t {
  int                 tokc ;
  struct pasm_tok_t * tokv ;
  int                 symc ;
  struct pasm_sym_t * symv ;
} ;

int  pasm_par_ctor    (struct pasm_par_t * par) ;
void pasm_par_dtor    (struct pasm_par_t * par) ;
int  pasm_par_add_tok (struct pasm_par_t * par, const char * nam, u_word_t adr) ;
int  pasm_par_get_tok (struct pasm_par_t * par, const char * nam) ; 
int  pasm_par_set_tok (struct pasm_par_t * par, const char * nam, u_word_t adr) ; 
void pasm_par_clr_tok (struct pasm_par_t * par) ;
int  pasm_par_add_sym (struct pasm_par_t * par, struct pasm_sym_t * sym) ;
void pasm_par_clr_sym (struct pasm_par_t * par) ;

struct pasm_gen_t {
  char *     fn   ;
  u_word_t   binc ;
  u_word_t   bins ;
  u_byte_t * binv ;
} ;

struct pasm_com_t {
  struct pasm_lex_t lex ;
  struct pasm_par_t par ;
  struct pasm_gen_t gen ;
} ;

int  pasm_com_ctor (struct pasm_com_t * com, const char * infn, const char * outfn) ;
void pasm_com_dtor (struct pasm_com_t * com) ;
int  pasm_com_comp (struct pasm_com_t * com) ;
int  pasm_com_stp1 (struct pasm_com_t * com) ;
int  pasm_com_stp2 (struct pasm_com_t * com) ;

#endif
