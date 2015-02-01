/*

Copyright (c) 2000-2012 Samsung Electronics Co., Ltd All Rights Reserved

This file is part of e17-extra-config-modules
Written by Seunggyun Kim <sgyun.kim@smasung.com>

PROPRIETARY/CONFIDENTIAL

This software is the confidential and proprietary information of
SAMSUNG ELECTRONICS ("Confidential Information"). You shall not
disclose such Confidential Information and shall use it only in
accordance with the terms of the license agreement you entered
into with SAMSUNG ELECTRONICS.

SAMSUNG make no representations or warranties about the suitability
of the software, either express or implied, including but not limited
to the implied warranties of merchantability, fitness for a particular
purpose, or non-infringement. SAMSUNG shall not be liable for any
damages suffered by licensee as a result of using, modifying or
distributing this software or its derivatives.

*/

#include "e.h"
#include "e_mod_main.h"
#include "Elementary.h"
#include "e_randr.h"
#include <efl_assist.h>
#include <vconf.h>

#define BASE_DISTANCE         30
#define PI                    3.141592
#define BASE_LAYOUT_INCH      4.65
#define BASE_LAYOUT_WIDTH_PX  720
#define BASE_LAYOUT_HEIGHT_PX 1280
#define ROUND_DOUBLE(x)       (round(x*10)/10)
#define MOBILE_PROFILE        "mobile"
#define DESKTOP_MODE_PROFILE  "desktop"
#define BUF_SIZE              256

typedef struct _ConfigTIZEN_
{
   Ecore_Event_Handler *randr_output_handler;
   Ecore_Event_Handler *zone_add_handler;
   Ecore_Event_Handler *zone_del_handler;
   int extended_width_res;
   int extended_height_res;
   int extended_width_mm;
   int extended_height_mm;
   int output_changed;
} ConfigTIZEN;

ConfigTIZEN e_configtizen;

static int _e_mod_config_scale_update(void);
static double _e_elm_config_ppd_get(double d_inch, int w_px, int h_px, double distance);
static int _e_configtizen_cb_output_change (void *data, int type, void *ev);
static int _e_configtizen_cb_zone_add(void *data, int ev_type, void *event);
static int _e_configtizen_cb_zone_del(void *data, int ev_type, void *event);

/* this is needed to advertise a label for the module IN the code (not just
 * the .desktop file) but more specifically the api version it was compiled
 * for so E can skip modules that are compiled for an incorrect API version
 * safely) */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION, "Elementary Profile Module of Window Manager"
};

EAPI void*
e_modapi_init(E_Module* m)
{
   _e_mod_config_scale_update();

   e_configtizen.output_changed = 0;
   e_configtizen.randr_output_handler = ecore_event_handler_add (ECORE_X_EVENT_RANDR_OUTPUT_CHANGE, (Ecore_Event_Handler_Cb)_e_configtizen_cb_output_change, NULL);
   e_configtizen.zone_add_handler = ecore_event_handler_add(E_EVENT_ZONE_ADD, (Ecore_Event_Handler_Cb)_e_configtizen_cb_zone_add, NULL);
   e_configtizen.zone_del_handler = ecore_event_handler_add(E_EVENT_ZONE_DEL, (Ecore_Event_Handler_Cb)_e_configtizen_cb_zone_del, NULL);

   if (!e_configtizen.randr_output_handler) printf ("[e_config-tizen][%s] Failed to add ECORE_X_EVENT_RANDR_OUTPUT_CHANGE handler\n", __FUNCTION__);
   if (!e_configtizen.zone_add_handler) printf("[e_config-tizen][%s] Failed to add E_EVENT_ZONE_ADD handler\n", __FUNCTION__);
   if (!e_configtizen.zone_del_handler) printf("[e_config-tizen][%s] Failed to add E_EVENT_ZONE_DEL handler\n", __FUNCTION__);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module* m)
{
   return 1;
}

EAPI int
e_modapi_save(E_Module* m)
{
   /* Do Something */
   return 1;
}

