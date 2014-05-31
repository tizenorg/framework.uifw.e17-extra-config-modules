/**
 * @addtogroup Optional_Look
 * @{
 *
 * @defgroup Module_Shot BackkeyInfo
 *
 * Get touch information
 *
 * @}
 */
#include <e.h>
#include <Ecore_X.h>
#include <Elementary.h>
#include <Eina.h>
#include <Evas.h>
#include <Edje.h>
#include <e_manager.h>
#include <e_config.h>
#include <stdio.h>
#include <system_info.h>

#include "e_mod_backkey.h"
#include "e_mod_main.h"
#include "e_mod_backkey_event.h"

#define USE_E_COMP 1

const char *MODULE_NAME = "backkey-tizen";
EAPI E_Module * backkey_mod = NULL;

EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Backkey"
};

static Evas_Object *
__backkey_main_layout_add(Evas_Object *parent)
{
   BACKKEY_CHECK_NULL(parent);
   char edje_path[PATH_MAX];
   sprintf(edje_path, "%s/themes/e_mod_backkey.edj", e_module_dir_get(backkey_mod));

   Evas_Object *layout = edje_object_add(evas_object_evas_get(parent));
   BACKKEY_CHECK_NULL(layout);
   edje_object_file_set(layout, edje_path, "main_layout");

   return layout;
}

