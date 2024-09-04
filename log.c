#include "log.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

extern void log_r_write (
    const char*                       p_file_pc,
    const int                         p_line_d,
    const log_level_e                 p_level_e,
    const char*                       p_format_pc,
          ...)
{
    fprintf (stderr, "%5.5s %30.30s(%5d): ",
        log_r_level_text (p_level_e),
        p_file_pc,
        p_line_d);

    va_list                     l_va_list_z;
    va_start (l_va_list_z, p_format_pc);
    vfprintf(stderr, p_format_pc, l_va_list_z);
    va_end(l_va_list_z);

    fprintf (stderr, "\n");
    return;
}/*log_r_write()*/

extern void log_r_hex (
    const char*                       p_file_pc,
    const int                         p_line_d,
    const log_level_e                 p_level_e,
    const char*                       p_title_pc,
    const void*                       p_data_p,
    const uint32_t                    p_size_ud)
{
    if (  (p_title_pc == NULL)
       || (p_data_p == NULL))
        return;

    fprintf (stderr, "%5.5s %30.30s(%5d): %s (%u bytes):\n",
        log_r_level_text (p_level_e),
        p_file_pc,
        p_line_d,
        p_title_pc,
        p_size_ud);

    uint32_t                    l_ofs_ud = 0;
    uint32_t                    l_rem_ud = p_size_ud;
    while (l_rem_ud > 0) {
        fprintf (stderr, "%5.5s %30.30s(%5d): [%08x..%08x]",
            log_r_level_text (p_level_e),
            p_file_pc,
            p_line_d,
            l_ofs_ud,
            l_ofs_ud + 15);

        for (uint32_t i = 0; i < 16; i ++) {
            if (i < l_rem_ud) {
                const unsigned char l_byte_uc = *((const unsigned char*)p_data_p + l_ofs_ud + i);
                fprintf (stderr, " %02X", l_byte_uc);
            } else {
                fprintf (stderr, " ..");
            }
        }
        fprintf (stderr, " | ");
        for (uint32_t i = 0; ((i < 16) && (i < l_rem_ud)); i ++) {
            const unsigned char l_byte_uc = *((const unsigned char*)p_data_p + l_ofs_ud + i);
            if (isprint (l_byte_uc)) {
                fprintf (stderr, "%c", (const char)l_byte_uc);
            } else {
                fprintf (stderr, ".");
            }
        }
        fprintf (stderr, "\n");

        if (l_rem_ud <= 16) {
            break;
        }

        l_ofs_ud += 16;
        l_rem_ud -= 16;
    }//while more data
}/*log_r_hex()*/

extern const char* log_r_level_text (
    const log_level_e                 p_level_e)
{
    switch (p_level_e)
    {
        case LOG_K_LEVEL_ERROR:
            return "ERROR";
        case LOG_K_LEVEL_WARNING:
            return "WARNING";
        case LOG_K_LEVEL_INFO:
            return "INFO";
        case LOG_K_LEVEL_DEBUG:
            return "DEBUG";
        case LOG_K_LEVEL_TRACE:
            return "TRACE";
        default:
            return "UNKNOWN";
    }
}/*log_r_level_text()*/
