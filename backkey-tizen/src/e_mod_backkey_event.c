#include <Ecore.h>
#include <Ecore_Input.h>
#include "Ecore_X.h"
#include "e_mod_backkey_event.h"
#include "e_mod_backkey.h"
#include <string.h>

#define LONG_PRESS_ELAPSED_TIME 0.6

typedef struct
{
   int          device;
   unsigned int timestamp;

   int          x;
   int          y;

   double       pressure;
} backkey_mouse_info_s;

typedef struct
{
   void                 *user_data;

   Ecore_Event_Handler  *handler[BACKKEY_EVENT_NUM];
   Eina_List            *pressed_mouse_list;

   int                   xi_opcode;
   int                   xi_event;
   int                   xi_error;
   int                   xtst_opcode;
   int                   xtst_event;
   int                   xtst_error;
   int                  *slots;
   int                   n_slots;
   unsigned int          device_num;
   unsigned int         *button_pressed;
   unsigned int         *button_moved;
   backkey_mouse_info_s *last_raw;
} backkey_event_mgr_s;

static backkey_event_mgr_s g_backkey_event_mgr;
static Eina_Bool confirm_move_button = EINA_FALSE;

#define BUTTON_MOVE_THRESHHOLD 3
#define USE_GENERIC_EVENT      0

#if 0
static void
send_backkey_event(BackkeyInfo *info, Bool pressed)
{
   Ecore_X_Display *d = NULL;
   int kc = 0;
   BACKKEY_FUNC_ENTER();

   d = info->d;
   if (!d)
     {
        BACKKEY_ERROR("[backkey-tizen] Failed to get Display info");
        return;
     }

   kc = XKeysymToKeycode(d, XK_Cancel);
   XTestFakeKeyEvent(d, kc, pressed, 0);
   BACKKEY_DEBUG("[backkey-tizen], backkey is %s, kc : %d", (pressed == True) ? "pressed" : "released", kc);
   return;
}

static Eina_Bool
position_is_inside_of_backkey(BackkeyInfo *info, backkey_mouse_info_s *raw)
{
   Eina_Rectangle rect;
   BACKKEY_FUNC_ENTER();

   rect.x = info->width - BACKKEY_ICON_WIDTH;
   rect.y = info->height - BACKKEY_ICON_HEIGHT;
   rect.w = BACKKEY_ICON_WIDTH;
   rect.h = BACKKEY_ICON_HEIGHT;

   BACKKEY_DEBUG("[backkey-tizen], rect = [%d, %d, %d, %d], cord_x : %d, cord_y : %d", rect.x, rect.y, rect.w, rect.h, raw->x, raw->y);

   return eina_rectangle_coords_inside(&rect, raw->x, raw->y);
}

static Eina_Bool
position_is_inside_of_morekey(BackkeyInfo *info, backkey_mouse_info_s *raw)
{
   Eina_Rectangle rect;
   BACKKEY_FUNC_ENTER();

   rect.x = 0;
   rect.y = info->height - MOREKEY_ICON_HEIGHT;
   rect.w = MOREKEY_ICON_WIDTH;
   rect.h = MOREKEY_ICON_HEIGHT;

   BACKKEY_DEBUG("[backkey-tizen], rect = [%d, %d, %d, %d], cord_x : %d, cord_y : %d", rect.x, rect.y, rect.w, rect.h, raw->x, raw->y);

   return eina_rectangle_coords_inside(&rect, raw->x, raw->y);
}

#endif

#if USE_GENERIC_EVENT
static int
get_finger_index(int deviceid)
{
   int i;
   for (i = 0; i < g_backkey_event_mgr.n_slots; i++)
     {
        if (g_backkey_event_mgr.slots[i] == deviceid)
          return i;
     }

   return -1;
}

static const char *
type_to_name(int evtype)
{
   const char *name;

   switch (evtype)
     {
      case XI_RawKeyPress:
        name = "RawKeyPress";
        break;

      case XI_RawKeyRelease:
        name = "RawKeyRelease";
        break;

      case XI_RawButtonPress:
        name = "RawButtonPress";
        break;

      case XI_RawButtonRelease:
        name = "RawButtonRelease";
        break;

      case XI_RawMotion:
        name = "RawMotion";
        break;

      case XI_RawTouchBegin:
        name = "RawTouchBegin";
        break;

      case XI_RawTouchUpdate:
        name = "RawTouchUpdate";
        break;

      case XI_RawTouchEnd:
        name = "RawTouchEnd";
        break;

      default:
        name = "unknown event type";
        break;
     }

   return name;
}

