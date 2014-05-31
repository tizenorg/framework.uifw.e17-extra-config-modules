/**
 * @addtogroup Optional_Look
 * @{
 *
 * @defgroup Module_Shot Screenshot
 *
 * Takes screen shots and can submit them to http://enlightenment.org
 *
 * @}
 */
#include "e.h"
#include <sys/time.h>
#include <time.h>
#include <utilX.h>
#include <Ecore_X.h>
#include <Ecore_File.h>
#include <Elementary.h>
#include <media_content.h>
#include <vconf.h>
#include <mm_sound.h>
#include <dlog.h>

#define LOG_TAG "SHOT_TIZEN"

//FIXME: get off these vars to configure
#define IMAGE_SAVE_DIR "/opt/usr/media/Screenshots"
#define IMAGE_SAVE_FILE_TYPE ".png"

const double CAMERA_FOCAL = 2000;

static E_Action *act = NULL;
static Ecore_X_Atom atom_xkey_msg = 0;
static Ecore_Event_Handler *xclient_msg_handler = NULL;
static Eina_Bool on_snd_play = EINA_FALSE;
static Eina_Bool on_capture = EINA_FALSE;
const char *MODULE_NAME = "shot-tizen";

typedef struct _Custom_Effect2 Custom_Effect2;
typedef struct _Custom_Effect1 Custom_Effect1;

struct _Custom_Effect1
{
   double z_from, z_to;
   int rot;
   Evas_Object *edje;
};

struct _Custom_Effect2
{
   struct {
      int c_from, c_to;
      Evas_Coord x_from, x_to, y_from, y_to;
      double z_from, z_to;
      double rot_from, rot_to;
      Evas_Object *edje;
      int rot;
   } edje;

   struct {
      int a_from, a_to;
      Evas_Object *bg;
   } bg;

   Evas_Object *img;
   E_Zone *zone;
   Ecore_X_Display *disp;
   char img_path[PATH_MAX];
};

static Evas_Object *
_create_rect(Evas *evas, int w, int h)
{
   Evas_Object *bg = evas_object_rectangle_add(evas);
   if (!bg) return NULL;
   evas_object_resize(bg, w, h);
   evas_object_color_set(bg, 0, 0, 0, 200);
   evas_object_layer_set(bg, EVAS_LAYER_MAX);
   evas_object_show(bg);

   return bg;
}

static Evas_Object *
_create_edje(Evas *evas, int w, int h, int rot, Evas_Object *img)
{
   char edj_path[PATH_MAX];

   Evas_Object *edje = edje_object_add(evas);
   if (!edje) return NULL;

   sprintf(edj_path, "%s/data/themes/shot_effect.edj", e_prefix_data_get());
   edje_object_file_set(edje, edj_path, "effect");
   edje_object_part_swallow(edje, "capture_img", img);

   //These position and size offsets are regarded with the shadow +
   //frame_outline area. Additionally it adds 10 for image crop area. See the
   // effect edc "capture_img".
   if ((rot == 0) || (rot == 180))
     {
        evas_object_move(edje, -46, -46);
        evas_object_resize(edje, (w + 94), (h + 92));
     }
   else if ((rot == 90) || (rot == 270))
     {
        evas_object_move(edje, -((h / 2) + 46) + ((w / 2)),
                         (((h / 2)) - ((w / 2) + 48)));
        evas_object_resize(edje, (h + 92), (w + 94));
     }
   evas_object_layer_set(edje, EVAS_LAYER_MAX);
   evas_object_show(edje);

   return edje;
}

static Eina_Bool
_capture_image_save(Evas_Object *img, char *filepath)
{
   time_t tim;
   struct tm *now;
   struct timeval tv;

   if (!ecore_file_is_dir(IMAGE_SAVE_DIR))
     ecore_file_mkdir(IMAGE_SAVE_DIR);

   //file path get
   tim = time(NULL);
   now = localtime(&tim);
   gettimeofday(&tv, NULL);

   sprintf(filepath, "%s/Screen-%d%02d%02d%02d%02d%02d%01d%s",
           IMAGE_SAVE_DIR, now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
           now->tm_hour, now->tm_min, now->tm_sec, (int) tv.tv_usec,
           IMAGE_SAVE_FILE_TYPE);
   return evas_object_image_save(img, filepath, NULL, "quality=100 compress=1");
}

