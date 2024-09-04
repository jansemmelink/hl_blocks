#include "error_stack.h"
#include <stdarg.h>
#include <string.h>

#define M_STACK_ENTRY_FILE_SIZE       120   //128-4-4
#define M_STACK_ENTRY_TEXT_SIZE       128


// maximum stack entries allowed
// when reached, new entry just replace the top entry
// not to lose deepest entries which indicate where problem started
#define M_STACK_MAX_DEPTH             10

/*****************************************************************************
 *   L O C A L   D A T A   T Y P E   D E F I N I T I O N S
 *****************************************************************************/

typedef struct m_stack_entry_s {
    // bytes 0..128
    char                        file_ac[M_STACK_ENTRY_FILE_SIZE];
    int                         line_d;
    int                         code_d;
    // bytes 128..256
    char                        text_ac[M_STACK_ENTRY_TEXT_SIZE];
} m_stack_entry_t;

// error stack must be initialised for each thread to use its own stack
struct error_stack_s {
    unsigned int                depth_ud; //nr of entries
    m_stack_entry_t             entry_az[M_STACK_MAX_DEPTH];
};


/*****************************************************************************
 *   L O C A L   D A T A    D E F I N I T I O N S
 *****************************************************************************/

static error_stack_t*           m_d_error_stack_pz = NULL;


extern void error_stack_r_init (void)
{
    m_d_error_stack_pz = (error_stack_t*)malloc (sizeof (error_stack_t));
    if (m_d_error_stack_pz == NULL) {
        fprintf (stderr, "error_stack_r_init: out of memory\n");
        exit (1);
    }
}/*error_stack_r_init()*/


extern int success (void) {
    if (m_d_error_stack_pz != NULL) {
        m_d_error_stack_pz->depth_ud = 0;
    }
    return 0;
}/*success()*/


extern int error (
    const char*                       p_file_pc,
    const int                         p_line_d,
    const int                         p_code_d,
    const char*                       p_format_pc,
          ...)
{
    if (  (p_file_pc == NULL)
       || (p_line_d <= 0)
       || (p_code_d == 0)
       || (p_format_pc == NULL)) {
        fprintf (stderr, "error: invalid parameter (file=%s, line=%d, code=%d, format=%s)\n",
                (p_file_pc == NULL ? "NULL" : p_file_pc),
                p_line_d,
                p_code_d,
                (p_format_pc == NULL ? "NULL" : p_format_pc));
        return p_code_d;
    }

    if (m_d_error_stack_pz == NULL) {
        error_stack_r_init ();
    }

    // get next stack entry, not exceeding depth
    if (m_d_error_stack_pz->depth_ud >= M_STACK_MAX_DEPTH) {
        m_d_error_stack_pz->depth_ud = M_STACK_MAX_DEPTH - 1;
    }
    m_stack_entry_t*            l_entry_pz = &m_d_error_stack_pz->entry_az[m_d_error_stack_pz->depth_ud];

    // write the file name, using the last part if too long
    size_t                      l_file_len_ud = strlen (p_file_pc);
    size_t                      l_file_ofs_ud = 0;
    if (l_file_len_ud > M_STACK_ENTRY_FILE_SIZE - 1) {
        l_file_ofs_ud = l_file_len_ud - M_STACK_ENTRY_FILE_SIZE;
    }
    strlcpy (
        l_entry_pz->file_ac,
        p_file_pc + l_file_ofs_ud,
        sizeof (l_entry_pz->file_ac));

    l_entry_pz->line_d = p_line_d;
    l_entry_pz->code_d = p_code_d;

    // write the formatted text, truncating if too long
    va_list                     l_va_list_z;
    va_start (l_va_list_z, p_format_pc);
    vsnprintf(l_entry_pz->text_ac, sizeof (l_entry_pz->text_ac), p_format_pc, l_va_list_z);
    va_end(l_va_list_z);

    m_d_error_stack_pz->depth_ud++;
    return p_code_d;
}/*error()*/


extern void error_stack_r_print (
          FILE*                       p_file_fp)
{
    if (m_d_error_stack_pz == NULL) {
        fprintf (p_file_fp, "No error stack to print\n");
        return;
    }

    // print the stack entries from last to first
    unsigned int                l_depth_ud = m_d_error_stack_pz->depth_ud;
    while (l_depth_ud > 0) {
        l_depth_ud --;

        const m_stack_entry_t*      l_entry_pz = &m_d_error_stack_pz->entry_az[l_depth_ud];
        fprintf (p_file_fp, "  ERR[%d]: %s(%d): %s\n",
            l_depth_ud,
            l_entry_pz->file_ac,
            l_entry_pz->line_d,
            l_entry_pz->text_ac);
    }/*while step through entries*/
}/*error_stack_r_print()*/