static void
get_rawdata_from_event(XIRawEvent *event, int *x, int *y, int *deviceid, int *pressure)
{
   int i;
   double *val, *raw_val;

   if (!event)
     {
        BACKKEY_DEBUG("[backkey-tizen] event is NIL");
        return;
     }

   val = event->valuators.values;
   raw_val = event->raw_values;

   if (!val || !raw_val)
     {
        BACKKEY_DEBUG("[backkey-tizen] [%s] : %d, val : %p, raw_val : %p", __FUNCTION__, __LINE__, val, raw_val);
        return;
     }

   for (i = 0; (i < event->valuators.mask_len * 8) && (i < 2); i++)
     {
        if (event->valuators.mask)
          {
             if (XIMaskIsSet(event->valuators.mask, i))
               {
                  *deviceid = get_finger_index(event->deviceid);
                  if (i == 0)
                    {
                       *x = (int)(*(val + i));
                    }
                  else if (i == 1)
                    {
                       *y = (int)(*(val + i));
                    }
               }
             else
               *deviceid = get_finger_index(event->deviceid);
          }
     }
}

static Eina_Bool
backkey_event_mgr_event_mouse_generic_handler(void *data, int type, void *event_info)
{
   Ecore_X_Event_Generic *e = (Ecore_X_Event_Generic *)event_info;
   int x = -1, y = -1, deviceid = -1, pressure = 1;
   backkey_mouse_info_s info;
   XIRawEvent *raw = NULL;
   XGenericEventCookie *cookie = NULL;

   BACKKEY_FUNC_ENTER();

   if ((!e) || (e->extension != g_backkey_event_mgr.xi_opcode))
     return ECORE_CALLBACK_PASS_ON;

   BACKKEY_DEBUG("[backkey-tizen] x generic event's evtype : %d, XI_RawButtonPress : %d, XI_RawButtonRelease : %d, XI_RawMotion : %d", e->evtype, XI_RawButtonPress, XI_RawButtonRelease, XI_RawMotion);

   if (e->evtype != XI_RawButtonPress && e->evtype != XI_RawButtonRelease && e->evtype != XI_RawMotion)
     return ECORE_CALLBACK_PASS_ON;

   raw = (XIRawEvent *)e->data;
   cookie = (XGenericEventCookie *)e->cookie;

   if (!raw || !cookie)
     return ECORE_CALLBACK_PASS_ON;

   if ((raw->deviceid == VIRTUAL_CORE_POINTER) || (raw->deviceid == VCP_XTEST_POINTER))
     return ECORE_CALLBACK_PASS_ON;

   get_rawdata_from_event(raw, &x, &y, &deviceid, &pressure);
   if (deviceid == -1)
     return ECORE_CALLBACK_PASS_ON;

   BACKKEY_DEBUG("[backkey-tizen] x : %d, y : %d, deviceid : %d, preesure : %d", x, y, deviceid, pressure);

   if (e->evtype == XI_RawMotion)
     {
        memset(&info, 0, sizeof(backkey_mouse_info_s));
        info.device = deviceid;
        info.timestamp = ecore_loop_time_get() * 1000.0;
        info.x = x;
        info.y = y;
        info.pressure = pressure;
        memcpy(&(g_backkey_event_mgr.last_raw[info.device]), &info, sizeof(backkey_mouse_info_s));
     }

   switch (e->evtype)
     {
      case XI_RawButtonPress: /*XI_RawButtonPress*/
        if ((g_backkey_event_mgr.button_pressed[deviceid] == 0) && &(g_backkey_event_mgr.last_raw[deviceid]))
          {
             //backkey_main_view_mouse_down(g_backkey_event_mgr.user_data, &(g_backkey_event_mgr.last_raw[deviceid]));
             BACKKEY_DEBUG("ButtonPress");
             if (position_is_inside_of_backkey(data, &g_backkey_event_mgr.last_raw[deviceid]) == EINA_TRUE)
               {
                  BACKKEY_DEBUG("Inside of Back Button");
                  if (ecore_x_test_fake_key_down(BACKKEY_KEYCODE) != EINA_TRUE)
                    BACKKEY_ERROR("[backkey-tizen] Failed to send XF86Stop Press");
               }
             else if (position_is_inside_of_morekey(data, &g_backkey_event_mgr.last_raw[deviceid]) == EINA_TRUE)
               {
                  BACKKEY_DEBUG("Inside of More Button");
                  if (ecore_x_test_fake_key_down(MOREKEY_KEYCODE) != EINA_TRUE)
                    BACKKEY_ERROR("[backkey-tizen] Failed to send XF86Send Press");
               }

             g_backkey_event_mgr.button_pressed[deviceid] = 1;
          }
        break;

      case XI_RawButtonRelease: /*XI_RawButtonRelease*/
        if ((g_backkey_event_mgr.button_pressed[deviceid] == 1) && &(g_backkey_event_mgr.last_raw[deviceid]))
          {
             BACKKEY_DEBUG("ButtonRelease");
             if (position_is_inside_of_backkey(data, &g_backkey_event_mgr.last_raw[deviceid]) == EINA_TRUE)
               {
                  BACKKEY_DEBUG("Inside of Back Button");
                  if (ecore_x_test_fake_key_up(BACKKEY_KEYCODE) != EINA_TRUE)
                    BACKKEY_ERROR("[backkey-tizen] Failed to send XF86Stop Release");
               }
             else if (position_is_inside_of_morekey(data, &g_backkey_event_mgr.last_raw[deviceid]) == EINA_TRUE)
               {
                  BACKKEY_DEBUG("Inside of More Button");
                  if (ecore_x_test_fake_key_up(MOREKEY_KEYCODE) != EINA_TRUE)
                    BACKKEY_ERROR("[backkey-tizen] Failed to send XF86Send Release");
               }

             //backkey_main_view_mouse_up(g_backkey_event_mgr.user_data, &(g_backkey_event_mgr.last_raw[deviceid]));
             g_backkey_event_mgr.button_pressed[deviceid] = 0;
          }
        break;

      case XI_RawMotion: /*XI_RawMotion*/
        if (g_backkey_event_mgr.button_pressed[deviceid] == 1)
          {
             BACKKEY_DEBUG("ButtonMotion");
             //backkey_main_view_mouse_move(g_backkey_event_mgr.user_data, &info);
          }

        break;

      case XI_RawTouchBegin:
        break;

      case XI_RawTouchUpdate:
        break;

      case XI_RawTouchEnd:
        break;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
backkey_event_mgr_backkey_init(void *data)
{
   XIEventMask *mask = NULL;
   int num_mask = 0;
   int deviceid = -1;
   int rc;
   Ecore_X_Display *d = NULL;
   BackkeyInfo *info = (BackkeyInfo *)data;

   BACKKEY_FUNC_ENTER();

   d = info->d;
   if (!d)
     {
        BACKKEY_ERROR("[backkey-tizen] Failed to get display !");
        return EINA_FALSE;
     }

   if (!XQueryExtension(d, "XInputExtension", &g_backkey_event_mgr.xi_opcode, &g_backkey_event_mgr.xi_event, &g_backkey_event_mgr.xi_error))
     {
        BACKKEY_ERROR("[backkey-tizen] No XInput extension is available.");
        return EINA_FALSE;
     }
   if (!XQueryExtension(d, "XTEST", &g_backkey_event_mgr.xtst_opcode, &g_backkey_event_mgr.xtst_event, &g_backkey_event_mgr.xtst_error))
     {
        BACKKEY_ERROR("[backkey-tizen] No XTEST(Xtst) extension is available.");
        return EINA_FALSE;
     }

   Ecore_X_Window xroot = ecore_x_window_root_first_get();

   mask = XIGetSelectedEvents(d, xroot, &num_mask);

   if (mask)
     {
        XISetMask(mask->mask, XI_RawButtonPress);
        XISetMask(mask->mask, XI_RawButtonRelease);
        XISetMask(mask->mask, XI_RawMotion);
#if 0
        XISetMask(mask->mask, XI_RawTouchBegin);
        XISetMask(mask->mask, XI_RawTouchUpdate);
        XISetMask(mask->mask, XI_RawTouchEnd);
#endif

        rc = XISelectEvents(d, xroot, mask, 1);
        if (Success != rc)
          {
             BACKKEY_ERROR("[backkey-tizen] Failed to select XInput extension events !");
             return EINA_FALSE;
          }
     }
   else
     {
        mask = calloc(1, sizeof(XIEventMask));
        if (!mask)
          {
             BACKKEY_ERROR("[backkey-tizen] [%s]:[%d] Failed to allocate memory !", __FUNCTION__, __LINE__);
             return EINA_FALSE;
          }

        mask->deviceid = (deviceid == -1) ? XIAllDevices : deviceid;
        mask->mask_len = XIMaskLen(XI_LASTEVENT);
        mask->mask = calloc(mask->mask_len, sizeof(char));
        if (!mask->mask)
          {
             BACKKEY_ERROR("[backkey-tizen] [%s][%d] Failed to allocate memory !", __FUNCTION__, __LINE__);
             return EINA_FALSE;
          }

        XISetMask(mask->mask, XI_RawButtonPress);
        XISetMask(mask->mask, XI_RawButtonRelease);
        XISetMask(mask->mask, XI_RawMotion);
        XISetMask(mask->mask, XI_RawTouchBegin);
        XISetMask(mask->mask, XI_RawTouchUpdate);
        XISetMask(mask->mask, XI_RawTouchEnd);

        rc = XISelectEvents(d, xroot, mask, 1);
        if (Success != rc)
          {
             BACKKEY_ERROR("[backkey-tizen] Failed to select XInput extension events !");
             return EINA_FALSE;
          }

        if (mask->mask) free(mask->mask);
        if (mask) free(mask);
     }

   return EINA_TRUE;
}

static int
backkey_event_mgr_backkey_get_multi_touch_info(void *data)
{
   int i, idx = 0;
   int ndevices = 0;
   XIDeviceInfo *dev, *info = NULL;
   Ecore_X_Display *d = NULL;
   BackkeyInfo *backkey_info = (BackkeyInfo *)data;

   BACKKEY_FUNC_ENTER();

   d = backkey_info->d;
   if (!d)
     {
        BACKKEY_ERROR("[backkey-tizen] Failed to get display !");
        return 0;
     }

   if (g_backkey_event_mgr.xi_opcode < 0 ) return 0;

   info = XIQueryDevice(d, XIAllDevices, &ndevices);

   if (!info)
     {
        BACKKEY_ERROR("[backkey-tizen] There is no queried XI device.");
        return 0;
     }

   for ( i = 0; i < ndevices; i++ )
     {
        dev = &info[i];

        if ((XISlavePointer == dev->use) || (XIFloatingSlave == dev->use))
          {
             //skip XTEST Pointer and non-touch device(s)
             if ( strcasestr(dev->name, "XTEST") || !strcasestr(dev->name, "touch"))
               continue;
             idx++;
          }
     }

   g_backkey_event_mgr.slots = malloc(idx * sizeof(int));
   if (!g_backkey_event_mgr.slots) return 0;

   idx = 0;
   for ( i = 0; i < ndevices; i++ )
     {
        dev = &info[i];

        if ((XISlavePointer == dev->use) || (XIFloatingSlave == dev->use))
          {
             //skip XTEST Pointer and non-touch device(s)
             if ( strcasestr(dev->name, "XTEST") || !strcasestr(dev->name, "touch"))
               continue;

             g_backkey_event_mgr.slots[idx] = dev->deviceid;
             idx++;
          }
     }

   XIFreeDeviceInfo(info);

   g_backkey_event_mgr.n_slots = idx;
   return idx;
}

#else
static Eina_Bool
button_long_press_handler(void *data)
{
   BackkeyInfo *info = NULL;
   info = (BackkeyInfo *)data;

   BACKKEY_FUNC_ENTER();
   /*
    * TODO
    * As this function always return CANCEL
    * I don't need to keep this timer
    */
   info->long_press_timer = NULL;
   /*
    * TODO
    * Let's move MoreKey & Backkey to new coord.
    */
   confirm_move_button = EINA_TRUE;

   return ECORE_CALLBACK_CANCEL;
}

static void
bothkey_object_mouse_move_handler(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev = NULL;
   BackkeyInfo *info = NULL;
   int dx = -1, dy = -1, c_y = 0, o_y = 0;

   BACKKEY_FUNC_ENTER();

   if (confirm_move_button != EINA_TRUE)
     {
        BACKKEY_DEBUG("[backkey-tizen] No long press!!! move will be ignored!!!!!!!!!!!!!!!!1");
        return;
     }

   ev = (Evas_Event_Mouse_Move *)event_info;
   info = (BackkeyInfo *)data;

   if (!ev || !info)
     {
        BACKKEY_ERROR("[backkey-tizen] [%s] : %d ev or info is NIL", __FUNCTION__, __LINE__);
        return;
     }

   /*
    * TODO
    * Let's tune a bit to improve performance
    * If dy is less than BUTTON_MOVE_THRESHHOLD
    * I will ignore moving button
    *
    * My button only will be move vertically(dy), I don't need to consider dx
    */

   c_y = ev->cur.output.y;
   o_y = ev->prev.output.y;

   evas_object_geometry_get(info->backkey_icon, &dx, &dy, NULL, NULL);
   if (dx < 0 || dy < 0)
     {
        BACKKEY_ERROR("[backkey-tizen] In this app, button should not be at '-' coord");
        return;
     }

#if 0
   if (abs(dy - c_y) < BUTTON_MOVE_THRESHHOLD)
     {
        BACKKEY_DEBUG("[backkey-tizen] dy is too small & move is tuned out!!!!!!!!!!!!!!!!1");
        return;
     }
   else if (abs(c_y - o_y) < BUTTON_MOVE_THRESHHOLD)
     {
        BACKKEY_DEBUG("[backkey-tizen] dy is too small & move is tuned out!!!!!!!!!!!!!!!!1");
        return;
     }
#endif

   /*
    * TODO
    * CHECK new dy + ICON_HEIGHT is valid!!!!!!!
    * I will reuse dx.
    */

   dy = c_y;

   if ((c_y - BACKKEY_ICON_HEIGHT / 2) < 0)
     dy = 0;
   if ((c_y + BACKKEY_ICON_HEIGHT / 2) > info->height)
     dy = info->height - BACKKEY_ICON_HEIGHT;

   BACKKEY_DEBUG("[backkey-tizen] dx : %d, dy : %d, c_y : %d, o_y : %d", dx, dy, c_y, o_y);

   evas_object_move(info->backkey_icon, dx, dy);
   evas_object_move(info->morekey_icon, 0 + MOREKEY_ICON_LEFT_MARGIN, dy);

   evas_object_show(info->backkey_icon);
   evas_object_show(info->morekey_icon);
}

static Eina_Bool
backkey_key_event_send(const char *keyname, const Eina_Bool press)
{
   Ecore_X_Window keygrab_win;
   Ecore_X_Atom type = ecore_x_atom_get("_HWKEY_EMULATION");
   char msg_data[20];
   int ret = -1;

   Ecore_X_Window *_keygrab_win = NULL;
   int num;
   ret = ecore_x_window_prop_property_get(0, type, ECORE_X_ATOM_WINDOW, 32, (unsigned char **)&_keygrab_win, &num);
   if (ret == 0)
     {
        BACKKEY_ERROR("Failed to call ecore_x_window_prop_property_get");
        if (_keygrab_win) free(_keygrab_win);
        return EINA_FALSE;
     }
   if (!_keygrab_win)
     {
        BACKKEY_ERROR("Failed to get the key grabber window");
        return EINA_FALSE;
     }
   keygrab_win = *_keygrab_win;
   free(_keygrab_win);

   if (press == EINA_TRUE)
     {
        snprintf(msg_data, sizeof(msg_data), "Px%s", keyname);
        if (!ecore_x_client_message8_send(keygrab_win, type, msg_data, sizeof(msg_data)))
          {
             BACKKEY_ERROR("Failed to send message for h/w press");
             return EINA_FALSE;
          }
     }
   else
     {
        //Release
        snprintf(msg_data, sizeof(msg_data), "Rx%s", keyname);
        if (!ecore_x_client_message8_send(keygrab_win, type, msg_data, sizeof(msg_data)))
          {
             BACKKEY_ERROR("Failed to send message for h/w release");
             return EINA_FALSE;
          }
     }
   return EINA_TRUE;
}

static void
backkey_object_mouse_down_handler(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev = NULL;
   BackkeyInfo *info = NULL;
   char backkey_press_path[PATH_MAX] = {0, };
   Evas_Load_Error err;
   Ecore_X_Window win;

   sprintf(backkey_press_path, "%s/%s", e_module_dir_get(backkey_mod), BACKKEY_ICON_PRESS);

   BACKKEY_FUNC_ENTER();
   ev = (Evas_Event_Mouse_Down *)event_info;
   info = (BackkeyInfo *)data;

   if (!ev || !info)
     {
        BACKKEY_ERROR("[backkty-tizen] [%s] : %d ev or info is NIL", __FUNCTION__, __LINE__);
        return;
     }

   if (info->long_press_timer)
     {
        BACKKEY_DEBUG("Already other timer exist, it will deleted and New one will be added");
        ecore_timer_del(info->long_press_timer);
        info->long_press_timer = ecore_timer_add(LONG_PRESS_ELAPSED_TIME, (Ecore_Task_Cb)button_long_press_handler, data);
        if (!info->long_press_timer)
          {
             BACKKEY_ERROR("[backkey-tizen] [%s] : %d failed to make a timer", __FUNCTION__, __LINE__);
             return;
          }
     }
   else
     {
        BACKKEY_DEBUG("add new timer");
        info->long_press_timer = ecore_timer_add(LONG_PRESS_ELAPSED_TIME, (Ecore_Task_Cb)button_long_press_handler, data);
        if (!info->long_press_timer)
          {
             BACKKEY_ERROR("[backkey-tizen] [%s] : %d failed to make a timer", __FUNCTION__, __LINE__);
             return;
          }
     }

   evas_object_image_file_set(info->backkey_icon, backkey_press_path, NULL);
   err = evas_object_image_load_error_get(info->backkey_icon);
   if (err != EVAS_LOAD_ERROR_NONE)
     BACKKEY_ERROR("[backkey-tizen] Failed to load image [%s]", backkey_press_path);

   BACKKEY_DEBUG("[backkey-tizen] x : %d, y : %d", ev->output.x, ev->output.y);

   if (backkey_key_event_send(BACKKEY_KEYCODE, EINA_TRUE) != EINA_TRUE)
     BACKKEY_ERROR("[backkey-tizen] Failed to send XF86Stop Press");
}

static void
backkey_object_mouse_up_handler(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev = NULL;
   BackkeyInfo *info = NULL;
   char backkey_path[PATH_MAX] = {0, };
   Evas_Load_Error err;
   Ecore_X_Window win;
   int dx = 0, dy = 0, w = 0, h = 0;
   Eina_Bool inside = EINA_FALSE;
   Eina_Rectangle rect;

   sprintf(backkey_path, "%s/%s", e_module_dir_get(backkey_mod), BACKKEY_ICON);

   BACKKEY_FUNC_ENTER();
   ev = (Evas_Event_Mouse_Up *)event_info;
   info = (BackkeyInfo *)data;

   if (!ev || !info)
     {
        BACKKEY_ERROR("[backkty-tizen] [%s] : %d ev or info is NIL", __FUNCTION__, __LINE__);
        return;
     }

   if (info->long_press_timer)
     {
        /*
         * TODO
         * You don't want to long press
         * Delete timer;
         */
        /*
         * FIXME
         * Is this a timing problem???
         * Race condition?
         */
        if (ecore_timer_del(info->long_press_timer) == NULL)
          {
             BACKKEY_ERROR("Failed to delete timer");
             return;
          }
        info->long_press_timer = NULL;
     }

   evas_object_image_file_set(info->backkey_icon, backkey_path, NULL);
   err = evas_object_image_load_error_get(info->backkey_icon);
   if (err != EVAS_LOAD_ERROR_NONE)
     BACKKEY_ERROR("[backkey-tizen] Failed to load image [%s]", backkey_path);

   evas_object_geometry_get(info->backkey_icon, &dx, &dy, &w, &h);
   rect.x = dx;
   rect.y = dy;
   rect.w = w;
   rect.h = h;
   inside = eina_rectangle_coords_inside(&rect, ev->output.x, ev->output.y);
   BACKKEY_DEBUG("[backkey-tizen] x : %d, y : %d", ev->output.x, ev->output.y);

   /*
    * TODO
    * Let's move MoreKey & Backkey to new coord.
    */
   if (confirm_move_button == EINA_TRUE)
     {
        confirm_move_button = EINA_FALSE;
        /*
         * TODO
         * Delete preview input mask and create new one!!!
         */

        if (info->backkey_input_region_id == 0 || info->morekey_input_region_id == 0)
          {
             BACKKEY_ERROR("[backkey-tizen] This code should not be called!!!");
             return;
          }

        if (e_manager_comp_input_region_id_set(info->manager, info->backkey_input_region_id,
                                               dx, dy, w, h) != EINA_TRUE )
          {
             BACKKEY_ERROR("[backkey-tizen] Failed to set comp input region, x : %d, y : %d, w : %d, h : %d", dx, dy, w, h);
             return;
          }

        if (e_manager_comp_input_region_id_set(info->manager, info->morekey_input_region_id,
                                               0, dy, w, h) != EINA_TRUE )
          {
             BACKKEY_ERROR("[backkey-tizen] Failed to set comp input region, x : %d, y : %d, w : %d, h : %d", 0, dy, w, h);
             return;
          }
     }
   else if (inside)
     {
        /* The key event will be sent on normal click only, not in case of long press
         * or if touch is released outside the button geometry
         */
        if (backkey_key_event_send(BACKKEY_KEYCODE, EINA_FALSE) != EINA_TRUE)
           BACKKEY_ERROR("[backkey-tizen] Failed to send XF86Stop Release");
     }
}

static void
morekey_object_mouse_down_handler(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev = NULL;
   BackkeyInfo *info = NULL;
   char morekey_press_path[PATH_MAX] = {0, };
   Evas_Load_Error err;
   Ecore_X_Window win;

   sprintf(morekey_press_path, "%s/%s", e_module_dir_get(backkey_mod), MOREKEY_ICON_PRESS);

   BACKKEY_FUNC_ENTER();
   ev = (Evas_Event_Mouse_Down *)event_info;
   info = (BackkeyInfo *)data;

   if (!ev || !info)
     {
        BACKKEY_ERROR("[backkty-tizen] [%s] : %d ev or info is NIL", __FUNCTION__, __LINE__);
        return;
     }

   if (info->long_press_timer)
     {
        BACKKEY_DEBUG("Already other timer exist, it will deleted and New one will be added");
        ecore_timer_del(info->long_press_timer);
        info->long_press_timer = ecore_timer_add(LONG_PRESS_ELAPSED_TIME, (Ecore_Task_Cb)button_long_press_handler, data);
        if (!info->long_press_timer)
          {
             BACKKEY_ERROR("[backkey-tizen] [%s] : %d failed to make a timer", __FUNCTION__, __LINE__);
             return;
          }
     }
   else
     {
        BACKKEY_DEBUG("add new timer");
        info->long_press_timer = ecore_timer_add(LONG_PRESS_ELAPSED_TIME, (Ecore_Task_Cb)button_long_press_handler, data);
        if (!info->long_press_timer)
          {
             BACKKEY_ERROR("[backkey-tizen] [%s] : %d failed to make a timer", __FUNCTION__, __LINE__);
             return;
          }
     }

   evas_object_image_file_set(info->morekey_icon, morekey_press_path, NULL);
   err = evas_object_image_load_error_get(info->morekey_icon);
   if (err != EVAS_LOAD_ERROR_NONE)
     BACKKEY_ERROR("[backkey-tizen] Failed to load image [%s]", morekey_press_path);

   BACKKEY_DEBUG("[backkey-tizen] x : %d, y : %d", ev->output.x, ev->output.y);
   if (backkey_key_event_send(MOREKEY_KEYCODE, EINA_TRUE) != EINA_TRUE)
     BACKKEY_ERROR("[backkey-tizen] Failed to send XF86Send Press");
}

static void
morekey_object_mouse_up_handler(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev = NULL;
   BackkeyInfo *info = NULL;
   char morekey_path[PATH_MAX] = {0, };
   Evas_Load_Error err;
   Ecore_X_Window win;
   int dx = 0, dy = 0, w = 0, h = 0;
   Eina_Bool inside = EINA_FALSE;
   Eina_Rectangle rect;

   sprintf(morekey_path, "%s/%s", e_module_dir_get(backkey_mod), MOREKEY_ICON);

   BACKKEY_FUNC_ENTER();
   ev = (Evas_Event_Mouse_Up *)event_info;
   info = (BackkeyInfo *)data;

   if (!ev || !info)
     {
        BACKKEY_ERROR("[backkty-tizen] [%s] : %d ev or info is NIL", __FUNCTION__, __LINE__);
        return;
     }

   if (info->long_press_timer)
     {
        /*
         * TODO
         * You don't want to long press
         * Delete timer;
         */
        /*
         * FIXME
         * Is this a timing problem???
         * Race condition?
         */
        if (ecore_timer_del(info->long_press_timer) == NULL)
          {
             BACKKEY_ERROR("Failed to delete timer");
             return;
          }

        info->long_press_timer = NULL;
     }

   evas_object_image_file_set(info->morekey_icon, morekey_path, NULL);
   err = evas_object_image_load_error_get(info->morekey_icon);
   if (err != EVAS_LOAD_ERROR_NONE)
     BACKKEY_ERROR("[backkey-tizen] Failed to load image [%s]", morekey_path);

   evas_object_geometry_get(info->backkey_icon, &dx, &dy, &w, &h);
   rect.x = 0;
   rect.y = dy;
   rect.w = w;
   rect.h = h;
   inside = eina_rectangle_coords_inside(&rect, ev->output.x, ev->output.y);
   BACKKEY_DEBUG("[backkey-tizen] x : %d, y : %d", ev->output.x, ev->output.y);

   /*
    * TODO
    * Let's move MoreKey & Backkey to new coord.
    */
   if (confirm_move_button == EINA_TRUE)
     {
        confirm_move_button = EINA_FALSE;
        /*
         * TODO
         * Delete preview input mask and create new one!!!
         */

        if (info->backkey_input_region_id == 0 || info->morekey_input_region_id == 0)
          {
             BACKKEY_ERROR("[backkey-tizen] This code should not be called!!!");
             return;
          }
        if (e_manager_comp_input_region_id_set(info->manager, info->backkey_input_region_id,
                                               dx, dy, w, h) != EINA_TRUE )
          {
             BACKKEY_ERROR("[backkey-tizen] Failed to set comp input region, x : %d, y : %d, w : %d, h : %d", dx, dy, w, h);
             return;
          }

        if (e_manager_comp_input_region_id_set(info->manager, info->morekey_input_region_id,
                                               0, dy, w, h) != EINA_TRUE )
          {
             BACKKEY_ERROR("[backkey-tizen] Failed to set comp input region, x : %d, y : %d, w : %d, h : %d", 0, dy, w, h);
             return;
          }
     }
   else if (inside)
     {
        /* The key event will be sent on normal click only, not in case of long press
         * or if touch is released outside the button geometry
         */
        if (backkey_key_event_send(MOREKEY_KEYCODE, EINA_FALSE) != EINA_TRUE)
          BACKKEY_ERROR("[backkey-tizen] Failed to send XF86Send Release");
     }
}

#endif

Eina_Bool
backkey_event_mgr_init(void *user_data)
{
   BackkeyInfo *info = NULL;
   //BACKKEY_FUNC_ENTER();

   info = (BackkeyInfo *)user_data;

   memset(&g_backkey_event_mgr, 0x0, sizeof(backkey_event_mgr_s));

   g_backkey_event_mgr.user_data = user_data;

#if USE_GENERIC_EVENT
   if (backkey_event_mgr_backkey_init(user_data) != EINA_TRUE)
     {
        BACKKEY_ERROR("[backkey-tizen] [%s] : %d, failed to event manager init", __FUNCTION__, __FUNCTION__);
        return EINA_FALSE;
     }

   g_backkey_event_mgr.device_num = backkey_event_mgr_backkey_get_multi_touch_info(user_data);
   g_backkey_event_mgr.button_pressed = (unsigned int *)calloc(1, sizeof(unsigned int) * g_backkey_event_mgr.device_num);
   g_backkey_event_mgr.button_moved = (unsigned int *)calloc(1, sizeof(unsigned int) * g_backkey_event_mgr.device_num);
   g_backkey_event_mgr.last_raw = (backkey_mouse_info_s *)calloc(1, sizeof(backkey_mouse_info_s) * g_backkey_event_mgr.device_num);

   g_backkey_event_mgr.handler[BACKKEY_EVENT_MOUSE_GENERIC] = ecore_event_handler_add(ECORE_X_EVENT_GENERIC, backkey_event_mgr_event_mouse_generic_handler, user_data);
#else
   if (!info->backkey_icon || !info->morekey_icon)
     {
        BACKKEY_ERROR("[backkey-tizen] backey_icon or morekey_icon is NIL");
        return EINA_FALSE;
     }

   if (info->manager)
     {
        info->backkey_input_region_id = e_manager_comp_input_region_id_new(info->manager);
        info->morekey_input_region_id = e_manager_comp_input_region_id_new(info->manager);
     }
   else
     {
        BACKKEY_ERROR("[backkey-tizen] There is no E_Manager now");
        return EINA_FALSE;
     }

   if (e_manager_comp_input_region_id_set(info->manager, info->backkey_input_region_id,
                                          info->width - BACKKEY_ICON_WIDTH,
                                          info->height - BACKKEY_ICON_HEIGHT,
                                          BACKKEY_ICON_WIDTH,
                                          BACKKEY_ICON_HEIGHT) != EINA_TRUE )
     {
        BACKKEY_ERROR("[backkey-tizen] Failed to set comp input region");
        return EINA_FALSE;
     }

   if (e_manager_comp_input_region_id_set(info->manager, info->morekey_input_region_id,
                                          0,
                                          info->height - MOREKEY_ICON_HEIGHT,
                                          MOREKEY_ICON_WIDTH,
                                          MOREKEY_ICON_HEIGHT) != EINA_TRUE )
     {
        BACKKEY_ERROR("[backkey-tizen] Failed to set comp input region");
        return EINA_FALSE;
     }

   evas_object_event_callback_add(info->backkey_icon, EVAS_CALLBACK_MOUSE_DOWN, backkey_object_mouse_down_handler, user_data);
   evas_object_event_callback_add(info->backkey_icon, EVAS_CALLBACK_MOUSE_UP, backkey_object_mouse_up_handler, user_data);
   evas_object_event_callback_add(info->backkey_icon, EVAS_CALLBACK_MOUSE_MOVE, bothkey_object_mouse_move_handler, user_data);
   evas_object_event_callback_add(info->morekey_icon, EVAS_CALLBACK_MOUSE_DOWN, morekey_object_mouse_down_handler, user_data);
   evas_object_event_callback_add(info->morekey_icon, EVAS_CALLBACK_MOUSE_UP, morekey_object_mouse_up_handler, user_data);
   evas_object_event_callback_add(info->morekey_icon, EVAS_CALLBACK_MOUSE_MOVE, bothkey_object_mouse_move_handler, user_data);

   return EINA_TRUE;

#endif
}

void
backkey_event_mgr_deinit(void *user_data)
{
   BackkeyInfo *info = NULL;
   BACKKEY_FUNC_ENTER();

   info = (BackkeyInfo *)user_data;

#if USE_GENERIC_EVENT
   int i = 0;
   for (; i < BACKKEY_EVENT_NUM; i++) {
        if (g_backkey_event_mgr.handler[i])
          {
             ecore_event_handler_del(g_backkey_event_mgr.handler[i]);
             g_backkey_event_mgr.handler[i] = NULL;
          }
     }
   free(g_backkey_event_mgr.button_pressed);
   free(g_backkey_event_mgr.button_moved);
   free(g_backkey_event_mgr.last_raw);
#else
   e_manager_comp_input_region_id_del(info->manager, info->backkey_input_region_id);
   e_manager_comp_input_region_id_del(info->manager, info->morekey_input_region_id);

   evas_object_event_callback_del(info->backkey_icon, EVAS_CALLBACK_MOUSE_DOWN, backkey_object_mouse_down_handler);
   evas_object_event_callback_del(info->backkey_icon, EVAS_CALLBACK_MOUSE_UP, backkey_object_mouse_up_handler);
   evas_object_event_callback_del(info->morekey_icon, EVAS_CALLBACK_MOUSE_DOWN, morekey_object_mouse_down_handler);
   evas_object_event_callback_del(info->morekey_icon, EVAS_CALLBACK_MOUSE_UP, morekey_object_mouse_up_handler);
#endif

   memset(&g_backkey_event_mgr, 0x0, sizeof(backkey_event_mgr_s));
}

