#include "hl_blocks.h"
#include <stdio.h>
#include <string.h>

#include "test.h"
#include "log.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

static uint32_t            m_d_max_msg_size_ud      = 0;
static uint32_t            m_d_block_size_ud        = 0;
static uint32_t            m_d_nr_blocks_ud         = 0;
static unsigned char*      m_d_mock_flash_mem_auc   = NULL;

static int m_r_start (
    const uint32_t                    p_block_size_ud,
    const uint32_t                    p_nr_blocks_ud,
    const uint32_t                    p_max_msg_size_ud,
    const uint32_t                    p_min_part_size_ud,
          hl_blocks_t**               p_block_ppz);

static int m_r_cleanup (
          hl_blocks_t**               p_block_ppz);

static int m_r_block_write (
    const uint32_t                    p_idx_ud,
    const void*                       p_block_p);

static int m_r_block_addr (
    const uint32_t                    p_idx_ud,
    const void**                      p_block_pp);

static void m_r_make_test_msg (
          void*                       p_buff_data_p,
    const size_t                      p_buff_size_ud,
    const uint32_t                    p_msg_id_ud,          //a number printed into the start of the message
    const uint32_t                    p_test_msg_len_ud);   //how long the message must be


#define START(block_size,nr_blocks,max_msg_size,min_part_size)                  \
    const uint32_t              l_nr_blocks_ud      = nr_blocks;                \
    const uint32_t              l_block_size_ud     = block_size;               \
    const uint32_t              l_min_part_size_ud  = min_part_size;            \
    hl_blocks_t*                l_blocks_pz         = NULL;                     \
    if (m_r_start (block_size, nr_blocks, max_msg_size, min_part_size, &l_blocks_pz) != 0) \
        return ERROR (-1, "test setup failed")

#define SAFE_STR(str) ((str) == NULL ? "<null>" : (str))

#define _ASSERT_INT_EQ(file, line, expected, actual)                            \
    if ((expected) != (actual))                                                 \
        return ERROR (-1,                                                       \
            "assertion failed at %s(%d): expected %u != %u actual",             \
            file, line,                                                         \
            expected, actual);

#define ASSERT_INT_EQ(expected, actual)                                         \
    _ASSERT_INT_EQ(__FILE__, __LINE__, expected, actual)

#define _ASSERT_STR_EQ(file, line, expected, actual)                            \
    if (strcmp (expected, actual) != 0)                                         \
        return ERROR (-1,                                                       \
            "assertion failed at %s(%d): expected \"%s\" != \"%s\" actual",     \
            file, line,                                                         \
            SAFE_STR(expected), SAFE_STR(actual));                              \

#define ASSERT_STR_EQ(expected, actual)                                         \
    _ASSERT_STR_EQ(__FILE__, __LINE__, expected, actual)

#define _ASSERT_NOTHING_MORE_TO_READ(file, line, blocks)                        \
    {                                                                           \
        char                        l_buf_ac[32];                               \
        size_t                      l_read_size_ud = 0;                         \
        int                         l_result_d;                                 \
        l_result_d = hl_blocks_r_read (                                         \
            blocks,                                                             \
            l_buf_ac, sizeof (l_buf_ac),                                        \
            &l_read_size_ud,                                                    \
            NULL);                                                              \
        if (l_result_d != HL_BLOCKS_K_ERROR_READ_ALL)                           \
            return ERROR (-1,                                                   \
            "assertion failed at %s(%d): expected READ_ALL=%d != %d actual",    \
            HL_BLOCKS_K_ERROR_READ_ALL,                                         \
            l_result_d);                                                        \
    }

#define ASSERT_NOTHING_MORE_TO_READ(blocks)                                     \
    _ASSERT_NOTHING_MORE_TO_READ(__FILE__, __LINE__, blocks)

TEST(write_first_small_and_read_from_heap) {
    START(
        512,    //block size
        2,      //nr of blocks
        128,    //max message size
        32);    //min data per message part

    //write small message that fits in block
    char                        l_msg_ac[]      = "This is a test message";
    size_t                      l_len_ud        = strlen(l_msg_ac);
    hl_blocks_msg_seq_t         l_write_seq_ud  = 0;
    if (hl_blocks_r_write (
            l_blocks_pz,
            l_msg_ac, l_len_ud + 1, //+1 to include '\0' terminator
            &l_write_seq_ud)
            != 0)
        return ERROR(-1,
            "failed to write msg");

    ASSERT_INT_EQ (1, l_write_seq_ud);

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

    ASSERT_INT_EQ (l_write_seq_ud, l_read_seq_ud);
    ASSERT_INT_EQ (l_len_ud + 1, l_read_size_ud);
    ASSERT_STR_EQ (l_msg_ac, l_buf_ac);
    ASSERT_INT_EQ (0, hl_blocks_r___get_write_count (l_blocks_pz));
    ASSERT_NOTHING_MORE_TO_READ (l_blocks_pz);
    return m_r_cleanup (&l_blocks_pz);
}//TEST(write_first_small_and_read_from_heap)