static double _ppd_distance_get(double d_inch, int desktop_mode)
{
   double distance;

   if (d_inch < 6) distance = 30;
   else if (d_inch < 13) distance = 45;
   else if (d_inch < 28) distance = 50;
   else distance = 100;

   if (desktop_mode) return distance;
   else return -1;
}

static double
_e_elm_config_ppd_get(double d_inch, int w_px, int h_px, double distance)
{
   double dpi, ppd;

   dpi = (sqrt((w_px * w_px) + (h_px * h_px))) / d_inch;

   ppd = (2 * distance * tan(0.5 * (PI / 180)) * dpi / 2.54);
   return ppd;
}

static int
_e_mod_config_elm_profile_save(const char *profile_name, double scale)
{
   const char *path;
   char buf[BUF_SIZE];
   Eina_Bool config_saved = EINA_FALSE;

   elm_init(0, NULL);
   path = elm_config_profile_dir_get(elm_config_profile_get(), EINA_TRUE);

   // scale
   snprintf(buf, BUF_SIZE, "%s/base.cfg", path);
   if (!ecore_file_exists(buf))
     {
        char *s;

        s = getenv("ELM_SCALE");
        if (!s) elm_config_scale_set(scale);
     }

   // color overlay
   snprintf(buf, BUF_SIZE, "%s/color.cfg", path);
   if (!ecore_file_exists(buf))
     {
        char *cc;
        Eina_List *clist;
        int r, g, b, a;
        int r2, g2, b2, a2;
        int r3, b3, g3, a3;

        ea_theme_input_colors_set(0, EA_THEME_STYLE_DEFAULT);
        ea_theme_style_set(EA_THEME_STYLE_DEFAULT);
        clist = edje_color_class_list();
        EINA_LIST_FREE(clist, cc)
          {
             edje_color_class_get(cc, &r, &g, &b, &a,
                                  &r2, &g2, &b2, &a2,
                                  &r3, &g3, &b3, &a3);
             elm_config_color_overlay_set(cc, r, g, b, a,
                                          r2, g2, b2, a2,
                                          r3, g3, b3, a3);
             free(cc);
          }
     }

   // font overlay
   snprintf(buf, BUF_SIZE, "%s/font.cfg", path);
   if (!ecore_file_exists(buf))
     {
        ea_theme_system_font_set("Tizen", -100);
        config_saved = EINA_TRUE;
     }

   if (!config_saved)
     elm_config_save();

   elm_config_profile_dir_free(path);
   elm_shutdown();

   return 1;
}

static int
_e_mod_config_scale_update(void)
{
   int target_width, target_height, target_width_mm, target_height_mm;
   double target_inch, dpi, scale, profile_factor;
   char *profile;

   target_width = 0;
   target_height = 0;
   target_width_mm = 0;
   target_height_mm = 0;
   profile_factor = 1.0;
   scale = 1.0;

   ecore_x_randr_screen_current_size_get(ecore_x_window_root_first_get(), &target_width, &target_height, &target_width_mm, &target_height_mm);
   target_inch = ROUND_DOUBLE(sqrt(target_width_mm * target_width_mm + target_height_mm * target_height_mm) / 25.4);
   dpi = ROUND_DOUBLE(sqrt(target_width * target_width + target_height * target_height) / target_inch);

   profile = elm_config_profile_get();
   if (!strcmp(profile, "wearable")) profile_factor = 0.4;
   else if (!strcmp(profile, "mobile")) profile_factor = 0.7;
   else if (!strcmp(profile, "desktop")) profile_factor = 1.0;

   scale = ROUND_DOUBLE(dpi / 90.0 * profile_factor);

   e_config->scale.factor = scale;

   // calculate elementray scale factor
   _e_mod_config_elm_profile_save(MOBILE_PROFILE, scale);

   return 1;
}

