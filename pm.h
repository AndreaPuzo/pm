#ifndef __PM_H
# define __PM_H

# define __PM_VERSION_MAJOR 0x00
# define __PM_VERSION_MINOR 0x00
# define __PM_VERSION_PATCH 0x0000

# define __PM_VERSION              \
  (                                \
    ( __PM_VERSION_MAJOR << 24 ) | \
    ( __PM_VERSION_MINOR << 16 ) | \
    ( __PM_VERSION_PATCH <<  0 )   \
  )

# define __PM_ENDIAN_LE 0x01234567 /* little endian */
# define __PM_ENDIAN_BE 0x67452301 /* big endian    */
# define __PM_ENDIAN    __PM_ENDIAN_BE /* change it to select the memory endian */

# include <stdint.h>

typedef uint8_t  u_byte_t ;
typedef uint16_t u_half_t ;
typedef uint32_t u_word_t ;
typedef  int8_t  s_byte_t ;
typedef  int16_t s_half_t ;
typedef  int32_t s_word_t ;

/* Central Processing Unit */

# define PM_INT_MC 0x00
# define PM_INT_MR 0x01
# define PM_INT_SS 0x02
# define PM_INT_BK 0x03
# define PM_INT_UD 0x04
# define PM_INT_DZ 0x05
# define PM_INT_PP 0x06
# define PM_INT_AP 0x07
# define PM_INT_BF 0x08
# define PM_INT_IF 0x09
# define PM_INT_RF 0x0A
# define PM_INT_PF 0x0B
# define PM_INT_SI(n) (0x0C | ((n) & 0x03))
# define PM_INT_HI(n) (0x10 | ((n) & 0x0F))

# define PM_SRS_PL 0
# define PM_SRS_RN 2
# define PM_SRS_EI 3
# define PM_SRS_WI 4
# define PM_SRS_SI 5
# define PM_SRS_SD 6
# define PM_SRS_PM 7
# define PM_SRS_SS 8

# define PM_SRM_PL 3
# define PM_SRM_RN 1
# define PM_SRM_EI 1
# define PM_SRM_WI 1
# define PM_SRM_SI 1
# define PM_SRM_SD 1
# define PM_SRM_PM 1
# define PM_SRM_SS 1

# define PM_SRF_PL (PM_SRM_PL << PM_SRS_PL)
# define PM_SRF_RN (PM_SRM_RN << PM_SRS_RN)
# define PM_SRF_EI (PM_SRM_EI << PM_SRS_EI)
# define PM_SRF_WI (PM_SRM_WI << PM_SRS_WI)
# define PM_SRF_SI (PM_SRM_SI << PM_SRS_SI)
# define PM_SRF_SD (PM_SRM_SD << PM_SRS_SD)
# define PM_SRF_PM (PM_SRM_PM << PM_SRS_PM)
# define PM_SRF_SS (PM_SRM_SS << PM_SRS_SS)

# define PM_CSR_SR  0x00
# define PM_CSR_IRR 0x01
# define PM_CSR_IMR 0x02
# define PM_CSR_IVT 0x03
# define PM_CSR_ISP 0x04
# define PM_CSR_ID0 0x05
# define PM_CSR_ID1 0x06
# define PM_CSR_PDT 0x07

# define PM_CSR(pl, idx) ((pl << 3) | (idx))

struct pm_cpu_t {
  struct pm_bus_t * bus ;
  u_word_t          pc0 ;
  u_word_t          pc1 ;
  u_word_t          ck0 ;
  u_word_t          ck1 ;
  u_word_t          csr [ 0x20 ] ;
  u_word_t          xpr [ 0x20 ] ;
  u_word_t          ins ;
} ;

int      pm_cpu_ctor (struct pm_cpu_t * cpu, struct pm_bus_t * bus) ;
void     pm_cpu_dtor (struct pm_cpu_t * cpu) ;
void     pm_cpu_int  (struct pm_cpu_t * cpu, u_word_t irq) ;
void     pm_cpu_rst  (struct pm_cpu_t * cpu) ;
void     pm_cpu_clk  (struct pm_cpu_t * cpu) ;
void     pm_cpu_stb  (struct pm_cpu_t * cpu, u_word_t adr, u_byte_t dat) ;
void     pm_cpu_sth  (struct pm_cpu_t * cpu, u_word_t adr, u_half_t dat) ;
void     pm_cpu_stw  (struct pm_cpu_t * cpu, u_word_t adr, u_word_t dat) ;
u_byte_t pm_cpu_ldb  (struct pm_cpu_t * cpu, u_word_t adr) ;
u_half_t pm_cpu_ldh  (struct pm_cpu_t * cpu, u_word_t adr) ;
u_word_t pm_cpu_ldw  (struct pm_cpu_t * cpu, u_word_t adr) ;

/* Random Access Memory */

struct pm_ram_t {
  struct pm_bus_t * bus ;
  u_word_t          len ;
  u_byte_t *        buf ;
} ;

