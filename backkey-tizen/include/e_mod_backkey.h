#ifndef __E_MOD_BACKKEY_H__
#define __E_MOD_BACKKEY_H__

#include <e.h>
#include <Elementary.h>
#include "e_mod_backkey_log.h"
#include "e_mod_backkey_define.h"

typedef enum
{
   COLOR_SET_WHITE = 0,
   COLOR_SET_BLACK,
   COLOR_SET_BLUE,
   COLOR_SET_DOT,
   COLOR_SET_MAX
} color_set_e;

typedef struct
{
   void            *module;
   Ecore_X_Display *d;
   E_Manager       *manager;
   Eina_List       *cons;
   Eina_List       *zones;
   Evas            *canvas;

   Evas_Object     *backkey_icon;
   int              backkey_input_region_id;
   Evas_Object     *morekey_icon;
   int              morekey_input_region_id;
   Ecore_Timer     *long_press_timer;

   //Evas_Object *layout_main;

   int              x;
   int              y;
   int              width;
   int              height;
} BackkeyInfo;

#define USE_BIG_ICON 1

#if USE_BIG_ICON
#define BACKKEY_ICON               "00_icon_back_web_big.png"
#define BACKKEY_ICON_PRESS         "00_icon_back_press_web_big.png"
#define BACKKEY_ICON_WIDTH         126
#define BACKKEY_ICON_HEIGHT        126
#define BACKKEY_ICON_RIGHT_MARGIN  24
#define BACKKEY_ICON_BOTTOM_MARGIN 24

#define MOREKEY_ICON               "00_icon_more_web_big.png"
#define MOREKEY_ICON_PRESS         "00_icon_more_press_web_big.png"
#define MOREKEY_ICON_WIDTH         126
#define MOREKEY_ICON_HEIGHT        126
#define MOREKEY_ICON_LEFT_MARGIN   24
#define MOREKEY_ICON_BOTTOM_MARGIN 24
#else
#define BACKKEY_ICON               "00_icon_back_web.png"
#define BACKKEY_ICON_PRESS         "00_icon_back_press_web.png"
#define BACKKEY_ICON_WIDTH         63
#define BACKKEY_ICON_HEIGHT        63
#define BACKKEY_ICON_RIGHT_MARGIN  24
#define BACKKEY_ICON_BOTTOM_MARGIN 24

#define MOREKEY_ICON               "00_icon_more_web.png"
#define MOREKEY_ICON_PRESS         "00_icon_more_press_web.png"
#define MOREKEY_ICON_WIDTH         63
#define MOREKEY_ICON_HEIGHT        63
#define MOREKEY_ICON_LEFT_MARGIN   24
#define MOREKEY_ICON_BOTTOM_MARGIN 24
#endif

#define BACKKEY_KEYCODE            "XF86Stop"
#define MOREKEY_KEYCODE            "XF86Send"

extern E_Module *backkey_mod;

EAPI extern E_Module_Api e_modapi;

#endif /* __E_MOD_BACKKEY_H__ */

