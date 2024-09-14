#ifndef _HL_BLOCKS_H_
#define _HL_BLOCKS_H_

/*****************************************************************************
 * I N C L U D E D   H E A D E R   F I L E S
 *****************************************************************************/

#include <stdlib.h>


/*****************************************************************************
 * P U B L I C   D A T A   T Y P E   D E F I N I T I O N S
 *****************************************************************************/

typedef uint32_t hl_blocks_msg_seq_t;

typedef struct hl_blocks_s hl_blocks_t;

//writing is a control operation
typedef int (hl_blocks_write_r) (
    const uint32_t                    p_idx_ud,     //0..N-1
    const void*                       p_block_p);


//reading can directly read from flash memory address given the offset
//this function return pointer to block
typedef int (hl_blocks_addr_r) (
    const uint32_t                    p_idx_ud,     //0..N-1
    const void**                      p_block_pp);

typedef enum hl_blocks_error_enum_s {
    HL_BLOCKS_K_ERROR_CORRUPTED = -1,
    HL_BLOCKS_K_ERROR_READ_ALL = -2,
    HL_BLOCKS_K_ERROR_READ_BUFF_TOO_SMALL = -3,
    HL_BLOCKS_K_ERROR_NO_SPACE_LEFT_IN_BUFFER = -4,
    /*
     * terminator
     */
    HL_BLOCKS_K_ERROR_NR_OF
} hl_blocks_error_e;


/*****************************************************************************
 * P U B L I C   F U N C T I O N   D E C L A R A T I O N S
 *****************************************************************************/

/*
 * PURPOSE:
 *     Created object to manage a block of consecutive memory as
 *     blocks with the same size, writing once and reading once per block,
 *     but pakcing multiple small messages per block and spaning block
 *     boundaries to fill each block to the limit.
 * 
 *     Write in heap memory until block is full or until block is synced.
 * 
 *     Use the specified read and write functions to access the flash memory.
 * 
 * PARAMETERS:
 *     p_block_size_ud          Size of each block
 *     p_nr_blocks_ud           Number of blocks
 *     p_max_msg_size_ud        Maximum size of data allowed in a message
 *     p_min_data_per_part_ud   Minumum data written in a message part.
 *                              When not fit in remainder of block, start new block.
 *     p_write_pr               Block write function
 *     p_addr_pr                Block address functions
 *     p_blocks_ppz             Output: Blocks management object
 * 
 * RETURN:
 *     SUCCESS or ERROR
 */
extern int hl_blocks_r_open (
    const uint32_t                    p_block_size_ud,
    const uint32_t                    p_nr_blocks_ud,
    const uint32_t                    p_max_msg_size_ud,
    const uint32_t                    p_min_data_per_part_ud,
          hl_blocks_write_r*          p_write_pr,
          hl_blocks_addr_r*           p_addr_pr,
          hl_blocks_t**               p_blocks_ppz);

extern int hl_blocks_r_close (
          hl_blocks_t**               p_blocks_ppz);

extern int hl_blocks_r_write (
          hl_blocks_t*                p_blocks_pz,
    const void*                       p_data_p,
    const size_t                      p_size_ud,
          hl_blocks_msg_seq_t*        p_write_seq_pud);

extern int hl_blocks_r_sync (
          hl_blocks_t*                p_blocks_pz);

extern int hl_blocks_r_read (
          hl_blocks_t*                p_blocks_pz,
          void*                       p_buff_data_p,
    const size_t                      p_buff_size_ud,
          size_t*                     p_read_size_pud,
          hl_blocks_msg_seq_t*        p_read_seq_pud);

/*
 * ===================[ ONLY FOR UNIT TESTING ]===================
 */
extern uint32_t hl_blocks_r___get_write_count (
    const hl_blocks_t*                p_blocks_pz);


#endif /*_HL_BLOCKS_H_*/