TEST(write_two_messages_then_read_both) {
    START(
        512,    //block size
        2,      //nr of blocks
        128,    //max message size
        32);    //min data per message part

    //write 2 small messages to the heap buffer
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

    ASSERT_INT_EQ (1, l_write_seq_ud);

    char l_msg2_ac[] = "This is diff len test message2";
    size_t l_len2_ud = strlen(l_msg2_ac);
    if (hl_blocks_r_write (
            l_blocks_pz,
            l_msg2_ac, l_len2_ud + 1,
            &l_write_seq_ud)
            != 0)
        return ERROR(-1,
            "failed to write msg2");

    ASSERT_INT_EQ (2, l_write_seq_ud);

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

    ASSERT_INT_EQ (1, l_read_seq_ud);
    ASSERT_INT_EQ (l_len1_ud + 1, l_read_size_ud);
    ASSERT_STR_EQ (l_msg1_ac, l_buf_ac);

    //read second msg directly from write buffer - not hitting flash
    if (hl_blocks_r_read (
            l_blocks_pz,
            l_buf_ac, sizeof (l_buf_ac),
            &l_read_size_ud, &l_read_seq_ud)
            != 0)
        return ERROR (-1, "failed to read msg2");

    ASSERT_INT_EQ (2, l_read_seq_ud);
    ASSERT_INT_EQ (l_len2_ud + 1, l_read_size_ud);
    ASSERT_STR_EQ (l_msg2_ac, l_buf_ac);
    ASSERT_INT_EQ (0, hl_blocks_r___get_write_count (l_blocks_pz));
    ASSERT_NOTHING_MORE_TO_READ (l_blocks_pz);
    return m_r_cleanup (&l_blocks_pz);
}/*TEST(two)*/

TEST(first_msg_span_into_2nd_block) {
    START(
        64,     //block size
        4,      //nr of blocks
        64,     //max message size
        32);    //min data per message part

    //write message that, along with its header, does not fit
    //block if 64 bytes: blk_head(8) + msg_head(8) + data
    //                   0..7          8..15     16..63 (max 63-16=47)
    //writing strlen()+1 to include string null-terminator
    char                        l_msg_ac[100];
    m_r_make_test_msg(l_msg_ac, sizeof (l_msg_ac), 1, 50);
    size_t                      l_len_ud        = strlen(l_msg_ac);
    hl_blocks_msg_seq_t         l_write_seq_ud  = 0;
    if (hl_blocks_r_write (
            l_blocks_pz,
            l_msg_ac, l_len_ud + 1,
            &l_write_seq_ud)
            != 0)
        return ERROR(-1,
            "failed to write msg");

    ASSERT_INT_EQ (1, l_write_seq_ud);

    //read msg will start in flash and end in heap
    char                        l_buf_ac[100];
    size_t                      l_read_size_ud = 0;
    hl_blocks_msg_seq_t         l_read_seq_ud = 0;
    if (hl_blocks_r_read (
            l_blocks_pz,
            l_buf_ac, sizeof (l_buf_ac),
            &l_read_size_ud, &l_read_seq_ud)
            != 0)
        return ERROR (-1, "failed to read");

    ASSERT_INT_EQ (l_write_seq_ud, l_read_seq_ud);
    ASSERT_INT_EQ (l_len_ud + 1, l_read_size_ud);
    ASSERT_STR_EQ (l_msg_ac, l_buf_ac);
    ASSERT_INT_EQ (1, hl_blocks_r___get_write_count (l_blocks_pz));
    ASSERT_NOTHING_MORE_TO_READ (l_blocks_pz);
    return m_r_cleanup (&l_blocks_pz);
}//TEST()

TEST(read_small_messages_from_flash) {
    START(
        128,    //block size
        4,      //nr of blocks
        128,    //max message size
        32);    //min data per message part

    //write several small messages, with more than 1 fitting in one block,
    //and enough that blocks write to flash at least twice before starting
    //to read, then see if can read and catch up to read all messages from
    //both flash blocks and heap.
    //this message is 10 bytes data + header of 32 bytes = 42bytes
    //block of 128 bytes - 32bytes block header = 96 bytes means 2 messages
    //will fit in a block. Write 6 of them, so two blocks will save to flash
    //last two may still be in heap
    const int l_nr_msgs_d = 10;
    for (int i = 0; i < l_nr_msgs_d; i ++)
    {
        char                        l_msg_ac[16];
        snprintf (l_msg_ac, sizeof (l_msg_ac), "test(%03d)", i); //9+1=10 bytes
        size_t l_len_ud = strlen(l_msg_ac);
        hl_blocks_msg_seq_t l_write_seq_ud = 0;
        if (hl_blocks_r_write (
                l_blocks_pz,
                l_msg_ac, l_len_ud + 1,
                &l_write_seq_ud)
                != 0)
            return ERROR(-1,
                "failed to write msg");

        ASSERT_INT_EQ (i + 1, l_write_seq_ud);
    }/*for each messages to write*/

    //read all messages
    for (int i = 0; i < l_nr_msgs_d; i ++)
    {
        char                        l_exp_msg_ac[16];
        snprintf (l_exp_msg_ac, sizeof (l_exp_msg_ac), "test(%03d)", i); //9+1=10 bytes
        size_t l_exp_len_ud = strlen(l_exp_msg_ac);

        char                        l_buf_ac[100];
        size_t                      l_read_size_ud = 0;
        hl_blocks_msg_seq_t         l_read_seq_ud = 0;
        if (hl_blocks_r_read (
                l_blocks_pz,
                l_buf_ac, sizeof (l_buf_ac),
                &l_read_size_ud,
                &l_read_seq_ud)
                != 0)
            return ERROR (-1, "failed to read");

        ASSERT_INT_EQ (i + 1, l_read_seq_ud);
        ASSERT_INT_EQ (l_exp_len_ud + 1, l_read_size_ud);
        ASSERT_STR_EQ (l_exp_msg_ac, l_buf_ac);
    }/*for each message*/

    ASSERT_INT_EQ (2, hl_blocks_r___get_write_count (l_blocks_pz));
    ASSERT_NOTHING_MORE_TO_READ (l_blocks_pz);
    return m_r_cleanup (&l_blocks_pz);
}//TEST()

