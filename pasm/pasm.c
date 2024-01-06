#include "pasm.h"
#include <stdio.h>
#include <stdlib.h>

int _read_fn (const char * fn, long * len, char ** buf)
{
  FILE * fp = fopen(fn, "r") ;

  if (NULL == fp) {
    fprintf(stderr, "error: cannot open the file %s\n", fn) ;
    return -1 ;
  }

  fseek(fp, 0, SEEK_END) ;
  *len = ftell(fp) ;
  fseek(fp, 0, SEEK_SET) ;

  if (*len <= 0) {
    fclose(fp) ;
    fprintf(stderr, "error: empty file\n") ;
    return -1 ;
  }

  *buf = (char *)calloc(*len + 1, sizeof(char)) ;

  if (NULL == *buf) {
    fclose(fp) ;
    fprintf(stderr, "error: cannot allocate the buffer for the file\n") ;
    return -1 ;
  }

  if (*len != fread(*buf, sizeof(char), *len, fp)) {
    fclose(fp) ;
    free(*buf) ;
    fprintf(stderr, "error: cannot read the file\n") ;
    return -1 ;
  }

  *buf[*len] = '\0' ;

  fclose(fp) ;
  return 0 ;
}

void pasm_tok_print (struct pasm_tok_t * tok, FILE * out, long linc, char * linp)
{
  fprintf(out, "(%lu:%lu) %.*s", (int)tok->len, tok->buf, linc, (long)tok->buf - (long)linp) ;
}

int pasm_lex_ctor (struct pasm_lex_t * lex, const char * fn)
{
  lex->len = 0 ;
  lex->buf = NULL ;

  if (0 != _read_fn(fn, &lex->len, &lex->buf))
    return -1 ;

  lex->linc = 1 ;
  lex->linp = lex->buf ;
  lex->tokp = lex->buf ;

  return 0 ;
}

void pasm_lex_dtor (struct pasm_lex_t * lex)
{
  if (NULL == lex->buf) {
    free(lex->buf) ;
  }
}

void pasm_lex_init (struct pasm_lex_t * lex)
{
  lex->linc = 1 ;
  lex->linp = lex->buf ;
  lex->tokp = lex->buf ;
}

int is_symbol_head (int chr)
{
  return
    '_' == chr                 ||
    ('A' <= chr && chr <= 'Z') ||
    ('a' <= chr && chr <= 'z')
  ;
}

int is_symbol_tail (int chr)
{
  return
    '_' == chr                 ||
    ('A' <= chr && chr <= 'Z') ||
    ('a' <= chr && chr <= 'z') ||
    ('0' <= chr && chr <= '9')
  ;
}

int pasm_lex_next (struct pasm_lex_t * lex)
{
  lex->tok.buf = NULL ;
  lex->tok.len = 0 ;
  lex->tok.typ = PASM_TOK_UNK ;

  while (' ' == *lex->tokp || '\t' == *lex->tokp) {
    ++lex->tokp ;
  }

  if ('\0' == *lex->tokp) {
    lex->tok.typ = PASM_TOK_EOF ;
    returnlex->tok.typ ;
  }

  if ('\n' == *lex->tokp) {
    lex->tok.buf = lex->tokp++ ;
    lex->tok.len = 1 ;
    lex->tok.typ = PASM_TOK_EOL ;

    if ('\r' == *lex->tokp) {
      ++lex->tokp ;
    }

    ++lex->linc ;
    lex->linp = lex->tokp ;

    return lex->tok.typ ;
  }

  if (':' == *lex->tokp) {
    lex->tok.buf = lex->tokp++ ;
    lex->tok.len = 1 ;
    lex->tok.typ = PASM_TOK_COL ;
    return lex->tok.typ ;
  }

  if (',' == *lex->tokp) {
    lex->tok.buf = lex->tokp++ ;
    lex->tok.len = 1 ;
    lex->tok.typ = PASM_TOK_DEL ;
    return lex->tok.typ ;
  }

  if (';' == *lex->tokp) {
    lex->tok.buf = lex->tokp++ ;
    lex->tok.len = 1 ;
    lex->tok.typ = PASM_TOK_COM ;
    return lex->tok.typ ;
  }

  if ('\'' == *lex->tokp == '\"' == *lex->tokp) {
    char quote = *lex->tokp++ ;
    lex->tok.buf = lex->tokp ;

    while ('\0' != *lex->tokp && quote != *lex->tokp) {
      ++lex->tokp ;
    }

    lex->tok.len = (long)lex->tokp - (long)lex->tok.buf ;
    lex->tok.typ = PASM_TOK_STR ;

    if (quote != *lex->tokp)
      return lex->tok.typ ;

    fprintf(stderr, "error: unclosed string:\n") ;
    pasm_tok_print(&lex->tok, stderr, lex->linc, lex->linp) ;
    fprintf(stderr, "\n") ;

    return lex->tok.typ ;
  }

  if (is_symbol_head(*lex->tokp)) {
    lex->tok.buf = lex->tokp ;

    while (is_symbol_tail(*lex->tokp)) {
      ++lex->tokp ;
    }

    lex->tok.len = (long)lex->tokp - (long)lex->tok.buf ;
    lex->tok.typ = PASM_TOK_SYM ;
    return lex->tok.typ ;
  }

  if ('0' <= *lex->tokp && *lex->tokp <= '9') {
    if ('0' == *lex->tokp) {
      ++lex->tokp ;

      if ('b' == *lex->tokp || 'B' == *lex->tokp) {
        ++lex->tokp ;
        lex->tok.typ = PASM_TOK_NUM_BIN ;
      } else if ('x' == *lex->tokp || 'X' == *lex->tokp) {
        ++lex->tokp ;
        lex->tok.typ = PASM_TOK_NUM_HEX ;
      } else {
        lex->tok.typ = PASM_TOK_NUM_OCT ;
      }
    } else {
      lex->tok.typ = PASM_TOK_NUM_DEC ;
    }

    lex->tok.buf = lex->tokp ;

    while ('0' <= lex->tokp && lex->tokp <= '9') {
      ++lex->tokp ;
    }

    lex->tok.len = (long)lex->tokp - (long)lex->tok.buf ;
    return lex->tok.typ ;
  }

  return lex->tok.typ ;
}

