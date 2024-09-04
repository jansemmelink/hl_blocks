#include <stdio.h>
#include "error_stack.h"

// include test files:
#include "test_hl_qspi_mem.c"

// main test function to run all tests
int main() {
    //calling all tests:
    printf("\n\n===== TEST: test_r_write_first_small_and_read_from_heap ======\n");
    if (test_r_write_first_small_and_read_from_heap() != 0)
    {
        printf ("test_r_write_first_small_and_read_from_heap FAILED.\n");
        error_stack_r_print (stderr);
        exit (1);
    } else {
        printf ("test_r_write_first_small_and_read_from_heap PASSED.\n");
    }

    printf("\n\n===== TEST: test_r_write_two_messages_then_read_both ======\n");
    if (test_r_write_two_messages_then_read_both() != 0)
    {
        printf ("test_r_write_two_messages_then_read_both FAILED.\n");
        error_stack_r_print (stderr);
        exit (1);
    } else {
        printf ("test_r_write_two_messages_then_read_both PASSED.\n");
    }

    printf("\n\n===== TEST: test_r_first_msg_span_into_2nd_block ======\n");
    if (test_r_first_msg_span_into_2nd_block() != 0)
    {
        printf ("test_r_first_msg_span_into_2nd_block FAILED.\n");
        error_stack_r_print (stderr);
        exit (1);
    } else {
        printf ("test_r_first_msg_span_into_2nd_block PASSED.\n");
    }

    return SUCCESS();
}/*main*/