TEST(cold_start_with_message_at_start_of_first_block) {
    START(
        128,    //block size
        4,      //nr of blocks
        128,    //max message size
        32);    //min data per message part

    //write two small messages - not yet filling the block
    const int l_nr_msgs_d = 2;
    for (int i = 0; i < l_nr_msgs_d; i ++)
    {
        char                        l_msg_ac[16];
        snprintf (l_msg_ac, sizeof (l_msg_ac), "test(%03d)", i); //9+1=10 bytes
        size_t l_len_ud = strlen(l_msg_ac);
        hl_blocks_msg_seq_t l_write_seq_ud = 0;
        if (hl_blocks_r_write (
                l_blocks_pz,
                l_msg_ac, l_len_ud + 1,
                &l_write_seq_ud)
                != 0)
            return ERROR(-1,
                "failed to write msg");
    }/*for each messages to write*/

    ASSERT_INT_EQ (0, hl_blocks_r___get_write_count (l_blocks_pz));
    if (hl_blocks_r_sync (l_blocks_pz) != 0)
        return ERROR(-1, "Failed to sync on shutdown");
    ASSERT_INT_EQ (1, hl_blocks_r___get_write_count (l_blocks_pz));
    l_blocks_pz = NULL; //after shutdown, that instance is no longer

    //now open blocks based on that same memory contents
    if (hl_blocks_r_open (
                128,    //from above
                4,      //from above
                128,    //from above
                32,
                m_r_block_write,
                m_r_block_addr,
                &l_blocks_pz)
                != 0)
        return ERROR(-1,
            "failed to open blocks for cold start");

    //read all messages
    for (int i = 0; i < l_nr_msgs_d; i ++)
    {
        char                        l_exp_msg_ac[16];
        snprintf (l_exp_msg_ac, sizeof (l_exp_msg_ac), "test(%03d)", i); //9+1=10 bytes
        size_t l_exp_len_ud = strlen(l_exp_msg_ac);

        char                        l_buf_ac[100];
        size_t                      l_read_size_ud = 0;
        hl_blocks_msg_seq_t         l_read_seq_ud = 0;
        if (hl_blocks_r_read (
                l_blocks_pz,
                l_buf_ac, sizeof (l_buf_ac),
                &l_read_size_ud,
                &l_read_seq_ud)
                != 0)
            return ERROR (-1, "failed to read");

        ASSERT_INT_EQ (i + 1, l_read_seq_ud);
        ASSERT_INT_EQ (l_exp_len_ud + 1, l_read_size_ud);
        ASSERT_STR_EQ (l_exp_msg_ac, l_buf_ac);
    }/*for each message*/
    ASSERT_NOTHING_MORE_TO_READ (l_blocks_pz);
    return m_r_cleanup (&l_blocks_pz);
}//TEST()