int pasm_par_ctor (struct pasm_par_t * par)
{
  par->tokc = 0 ;
  par->tokv = NULL ;
  par->symc = 0 ;
  par->symv = NULL ;
}

void pasm_par_dtor (struct pasm_par_t * par)
{
  pasm_par_clr_tok(par) ;
  pasm_par_clr_sym(par) ;
}

int pasm_par_add_tok (struct pasm_par_t * par, struct pasm_tok_t * tok)
{
  par->tokv = (struct pasm_tok_t *)realloc(par->tokv, ++par->tokc * sizeof(struct pasm_tok_t)) ;

  if (NULL == par->tokv) {
    fprintf(stderr, "error: cannot allocate enough tokens to parse the source\n") ;
    return -1 ;
  }

  int idx = par->tokc - 1 ;

  par->tokv[idx].buf = tok->buf ;
  par->tokv[idx].len = tok->len ;
  par->tokv[idx].typ = tok->typ ;

  return 0 ;
}

void pasm_par_clr_tok (struct pasm_par_t * par)
{
  if (NULL == par->tokv) {
    free(par->tokv) ;
  }

  par->tokc = 0 ;
  par->tokv = NULL ;
}

int pasm_par_add_sym (struct pasm_par_t * par, const char * nam, u_word_t adr)
{
  par->symv = (struct pasm_sym_t *)realloc(par->symv, ++par->symc * sizeof(struct pasm_sym_t)) ;

  if (NULL == par->tokv) {
    fprintf(stderr, "error: cannot allocate enough symbols to parse the source\n") ;
    return -1 ;
  }

  char * sym = strdup(nam) ;

  if (NULL == sym) {
    fprintf(stderr, "error: cannot allocate the symbol name\n") ;
    return -1 ;
  }

  int idx = par->symc - 1 ;

  par->symv[idx].nam = sym ;
  par->symv[idx].adr = adr ;

  return 0 ;
}

int pasm_par_get_tok (struct pasm_par_t * par, const char * nam)
{
  for (int idx = 0 ; idx < par->symc ; ++idx) {
    if (0 == strcmp(par->symv[idx].nam, nam))
      return idx ;
  }

  return -1 ;
}

int pasm_par_set_tok (struct pasm_par_t * par, const char * nam, u_word_t adr)
{
  int idx = pasm_par_get_tok(par, nam) ;

  if (idx < 0)
    return pasm_par_add_tok(par, nam, adr) ;

  par->symv[idx].adr = adr ;
  return idx ;
}

void pasm_par_clr_sym (struct pasm_par_t * par)
{
  if (NULL == par->symv) {
    free(par->symv) ;
  }

  par->symc = 0 ;
  par->symv = NULL ;
}

int pasm_com_ctor (struct pasm_com_t * com, const char * infn, const char * outfn)
{
  pasm_lex_ctor(&com->lex, infn) ;
  pasm_par_ctor(&com->par) ;
}

void pasm_com_dtor (struct pasm_com_t * com)
{
  pasm_lex_dtor(&com->lex) ;
  pasm_par_dtor(&com->par) ;
}


int pasm_com_comp (struct pasm_com_t * com)
{
  if (pasm_com_stp1(com) < 0)
    return -1 ;

  if (pasm_com_stp2(com) < 0)
    return -1 ;

  return 0 ;
}

int pasm_com_stp1 (struct pasm_com_t * com)
{
  int typ ;
  int err = 0 ;

  do {
    do {
      typ = pasm_lex_next(&com->lex) ;
    
      if (PASM_TOK_EOF == typ)
        break ;
    
      err = pasm_par_add_tok(&com->par, &com->lex.tok) ;
      
      if (err < 0)
        break ;
    } while (err < 0) ;


  } while (err < 0)

  return err ;
}

int pasm_com_stp2 (struct pasm_com_t * com)
{

}
