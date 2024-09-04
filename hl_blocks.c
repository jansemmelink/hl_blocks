/*****************************************************************************
 * I N C L U D E D   H E A D E R   F I L E S
 *****************************************************************************/

#include "error_stack.h"
#include "hl_blocks.h"
#include "log.h"
#include <string.h>

#define M_MIN_DATA_PER_HEADER   16

#define MIN(a,b) ((a) < (b) ? (a) : (b))

/*****************************************************************************
 *   L O C A L   D A T A   T Y P E   D E F I N I T I O N S
 *****************************************************************************/

typedef uint32_t blk_seq_t;

struct hl_blocks_s {
    size_t                      max_msg_size_ud;
    size_t                      block_size_ud;
    int                         nr_blocks_d;
    hl_blocks_write_r*          write_pr;
    hl_blocks_addr_r*           addr_pr;

    blk_seq_t                   last_blk_seq_ud;//last block seq written, 0=none, 1=first,2,3...
    uint32_t                    wr_idx_ud;      //next flash block to write to
    uint32_t                    rd_idx_ud;      //next flash block to read from
    uint32_t                    rd_ofs_ud;      //read offset inside the current block = pos of next msg_head to read

    //metrics
    uint32_t                    wr_count_ud;    //incr each time write_pr() is called

    //buffer in heap memory to write to and read from until necessary to sync
    //it has the size of one block and when ready is written as is into a block
    //of flash memory, i.e. the exact same layout.
    //when read, shift remaining data up, cause no need to sync that ever
    //read from this when wr_idx_ud == rd_idx_ud
    unsigned char*              wr_blk_data_auc;
    size_t                      wr_blk_used_ud; //data bytes after block header
    hl_blocks_msg_seq_t         last_msg_seq_ud;//last message seq written, 0=none, 1=first,2,3...
};

typedef struct block_head_s {
    blk_seq_t                   seq_ud;         //1,2,3, ... rollover to 1 when necessary
    uint32_t                    used_size_ud;   //byte used in this block (after the block header)
} blk_head_t;

typedef struct msg_head_s {
    hl_blocks_msg_seq_t         seq_ud;         //1,2,3, ... rollover to 1 when necessary
    uint32_t                    tot_size_ud;    //total bytes spanning all parts
    uint32_t                    part_ud;        //0,1,2, ... within this message
    uint32_t                    part_size_ud;   //bytes in this part (after the message header)
} msg_head_t;

/*****************************************************************************
 *   L O C A L   D A T A    D E F I N I T I O N S
 *****************************************************************************/


/*****************************************************************************
 *   L O C A L   F U N C T I O N   D E C L A R A T I O N S
 *****************************************************************************/

static uint32_t m_r_block_seq (
    const hl_blocks_t*                p_blocks_pz,
    const uint32_t                    p_block_idx_ud);


/*****************************************************************************
 *****************************************************************************
 *   P U B L I C   F U N C T I O N   D E F I N I T I O N S
 *****************************************************************************
 *****************************************************************************/