TEST(cold_start_with_messages_spanning_two_blocks) {
    START(
        128,    //block size
        4,      //nr of blocks
        128,    //max message size
        32);    //min data per message part

    //write small messages flowing into next block
    const int l_nr_msgs_d = 6;
    for (int i = 0; i < l_nr_msgs_d; i ++)
    {
        char                        l_msg_ac[16];
        snprintf (l_msg_ac, sizeof (l_msg_ac), "test(%03d)", i); //9+1=10 bytes
        size_t l_len_ud = strlen(l_msg_ac);
        hl_blocks_msg_seq_t l_write_seq_ud = 0;
        if (hl_blocks_r_write (
                l_blocks_pz,
                l_msg_ac, l_len_ud + 1,
                &l_write_seq_ud)
                != 0)
            return ERROR(-1,
                "failed to write msg");
    }/*for each messages to write*/

    ASSERT_INT_EQ (1, hl_blocks_r___get_write_count (l_blocks_pz));
    if (hl_blocks_r_sync (l_blocks_pz) != 0)
        return ERROR(-1, "Failed to sync on shutdown");
    ASSERT_INT_EQ (2, hl_blocks_r___get_write_count (l_blocks_pz));
    l_blocks_pz = NULL; //after shutdown, that instance is no longer

    //now open blocks based on that same memory contents
    if (hl_blocks_r_open (
                128,    //from above
                4,    //from above
                128,    //from above
                32,
                m_r_block_write,
                m_r_block_addr,
                &l_blocks_pz)
                != 0)
        return ERROR(-1,
            "failed to open blocks for cold start");

    //read all messages
    for (int i = 0; i < l_nr_msgs_d; i ++)
    {
        char                        l_exp_msg_ac[16];
        snprintf (l_exp_msg_ac, sizeof (l_exp_msg_ac), "test(%03d)", i); //9+1=10 bytes
        size_t l_exp_len_ud = strlen(l_exp_msg_ac);

        char                        l_buf_ac[100];
        size_t                      l_read_size_ud = 0;
        hl_blocks_msg_seq_t         l_read_seq_ud = 0;
        if (hl_blocks_r_read (
                l_blocks_pz,
                l_buf_ac, sizeof (l_buf_ac),
                &l_read_size_ud,
                &l_read_seq_ud)
                != 0)
            return ERROR (-1, "failed to read");

        ASSERT_INT_EQ (i + 1, l_read_seq_ud);
        ASSERT_INT_EQ (l_exp_len_ud + 1, l_read_size_ud);
        ASSERT_STR_EQ (l_exp_msg_ac, l_buf_ac);
    }/*for each message*/
    ASSERT_INT_EQ (0, hl_blocks_r___get_write_count (l_blocks_pz));
    ASSERT_NOTHING_MORE_TO_READ (l_blocks_pz);
    return m_r_cleanup (&l_blocks_pz);
}//TEST()

TEST(cold_start_with_messages_starting_in_later_blocks) {
    START(
        128,    //block size
        4,      //nr of blocks
        128,    //max message size
        32);    //min data per message part

    //write small messages filling blk[0] and blk[1] and a bit of heap destined for blk[2]
    //then read all in blk[0] and some in blk[1]
    //then sync and do cold start
    //all in blk[1] will be read after cold start, including the ones read before shut down
    //because partial block reads are not indicated in flash
    const int l_nr_msgs_d = 10;
    for (int i = 0; i < l_nr_msgs_d; i ++)
    {
        char                        l_msg_ac[16];
        snprintf (l_msg_ac, sizeof (l_msg_ac), "test(%03d)", i); //9+1=10 bytes
        size_t l_len_ud = strlen(l_msg_ac);
        hl_blocks_msg_seq_t l_write_seq_ud = 0;
        if (hl_blocks_r_write (
                l_blocks_pz,
                l_msg_ac, l_len_ud + 1,
                &l_write_seq_ud)
                != 0)
            return ERROR(-1,
                "failed to write msg");
    }/*for each messages to write*/

    //read messages from blk[0] and part of blk[1]
    for (int i = 0; i < 5; i ++)
    {
        char                        l_exp_msg_ac[16];
        snprintf (l_exp_msg_ac, sizeof (l_exp_msg_ac), "test(%03d)", i); //9+1=10 bytes
        size_t l_exp_len_ud = strlen(l_exp_msg_ac);

        char                        l_buf_ac[100];
        size_t                      l_read_size_ud = 0;
        hl_blocks_msg_seq_t         l_read_seq_ud = 0;
        if (hl_blocks_r_read (
                l_blocks_pz,
                l_buf_ac, sizeof (l_buf_ac),
                &l_read_size_ud,
                &l_read_seq_ud)
                != 0)
            return ERROR (-1, "failed to read");

        ASSERT_INT_EQ (i + 1, l_read_seq_ud);
        ASSERT_INT_EQ (l_exp_len_ud + 1, l_read_size_ud);
        ASSERT_STR_EQ (l_exp_msg_ac, l_buf_ac);
    }/*for each message*/

    //now shutdown with remaining messages in blk[1] and heap writes to blk[2]
    //then cold start on that
    ASSERT_INT_EQ (2, hl_blocks_r___get_write_count (l_blocks_pz));
    if (hl_blocks_r_sync (l_blocks_pz) != 0)
        return ERROR(-1, "Failed to sync on shutdown");
    ASSERT_INT_EQ (3, hl_blocks_r___get_write_count (l_blocks_pz));
    l_blocks_pz = NULL; //after shutdown, that instance is no longer

    //now open blocks based on that same memory contents
    if (hl_blocks_r_open (
                128,    //from above
                4,    //from above
                128,    //from above
                32,
                m_r_block_write,
                m_r_block_addr,
                &l_blocks_pz)
                != 0)
        return ERROR(-1,
            "failed to open blocks for cold start");

    //read remaining messages (all from blk[1] and blk[2])
    for (int i = 4; i < l_nr_msgs_d; i ++)
    {
        char                        l_exp_msg_ac[16];
        snprintf (l_exp_msg_ac, sizeof (l_exp_msg_ac), "test(%03d)", i); //9+1=10 bytes
        size_t l_exp_len_ud = strlen(l_exp_msg_ac);

        char                        l_buf_ac[100];
        size_t                      l_read_size_ud = 0;
        hl_blocks_msg_seq_t         l_read_seq_ud = 0;
        if (hl_blocks_r_read (
                l_blocks_pz,
                l_buf_ac, sizeof (l_buf_ac),
                &l_read_size_ud,
                &l_read_seq_ud)
                != 0)
            return ERROR (-1, "failed to read");

        ASSERT_INT_EQ (i + 1, l_read_seq_ud);
        ASSERT_INT_EQ (l_exp_len_ud + 1, l_read_size_ud);
        ASSERT_STR_EQ (l_exp_msg_ac, l_buf_ac);
    }/*for each message*/
    ASSERT_INT_EQ (0, hl_blocks_r___get_write_count (l_blocks_pz));
    ASSERT_NOTHING_MORE_TO_READ (l_blocks_pz);
    return m_r_cleanup (&l_blocks_pz);
}//TEST()

