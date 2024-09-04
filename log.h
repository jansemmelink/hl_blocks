#ifndef _LOG_H_
#define _LOG_H_

#include <inttypes.h>

#define ERROR_LOG(format, args...)   log_r_write (__FILE__, __LINE__, LOG_K_LEVEL_ERROR, format, ##args)
#define WARNING(format, args...) log_r_write (__FILE__, __LINE__, LOG_K_LEVEL_WARNING, format, ##args)
#define INFO(format, args...)    log_r_write (__FILE__, __LINE__, LOG_K_LEVEL_INFO, format, ##args)
#define DEBUG(format, args...)   log_r_write (__FILE__, __LINE__, LOG_K_LEVEL_DEBUG, format, ##args)
#define TRACE(format, args...)   log_r_write (__FILE__, __LINE__, LOG_K_LEVEL_TRACE, format, ##args)

#define DEBUG_HEX(title, data, size)    log_r_hex(__FILE__, __LINE__, LOG_K_LEVEL_DEBUG, title, data, size)

typedef enum log_level_enum_s {
    LOG_K_LEVEL_ERROR,
    LOG_K_LEVEL_WARNING,
    LOG_K_LEVEL_INFO,
    LOG_K_LEVEL_DEBUG,
    LOG_K_LEVEL_TRACE,
} log_level_e;

extern void log_r_write (
    const char*                       p_file_pc,
    const int                         p_line_d,
    const log_level_e                 p_level_e,
    const char*                       p_format_pc,
          ...);

extern void log_r_hex (
    const char*                       p_file_pc,
    const int                         p_line_d,
    const log_level_e                 p_level_e,
    const char*                       p_title_pc,
    const void*                       p_data_p,
    const uint32_t                    p_size_ud);

extern const char* log_r_level_text (
    const log_level_e                 p_level_e);

#endif /*_LOG_H_*/
