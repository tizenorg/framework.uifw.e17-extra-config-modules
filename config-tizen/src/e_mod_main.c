/*
 * e17-extra-config-modules
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "e.h"
#include "e_mod_main.h"
#include "Elementary.h"

#define RESOLUTION_BASE         1
#define PI                      3.141592
#define BASE_LAYOUT_INCH        4.65
#define BASE_LAYOUT_WIDTH_PX    720
#define BASE_LAYOUT_HEIGHT_PX   1280
#define ROUND_DOUBLE(x)         (round(((double)x)*100)/100)
#define ELM_FINGER_SIZE         60

static int _e_elm_config_scale_update(void);
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
   _e_elm_config_scale_update();
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

#ifndef RESOLUTION_BASE
static double
_e_elm_config_dpi_get(double d_inch, int w_px, int h_px)
{
   double dpi;

   dpi = (sqrt((w_px * w_px) + (h_px * h_px))) / d_inch;

   return dpi;
}
#endif

static int
_e_mod_config_elm_profile_save(double scale)
{
   elm_init(0, NULL);
   elm_config_scale_set(scale);
   elm_config_finger_size_set(scale * ELM_FINGER_SIZE);
   elm_config_save();
   elm_shutdown();

   return 1;
}

static int
_e_elm_config_scale_update (void)
{
   int target_width, target_height, target_width_mm, target_height_mm;
   double scale;

   target_width = 0;
   target_height = 0;
   target_width_mm = 0;
   target_height_mm = 0;

   ecore_x_randr_screen_current_size_get(ecore_x_window_root_first_get(), &target_width, &target_height, &target_width_mm, &target_height_mm);

#if RESOLUTION_BASE

   if (target_width < target_height)
      scale = ROUND_DOUBLE(target_width / BASE_LAYOUT_WIDTH_PX);
   else
      scale = ROUND_DOUBLE(target_height / BASE_LAYOUT_WIDTH_PX);

#else // DPI BASE
   double target_inch, target_dpi, base_dpi;

   target_inch = ROUND_DOUBLE(sqrt(target_width_mm * target_width_mm + target_height_mm * target_height_mm) / 25.4);

   // Calculate DPI
   base_dpi = ROUND_DOUBLE(_e_elm_config_dpi_get(BASE_LAYOUT_INCH, BASE_LAYOUT_WIDTH_PX, BASE_LAYOUT_HEIGHT_PX));
   target_dpi = ROUND_DOUBLE(_e_elm_config_dpi_get(target_inch, target_width, target_height));

     // Calculate Scale factor
   scale = ROUND_DOUBLE(target_dpi / base_dpi);

#endif
   e_config->scale.factor = scale;

   // calculate elementray scale factor
   _e_mod_config_elm_profile_save(scale);

   return 1;
}