TEST(cold_start_with_large_messages_in_later_blocks) {
    START(
        80,     //block size
        4,      //nr of blocks
        80,     //max message size
        16);    //min data per message part

    const uint32_t l_test_msg_len_ud = 50;

    //write large messages running into blk[1],
    //next starting at offset in blk[1] into blk[2]
    //read first
    //then cold start and see that skip over last part of read first message
    //to start reading with 2nd complete message
    const int l_nr_msgs_d = 2;
    for (int i = 0; i < l_nr_msgs_d; i ++)
    {
        char                        l_msg_ac[100];
        m_r_make_test_msg (l_msg_ac, sizeof (l_msg_ac), i, l_test_msg_len_ud);
        size_t l_len_ud = strlen(l_msg_ac);
        hl_blocks_msg_seq_t l_write_seq_ud = 0;
        if (hl_blocks_r_write (
                l_blocks_pz,
                l_msg_ac, l_len_ud + 1,
                &l_write_seq_ud)
                != 0)
            return ERROR(-1,
                "failed to write msg");
    }/*for each messages to write*/

    //read only 1 message from blk[0] and part of blk[1]
    for (int i = 0; i < 1; i ++)
    {
        char                        l_exp_msg_ac[100];
        m_r_make_test_msg (l_exp_msg_ac, sizeof (l_exp_msg_ac), i, l_test_msg_len_ud);
        size_t l_exp_len_ud = strlen(l_exp_msg_ac);

        char                        l_buf_ac[100];
        size_t                      l_read_size_ud = 0;
        hl_blocks_msg_seq_t         l_read_seq_ud = 0;
        if (hl_blocks_r_read (
                l_blocks_pz,
                l_buf_ac, sizeof (l_buf_ac),
                &l_read_size_ud,
                &l_read_seq_ud)
                != 0)
            return ERROR (-1, "failed to read");

        ASSERT_INT_EQ (i + 1, l_read_seq_ud);
        ASSERT_INT_EQ (l_exp_len_ud + 1, l_read_size_ud);
        ASSERT_STR_EQ (l_exp_msg_ac, l_buf_ac);
    }/*for each message*/

    //now shutdown with remaining messages in blk[1] and heap writes to blk[2]
    //then cold start on that
    if (hl_blocks_r_sync (l_blocks_pz) != 0)
        return ERROR(-1, "Failed to sync on shutdown");

    l_blocks_pz = NULL; //after shutdown, that instance is no longer

    //now open blocks based on that same memory contents
    if (hl_blocks_r_open (
                l_block_size_ud,
                l_nr_blocks_ud,
                l_block_size_ud,
                l_min_part_size_ud,
                m_r_block_write,
                m_r_block_addr,
                &l_blocks_pz)
                != 0)
        return ERROR(-1,
            "failed to open blocks for cold start");

    //read remaining messages (all from blk[1] and blk[2])
    for (int i = 1; i < l_nr_msgs_d; i ++)
    {
        char                        l_exp_msg_ac[100];
        m_r_make_test_msg (l_exp_msg_ac, sizeof (l_exp_msg_ac), i, l_test_msg_len_ud);
        size_t l_exp_len_ud = strlen(l_exp_msg_ac);

        char                        l_buf_ac[100];
        size_t                      l_read_size_ud = 0;
        hl_blocks_msg_seq_t         l_read_seq_ud = 0;
        if (hl_blocks_r_read (
                l_blocks_pz,
                l_buf_ac, sizeof (l_buf_ac),
                &l_read_size_ud,
                &l_read_seq_ud)
                != 0)
            return ERROR (-1, "failed to read");

        ASSERT_INT_EQ (i + 1, l_read_seq_ud);
        ASSERT_INT_EQ (l_exp_len_ud + 1, l_read_size_ud);
        ASSERT_STR_EQ (l_exp_msg_ac, l_buf_ac);
    }/*for each message*/
    ASSERT_NOTHING_MORE_TO_READ (l_blocks_pz);
    return m_r_cleanup (&l_blocks_pz);
}//TEST()

