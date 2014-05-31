#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "e_mod_backkey.h"

typedef struct _Mod Mod;
struct _Mod
{
   E_Module *module;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int   e_modapi_shutdown(E_Module *m);
EAPI int   e_modapi_save(E_Module *m);

/**
 * @t_infodtogroup Optional_Look
 * @{
 *
 * @defgroup Module_Shot BackkeyInfo
 *
 * Get Backkey information
 *
 * @}
 */

#endif
