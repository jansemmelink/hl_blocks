#include "hl_blocks.h"
#include <stdio.h>
#include <string.h>

#include "test.h"

static uint32_t            m_d_max_msg_size_ud      = 0;
static uint32_t            m_d_block_size_ud        = 0;
static uint32_t            m_d_nr_blocks_ud         = 0;
static unsigned char*      m_d_mock_flash_mem_auc   = NULL;

static int m_r_start (
    const uint32_t                    p_nr_blocks_ud,
    const uint32_t                    p_block_size_ud,
    const uint32_t                    p_max_msg_size_ud,
          hl_blocks_t**               p_block_ppz);

static int m_r_cleanup (
          hl_blocks_t**               p_block_ppz);

static int m_r_block_write (
    const uint32_t                    p_idx_ud,
    const void*                       p_block_p);

static int m_r_block_addr (
    const uint32_t                    p_idx_ud,
    const void**                      p_block_pp);


TEST(write_first_small_and_read_from_heap) {
    hl_blocks_t*                l_blocks_pz = NULL;
    if (m_r_start (2, 512, 128, &l_blocks_pz) != 0)
        return ERROR (-1, "test setup failed");

    //write small message that fits in block
    //writing strlen()+1 to include string null-terminator
    char l_msg_ac[] = "This is a test message";
    size_t l_len_ud = strlen(l_msg_ac);
    hl_blocks_msg_seq_t l_write_seq_ud = 0;
    if (hl_blocks_r_write (
            l_blocks_pz,
            l_msg_ac, l_len_ud + 1,
            &l_write_seq_ud)
            != 0)
        return ERROR(-1,
            "failed to write msg");

    if (l_write_seq_ud != 1)
        return ERROR (-1,
            "wrote seq=%u != exp 1",
            l_write_seq_ud);

    //read msg directly from write buffer - not hitting flash
    char                        l_buf_ac[100];
    size_t                      l_read_size_ud = 0;
    hl_blocks_msg_seq_t         l_read_seq_ud = 0;
    if (hl_blocks_r_read (
            l_blocks_pz,
            l_buf_ac, sizeof (l_buf_ac),
            &l_read_size_ud, &l_read_seq_ud)
            != 0)
        return ERROR (-1, "failed to read");

    if (l_read_seq_ud != l_write_seq_ud)
        return ERROR (-1, "read seq %u != exp %u", l_read_seq_ud, l_write_seq_ud);

    if (l_read_size_ud != l_len_ud + 1)
        return ERROR (-1, "read size %u != exp %u", l_read_size_ud, l_len_ud + 1);

    printf ("read: (len=%zu) \"%s\"\n", strlen (l_buf_ac), l_buf_ac);
    if (strncmp (l_buf_ac, l_msg_ac, l_len_ud) != 0)
        return ERROR (-1,
            "Read \"%s\" != Write \"%s\"",
            l_buf_ac,
            l_msg_ac);

    return m_r_cleanup (&l_blocks_pz);
}//TEST(one)

TEST(write_two_messages_then_read_both) {
    hl_blocks_t*                l_blocks_pz = NULL;
    if (m_r_start (2, 512, 128, &l_blocks_pz) != 0)
        return ERROR (-1, "test setup failed");

    //write 2 small messages to the buffer
    //writing strlen()+1 to include string null-terminator
    char l_msg1_ac[] = "This is a test message1";
    size_t l_len1_ud = strlen(l_msg1_ac);
    hl_blocks_msg_seq_t l_write_seq_ud = 0;
    if (hl_blocks_r_write (
            l_blocks_pz,
            l_msg1_ac, l_len1_ud + 1,
            &l_write_seq_ud)
            != 0)
        return ERROR(-1,
            "failed to write msg1");

    if (l_write_seq_ud != 1)
        return ERROR (-1,
            "wrote seq=%u != exp 1",
            l_write_seq_ud);

    char l_msg2_ac[] = "This is diff len test message2";
    size_t l_len2_ud = strlen(l_msg2_ac);
    if (hl_blocks_r_write (
            l_blocks_pz,
            l_msg2_ac, l_len2_ud + 1,
            &l_write_seq_ud)
            != 0)
        return ERROR(-1,
            "failed to write msg2");

    if (l_write_seq_ud != 2)
        return ERROR (-1,
            "wrote seq=%u != exp 2",
            l_write_seq_ud);



    //read first msg directly from write buffer - not hitting flash
    char                        l_buf_ac[100];
    size_t                      l_read_size_ud = 0;
    hl_blocks_msg_seq_t         l_read_seq_ud = 0;
    if (hl_blocks_r_read (
            l_blocks_pz,
            l_buf_ac, sizeof (l_buf_ac),
            &l_read_size_ud, &l_read_seq_ud)
            != 0)
        return ERROR (-1, "failed to read msg1");

    if (l_read_seq_ud != 1)
        return ERROR (-1, "read msg1 seq %u != exp %u", l_read_seq_ud, 1);

    if (l_read_size_ud != l_len1_ud + 1)
        return ERROR (-1, "read msg1 size %u != exp %u", l_read_size_ud, l_len1_ud + 1);

    printf ("read msg1: (len=%zu) \"%s\"\n", strlen (l_buf_ac), l_buf_ac);
    if (strncmp (l_buf_ac, l_msg1_ac, l_len1_ud) != 0)
        return ERROR (-1,
            "Read msg1 \"%s\" != Write \"%s\"",
            l_buf_ac,
            l_msg1_ac);


    //read second msg directly from write buffer - not hitting flash
    if (hl_blocks_r_read (
            l_blocks_pz,
            l_buf_ac, sizeof (l_buf_ac),
            &l_read_size_ud, &l_read_seq_ud)
            != 0)
        return ERROR (-1, "failed to read msg2");

    if (l_read_seq_ud != 2)
        return ERROR (-1, "read msg2 seq %u != exp %u", l_read_seq_ud, 2);

    if (l_read_size_ud != l_len2_ud + 1)
        return ERROR (-1, "read msg2 size %u != exp %u", l_read_size_ud, l_len2_ud + 1);

    printf ("read msg2: (len=%zu) \"%s\"\n", strlen (l_buf_ac), l_buf_ac);
    if (strncmp (l_buf_ac, l_msg2_ac, l_len2_ud) != 0)
        return ERROR (-1,
            "Read msg2 \"%s\" != Write \"%s\"",
            l_buf_ac,
            l_msg2_ac);

    return m_r_cleanup (&l_blocks_pz);
}/*TEST(two)*/