TEST(write_and_read_over_buffer_rotation) {
    START(
        128,    //block size
        4,      //nr of blocks
        128,    //max message size
        16);    //min data per message part

    const uint32_t l_test_msg_len_ud = 50;

    //write into all blocks, not yet rotating, because there would be no space to rotate
    int l_next_wr_id_d = 0;
    int l_next_rd_id_d = 0;
    for (int i = 0; i < 3; i ++)
    {
        char                        l_msg_ac[100];
        m_r_make_test_msg (l_msg_ac, sizeof (l_msg_ac), l_next_wr_id_d, l_test_msg_len_ud);
        size_t l_len_ud = strlen(l_msg_ac);
        hl_blocks_msg_seq_t l_write_seq_ud = 0;
        if (hl_blocks_r_write (
                l_blocks_pz,
                l_msg_ac, l_len_ud + 1,
                &l_write_seq_ud)
                != 0)
            return ERROR(-1,
                "failed to write msg[%d]", l_next_wr_id_d);

        l_next_wr_id_d ++;
    }/*for each messages to write*/

    //now read and write, which release some space then write more
    for (int i = 0; i < 100; i ++)
    {
        //read one message and verify
        {
            char                        l_buf_ac[100];
            size_t                      l_read_size_ud = 0;
            hl_blocks_msg_seq_t         l_read_seq_ud = 0;
            if (hl_blocks_r_read (
                    l_blocks_pz,
                    l_buf_ac, sizeof (l_buf_ac),
                    &l_read_size_ud,
                    &l_read_seq_ud)
                    != 0)
                return ERROR (-1,
                    "failed to read msg[%d]", l_next_rd_id_d);

            char                        l_exp_msg_ac[100];
            m_r_make_test_msg (l_exp_msg_ac, sizeof (l_exp_msg_ac), l_next_rd_id_d, l_test_msg_len_ud);
            size_t l_exp_len_ud = strlen(l_exp_msg_ac);
            ASSERT_INT_EQ (l_next_rd_id_d + 1, l_read_seq_ud);
            ASSERT_INT_EQ (l_exp_len_ud + 1, l_read_size_ud);
            ASSERT_STR_EQ (l_exp_msg_ac, l_buf_ac);
            l_next_rd_id_d ++;
        }/*scope*/

        //write another message
        char                        l_msg_ac[100];
        m_r_make_test_msg (l_msg_ac, sizeof (l_msg_ac), l_next_wr_id_d, l_test_msg_len_ud);
        size_t l_len_ud = strlen(l_msg_ac);
        hl_blocks_msg_seq_t l_write_seq_ud = 0;
        if (hl_blocks_r_write (
                l_blocks_pz,
                l_msg_ac, l_len_ud + 1,
                &l_write_seq_ud)
                != 0)
            return ERROR(-1,
                "failed to write msg[%d]", l_next_wr_id_d);

        l_next_wr_id_d ++;
    }/*for each messages to write*/

    //now close and do a cold start then continue
    DEBUG ("*** CLOSING ***");
    if (hl_blocks_r_close (&l_blocks_pz) != 0)
        return ERROR (-1,
            "Failed to close before cold start");

    DEBUG ("*** COLD START ***");
    l_blocks_pz = NULL; //after shutdown, that instance is no longer

    //now open blocks based on that same memory contents
    if (hl_blocks_r_open (
                128,
                4,
                128,
                16,
                m_r_block_write,
                m_r_block_addr,
                &l_blocks_pz)
                != 0)
        return ERROR(-1,
            "failed to open blocks for cold start");

    DEBUG ("*** CONTINUE ***");
    //now read and write, which release some space then write more
    for (int i = 0; i < 100; i ++)
    {
        //read one message and verify
        {
            char                        l_buf_ac[100];
            size_t                      l_read_size_ud = 0;
            hl_blocks_msg_seq_t         l_read_seq_ud = 0;
            if (hl_blocks_r_read (
                    l_blocks_pz,
                    l_buf_ac, sizeof (l_buf_ac),
                    &l_read_size_ud,
                    &l_read_seq_ud)
                    != 0)
                return ERROR (-1,
                    "failed to read msg[%d]", l_next_rd_id_d);

            char                        l_exp_msg_ac[100];
            m_r_make_test_msg (l_exp_msg_ac, sizeof (l_exp_msg_ac), l_next_rd_id_d, l_test_msg_len_ud);
            size_t l_exp_len_ud = strlen(l_exp_msg_ac);

            //the first message(s) after cold start, may already be read when the block
            //was not completely read before shut down
            //if so, skip them...
            if (l_read_seq_ud < l_next_rd_id_d + 1) {
                DEBUG ("Skip seq[%u] already read before shutdown.", l_read_seq_ud);
                continue;
            }

            ASSERT_INT_EQ (l_next_rd_id_d + 1, l_read_seq_ud);
            ASSERT_INT_EQ (l_exp_len_ud + 1, l_read_size_ud);
            ASSERT_STR_EQ (l_exp_msg_ac, l_buf_ac);
            l_next_rd_id_d ++;
        }/*scope*/

        //write another message
        char                        l_msg_ac[100];
        m_r_make_test_msg (l_msg_ac, sizeof (l_msg_ac), l_next_wr_id_d, l_test_msg_len_ud);
        size_t l_len_ud = strlen(l_msg_ac);
        hl_blocks_msg_seq_t l_write_seq_ud = 0;
        if (hl_blocks_r_write (
                l_blocks_pz,
                l_msg_ac, l_len_ud + 1,
                &l_write_seq_ud)
                != 0)
            return ERROR(-1,
                "failed to write msg[%d]", l_next_wr_id_d);

        l_next_wr_id_d ++;
    }/*for each messages to write*/

    //now read the last written message until the buffer is empty
    while (l_next_rd_id_d != l_next_wr_id_d)
    {
        //read one message and verify
        {
            char                        l_buf_ac[100];
            size_t                      l_read_size_ud = 0;
            hl_blocks_msg_seq_t         l_read_seq_ud = 0;
            if (hl_blocks_r_read (
                    l_blocks_pz,
                    l_buf_ac, sizeof (l_buf_ac),
                    &l_read_size_ud,
                    &l_read_seq_ud)
                    != 0)
                return ERROR (-1,
                    "failed to read msg[%d]", l_next_rd_id_d);

            char                        l_exp_msg_ac[100];
            m_r_make_test_msg (l_exp_msg_ac, sizeof (l_exp_msg_ac), l_next_rd_id_d, l_test_msg_len_ud);
            size_t l_exp_len_ud = strlen(l_exp_msg_ac);
            ASSERT_INT_EQ (l_next_rd_id_d + 1, l_read_seq_ud);
            ASSERT_INT_EQ (l_exp_len_ud + 1, l_read_size_ud);
            ASSERT_STR_EQ (l_exp_msg_ac, l_buf_ac);
            l_next_rd_id_d ++;
        }/*scope*/
    }/*while emptying*/

    ASSERT_NOTHING_MORE_TO_READ (l_blocks_pz);
    return m_r_cleanup (&l_blocks_pz);
}//TEST()