extern int hl_blocks_r_open (
    const size_t                      p_max_msg_size_ud,
    const size_t                      p_block_size_ud,
    const int                         p_nr_blocks_d,
          hl_blocks_write_r*          p_write_pr,
          hl_blocks_addr_r*           p_addr_pr,
          hl_blocks_t**               p_blocks_ppz)
{
    //start with empty and clear buffer settings
    hl_blocks_t* l_blocks_pz = (hl_blocks_t*)malloc (sizeof (hl_blocks_t));
    l_blocks_pz->max_msg_size_ud = p_max_msg_size_ud;
    l_blocks_pz->block_size_ud   = p_block_size_ud;
    l_blocks_pz->nr_blocks_d     = p_nr_blocks_d;
    l_blocks_pz->write_pr        = p_write_pr;
    l_blocks_pz->addr_pr         = p_addr_pr;

    l_blocks_pz->last_blk_seq_ud = 0;
    l_blocks_pz->wr_idx_ud       = 0;
    l_blocks_pz->rd_idx_ud       = 0;
    l_blocks_pz->rd_ofs_ud       = 0;   //offset after block head to next msg_head
    l_blocks_pz->wr_count_ud     = 0;

    l_blocks_pz->wr_blk_data_auc = (unsigned char*)malloc (l_blocks_pz->block_size_ud);
    l_blocks_pz->wr_blk_used_ud  = 0;
    l_blocks_pz->last_msg_seq_ud = 0;

    //see if any data already exists in the blocks
    uint32_t                     l_min_idx_ud = 0;
    blk_seq_t                    l_min_seq_ud = 0;
    uint32_t                     l_max_idx_ud = 0;
    blk_seq_t                    l_max_seq_ud = 0;
    for (uint32_t l_idx_ud = 0; l_idx_ud < p_nr_blocks_d; l_idx_ud++) {
        uint32_t l_seq_ud = m_r_block_seq (l_blocks_pz, l_idx_ud);
        if (l_seq_ud == 0)
            continue;
        if ((l_min_idx_ud == 0) || (l_seq_ud < l_min_seq_ud)) {
            l_min_idx_ud = l_idx_ud;
            l_min_seq_ud = l_seq_ud;
        }
        if ((l_max_seq_ud == 0) || (l_seq_ud > l_max_seq_ud)) {
            l_max_idx_ud = l_idx_ud;
            l_max_seq_ud = l_seq_ud;
        }
    }/*for each block*/

    //must have both or neither of min/max
    if ((l_min_seq_ud > 0) ^ (l_max_seq_ud > 0))
        return ERROR (HL_BLOCKS_K_ERROR_CORRUPTED,
                "min(seq=%u, idx=%u), max(seq=%u, idx=%u) (requires either or both non-zero seq)",
                l_min_seq_ud,
                l_min_idx_ud,
                l_max_seq_ud,
                l_max_idx_ud);

    //if seq min==max, then idx min must also be max, i.e. the same block
    if ((l_min_seq_ud == l_max_seq_ud) ^ (l_min_idx_ud == l_max_idx_ud))
        return ERROR (HL_BLOCKS_K_ERROR_CORRUPTED,
                "min(seq=%u, idx=%u), max(seq=%u, idx=%u) (require none or both the same)",
                l_min_seq_ud,
                l_min_idx_ud,
                l_max_seq_ud,
                l_max_idx_ud);

    if (l_min_seq_ud > 0) {
        //found data to read
        l_blocks_pz->rd_idx_ud = l_min_idx_ud;
        l_blocks_pz->wr_idx_ud = (l_max_idx_ud + 1) % l_blocks_pz->nr_blocks_d;
        l_blocks_pz->last_blk_seq_ud = l_max_idx_ud;

        //need to skip over partial messages at the head of the rd block
        //i.e. parts of messages already read from earlier blocks
        //todo: ...
    }/*if found data to read*/

    *p_blocks_ppz = l_blocks_pz;
    DEBUG ("Opened with %u blocks x %u bytes",
        l_blocks_pz->nr_blocks_d,
        l_blocks_pz->block_size_ud);
    return SUCCESS ();
}/*hl_blocks_r_open()*/


extern int hl_blocks_r_close (
          hl_blocks_t**               p_blocks_ppz)
{
    //todo: write partial block
    hl_blocks_t* l_blocks_pz = *p_blocks_ppz;
    if (l_blocks_pz->wr_blk_used_ud > 0)
        return ERROR (-1, "Not yet able to sync on close...");  //todo!

    free (l_blocks_pz);
    *p_blocks_ppz = NULL;
    return SUCCESS ();
}/*hl_blocks_r_close()*/


