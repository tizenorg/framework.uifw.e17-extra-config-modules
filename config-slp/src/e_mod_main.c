#include "e.h"
#include "e_mod_main.h"
#include "Elementary.h"

#define PI                    3.141592
#define BASE_LAYOUT_INCH      4.65
#define BASE_LAYOUT_WIDTH_PX  720
#define BASE_LAYOUT_HEIGHT_PX 1280
#define ROUND_DOUBLE(x)       (round((x)*100)/100)

static int _e_elm_config_scale_update(void);
static double _e_elm_config_dpi_get(double d_inch, int w_px, int h_px);

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

static double
_e_elm_config_dpi_get(double d_inch, int w_px, int h_px)
{
   double dpi;

   dpi = (sqrt((w_px * w_px) + (h_px * h_px))) / d_inch;

   return dpi;
}

static int
_e_elm_config_scale_update (void)
{
   int target_width, target_height, target_width_mm, target_height_mm, ret;
   double target_inch, scale, target_dpi, base_dpi;
   struct dirent *directory;
   struct stat file_info;
   DIR *dp;
   char buf[256];
   const char *home;

   target_width = 0;
   target_height = 0;
   target_width_mm = 0;
   target_height_mm = 0;

   ecore_x_randr_screen_current_size_get(ecore_x_window_root_first_get(), &target_width, &target_height, &target_width_mm, &target_height_mm);
   target_inch = ROUND_DOUBLE(sqrt(target_width_mm * target_width_mm + target_height_mm * target_height_mm) / 25.4);

   // Calculate PPD
   base_dpi = ROUND_DOUBLE(_e_elm_config_dpi_get(BASE_LAYOUT_INCH, BASE_LAYOUT_WIDTH_PX, BASE_LAYOUT_HEIGHT_PX));
   target_dpi = ROUND_DOUBLE(_e_elm_config_dpi_get(target_inch, target_width, target_height));

     // Calculate Scale factor
   scale = ROUND_DOUBLE(target_dpi / base_dpi);

   e_config->scale.factor = scale;

   elm_init(0, NULL);

   elm_config_scale_set(scale);
   elm_config_finger_size_set(scale * 60);

   home = getenv("HOME");
   dp = opendir("/opt/home/");
   if (!dp) return 0;
   while((directory = readdir(dp)))
     {
        if ((!strcmp(directory->d_name, ".")) || (!strcmp(directory->d_name, ".."))) continue;
        sprintf(buf, "/opt/home/%s", directory->d_name);
        if ((ret = lstat(buf, &file_info) == -1))
          {
             printf("error : can't get file stat \n");
             continue;
          }
        if (S_ISDIR(file_info.st_mode))
          {
             setenv("HOME", buf, 1);
             sprintf(buf, "/opt/home/%s/.elementary/config/slp/base.cfg", directory->d_name);

             ret = access(buf, F_OK);
             if (ret != 0)
               {
                  elm_config_save();
                  chown(buf, file_info.st_uid, file_info.st_gid);
               }
          }
     }
   closedir(dp);
   setenv("HOME", home, 1);
   elm_shutdown();

   system ("/bin/touch /opt/etc/.profile_ready");
   return 1;
}