static E_Randr_Output_Info *
_e_configtizen_external_get_output_info_from_output_xid (Ecore_X_Randr_Output output_xid)
{

   E_Randr_Output_Info *output_info = NULL;
   Eina_List *iter;

   EINA_LIST_FOREACH (e_randr_screen_info.rrvd_info.randr_info_12->outputs, iter, output_info)
     {
        if (output_info == NULL)
           continue;

        if (output_info->xid == output_xid)
             return output_info;
     }

   return NULL;
}

static int
_e_configtizen_cb_output_change (void *data, int type, void *ev)
{
   E_Randr_Output_Info *output_info = NULL;

   if (type == ECORE_X_EVENT_RANDR_OUTPUT_CHANGE)
     {
       Ecore_X_Event_Randr_Output_Change *event = (Ecore_X_Event_Randr_Output_Change *)ev;

       /* check status of a output */
       if (event->connection == ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED &&
           event->crtc == 0 &&
           event->mode == 0)
         {
            output_info = _e_configtizen_external_get_output_info_from_output_xid (event->output);
            if (!output_info)
              {
                 fprintf (stderr, "[_e_config-tizen] : fail to get output from xid\n");
                 return 0;
              }
            e_configtizen.output_changed = 1;
            e_configtizen.extended_width_mm = output_info->monitor->size_mm.width;
            e_configtizen.extended_height_mm = output_info->monitor->size_mm.height;
         }
     }
  return 1;
}

static int
_e_configtizen_cb_zone_add(void *data, int ev_type, void *event)
{

   double extended_inch, scale, extended_ppd, base_ppd, distance;
   E_Event_Zone_Add *ev;
   E_Zone *zone;

   ev = event;
   zone = ev->zone;

   if (!e_configtizen.output_changed) return 1;
   if (!zone || !zone->name) return 1;

   e_configtizen.extended_width_res = zone->w;
   e_configtizen.extended_height_res = zone->h;
   fprintf (stdout, "[_e_config-tizen] extended display resolution w = %d h = %d \n", e_configtizen.extended_width_res, e_configtizen.extended_height_res);
   fprintf (stdout, "[_e_config-tizen] extended display size width mm = %d  height mm = %d\n", e_configtizen.extended_width_mm, e_configtizen.extended_height_mm);

   extended_inch = ROUND_DOUBLE(sqrt(e_configtizen.extended_width_mm * e_configtizen.extended_width_mm + e_configtizen.extended_height_mm * e_configtizen.extended_height_mm) / 25.4);

   // this is exception handling about display that size specification is not correct
   if (extended_inch < 10)
     {
       fprintf(stderr, "[_e_config-tizen] Currently, perspective scaling only supports more than 10.0 inch extended display size.\n");
       fprintf(stderr, "[_e_config-tizen] Detected extension display is %lf inch size. so this will use default desktop profile.\n", extended_inch);
       return 1;
     }

   distance = _ppd_distance_get(extended_inch, 1);

   base_ppd = ROUND_DOUBLE(_e_elm_config_ppd_get(BASE_LAYOUT_INCH, BASE_LAYOUT_WIDTH_PX, BASE_LAYOUT_HEIGHT_PX, BASE_DISTANCE));
   extended_ppd = ROUND_DOUBLE(_e_elm_config_ppd_get(extended_inch, e_configtizen.extended_width_res, e_configtizen.extended_height_res, distance));

   scale = ROUND_DOUBLE(extended_ppd / base_ppd);

   fprintf(stdout, "[_e_config-tizen] extended display inch = %lf display scale = %lf \n", extended_inch, scale);

   // calculate elementary scale factor
   setenv("ELM_PROFILE", DESKTOP_MODE_PROFILE, 1);
   _e_mod_config_elm_profile_save(DESKTOP_MODE_PROFILE, scale);

   // set e_scale for e17 widgets
   edje_scale_set(e_scale);

   return 1;
}

static int
_e_configtizen_cb_zone_del(void *data, int ev_type, void *event)
{
   e_configtizen.output_changed = 0;

   return 1;
}
