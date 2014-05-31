#include "e_mod_backkey.h"
#include <Ecore_X.h>
#include <Ecore_X_Atoms.h>
#include <X11/Xlib.h>

static Eina_Bool
_backkey_is_enabled(const Ecore_X_Window win, const Ecore_X_Atom atom)
{
   unsigned int enable = -1;
   int ret = -1, count = 0;
   unsigned char *prop_data = NULL;

   BACKKEY_FUNC_ENTER();

   ret = ecore_x_window_prop_property_get(win, atom, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if (ret && prop_data)
     {
        memcpy(&enable, prop_data, sizeof(unsigned int));
        BACKKEY_DEBUG("backkey-tizen is enabled : %s", enable ? "true" : "false");
     }
   if (prop_data) free(prop_data);
   if (enable == 1) return EINA_TRUE;
   else return EINA_FALSE;
}

int
main(int argc, char **argv)
{
   Ecore_X_Atom x_atom_backkey = 0;
   Ecore_X_Window win = 0;
   unsigned int enable = 0;

   BACKKEY_FUNC_ENTER();

   if (argc != 2)
     {
        fprintf(stderr, "Usage : %s ON/OFF\n", argv[0]);
        return EXIT_FAILURE;
     }

   ecore_x_init(NULL);

   x_atom_backkey = ecore_x_atom_get("_E_BACKKEY_ENABLE");
   if (x_atom_backkey == None)
     {
        BACKKEY_ERROR("failed to get atom of _E_BACKKEY_ENABLE");
        return EXIT_FAILURE;
     }

   win = ecore_x_window_root_first_get();
   if (win <= 0)
     {
        BACKKEY_ERROR("failed to get root window");
        return EXIT_FAILURE;
     }

   if (!strcmp(argv[1], "ON"))
     {
        if (_backkey_is_enabled(win, x_atom_backkey) == EINA_TRUE)
          {
             fprintf(stderr, "backkey-module is already enabled\n");
             return EXIT_SUCCESS;
          }
        enable = 1;
        ecore_x_window_prop_card32_set(win, x_atom_backkey, &enable, 1);
        ecore_x_shutdown();

        return EXIT_SUCCESS;
     }
   else if (!strcmp(argv[1], "OFF"))
     {
        if (_backkey_is_enabled(win, x_atom_backkey) == EINA_FALSE)
          {
             fprintf(stderr, "backkey-module is already disabled\n");
             return EXIT_SUCCESS;
          }
        enable = 0;
        ecore_x_window_prop_card32_set(win, x_atom_backkey, &enable, 1);
        ecore_x_shutdown();

        return EXIT_SUCCESS;
     }
   else
     {
        fprintf(stderr, "Usage : %s ON/OFF\n", argv[0]);
        ecore_x_shutdown();
        return EXIT_FAILURE;
     }

   return EXIT_SUCCESS;
}