extern int hl_blocks_r_write (
          hl_blocks_t*                p_blocks_pz,
    const void*                       p_data_p,
    const size_t                      p_size_ud,
          hl_blocks_msg_seq_t*        p_write_seq_pud)
{
    if ((p_data_p == NULL) || (p_size_ud == 0))
        return ERROR (-1, "invalid parameters for hl_blocks_r_write(%p,%zu)", p_data_p, p_size_ud);

    DEBUG ("write(%u bytes)", p_size_ud);
    const unsigned char*        l_next_puc = (const unsigned char*)p_data_p;
    size_t                      l_remain_ud = p_size_ud;
    uint32_t                    l_part_index_ud = 0;
    while (l_remain_ud > 0)
    {
        //determine space left in current write buffer
        size_t l_buffer_space_ud = p_blocks_pz->block_size_ud
            - sizeof (blk_head_t)
            - p_blocks_pz->wr_blk_used_ud;
        DEBUG ("write block has space for %u bytes", l_buffer_space_ud);

        //determine min space required to write some/all into this block
        if (sizeof (msg_head_t) + MIN (M_MIN_DATA_PER_HEADER, l_remain_ud)
                > l_buffer_space_ud)
        {
            WARNING ("Not enough space for part/all!");            
            //not enough space in current block for reasonable part of data,
            //update the block header then
            //sync the buffer to flash memory and start a new clean buffer
            blk_head_t* l_blk_head_pz = (blk_head_t*)(p_blocks_pz->wr_blk_data_auc);
            l_blk_head_pz->seq_ud = p_blocks_pz->last_blk_seq_ud + 1;
            l_blk_head_pz->used_size_ud = p_blocks_pz->wr_blk_used_ud;
            if ((*p_blocks_pz->write_pr) (
                    p_blocks_pz->wr_idx_ud,
                    p_blocks_pz->wr_blk_data_auc)
                    != 0)
                return ERROR (-1,
                    "Failed to sync write to flash blk[%u]",
                    p_blocks_pz->wr_idx_ud);

            DEBUG ("written to block in heap");
            p_blocks_pz->wr_count_ud++;
            p_blocks_pz->wr_idx_ud++;
            p_blocks_pz->wr_blk_used_ud = 0;
            l_buffer_space_ud = p_blocks_pz->block_size_ud - sizeof (blk_head_t);
        }/*if can fit more into this block*/

        //write message header
        msg_head_t* l_msg_head_pz = (msg_head_t*)(p_blocks_pz->wr_blk_data_auc + sizeof (blk_head_t) + p_blocks_pz->wr_blk_used_ud);
        l_msg_head_pz->seq_ud = p_blocks_pz->last_msg_seq_ud + 1;
        l_msg_head_pz->tot_size_ud = p_size_ud;
        l_msg_head_pz->part_ud = l_part_index_ud;
        l_msg_head_pz->part_size_ud = (uint32_t)MIN(l_remain_ud, l_buffer_space_ud - sizeof (msg_head_t));
        DEBUG ("Remain to write %u, part size %u", l_remain_ud, l_msg_head_pz->part_size_ud);

        //copy message data after head
        memcpy (
            p_blocks_pz->wr_blk_data_auc + sizeof (blk_head_t) + p_blocks_pz->wr_blk_used_ud + sizeof (msg_head_t),
            l_next_puc,
            l_msg_head_pz->part_size_ud);

        DEBUG ("msg(seq=%u.%u).data[%u:%u]) -> blk[%u](seq=%u,data[%u:%u]=head(seq=%u,sz=%u),data[%u:%u])",
            //msg head
            l_msg_head_pz->seq_ud,
            l_msg_head_pz->part_ud,
            //part data
            p_size_ud - l_remain_ud,       //msg part start ofs
            l_msg_head_pz->part_size_ud,
            //blk
            p_blocks_pz->wr_idx_ud,
            p_blocks_pz->last_blk_seq_ud + 1,
            //block data ofs for msg head
            sizeof (blk_head_t) + p_blocks_pz->wr_blk_used_ud,
            sizeof (msg_head_t),
            //head contents
            l_msg_head_pz->seq_ud,
            l_msg_head_pz->part_size_ud,
            //block data ofs for msg data
            sizeof (blk_head_t) + p_blocks_pz->wr_blk_used_ud + sizeof (msg_head_t),
            l_msg_head_pz->part_size_ud);

        p_blocks_pz->wr_blk_used_ud += (sizeof (msg_head_t) + l_msg_head_pz->part_size_ud);
        DEBUG_HEX("wrblk after write",
            p_blocks_pz->wr_blk_data_auc,
            sizeof (blk_head_t) + p_blocks_pz->wr_blk_used_ud);

        l_next_puc  += l_msg_head_pz->part_size_ud;
        l_remain_ud -= l_msg_head_pz->part_size_ud;
        l_part_index_ud ++;
    }/*while more to write*/

    p_blocks_pz->last_msg_seq_ud ++;
    *p_write_seq_pud = p_blocks_pz->last_msg_seq_ud;
    return SUCCESS ();
}/*hl_blocks_r_write()*/