int      pm_ram_ctor (struct pm_ram_t * ram, struct pm_bus_t * bus, u_word_t len) ;
void     pm_ram_dtor (struct pm_ram_t * ram) ;
void     pm_ram_int  (struct pm_ram_t * ram) ;
void     pm_ram_rst  (struct pm_ram_t * ram) ;
void     pm_ram_clk  (struct pm_ram_t * ram) ;
void     pm_ram_stb  (struct pm_ram_t * ram, u_word_t adr, u_byte_t dat) ;
void     pm_ram_sth  (struct pm_ram_t * ram, u_word_t adr, u_half_t dat) ;
void     pm_ram_stw  (struct pm_ram_t * ram, u_word_t adr, u_word_t dat) ;
u_byte_t pm_ram_ldb  (struct pm_ram_t * ram, u_word_t adr) ;
u_half_t pm_ram_ldh  (struct pm_ram_t * ram, u_word_t adr) ;
u_word_t pm_ram_ldw  (struct pm_ram_t * ram, u_word_t adr) ;

/* Input/Output Memory */

struct pm_dev_t ;

typedef void     ( * pm_dev_rst_t ) (struct pm_dev_t * dev) ;
typedef void     ( * pm_dev_clk_t ) (struct pm_dev_t * dev) ;
typedef void     ( * pm_dev_stb_t ) (struct pm_dev_t * dev, u_word_t adr, u_byte_t dat) ;
typedef void     ( * pm_dev_sth_t ) (struct pm_dev_t * dev, u_word_t adr, u_half_t dat) ;
typedef void     ( * pm_dev_stw_t ) (struct pm_dev_t * dev, u_word_t adr, u_word_t dat) ;
typedef u_byte_t ( * pm_dev_ldb_t ) (struct pm_dev_t * dev, u_word_t adr) ;
typedef u_half_t ( * pm_dev_ldh_t ) (struct pm_dev_t * dev, u_word_t adr) ;
typedef u_word_t ( * pm_dev_ldw_t ) (struct pm_dev_t * dev, u_word_t adr) ;

struct pm_dev_t {
  struct pm_iom_t * bus ;
  u_word_t          id  ;
  u_word_t          adr ;
  u_word_t          len ;
  pm_dev_rst_t      rst ;
  pm_dev_clk_t      clk ;
  pm_dev_stb_t      stb ;
  pm_dev_sth_t      sth ;
  pm_dev_stw_t      stw ;
  pm_dev_ldb_t      ldb ;
  pm_dev_ldh_t      ldh ;
  pm_dev_ldw_t      ldw ;
} ;

struct pm_iom_t {
  struct pm_bus_t * bus ;
  u_word_t          adr ;
  u_word_t          len ;
  struct pm_dev_t * dev [ 0x10 ] ;
} ;

int      pm_iom_ctor (struct pm_iom_t * iom, struct pm_bus_t * bus, u_word_t adr, u_word_t len) ;
void     pm_iom_dtor (struct pm_iom_t * iom) ;
void     pm_iom_int  (struct pm_iom_t * iom, struct pm_dev_t * dev) ;
void     pm_iom_rst  (struct pm_iom_t * iom, int id) ;
void     pm_iom_clk  (struct pm_iom_t * iom) ;
void     pm_iom_stb  (struct pm_iom_t * iom, u_word_t adr, u_byte_t dat) ;
void     pm_iom_sth  (struct pm_iom_t * iom, u_word_t adr, u_half_t dat) ;
void     pm_iom_stw  (struct pm_iom_t * iom, u_word_t adr, u_word_t dat) ;
u_byte_t pm_iom_ldb  (struct pm_iom_t * iom, u_word_t adr) ;
u_half_t pm_iom_ldh  (struct pm_iom_t * iom, u_word_t adr) ;
u_word_t pm_iom_ldw  (struct pm_iom_t * iom, u_word_t adr) ;
int      pm_iom_mnt  (struct pm_iom_t * iom, struct pm_dev_t * dev) ;
int      pm_iom_umn  (struct pm_iom_t * iom, struct pm_dev_t * dev) ;

/* omniBUS */

struct pm_bus_t {
  int             hlt ;
  struct pm_cpu_t cpu ;
  struct pm_ram_t ram ;
  struct pm_iom_t iom ;
} ;

int      pm_bus_ctor (struct pm_bus_t * bus, u_word_t len) ;
void     pm_bus_dtor (struct pm_bus_t * bus) ;
void     pm_bus_int  (struct pm_bus_t * bus, u_word_t irq) ;
void     pm_bus_rst  (struct pm_bus_t * bus, int id) ;
void     pm_bus_clk  (struct pm_bus_t * bus) ;
void     pm_bus_stb  (struct pm_bus_t * bus, u_word_t adr, u_byte_t dat) ;
void     pm_bus_sth  (struct pm_bus_t * bus, u_word_t adr, u_half_t dat) ;
void     pm_bus_stw  (struct pm_bus_t * bus, u_word_t adr, u_word_t dat) ;
u_byte_t pm_bus_ldb  (struct pm_bus_t * bus, u_word_t adr) ;
u_half_t pm_bus_ldh  (struct pm_bus_t * bus, u_word_t adr) ;
u_word_t pm_bus_ldw  (struct pm_bus_t * bus, u_word_t adr) ;
int      pm_bus_mnt  (struct pm_bus_t * bus, struct pm_dev_t * dev) ;
int      pm_bus_umn  (struct pm_bus_t * bus, struct pm_dev_t * dev) ;

#endif