static Eina_Bool
_backkey_main_init(BackkeyInfo *backkey_info)
{
   char backkey_path[PATH_MAX] = {0, };
   char morekey_path[PATH_MAX] = {0, };
   Evas_Load_Error err;
   Ecore_X_Display *d = NULL;

   d = ecore_x_display_get();
   if (!d)
     {
        BACKKEY_ERROR("[backkey-tizen] Failed to open display !");
        return EINA_FALSE;
     }
   backkey_info->d = d;

   sprintf(backkey_path, "%s/%s", e_module_dir_get(backkey_mod), BACKKEY_ICON);
   sprintf(morekey_path, "%s/%s", e_module_dir_get(backkey_mod), MOREKEY_ICON);

   Eina_List *managers, *l, *l2, *l3;

   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
        E_Manager *man;

        man = l->data;
        backkey_info->manager = man;

        for (l2 = man->containers; l2; l2 = l2->next)
          {
             E_Container *con = NULL;
             con = l2->data;
             backkey_info->cons = eina_list_append(backkey_info->cons, con);
#if USE_E_CON
             backkey_info->canvas = con->bg_evas;
#elif USE_E_WIN
             if (con) backkey_info->win = e_win_new(con);
             if (backkey_info->win)
               {
                  e_win_move_resize(backkey_info->win, con->x, con->y, con->w, con->h);
                  e_win_title_set(backkey_info->win, "backkey window");
                  e_win_layer_set(backkey_info->win, 450);
                  e_win_show(backkey_info->win);
                  ecore_evas_alpha_set(backkey_info->win->ecore_evas, EINA_TRUE);
                  backkey_info->canvas = backkey_info->win->evas;
                  ecore_x_window_shape_rectangle_set(backkey_info->win->evas_win, 0, 0, 1, 1);
               }
#elif USE_E_COMP
             if (con->zones)
               {
                  for (l3 = con->zones; l3; l3 = l3->next)
                    {
                       E_Zone *zone;
                       zone = l3->data;
                       if (zone)
                         {
                            backkey_info->zones = eina_list_append(backkey_info->zones, zone);

                            /*Always enable comp module*/
                            e_manager_comp_composite_mode_set(man, zone, EINA_TRUE);
                            /*
                             * TODO
                             * While release e_manager_comp_composite_mode_set(, , FALSE)
                             * use list of backkey_info->zones with EINA_LIST_FOR_EACH
                             */
                            /*
                             * I just worry about if there are many zones, which zone shold be selected???
                             */
                            backkey_info->x = zone->x;
                            backkey_info->y = zone->y;
                            backkey_info->width = zone->w;
                            backkey_info->height = zone->h;
                         }
                       else
                         {
                            backkey_info->x = 0;
                            backkey_info->y = 0;
                            backkey_info->width = man->w;
                            backkey_info->height = man->h;
                         }

                       backkey_info->canvas = e_manager_comp_evas_get(man);
                       if (!backkey_info->canvas)
                         {
                            BACKKEY_ERROR("[backkkey-tizen] Failed to get canvas from comp module");
                            return EINA_FALSE;
                         }
                    }
               }
             else
               {
                  BACKKEY_ERROR("[backkey-tizen] Container doesn't have zones :(");
                  return EINA_FALSE;
               }
#endif
          }
     }

   BACKKEY_DEBUG("[backkey-tizen] viewport's width : %d, height : %d", backkey_info->width, backkey_info->height);

   backkey_info->backkey_icon = evas_object_image_add(backkey_info->canvas);
   BACKKEY_DEBUG("backkey_path : %s", backkey_path);
   evas_object_image_file_set(backkey_info->backkey_icon, backkey_path, NULL);
   err = evas_object_image_load_error_get(backkey_info->backkey_icon);
   if (err != EVAS_LOAD_ERROR_NONE)
     BACKKEY_ERROR("[backkey-tizen] Failed to load image [%s]", backkey_path);
   evas_object_image_fill_set(backkey_info->backkey_icon, 0, 0, BACKKEY_ICON_WIDTH, BACKKEY_ICON_HEIGHT);
   evas_object_move(backkey_info->backkey_icon, backkey_info->width - BACKKEY_ICON_WIDTH - BACKKEY_ICON_RIGHT_MARGIN, backkey_info->height - BACKKEY_ICON_HEIGHT - BACKKEY_ICON_BOTTOM_MARGIN);
   evas_object_resize(backkey_info->backkey_icon, BACKKEY_ICON_WIDTH, BACKKEY_ICON_HEIGHT);
   evas_object_layer_set(backkey_info->backkey_icon, EVAS_LAYER_MAX - 1);
   /*evas_object_pass_events_set(backkey_info->backkey_icon, EINA_TRUE);*/
   evas_object_show(backkey_info->backkey_icon);

   backkey_info->morekey_icon = evas_object_image_add(backkey_info->canvas);
   BACKKEY_DEBUG("morekey_path : %s", morekey_path);
   evas_object_image_file_set(backkey_info->morekey_icon, morekey_path, NULL);
   err = evas_object_image_load_error_get(backkey_info->morekey_icon);
   if (err != EVAS_LOAD_ERROR_NONE)
     BACKKEY_ERROR("[backkey-tizen] Failed to load image [%s]", morekey_path);
   evas_object_image_fill_set(backkey_info->morekey_icon, 0, 0, MOREKEY_ICON_WIDTH, MOREKEY_ICON_HEIGHT);
   evas_object_move(backkey_info->morekey_icon, 0 + MOREKEY_ICON_LEFT_MARGIN, backkey_info->height - MOREKEY_ICON_HEIGHT - MOREKEY_ICON_BOTTOM_MARGIN);
   evas_object_resize(backkey_info->morekey_icon, MOREKEY_ICON_WIDTH, MOREKEY_ICON_HEIGHT);
   evas_object_layer_set(backkey_info->morekey_icon, EVAS_LAYER_MAX - 1);
   /*evas_object_pass_events_set(backkey_info->morekey_icon, EINA_TRUE);*/
   evas_object_show(backkey_info->morekey_icon);
#if 0
   backkey_info->layout_main = __backkey_main_layout_add(backkey_info->base);
   BACKKEY_CHECK_FALSE(backkey_info->layout_main);
   evas_object_move(backkey_info->layout_main, 0, 0);
   evas_object_size_hint_weight_set(backkey_info->layout_main, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_resize(backkey_info->layout_main, backkey_info->width, backkey_info->height);
   evas_object_show(backkey_info->layout_main);
#endif
   return EINA_TRUE;
}