static void
_media_content_update(const char *img_path)
{
   int ret;

   ret = media_content_connect();
   if (ret != MEDIA_CONTENT_ERROR_NONE) return;

   ret = media_content_scan_file(img_path);
   if (ret != MEDIA_CONTENT_ERROR_NONE)
     {
        media_content_disconnect();
        return;
     }
   media_content_disconnect();
}

static void
_capture_finished(Evas_Object *img, char *img_path, Ecore_X_Display *disp, E_Zone *zone)
{
   Ecore_X_Atom atom;

   utilx_release_screen_shot();

   XCloseDisplay(disp);

   e_manager_comp_composite_mode_set(zone->container->manager, zone,
                                     EINA_FALSE);
   on_capture = EINA_FALSE;

   _media_content_update(img_path);
}

static void
_effect_step1_op(Elm_Transit_Effect *effect, Elm_Transit *transit,
                      double progress)
{
   Custom_Effect1 *ctx = effect;
   double camera_pos = -(ctx->z_from + (progress * ctx->z_to));
   Evas_Coord x, y, w, h;

   //Image
   Evas_Map *map = evas_map_new(4);
   if (!map) return;

   evas_map_util_points_populate_from_object_full(map, ctx->edje, 0);
   evas_object_geometry_get(ctx->edje, &x, &y, &w, &h);

   //Image Rotation
   evas_map_util_3d_rotate(map, 0, 0, -ctx->rot, x + (w/2),
                           y + (h/2), 0);

   //Image Zoom
   evas_map_util_3d_perspective(map, x + (w/2), y + (h/2), camera_pos,
                                CAMERA_FOCAL);

   evas_object_map_set(ctx->edje, map);
   evas_object_map_enable_set(ctx->edje, EINA_TRUE);
   evas_map_free(map);
}

static void
_effect_step1_free(Elm_Transit_Effect *effect, Elm_Transit *transit)
{
   Custom_Effect1 *ctx = (Custom_Effect1 *)effect;
   free(ctx);
}

static Elm_Transit_Effect *
_effect_step1_ctx_new(Evas_Object *edje, int rot)
{
   const double EDJE_ZOOM_FROM = 1.0;     //1.0x
   const double EDJE_ZOOM_TO = 0.9;      //0.9x

   Custom_Effect1 *effect;

   effect = E_NEW(Custom_Effect1, 1);
   if (!effect) return NULL;

   effect->z_from =
      (CAMERA_FOCAL - (EDJE_ZOOM_FROM * CAMERA_FOCAL)) * (1/EDJE_ZOOM_FROM);
   effect->z_to =
      ((CAMERA_FOCAL - (EDJE_ZOOM_TO * CAMERA_FOCAL)) * (1/EDJE_ZOOM_TO))
      - effect->z_from;

   effect->edje = edje;
   effect->rot = rot;

   return effect;
}

static void
_effect_step2_op(Elm_Transit_Effect *effect, Elm_Transit *transit,
                      double progress)
{
   Custom_Effect2 *ctx = effect;
   int edje_x = ctx->edje.x_from + (int) ((double) ctx->edje.x_to * progress);
   int edje_y = ctx->edje.y_from + (int) ((double) ctx->edje.y_to * progress);
   int edje_c = ctx->edje.c_from + (int) ((double) ctx->edje.c_to * progress);
   double rot = ctx->edje.rot_from + (progress * ctx->edje.rot_to);
   double camera_pos = -(ctx->edje.z_from + (progress * ctx->edje.z_to));
   int bg_a = ctx->bg.a_from + (int) ((double) ctx->bg.a_to * progress);
   int x, y, w, h;

   //Image
   Evas_Map *map = evas_map_new(4);
   if (!map) return;

   evas_object_geometry_get(ctx->edje.edje, NULL, NULL, &w, &h);

   //Image Translation
   evas_object_move(ctx->edje.edje, edje_x, edje_y);

   x = edje_x;
   y = edje_y;

   evas_map_util_points_populate_from_object_full(map, ctx->edje.edje, 0);

   //Image Color
   evas_map_point_color_set(map, 0, edje_c, edje_c, edje_c, edje_c);
   evas_map_point_color_set(map, 1, edje_c, edje_c, edje_c, edje_c);
   evas_map_point_color_set(map, 2, edje_c, edje_c, edje_c, edje_c);
   evas_map_point_color_set(map, 3, edje_c, edje_c, edje_c, edje_c);

   //Image Rotation
   if ((ctx->edje.rot == 0) || (ctx->edje.rot == 180))
     {
        if (ctx->edje.rot == 180) rot = -rot;
        evas_map_util_3d_rotate(map, rot, 0, -ctx->edje.rot, x + (w/2),
                                y + (h/2), 0);
     }
   else
     {
        if (ctx->edje.rot == 90) rot = -rot;
        evas_map_util_3d_rotate(map, 0, rot, -ctx->edje.rot, x + (w/2),
                                y + (h/2), 0);
     }

   //Image Zoom
   evas_map_util_3d_perspective(map, x + (w/2), y + (h/2), camera_pos,
                                CAMERA_FOCAL);

   evas_object_map_set(ctx->edje.edje, map);
   evas_object_map_enable_set(ctx->edje.edje, EINA_TRUE);
   evas_map_free(map);

   //BG Color
   evas_object_color_set(ctx->bg.bg, 0, 0, 0, bg_a);
}