//when reaching end of buffer, must be able to handle errors and continue
TEST(write_too_much_but_continue_safely) {
    START(
        128,    //block size
        4,      //nr of blocks
        128,    //max message size
        16);    //min data per message part

    const uint32_t l_test_msg_len_ud = 50;

    //write into all blocks, not yet rotating, because there would be no space to rotate
    int l_next_wr_id_d = 0;
    int l_next_rd_id_d = 0;
    for (int i = 0; i < 100; i ++)
    {
        //write until run out of space
        while (1)
        {
            char                        l_msg_ac[100];
            m_r_make_test_msg (l_msg_ac, sizeof (l_msg_ac), l_next_wr_id_d, l_test_msg_len_ud);
            size_t l_len_ud = strlen(l_msg_ac);
            hl_blocks_msg_seq_t l_write_seq_ud = 0;
            int l_write_result_d = hl_blocks_r_write (
                    l_blocks_pz,
                    l_msg_ac, l_len_ud + 1,
                    &l_write_seq_ud);
            if (l_write_result_d != 0)
            {
                if (l_write_result_d == HL_BLOCKS_K_ERROR_NO_SPACE_LEFT_IN_BUFFER)
                    break;

                return ERROR(-1,
                    "failed to write msg[%d] with result=%d != ",
                    l_next_wr_id_d,
                    l_write_result_d,
                    HL_BLOCKS_K_ERROR_NO_SPACE_LEFT_IN_BUFFER);
            }
            l_next_wr_id_d ++;
        }//while can write

        //read one message and verify
        {
            char                        l_buf_ac[100];
            size_t                      l_read_size_ud = 0;
            hl_blocks_msg_seq_t         l_read_seq_ud = 0;
            if (hl_blocks_r_read (
                    l_blocks_pz,
                    l_buf_ac, sizeof (l_buf_ac),
                    &l_read_size_ud,
                    &l_read_seq_ud)
                    != 0)
                return ERROR (-1,
                    "failed to read msg[%d]", l_next_rd_id_d);

            char                        l_exp_msg_ac[100];
            m_r_make_test_msg (l_exp_msg_ac, sizeof (l_exp_msg_ac), l_next_rd_id_d, l_test_msg_len_ud);
            size_t l_exp_len_ud = strlen(l_exp_msg_ac);
            ASSERT_INT_EQ (l_next_rd_id_d + 1, l_read_seq_ud);
            ASSERT_INT_EQ (l_exp_len_ud + 1, l_read_size_ud);
            ASSERT_STR_EQ (l_exp_msg_ac, l_buf_ac);
            l_next_rd_id_d ++;
        }/*scope*/
    }/*for each messages to write*/

    //now close and do a cold start then continue
    DEBUG ("*** CLOSING ***");
    if (hl_blocks_r_close (&l_blocks_pz) != 0)
        return ERROR (-1,
            "Failed to close before cold start");

    DEBUG ("*** COLD START ***");
    l_blocks_pz = NULL; //after shutdown, that instance is no longer

    //now open blocks based on that same memory contents
    if (hl_blocks_r_open (
                128,
                4,
                128,
                16,
                m_r_block_write,
                m_r_block_addr,
                &l_blocks_pz)
                != 0)
        return ERROR(-1,
            "failed to open blocks for cold start");

    DEBUG ("*** CONTINUE ***");
    //now read and write, which release some space then write more
    for (int i = 0; i < 100; i ++)
    {
        //read one message and verify
        {
            char                        l_buf_ac[100];
            size_t                      l_read_size_ud = 0;
            hl_blocks_msg_seq_t         l_read_seq_ud = 0;
            if (hl_blocks_r_read (
                    l_blocks_pz,
                    l_buf_ac, sizeof (l_buf_ac),
                    &l_read_size_ud,
                    &l_read_seq_ud)
                    != 0)
                return ERROR (-1,
                    "failed to read msg[%d]", l_next_rd_id_d);

            char                        l_exp_msg_ac[100];
            m_r_make_test_msg (l_exp_msg_ac, sizeof (l_exp_msg_ac), l_next_rd_id_d, l_test_msg_len_ud);
            size_t l_exp_len_ud = strlen(l_exp_msg_ac);

            //the first message(s) after cold start, may already be read when the block
            //was not completely read before shut down
            //if so, skip them...
            if (l_read_seq_ud < l_next_rd_id_d + 1) {
                DEBUG ("Skip seq[%u] already read before shutdown.", l_read_seq_ud);
                continue;
            }

            ASSERT_INT_EQ (l_next_rd_id_d + 1, l_read_seq_ud);
            ASSERT_INT_EQ (l_exp_len_ud + 1, l_read_size_ud);
            ASSERT_STR_EQ (l_exp_msg_ac, l_buf_ac);
            l_next_rd_id_d ++;
        }/*scope*/

        //write another message
        char                        l_msg_ac[100];
        m_r_make_test_msg (l_msg_ac, sizeof (l_msg_ac), l_next_wr_id_d, l_test_msg_len_ud);
        size_t l_len_ud = strlen(l_msg_ac);
        hl_blocks_msg_seq_t l_write_seq_ud = 0;
        if (hl_blocks_r_write (
                l_blocks_pz,
                l_msg_ac, l_len_ud + 1,
                &l_write_seq_ud)
                != 0)
            return ERROR(-1,
                "failed to write msg[%d]", l_next_wr_id_d);

        l_next_wr_id_d ++;
    }/*for each messages to write*/

    //now read the last written message until the buffer is empty
    while (l_next_rd_id_d != l_next_wr_id_d)
    {
        //read one message and verify
        {
            char                        l_buf_ac[100];
            size_t                      l_read_size_ud = 0;
            hl_blocks_msg_seq_t         l_read_seq_ud = 0;
            if (hl_blocks_r_read (
                    l_blocks_pz,
                    l_buf_ac, sizeof (l_buf_ac),
                    &l_read_size_ud,
                    &l_read_seq_ud)
                    != 0)
                return ERROR (-1,
                    "failed to read msg[%d]", l_next_rd_id_d);

            char                        l_exp_msg_ac[100];
            m_r_make_test_msg (l_exp_msg_ac, sizeof (l_exp_msg_ac), l_next_rd_id_d, l_test_msg_len_ud);
            size_t l_exp_len_ud = strlen(l_exp_msg_ac);
            ASSERT_INT_EQ (l_next_rd_id_d + 1, l_read_seq_ud);
            ASSERT_INT_EQ (l_exp_len_ud + 1, l_read_size_ud);
            ASSERT_STR_EQ (l_exp_msg_ac, l_buf_ac);
            l_next_rd_id_d ++;
        }/*scope*/
    }/*while emptying*/

    ASSERT_NOTHING_MORE_TO_READ (l_blocks_pz);
    return m_r_cleanup (&l_blocks_pz);
}//TEST()


