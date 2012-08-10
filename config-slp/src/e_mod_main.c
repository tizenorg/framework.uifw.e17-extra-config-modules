#include "e.h"
#include "e_mod_main.h"
#include "Elementary.h"

#define PI                    3.141592
#define BASE_LAYOUT_INCH      4.65
#define BASE_LAYOUT_WIDTH_PX  720
#define BASE_LAYOUT_HEIGHT_PX 1280
#define ROUND_DOUBLE(x)       (round((x)*100)/100)
#define MOBILE_PROFILE        "mobile"

static int _e_elm_config_scale_update(void);
static double _e_elm_config_dpi_get(double d_inch, int w_px, int h_px);

static char fullpath[1024];

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
_file_owner_change(uid_t uid, gid_t gid)
{
   struct stat    statbuf;
   struct dirent  *dirp;
   DIR            *dp;
   char           *ptr;
   int            ret;

   if (lstat(fullpath, &statbuf) < 0)
     {
        fprintf(stderr, "%s : stat error \n", fullpath);
        return 0;
     }
   if (S_ISDIR(statbuf.st_mode) == 0)
     return 0;

   ptr = fullpath + strlen(fullpath);
   *ptr++ = '/';
   *ptr = 0;

   if ((dp = opendir(fullpath)) == NULL)
     {
        fprintf(stderr, "can't read %s directory \n", fullpath);
        return 0;
     }

   while ((dirp = readdir(dp)) != NULL)
     {
        if (strcmp(dirp->d_name, ".") == 0  ||
            strcmp(dirp->d_name, "..") == 0)
          continue;

        strcpy(ptr, dirp->d_name);
        if ((chown(fullpath, uid, gid) == -1))
          fprintf(stderr, "%s chown error \n", fullpath);

        if ((ret = _file_owner_change(uid, gid)) != 0)
          break;
   }
   *(ptr-1) = 0;

   if (closedir(dp) < 0)
     fprintf(stderr, "can't close directory %s", fullpath);

   return ret;
}


static int
_e_mod_config_elm_profile_save(char *profile_name, double scale)
{
   struct dirent *directory;
   struct stat file_info;
   DIR *dp;
   char buf[256];
   int ret;

   setenv("HOME", "/home/app", 1);
   elm_init(0, NULL);

   elm_scale_set(scale);
   elm_finger_size_set(scale * 60);

   dp = opendir("/home/");
   if (!dp) return 0;

   while((directory = readdir(dp)))
     {
        if ((!strcmp(directory->d_name, ".")) || (!strcmp(directory->d_name, ".."))) continue;
        snprintf(buf, sizeof(buf), "/home/%s", directory->d_name);
        if ((ret = lstat(buf, &file_info) == -1))
          {
             printf("error : can't get file stat \n");
             continue;
          }
        if (S_ISDIR(file_info.st_mode))
          {
             setenv("HOME", buf, 1);
             elm_config_save();
             snprintf(buf, sizeof(buf), "/home/%s/.elementary", directory->d_name);
             strcpy(fullpath, buf);
             if ((chown(fullpath, file_info.st_uid, file_info.st_gid)) == -1)
               fprintf(stderr, "%s chown error \n", fullpath);
             _file_owner_change(file_info.st_uid, file_info.st_gid);
          }
     }
   closedir(dp);
   elm_shutdown();

   return 1;
}

static int
_e_elm_config_scale_update (void)
{
   int target_width, target_height, target_width_mm, target_height_mm;
   double target_inch, scale, target_dpi, base_dpi;

   target_width = 0;
   target_height = 0;
   target_width_mm = 0;
   target_height_mm = 0;

   ecore_x_randr_screen_current_size_get(ecore_x_window_root_first_get(), &target_width, &target_height, &target_width_mm, &target_height_mm);
   target_inch = ROUND_DOUBLE(sqrt(target_width_mm * target_width_mm + target_height_mm * target_height_mm) / 25.4);

   // Calculate DPI
   base_dpi = ROUND_DOUBLE(_e_elm_config_dpi_get(BASE_LAYOUT_INCH, BASE_LAYOUT_WIDTH_PX, BASE_LAYOUT_HEIGHT_PX));
   target_dpi = ROUND_DOUBLE(_e_elm_config_dpi_get(target_inch, target_width, target_height));

     // Calculate Scale factor
   scale = ROUND_DOUBLE(target_dpi / base_dpi);

   e_config->scale.factor = scale;

   // calculate elementray scale factor
   _e_mod_config_elm_profile_save(MOBILE_PROFILE, scale);

   system ("/bin/touch /opt/etc/.profile_ready");
   return 1;
}