static void
_effect_step2_free(Elm_Transit_Effect *effect, Elm_Transit *transit)
{
   Custom_Effect2 *ctx = (Custom_Effect2 *)effect;

   _capture_finished(ctx->img, ctx->img_path, ctx->disp, ctx->zone);

   evas_object_del(ctx->edje.edje);
   evas_object_del(ctx->bg.bg);
   evas_object_del(ctx->img);

   free(ctx);
}

static Elm_Transit_Effect *
_effect_step2_ctx_new(Evas_Object *edje, Evas_Object *bg, Evas_Object *img, E_Zone *zone, char *img_path, int rot, Ecore_X_Display *disp)
{
   const double EDJE_ZOOM_FROM = 0.9;     //0.9x
   const double EDJE_ZOOM_TO = 0.15;      //0.15x
   const int EDJE_COLOR_FROM = 255;
   const int EDJE_COLOR_TO = -205;        //actually 50
   const double EDJE_ROTATION_FROM = 0;
   const double EDJE_ROTATION_TO = 120;
   const int BG_ALPHA_FROM = 175;
   const int BG_ALPHA_TO = -175;         //actually 0

   Custom_Effect2 *effect;
   Evas_Coord x, y, w, h;

   effect = E_NEW(Custom_Effect2, 1);
   if (!effect) return NULL;

   //Edje
   evas_object_geometry_get(edje, &x, &y, &w, &h);
   switch (rot)
     {
      case 0:
         effect->edje.x_from = x;
         effect->edje.x_to = 0;
         effect->edje.y_from = y;
         effect->edje.y_to = -h;
         break;
      case 90:
         effect->edje.x_from = x;
         effect->edje.x_to = -h;
         effect->edje.y_from = y;
         effect->edje.y_to = 0;
         break;
      case 180:
         effect->edje.x_from = x;
         effect->edje.x_to = 0;
         effect->edje.y_from = y;
         effect->edje.y_to = h;
         break;
      case 270:
         effect->edje.x_from = x;
         effect->edje.x_to = h;
         effect->edje.y_from = y;
         effect->edje.y_to = 0;
         break;
     }
   effect->edje.c_from = EDJE_COLOR_FROM;
   effect->edje.c_to = EDJE_COLOR_TO;
   effect->edje.z_from =
      (CAMERA_FOCAL - (EDJE_ZOOM_FROM * CAMERA_FOCAL)) * (1/EDJE_ZOOM_FROM);
   effect->edje.z_to =
      ((CAMERA_FOCAL - (EDJE_ZOOM_TO * CAMERA_FOCAL)) * (1/EDJE_ZOOM_TO))
      - effect->edje.z_from;
   effect->edje.rot_from = EDJE_ROTATION_FROM;
   effect->edje.rot_to = EDJE_ROTATION_TO;
   effect->edje.edje = edje;
   effect->edje.rot = rot;

   //BG
   effect->bg.a_from = BG_ALPHA_FROM;
   effect->bg.a_to = BG_ALPHA_TO;
   effect->bg.bg = bg;

   effect->img = img;
   effect->zone = zone;
   effect->disp = disp;

   strncpy(effect->img_path, img_path, sizeof(effect->img_path));

   e_manager_comp_composite_mode_set(zone->container->manager, zone, EINA_TRUE);

   return (Elm_Transit_Effect *) effect;
}

static void
_transit_del_cb(void *data, Elm_Transit *transit)
{
   evas_object_del(data);
}

