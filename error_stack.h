#ifndef _ERROR_STACK_H_
#define _ERROR_STACK_H_

/*****************************************************************************
 * I N C L U D E D   H E A D E R   F I L E S
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>


/*****************************************************************************
 * P U B L I C   D A T A   T Y P E   D E F I N I T I O N S
 *****************************************************************************/

typedef struct error_stack_s error_stack_t;


/*****************************************************************************
 * P U B L I C   M A C R O   D E F I N I T I O N S
 *****************************************************************************/

//usage:   return SUCCESS();
#define SUCCESS success

//usage:   return ERROR(-1, "data size %zu > max %u", p_data_size_ud, M_MAX_SIZE);
#define ERROR(code, format, args...) \
    error(__FILE__, __LINE__, code, format, ## args)


/*****************************************************************************
 * P U B L I C   F U N C T I O N   D E C L A R A T I O N S
 *****************************************************************************/

// allocates an error stack for this thread
// call at start of main and each child thread
// it sets static data to use for the rest of the thread
// it will exit the process if no more memory is available
extern void error_stack_r_init (void);

// clear the error stack and return 0
extern int success (void);

// add to the error stack and return the specified code,
// which must be != 0, usually -1 unless your code mean something
extern int error (
    const char*                       p_file_pc,
    const int                         p_line_d,
    const int                         p_code_d,
    const char*                       p_format_pc,
          ...);

extern void error_stack_r_print (
          FILE*                       p_file_fp);

#endif /*_ERROR_STACK_H_*/