static void
_backkey_set_win_property(Eina_Bool set)
{
   Ecore_X_Atom x_atom_backkey = 0;
   Ecore_X_Window win = 0;
   unsigned int enable = -1;
   int ret = -1, count = 0;
   unsigned char *prop_data = NULL;

   BACKKEY_FUNC_ENTER();

   x_atom_backkey = ecore_x_atom_get("_E_BACKKEY_ENABLE");
   if (x_atom_backkey == None)
     {
        BACKKEY_ERROR("failed to get atom of _E_BACKKEY_ENABLE\n");
        return;
     }

   win = ecore_x_window_root_first_get();
   if (win <= 0)
     {
        BACKKEY_ERROR("failed to get root window\n");
        return;
     }

   ret = ecore_x_window_prop_property_get(win, x_atom_backkey, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if ( ret && prop_data )
     {
        memcpy(&enable, prop_data, sizeof (unsigned int));
        BACKKEY_DEBUG("backkey-tizen is enabled: %s ", enable ? "true" : "false");
     }

   if (enable == !!set) return;
   enable = !!set;
   ecore_x_window_prop_card32_set(win, x_atom_backkey, &enable, 1);
}

static Eina_Bool
_backkey_check_hard_backkey_is_supported(void)
{
   int ret = -1;
   char *str = NULL;
   int intvalue = -1;

   ret = system_info_get_internal_value("board.touch.tkey", &str);
   if (ret != SYSTEM_INFO_ERROR_NONE || !str)
     {
        BACKKEY_ERROR("[backkey-tizen] : Failed to get system info regarding board.touch.tkey, err : %d, str : %s", ret, (str != NULL) ? str : "NULL");
        return EINA_FALSE;
     }

   if (!strncmp(str, "TRUE", 4))
     {
        BACKKEY_DEBUG("[backkey-tizen] : Hard Key is supported");
        free(str);
        return EINA_TRUE;
     }
   else if (!strncmp(str, "FALSE", 5))
     {
        BACKKEY_DEBUG("[backkey-tizen] : Hard key is not supported");
        free(str);
        return EINA_FALSE;
     }
   if (str) free(str);
   return EINA_FALSE;
}

EAPI void *
e_modapi_init(E_Module *m)
{
   BACKKEY_FUNC_ENTER();

   BackkeyInfo *backkey_info = E_NEW(BackkeyInfo, 1);
   BACKKEY_CHECK_NULL(backkey_info);

   Mod *module = E_NEW(Mod, 1);
   if (!module)
     {
        BACKKEY_ERROR("[backkey-tizen] failed to allocate memory");
        if (backkey_info) free(backkey_info);
        return NULL;
     }

   module->module = m;

   backkey_info->module = (void *)module;
   backkey_mod = m;

   if (_backkey_check_hard_backkey_is_supported() == EINA_TRUE)
     {
        if (backkey_info) free(backkey_info);
        if (module) free(module);
        BACKKEY_DEBUG("[backkey-tizen] This HW has hard backkey and backkey-tizen will be disabled");
        return NULL;
     }

   if (_backkey_main_init(backkey_info) != EINA_TRUE)
     {
        if (backkey_info) free(backkey_info);
        if (module) free(module);
        BACKKEY_ERROR("[backkey-tizen] failed to init main of backkey tizen");
        return NULL;
     }

   if (backkey_event_mgr_init(backkey_info) != EINA_TRUE)
     {
        if (backkey_info) free(backkey_info);
        if (module) free(module);
        BACKKEY_ERROR("[backkey-tizen] failed to init event mgr of backkey tizen");
        return NULL;
     }

   _backkey_set_win_property(EINA_TRUE);

   return backkey_info;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   BACKKEY_FUNC_ENTER();

   BackkeyInfo *backkey_info = (BackkeyInfo *)m->data;
   BACKKEY_CHECK_VAL(backkey_info, 0);

   Mod *mod = backkey_info->module;

   backkey_event_mgr_deinit(backkey_info);

   backkey_evas_object_del(backkey_info->backkey_icon);
   backkey_evas_object_del(backkey_info->morekey_icon);

   Eina_List *managers, *l, *l2, *l3;

   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
        E_Manager *man;
        man = l->data;

        for (l2 = backkey_info->cons; l2; l2 = l2->next)
          {
             for (l3 = backkey_info->zones; l3; l3 = l3->next)
               {
                  E_Zone *zone;
                  zone = l3->data;
                  if (zone)
                    {
                       /*Always enable comp module*/
                       e_manager_comp_composite_mode_set(man, zone, EINA_FALSE);
                    }
               }
          }
     }

   SAFE_FREE(mod);
   SAFE_FREE(backkey_info);
   _backkey_set_win_property(EINA_FALSE);

   /*
    * FIXME : Free containers and etc...
    */

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   BackkeyInfo *backkey_info = NULL;
   Mod *mod = NULL;
   if (m) backkey_info = m->data;
   if (backkey_info) mod = backkey_info->module;

   return 1;
}