static void
_capture_visual_effect(Ecore_X_Display *disp, Evas_Object *img, char *img_path, Evas_Coord w, Evas_Coord h, E_Zone *zone, int rot)
{
   //FIXME: MIGRATE ALL EFFECT IMPLEMENTATION TO EDJE.
   const double ZOOM_STEP_ONE_TIME = 0.75;
   const double ZOOM_STEP_TWO_TIME = 0.5;
   const double FLASH_STEP_ONE_TIME = 0.25;
   const double FLASH_STEP_TWO_TIME = 0.5;

   Elm_Transit *trans = NULL, *trans2 = NULL, *trans3 = NULL, *trans4 = NULL;
   Elm_Transit_Effect *effect1 = NULL, *effect2 = NULL;
   Evas_Object *bg = NULL, *edje = NULL, *flash = NULL;
   Evas *evas = evas_object_evas_get(img);
   bg = _create_rect(evas, w, h);
   if (!bg) goto err;

   //Create Effect Layout
   edje = _create_edje(evas, w, h, rot, img);
   if (!edje) goto err;

   flash = _create_rect(evas, w, h);
   if (!flash) goto err;

   //Zoom Step1
   trans = elm_transit_add();
   if (!trans) goto err;

   effect1 = _effect_step1_ctx_new(edje, rot);
   if (!effect1) goto err;

   elm_transit_object_add(trans, edje);
   elm_transit_smooth_set(trans, EINA_FALSE);
   elm_transit_duration_set(trans, ZOOM_STEP_ONE_TIME);
   elm_transit_effect_add(trans, _effect_step1_op, effect1,
                          _effect_step1_free);
   elm_transit_tween_mode_set(trans, ELM_TRANSIT_TWEEN_MODE_ACCELERATE);
   elm_transit_objects_final_state_keep_set(trans, EINA_TRUE);

   //Zoom Step 2
   trans2 = elm_transit_add();
   if (!trans2) goto err;

   effect2 = _effect_step2_ctx_new(edje, bg, img, zone, img_path, rot, disp);
   if (!effect2) goto err;

   elm_transit_object_add(trans2, edje);
   elm_transit_object_add(trans2, bg);
   elm_transit_smooth_set(trans, EINA_FALSE);
   elm_transit_duration_set(trans2, ZOOM_STEP_TWO_TIME);
   elm_transit_effect_add(trans2, _effect_step2_op, effect2,
                          _effect_step2_free);
   elm_transit_tween_mode_set(trans2, ELM_TRANSIT_TWEEN_MODE_ACCELERATE);

   elm_transit_chain_transit_add(trans, trans2);

   //Flash Step 1
   trans3 = elm_transit_add();
   if (!trans3) goto err;

   elm_transit_object_add(trans3, flash);
   elm_transit_duration_set(trans3, FLASH_STEP_ONE_TIME);
   elm_transit_effect_color_add(trans3, 0, 0, 0, 0, 160, 160, 160, 160);
   elm_transit_objects_final_state_keep_set(trans3, EINA_TRUE);

   //Flash Step 2
   trans4 = elm_transit_add();
   if (!trans4) goto err;

   elm_transit_object_add(trans4, flash);
   elm_transit_duration_set(trans4, FLASH_STEP_TWO_TIME);
   elm_transit_effect_color_add(trans4, 160, 160, 160, 160, 0, 0, 0, 0);
   elm_transit_del_cb_set(trans4, _transit_del_cb, flash);

   elm_transit_chain_transit_add(trans3, trans4);

   elm_transit_go(trans);
   elm_transit_go(trans3);
   edje_object_signal_emit(edje, "effect,launch", "");

   return;

err:
   if (trans) elm_transit_del(trans);
   if (trans2) elm_transit_del(trans2);
   if (trans3) elm_transit_del(trans3);
   if (bg) evas_object_del(bg);
   if (flash) evas_object_del(flash);
   if (edje) evas_object_del(edje);
   if (effect1) free(effect1);
   if (effect2) free(effect2);
}

static void
_snd_stop_callback(void* data)
{
   on_snd_play = EINA_FALSE;
}

static void
_capture_snd_effect(void)
{
   char wav_path[PATH_MAX];

   int camera_status = 0;
   int shutter_snd_status = 0;
   int handle = -1;
   Eina_Bool vconf_ret = EINA_FALSE;

   if(on_snd_play) return;
   on_snd_play = EINA_TRUE;

   sprintf(wav_path, "%s/data/sounds/shutter.wav", e_prefix_data_get());

   mm_sound_play_solo_sound(wav_path, VOLUME_TYPE_SYSTEM,
                              _snd_stop_callback, NULL, &handle);
}

