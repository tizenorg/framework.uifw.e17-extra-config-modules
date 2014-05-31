#ifndef __E_BACKKEY_EVENT_H__
#define __E_BACKKEY_EVENT_H__

#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
//#include <X11/extensions/XInput.h>
//#include <X11/extensions/XInput2.h>
//#include <X11/extensions/XTest.h>

#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/XF86keysym.h>

//#include <X11/Xutil.h>

typedef enum
{
   BACKKEY_EVENT_MOUSE_DOWN,
   BACKKEY_EVENT_MOUSE_MOVE,
   BACKKEY_EVENT_MOUSE_UP,
   BACKKEY_EVENT_MOUSE_ANY,
   BACKKEY_EVENT_MOUSE_GENERIC,
   BACKKEY_EVENT_NUM,
} backkey_event_type_e;

#define VIRTUAL_CORE_POINTER  2
#define VIRTUAL_CORE_KEYBOARD 3
#define VCP_XTEST_POINTER     4
#define VCP_XTEST_KEYBOARD    5

Eina_Bool backkey_event_mgr_init(void *user_data);
void      backkey_event_mgr_deinit(void *user_data);

#endif /* __E_BACKKEY_EVENT_H__ */
