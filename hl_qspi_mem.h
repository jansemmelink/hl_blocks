/*****************************************************************************
 * I N C L U D E D   H E A D E R   F I L E S
 *****************************************************************************/

#include <stdlib.h>


/*****************************************************************************
 * P U B L I C   D A T A   T Y P E   D E F I N I T I O N S
 *****************************************************************************/

typedef struct hl_qspi_blocks_s hl_qspi_blocks_t;

typedef enum hl_qspi_error_code_enum_s {
    HL_QSPI_K_ERROR_NOTHING_TO_READ = -1,
    /*
     * terminator
     */
    HL_QSPI_K_ERROR_NR_OF
} hl_qspi_error_code_e;


/*****************************************************************************
 * P U B L I C   F U N C T I O N   D E C L A R A T I O N S
 *****************************************************************************/

extern int hl_qspi_r_init (
    const void*                      p_blk0_p,
    const uint32_t                   p_nr_blks_ud);

extern int hl_qspi_r_write (
    const void*                       p_data_p,
    const size_t                      p_size_ud);

extern int hl_qspi_r_read (
    void*                             p_data_p,
    const size_t                      p_size_ud);
