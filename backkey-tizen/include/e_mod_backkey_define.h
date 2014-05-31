
#ifndef __E_MOD_BACKKEY_DEFINE_H__
#define __E_MOD_BACKKEY_DEFINE_H__

#define EDJ_PATH PREFIX "/res/edje"
#define EDJ_NAME EDJ_PATH "/"PROJECT_NAME ".edj"

/* Macros */
#ifdef __cplusplus
extern "C" {
#endif
#if 0
#define _EDJ(x)                         elm_layout_edje_get(x)
#else
#define _EDJ(x)                         (x)
#endif
#define SAFE_FREE(ptr)                  if (ptr) {free(ptr); ptr = NULL; }

#define backkey_ecore_timer_del(timer)  do { \
       if (timer) {                          \
            ecore_timer_del(timer);          \
            timer = NULL;                    \
         }                                   \
  } while (0)

#define backkey_ecore_idler_del(idler)  do { \
       if (idler) {                          \
            ecore_idler_del(idler);          \
            idler = NULL;                    \
         }                                   \
  } while (0)

#define backkey_evas_object_del(object) do { \
       if (object) {                         \
            evas_object_del(object);         \
            object = NULL;                   \
         }                                   \
  } while (0)
#ifdef __cplusplus
}
#endif

#endif /* __E_MOD_BACKKEY_DEFINE_H__ */