TEST(first_msg_span_into_2nd_block) {
    hl_blocks_t*                l_blocks_pz = NULL;
    if (m_r_start (2, 64, 64, &l_blocks_pz) != 0)
        return ERROR (-1, "test setup failed");

    //write message that, along with its header, does not fit
    //block if 64 bytes: blk_head(8) + msg_head(8) + data
    //                   0..7          8..15     16..63 (max 63-16=47)
    //writing strlen()+1 to include string null-terminator
    char l_msg_ac[] = "start67890123456789012345678901234567890123456end"; //49+1=50 bytes
    size_t l_len_ud = strlen(l_msg_ac);
    hl_blocks_msg_seq_t l_write_seq_ud = 0;
    if (hl_blocks_r_write (
            l_blocks_pz,
            l_msg_ac, l_len_ud + 1,
            &l_write_seq_ud)
            != 0)
        return ERROR(-1,
            "failed to write msg");

    if (l_write_seq_ud != 1)
        return ERROR (-1,
            "wrote seq=%u != exp 1",
            l_write_seq_ud);

    //read msg directly from write buffer - not hitting flash
    char                        l_buf_ac[100];
    size_t                      l_read_size_ud = 0;
    hl_blocks_msg_seq_t         l_read_seq_ud = 0;
    if (hl_blocks_r_read (
            l_blocks_pz,
            l_buf_ac, sizeof (l_buf_ac),
            &l_read_size_ud, &l_read_seq_ud)
            != 0)
        return ERROR (-1, "failed to read");

    if (l_read_seq_ud != l_write_seq_ud)
        return ERROR (-1, "read seq %u != exp %u", l_read_seq_ud, l_write_seq_ud);

    if (l_read_size_ud != l_len_ud + 1)
        return ERROR (-1, "read size %u != exp %u", l_read_size_ud, l_len_ud + 1);

    printf ("read: (len=%zu) \"%s\"\n", strlen (l_buf_ac), l_buf_ac);
    if (strncmp (l_buf_ac, l_msg_ac, l_len_ud) != 0)
        return ERROR (-1,
            "Read \"%s\" != Write \"%s\"",
            l_buf_ac,
            l_msg_ac);

    return m_r_cleanup (&l_blocks_pz);
}//TEST()



static int m_r_start (
    const uint32_t                    p_nr_blocks_ud,
    const uint32_t                    p_block_size_ud,
    const uint32_t                    p_max_msg_size_ud,
          hl_blocks_t**               p_block_ppz)
{
    /*
     * start with clean block of memory
     */
    m_d_max_msg_size_ud     = p_max_msg_size_ud;
    m_d_block_size_ud       = p_block_size_ud;
    m_d_nr_blocks_ud        = p_nr_blocks_ud;
    m_d_mock_flash_mem_auc  = (unsigned char*)malloc (m_d_block_size_ud * m_d_nr_blocks_ud);
    hl_blocks_t*                l_blocks_pz = NULL;
    if (hl_blocks_r_open (
                m_d_max_msg_size_ud,
                m_d_block_size_ud,
                m_d_nr_blocks_ud,
                m_r_block_write,
                m_r_block_addr,
                &l_blocks_pz)
                != 0)
        return ERROR(-1,
            "failed to open blocks");

    *p_block_ppz = l_blocks_pz;
    return SUCCESS ();
}/*m_r_start()*/

static int m_r_cleanup (
          hl_blocks_t**               p_block_ppz)
{
    if (hl_blocks_r_close (p_block_ppz) != 0)
        return ERROR (-1, "failed to close blocks");

    free (m_d_mock_flash_mem_auc);
    m_d_mock_flash_mem_auc = NULL;
    return SUCCESS();
}/*m_r_cleanup()*/

static int m_r_block_write (
    const uint32_t                    p_idx_ud,
    const void*                       p_block_p)
{
    if (p_idx_ud >= m_d_nr_blocks_ud)
        return ERROR (-1, "invalid block idx %u not 0..%u", p_idx_ud, m_d_nr_blocks_ud - 1);

    void* l_blk_p = m_d_mock_flash_mem_auc + (m_d_block_size_ud * p_idx_ud);
    memcpy (l_blk_p, p_block_p, m_d_block_size_ud);
    return SUCCESS();
}/*m_r_block_write()*/

static int m_r_block_addr (
    const uint32_t                    p_idx_ud,
    const void**                      p_block_pp)
{
    if (p_idx_ud >= m_d_nr_blocks_ud)
        return ERROR (-1, "invalid block idx %u not 0..%u", p_idx_ud, m_d_nr_blocks_ud - 1);

    *p_block_pp = m_d_mock_flash_mem_auc + (m_d_block_size_ud * p_idx_ud);
    return SUCCESS();
}/*m_r_block_addr()*/