// extern int hl_blocks_r_sync (
//           hl_blocks_t*                p_blocks_pz)
// {
//     if (p_blocks_pz->buffer_used_ud > 0)
//     {
//         //write buffer to flash
//         (*p_blocks_pz->write_pr) (p_blocks_pz->buffer_auc, p_blocks_pz->buffer_used_ud);
//         p_blocks_pz->buffer_used_ud = 0;

//         //update metrics
//         p_blocks_pz->wr_count_ud++;
//         p_blocks_pz->last_blk_seq_ud++;
//         p_blocks_pz->wr_idx_ud = (p_blocks_pz->wr_idx_ud + 1) % p_blocks_pz->nr_blocks_d;
//     }/*if buffer used*/
//     return SUCCESS ();
// }/*hl_blocks_r_sync()*/

extern int hl_blocks_r_read (
          hl_blocks_t*                p_blocks_pz,
          void*                       p_buff_data_p,
    const size_t                      p_buff_size_ud,
          size_t*                     p_read_size_pud,
          hl_blocks_msg_seq_t*        p_read_seq_pud)
{
    if (  (p_blocks_pz == NULL)
       || (p_buff_data_p == NULL)
       || (p_buff_size_ud == 0)
       || (p_read_size_pud == NULL))
        return ERROR (-1, "invalid params for hl_blocks_r_read(%p,%p,%u,%p)",
            p_blocks_pz,
            p_buff_data_p,
            p_buff_size_ud,
            p_read_size_pud);

    //read message parts in loop until break when got the whole message
    uint32_t l_msg_seq_ud       = 0;
    uint32_t l_tot_size_ud      = 0;
    uint32_t l_buff_ofs_ud      = 0;     //this is also size of all parts already copied into the buffer
    uint32_t l_buff_rem_ud      = p_buff_size_ud;
    uint32_t l_parts_copied_ud  = 0;        //incr after got a part

    while (1) {
        //determine if read from flash or heap space
        uint32_t                    l_read_from_flash_ud    = (p_blocks_pz->rd_idx_ud != p_blocks_pz->wr_idx_ud);
        const blk_head_t*           l_flash_blk_head_pz     = NULL;
        const msg_head_t*           l_msg_head_pz           = NULL;

        //get msg_head in either flash or heap
        if (l_read_from_flash_ud)
        {
            //read from flash block[rd_idx_ud] at data ofs rd_ofs_ud after the block head
            //note: reading from flash does not shift other messages forward
            //      because flash is not changed after being written once)
            DEBUG ("Read from flash block rd=%u because != wr=%u",
                p_blocks_pz->rd_idx_ud,
                p_blocks_pz->wr_idx_ud);

            //get address of flash block to read
            const void*                 l_block_p;
            (*p_blocks_pz->addr_pr) (p_blocks_pz->rd_idx_ud, &l_block_p);
            DEBUG_HEX ("read block (flash)", l_block_p, p_blocks_pz->block_size_ud);

            l_flash_blk_head_pz = (const blk_head_t*)l_block_p;
            DEBUG_HEX ("blk head (flash)", l_flash_blk_head_pz, sizeof (blk_head_t));
            DEBUG ("blk[%u].head(seq=%u,used_sz=%u).ofs=%u",
                p_blocks_pz->rd_idx_ud,
                l_flash_blk_head_pz->seq_ud,
                l_flash_blk_head_pz->used_size_ud,
                p_blocks_pz->rd_ofs_ud);

            const unsigned char* l_blk_data_puc = (const unsigned char*)l_block_p + sizeof (blk_head_t);

            //get message header and copy the data
            l_msg_head_pz = (const msg_head_t*)(l_blk_data_puc + p_blocks_pz->rd_ofs_ud);
        } /*if read from flash*/
        else
        {
            //nothing more in flash to read, see if anything in heap space to read
            if (p_blocks_pz->wr_blk_used_ud == 0)
                return ERROR (HL_BLOCKS_K_ERROR_READ_ALL, "Nothing more to read.");

            DEBUG ("Reading from heap ...");
            l_msg_head_pz = (const msg_head_t*)(p_blocks_pz->wr_blk_data_auc + sizeof (blk_head_t));
        }/*if read from heap*/

        DEBUG_HEX ("msg head (flash)", l_msg_head_pz, sizeof (msg_head_t));
        DEBUG ("Read msg.head(seq=%u.%u,tot=%u,sz=%u)",
            l_msg_head_pz->seq_ud,
            l_msg_head_pz->part_ud,
            l_msg_head_pz->tot_size_ud,
            l_msg_head_pz->part_size_ud);

        if (  (  (l_parts_copied_ud == 0)
              && (l_msg_head_pz->part_ud > 0))
           || (  (l_parts_copied_ud > 0)
              && (  (l_msg_seq_ud != l_msg_head_pz->seq_ud)
                 || (l_tot_size_ud != l_msg_head_pz->tot_size_ud)
                 || (l_parts_copied_ud != l_msg_head_pz->part_ud)
                 || (l_buff_ofs_ud + l_msg_head_pz->part_size_ud > l_tot_size_ud)
                 )
              )
           )
        {
            //todo: should be able to deal with this is first read block starts with last part of other message
            ERROR_LOG ("Data corruption, msg(seq=%u,tot=%u,parts=%u,size=%u) next head(%u,%u,%u,%u)",
                l_msg_seq_ud,
                l_tot_size_ud,
                l_parts_copied_ud,
                l_buff_ofs_ud,
                l_msg_head_pz->seq_ud,
                l_msg_head_pz->tot_size_ud,
                l_msg_head_pz->part_ud,
                l_msg_head_pz->part_size_ud);

            if (l_read_from_flash_ud)
            {
                p_blocks_pz->rd_idx_ud ++; //next read start in next block
                p_blocks_pz->rd_ofs_ud = 0;
            }

            return ERROR (-1, "data corrupted - see error log");
        }//if corrupted

        if (l_parts_copied_ud == 0)
        {
            //store message overall properties from the first header
            l_msg_seq_ud     = l_msg_head_pz->seq_ud;
            l_tot_size_ud    = l_msg_head_pz->tot_size_ud;
            *p_read_seq_pud  = l_msg_head_pz->seq_ud;
            *p_read_size_pud = l_tot_size_ud;

            if (l_tot_size_ud > p_buff_size_ud)
            {
                return ERROR (-1,
                    "Message size %u will not fit in buffer size %u",
                    l_tot_size_ud,
                    p_buff_size_ud);
            }//if too small buffer specified by caller
        }/*if first part*/

        //message data follows directly after the message header
        const unsigned char* l_msg_data_puc = (const unsigned char*)l_msg_head_pz + sizeof (msg_head_t);
        DEBUG_HEX ("msg data (flash)",
            l_msg_data_puc,
            l_msg_head_pz->part_size_ud);

        //copy this part of the message data to caller's buffer
        memcpy (
            (unsigned char*)p_buff_data_p + l_buff_ofs_ud,
            l_msg_data_puc,
            l_msg_head_pz->part_size_ud);

        l_buff_ofs_ud += l_msg_head_pz->part_size_ud;
        l_buff_rem_ud -= l_msg_head_pz->part_size_ud;
        l_parts_copied_ud ++;
        DEBUG_HEX ("msg copied so far", p_buff_data_p, l_buff_ofs_ud);

        if (l_read_from_flash_ud)
        {
            //update the flash read offset and block to skip over this message
            if (p_blocks_pz->rd_ofs_ud + sizeof (msg_head_t) + l_msg_head_pz->part_size_ud
                    >= l_flash_blk_head_pz->used_size_ud)
            {
                DEBUG ("Read flash blk[%u].ofs=%u+msg(head(%u),data(%u)) -> ofs=%u >= used=%u -> GOTO NEXT BLOCK",
                    p_blocks_pz->rd_idx_ud,
                    p_blocks_pz->rd_ofs_ud,
                    sizeof (msg_head_t),
                    l_msg_head_pz->part_size_ud,
                    p_blocks_pz->rd_ofs_ud + sizeof (msg_head_t) + l_msg_head_pz->part_size_ud,
                    l_flash_blk_head_pz->used_size_ud);
                p_blocks_pz->rd_ofs_ud = 0;
                p_blocks_pz->rd_idx_ud ++;
            } else {
                DEBUG ("Read flash blk[%u].ofs=%u+msg(head(%u),data(%u)) -> ofs=%u <  used=%u -> CONT THIS BLOCK",
                    p_blocks_pz->rd_idx_ud,
                    p_blocks_pz->rd_ofs_ud,
                    sizeof (msg_head_t),
                    l_msg_head_pz->part_size_ud,
                    p_blocks_pz->rd_ofs_ud + sizeof (msg_head_t) + l_msg_head_pz->part_size_ud,
                    l_flash_blk_head_pz->used_size_ud);
                p_blocks_pz->rd_ofs_ud += sizeof (msg_head_t) + l_msg_head_pz->part_size_ud;
            }
        }/*if read from flash*/
        else
        {
            //shifting remaining messages in heap to front of buffer
            uint32_t l_head_and_data_size_ud = sizeof (msg_head_t) + l_msg_head_pz->part_size_ud;
            if (p_blocks_pz->wr_blk_used_ud > sizeof (msg_head_t) + l_msg_head_pz->part_size_ud)
            {
                memmove (
                    p_blocks_pz->wr_blk_data_auc + sizeof (blk_head_t),
                    p_blocks_pz->wr_blk_data_auc + sizeof (blk_head_t) + l_head_and_data_size_ud,
                    p_blocks_pz->wr_blk_used_ud - l_head_and_data_size_ud);
            }/*if something to move*/
            p_blocks_pz->wr_blk_used_ud -= l_head_and_data_size_ud;
            DEBUG_HEX("wrblk after read",
                p_blocks_pz->wr_blk_data_auc,
                p_blocks_pz->wr_blk_used_ud + sizeof (blk_head_t));
        }/*if read from heap*/

        //stop if got all message parts
        if (l_buff_ofs_ud >= l_tot_size_ud)
        {
            DEBUG ("Copied complete message");
            return SUCCESS();
        }/*if done*/
    }//while reading message parts
    return ERROR (-1, "Not expected to get here!");
}/*hl_blocks_r_read()*/


/*****************************************************************************
 *****************************************************************************
 *   L O C A L   F U N C T I O N   D E F I N I T I O N S
 *****************************************************************************
 *****************************************************************************/

static uint32_t m_r_block_seq (
    const hl_blocks_t*                p_blocks_pz,
    const uint32_t                    p_block_idx_ud)
{
    if (p_block_idx_ud >= p_blocks_pz->nr_blocks_d)
        return 0;

    //get address of block to read directly from flash
    const void*                 l_block_p;
    (*p_blocks_pz->addr_pr) (p_block_idx_ud, &l_block_p);
    blk_head_t* l_block_head_pz = (blk_head_t*)l_block_p;
    return l_block_head_pz->seq_ud;
}/*m_r_block_seq()*/
