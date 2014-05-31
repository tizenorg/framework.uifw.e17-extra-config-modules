
#ifndef __E_BACKKEY_LOG_H__
#define __E_BACKKEY_LOG_H__

#include <stdio.h>
#include <dlog.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG            "backkey"
#define BACKKEY_DEBUG_MODE 0

/* anci c color type */
#define FONT_COLOR_RESET   "\033[0m"
#define FONT_COLOR_RED     "\033[31m"
#define FONT_COLOR_GREEN   "\033[32m"
#define FONT_COLOR_YELLOW  "\033[33m"
#define FONT_COLOR_BLUE    "\033[34m"
#define FONT_COLOR_PURPLE  "\033[35m"
#define FONT_COLOR_CYAN    "\033[36m"
#define FONT_COLOR_GRAY    "\033[37m"

#define PRINT_FUNC_LINE
#if BACKKEY_DEBUG_MODE
#ifdef PRINT_FUNC_LINE
#define BACKKEY_DEBUG(fmt, args ...) LOGD("[%s : <%05d>] " fmt, __func__, __LINE__, ##args)
#define BACKKEY_WARN(fmt, args ...)  LOGW(FONT_COLOR_YELLOW "[%s : <%05d>] " fmt FONT_COLOR_RESET, __func__, __LINE__, ##args)
#define BACKKEY_ERROR(fmt, args ...) LOGE(FONT_COLOR_RED "[%s : <%05d>] ####" fmt "####"FONT_COLOR_RESET, __func__, __LINE__, ##args)
#define BACKKEY_FUNC_ENTER()         LOGD(FONT_COLOR_BLUE "[%s : <%05d>] <<<< enter!!"FONT_COLOR_RESET, __func__, __LINE__)
#define BACKKEY_FUNC_LEAVE()         LOGD(FONT_COLOR_BLUE "[%s : <%05d>] >>>> leave!!"FONT_COLOR_RESET, __func__, __LINE__)
#else
#define BACKKEY_DEBUG(fmt, args ...) LOGD("[AT] " fmt "\n", ##args)
#define BACKKEY_WARN(fmt, args ...)  LOGW(FONT_COLOR_YELLOW "[AT] " fmt "\n"FONT_COLOR_RESET, ##args)
#define BACKKEY_ERROR(fmt, args ...) LOGE(FONT_COLOR_RED "[AT] " fmt "\n"FONT_COLOR_RESET, ##args)
#define BACKKEY_FUNC_ENTER()         LOGD(FONT_COLOR_BLUE "*********************** [Func: %s] enter!! ********************* \n"FONT_COLOR_RESET, __func__)
#define BACKKEY_FUNC_LEAVE()         LOGD(FONT_COLOR_BLUE "*********************** [Func: %s] leave!! ********************* \n"FONT_COLOR_RESET, __func__)
#endif
#else
#ifdef BACKKEY_DEBUG
        #undef BACKKEY_DEBUG
#endif
#define BACKKEY_DEBUG(fmt, args ...)
#ifdef BACKKEY_WARN
        #undef BACKKEY_WARN
#endif
#define BACKKEY_WARN(fmt, args ...)
#ifdef BACKKEY_FUNC_ENTER
        #undef BACKKEY_FUNC_ENTER
#endif
#define BACKKEY_FUNC_ENTER(fmt, args ...)
#ifdef BACKKEY_FUNC_LEAVE
        #undef BACKKEY_FUNC_LEAVE
#endif
#define BACKKEY_FUNC_LEAVE(fmt, args ...)
#ifdef PRINT_FUNC_LINE
#define BACKKEY_ERROR(fmt, args ...)               LOGE(FONT_COLOR_RED "[%s : <%05d>] ####" fmt "####"FONT_COLOR_RESET, __func__, __LINE__, ##args)
#else
#define BACKKEY_ERROR(fmt, args ...)               LOGE(FONT_COLOR_RED "[AT] " fmt "\n"FONT_COLOR_RESET, ##args)
#endif
#endif

#define BACKKEY_RET_IF(expr)                       do { \
       if (expr) {                                      \
            BACKKEY_ERROR("[%s] Return", #expr);        \
            return;                                     \
         }                                              \
  } while (0)

#define BACKKEY_RETV_IF(expr, val)                 do { \
       if (expr) {                                      \
            BACKKEY_ERROR("[%s] Return value", #expr);  \
            return (val);                               \
         }                                              \
  } while (0)
#define BACKKEY_RETM_IF(expr, fmt, args ...)       do {               \
       if (expr) {                                                    \
            BACKKEY_ERROR("[%s] Return, message "fmt, #expr, ##args); \
            return;                                                   \
         }                                                            \
  } while (0)
#define BACKKEY_RETVM_IF(expr, val, fmt, args ...) do {                     \
       if (expr) {                                                          \
            BACKKEY_ERROR("[%s] Return value, message "fmt, #expr, ##args); \
            return (val);                                                   \
         }                                                                  \
  } while (0)

#define BACKKEY_CHECK(expr)                        BACKKEY_RETM_IF(!(expr), "Invalid param")
#define BACKKEY_CHECK_NULL(expr)                   BACKKEY_RETVM_IF(!(expr), NULL, "Invalid param")
#define BACKKEY_CHECK_FALSE(expr)                  BACKKEY_RETVM_IF(!(expr), false, "Invalid param")
#define BACKKEY_CHECK_VAL(expr, val)               BACKKEY_RETVM_IF(!(expr), val, "Invalid param")

#define BACKKEY_ASSERT(expr)                       do {                                                  \
       if (!(expr)) {                                                                                    \
            BACKKEY_ERROR("CRITICAL ERROR ########################################## CHECK BELOW ITEM"); \
            assert(false);                                                                               \
         }                                                                                               \
  } while (0)

#endif /* __E_BACKKEY_LOG_H__ */