static Evas_Object *
_create_img(Evas *evas, void *data, int w, int h)
{
   Evas_Object *img = evas_object_image_filled_add(evas);
   if (!img) return NULL;

   evas_object_image_size_set(img, w, h);
   evas_object_image_data_set(img, data);
   evas_object_image_smooth_scale_set(img, EINA_FALSE);
   evas_object_image_data_update_add(img, 0, 0, w, h);
   evas_object_show(img);

   return img;
}

static unsigned int
_scrn_rot_get(Ecore_X_Window xwin)
{
   int num;
   unsigned int *rot, ret;
   Ecore_X_Atom atom;

   //Get capture image based on the current port/land mode.
   atom = ecore_x_atom_get("_E_ILLUME_ROTATE_ROOT_ANGLE");
   if (!atom) return 0;

   if (!ecore_x_window_prop_property_get(xwin, atom, ECORE_X_ATOM_CARDINAL, 32,
                                        (unsigned char **) &rot, &num))
     return 0;

   ret = *rot;
   free(rot);
   return ret;
}

static void
_scrn_shot(void)
{
   E_Zone *zone;
   unsigned int rot;
   int w, h, w2, h2;
   void *data;
   Evas *evas;
   Evas_Object *img;
   char filepath[PATH_MAX];
   Ecore_X_Display *disp = NULL;

   //FIXME: Consider to capture a whole area of the screen.
   //Currently it takes a screen shot for a root window in a current zone.
   zone = e_util_zone_current_get(e_manager_current_get());
   if (!zone) return;

   evas = e_manager_comp_evas_get(e_manager_current_get());
   if (!evas) return;

   if (on_capture) return;
   on_capture = EINA_TRUE;

   rot = _scrn_rot_get(zone->container->manager->root);

   w = zone->container->manager->w;
   h = zone->container->manager->h;

   if ((rot == 90) || (rot == 270))
     {
        w2 = h; h2 = w;
     }
   else
     {
        w2 = w; h2 = h;
     }

   disp = XOpenDisplay(NULL);
   if (!disp) goto err;

   data = utilx_create_screen_shot(disp, w2, h2);
   if (!data) goto err;

   img = _create_img(evas, data, w2, h2);
   if (!img) goto err;

   if (!_capture_image_save(img, filepath)) goto err;

   _capture_visual_effect(disp, img, filepath, w, h, zone, rot);
   _capture_snd_effect();

   return;

err:
   utilx_release_screen_shot();
   if (disp) XCloseDisplay(disp);
   on_capture = EINA_FALSE;
}

static void
_e_mod_action_cb(E_Object *obj, const char *params)
{
   _scrn_shot();
}

static Eina_Bool _xclient_msg_cb(void *data, int type, void *event)
{
   const int _KEY_POWER = 124;
   const int _KEY_VOLUMEDOWN = 122;

   Ecore_X_Event_Client_Message *ev = event;
   if (!ev) return ECORE_CALLBACK_PASS_ON;

   if (ev->win != ecore_x_window_root_first_get() || ev->message_type != atom_xkey_msg) return ECORE_CALLBACK_PASS_ON;

   if (ev->message_type == atom_xkey_msg)
     if ((ev->data.l[0] + ev->data.l[1]) != (_KEY_POWER + _KEY_VOLUMEDOWN))
       return ECORE_CALLBACK_PASS_ON;

   LOGD("capture request by hardware key");
   _scrn_shot();

   return ECORE_CALLBACK_PASS_ON;
}

EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
      "Shot"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   e_module_delayed_set(m, 1);

   //Use the action event that e17 Keybinding provides
   act = e_action_add(MODULE_NAME);
   if (act)
     {
        act->func.go = _e_mod_action_cb;
        e_action_predef_name_set("Screen", "Take Screenshot", MODULE_NAME,
                                 NULL, NULL, 0);
     }

   //Use the key event that Keyrouter module provides
   atom_xkey_msg = ecore_x_atom_get("_XKEY_COMPOSITION");
   xclient_msg_handler = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
                                                 _xclient_msg_cb, NULL);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   if (act)
     {
        e_action_predef_name_del("Screen", "Take Screenshot");
        e_action_del(MODULE_NAME);
        act = NULL;
     }
   if (xclient_msg_handler)
     {
        ecore_event_handler_del(xclient_msg_handler);
        xclient_msg_handler = NULL;
     }
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}