static int m_r_start (
    const uint32_t                    p_block_size_ud,
    const uint32_t                    p_nr_blocks_ud,
    const uint32_t                    p_max_msg_size_ud,
    const uint32_t                    p_min_data_per_part_ud,
          hl_blocks_t**               p_block_ppz)
{
    /*
     * start with clean block of memory
     */
    m_d_block_size_ud       = p_block_size_ud;
    m_d_nr_blocks_ud        = p_nr_blocks_ud;
    m_d_max_msg_size_ud     = p_max_msg_size_ud;
    m_d_mock_flash_mem_auc  = (unsigned char*)malloc (m_d_block_size_ud * m_d_nr_blocks_ud);
    hl_blocks_t*                l_blocks_pz = NULL;
    if (hl_blocks_r_open (
                m_d_block_size_ud,
                m_d_nr_blocks_ud,
                m_d_max_msg_size_ud,
                p_min_data_per_part_ud,
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

static void m_r_make_test_msg (
          void*                       p_buff_data_p,
    const size_t                      p_buff_size_ud,
    const uint32_t                    p_msg_id_ud,       //a number printed into the start of the message
    const uint32_t                    p_test_msg_len_ud) //how long the message must be
{
    if (p_test_msg_len_ud + 1 > p_buff_size_ud)
    {
        ERROR_LOG("Buffer size=%u too small for test message len=%u",
                p_buff_size_ud,
                p_test_msg_len_ud);
        exit (1);
    }

    uint32_t l_len_ud = (uint32_t)snprintf (
        (char*)p_buff_data_p, MIN (p_buff_size_ud, p_test_msg_len_ud + 1),
        "test(%03u)",
        p_msg_id_ud);
    while (l_len_ud < p_test_msg_len_ud)
    {
        *(((char*)p_buff_data_p) + l_len_ud) = l_len_ud % 10 + '0';
        l_len_ud ++;
    }
    *(((char*)p_buff_data_p) + l_len_ud) = '\0';
    return;
}//m_r_make_test_msg()
