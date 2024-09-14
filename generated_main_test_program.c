#include <stdio.h>
#include <string.h>
#include "error_stack.h"

// include test files:
#include "test_hl_qspi_mem.c"

static int m_r_must_run_test (
    const int argc,
    const char* arg_apc[],
    const char* p_test_name_pc)
{
    if (argc < 2)
    {
        return 1;   //no args, run all tests
    }
    for (int i = 1; i < argc; i ++) {
        if (strstr (p_test_name_pc, arg_apc[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

// main test function to run all tests
int main(int argc, const char* arg_apc[]) {
    //calling all tests:
    if (m_r_must_run_test (argc, arg_apc, "test_r_write_first_small_and_read_from_heap")) {
        printf("\n\n===== TEST: test_r_write_first_small_and_read_from_heap ======\n");
        if (test_r_write_first_small_and_read_from_heap() != 0)
        {
            printf ("test_r_write_first_small_and_read_from_heap FAILED.\n");
            error_stack_r_print (stderr);
            exit (1);
        } else {
            printf ("test_r_write_first_small_and_read_from_heap PASSED.\n");
        }
    }
    
    if (m_r_must_run_test (argc, arg_apc, "test_r_write_two_messages_then_read_both")) {
        printf("\n\n===== TEST: test_r_write_two_messages_then_read_both ======\n");
        if (test_r_write_two_messages_then_read_both() != 0)
        {
            printf ("test_r_write_two_messages_then_read_both FAILED.\n");
            error_stack_r_print (stderr);
            exit (1);
        } else {
            printf ("test_r_write_two_messages_then_read_both PASSED.\n");
        }
    }
    
    if (m_r_must_run_test (argc, arg_apc, "test_r_first_msg_span_into_2nd_block")) {
        printf("\n\n===== TEST: test_r_first_msg_span_into_2nd_block ======\n");
        if (test_r_first_msg_span_into_2nd_block() != 0)
        {
            printf ("test_r_first_msg_span_into_2nd_block FAILED.\n");
            error_stack_r_print (stderr);
            exit (1);
        } else {
            printf ("test_r_first_msg_span_into_2nd_block PASSED.\n");
        }
    }
    
    if (m_r_must_run_test (argc, arg_apc, "test_r_read_small_messages_from_flash")) {
        printf("\n\n===== TEST: test_r_read_small_messages_from_flash ======\n");
        if (test_r_read_small_messages_from_flash() != 0)
        {
            printf ("test_r_read_small_messages_from_flash FAILED.\n");
            error_stack_r_print (stderr);
            exit (1);
        } else {
            printf ("test_r_read_small_messages_from_flash PASSED.\n");
        }
    }
    
    if (m_r_must_run_test (argc, arg_apc, "test_r_cold_start_with_message_at_start_of_first_block")) {
        printf("\n\n===== TEST: test_r_cold_start_with_message_at_start_of_first_block ======\n");
        if (test_r_cold_start_with_message_at_start_of_first_block() != 0)
        {
            printf ("test_r_cold_start_with_message_at_start_of_first_block FAILED.\n");
            error_stack_r_print (stderr);
            exit (1);
        } else {
            printf ("test_r_cold_start_with_message_at_start_of_first_block PASSED.\n");
        }
    }
    
    if (m_r_must_run_test (argc, arg_apc, "test_r_cold_start_with_messages_spanning_two_blocks")) {
        printf("\n\n===== TEST: test_r_cold_start_with_messages_spanning_two_blocks ======\n");
        if (test_r_cold_start_with_messages_spanning_two_blocks() != 0)
        {
            printf ("test_r_cold_start_with_messages_spanning_two_blocks FAILED.\n");
            error_stack_r_print (stderr);
            exit (1);
        } else {
            printf ("test_r_cold_start_with_messages_spanning_two_blocks PASSED.\n");
        }
    }
    
    if (m_r_must_run_test (argc, arg_apc, "test_r_cold_start_with_messages_starting_in_later_blocks")) {
        printf("\n\n===== TEST: test_r_cold_start_with_messages_starting_in_later_blocks ======\n");
        if (test_r_cold_start_with_messages_starting_in_later_blocks() != 0)
        {
            printf ("test_r_cold_start_with_messages_starting_in_later_blocks FAILED.\n");
            error_stack_r_print (stderr);
            exit (1);
        } else {
            printf ("test_r_cold_start_with_messages_starting_in_later_blocks PASSED.\n");
        }
    }
    
    if (m_r_must_run_test (argc, arg_apc, "test_r_cold_start_with_large_messages_in_later_blocks")) {
        printf("\n\n===== TEST: test_r_cold_start_with_large_messages_in_later_blocks ======\n");
        if (test_r_cold_start_with_large_messages_in_later_blocks() != 0)
        {
            printf ("test_r_cold_start_with_large_messages_in_later_blocks FAILED.\n");
            error_stack_r_print (stderr);
            exit (1);
        } else {
            printf ("test_r_cold_start_with_large_messages_in_later_blocks PASSED.\n");
        }
    }
    
    if (m_r_must_run_test (argc, arg_apc, "test_r_write_and_read_over_buffer_rotation")) {
        printf("\n\n===== TEST: test_r_write_and_read_over_buffer_rotation ======\n");
        if (test_r_write_and_read_over_buffer_rotation() != 0)
        {
            printf ("test_r_write_and_read_over_buffer_rotation FAILED.\n");
            error_stack_r_print (stderr);
            exit (1);
        } else {
            printf ("test_r_write_and_read_over_buffer_rotation PASSED.\n");
        }
    }
    
    if (m_r_must_run_test (argc, arg_apc, "test_r_write_too_much_but_continue_safely")) {
        printf("\n\n===== TEST: test_r_write_too_much_but_continue_safely ======\n");
        if (test_r_write_too_much_but_continue_safely() != 0)
        {
            printf ("test_r_write_too_much_but_continue_safely FAILED.\n");
            error_stack_r_print (stderr);
            exit (1);
        } else {
            printf ("test_r_write_too_much_but_continue_safely PASSED.\n");
        }
    }
    
    return SUCCESS();
}/*main*/
