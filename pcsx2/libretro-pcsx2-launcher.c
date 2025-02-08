#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "libretro.h"

#ifdef __WIN32__
   #include <windows.h>
#elif defined __linux__ || __APPLE__
   #include <glob.h>
#endif

static uint32_t *frame_buf;
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   (void)level;
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

void retro_init(void)
{
   frame_buf = calloc(320 * 240, sizeof(uint32_t));
}

void retro_deinit(void)
{
   free(frame_buf);
   frame_buf = NULL;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "pcsx2 Launcher";
   info->library_version  = "0.1a";
   info->need_fullpath    = true;
   info->valid_extensions = "iso|chd";
}

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   float aspect = 4.0f / 3.0f;
   float sampling_rate = 30000.0f;

   info->timing = (struct retro_system_timing) {
      .fps = 60.0,
      .sample_rate = sampling_rate,
   };

   info->geometry = (struct retro_game_geometry) {
      .base_width   = 320,
      .base_height  = 240,
      .max_width    = 320,
      .max_height   = 240,
      .aspect_ratio = aspect,
   };
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   bool no_content = true;
   cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

   if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_reset(void)
{
   // Nothing needs to happen when the game is reset.
}

/**
 * libretro callback; Called every game tick.
 *
 * Once the core has run, we will attempt to exit, since xemu is done.
 */
void retro_run(void)
{
   // Clear the display.
   unsigned stride = 320;
   video_cb(frame_buf, 320, 240, stride << 2);

   // Shutdown the environment now that xemu has loaded and quit.
   environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

/**
 * libretro callback; Called when a game is to be loaded.
 */
bool retro_load_game(const struct retro_game_info *info)
{
   #ifdef __linux__
      char pcsx2_exec[512];
      glob_t buf;

      if (glob("~/.config/retroarch/system/pcsx2/pcsx2*.*", 0, NULL, &buf) == 0) {
         snprintf(pcsx2_exec, sizeof(pcsx2_exec), "%s", buf.gl_pathv[0]);
         globfree(&buf);
      }
   #elif defined __WIN32__
      WIN32_FIND_DATA findFileData;
      HANDLE hFind;
      char pcsx2_exec[MAX_PATH];
      char pcsx2_dir[256] = "C:\\RetroArch-Win64\\system\\pcsx2";
      char searchPath[MAX_PATH];

      snprintf(searchPath, MAX_PATH, "%s\\pcsx2*.exe", pcsx2_dir);
      hFind = FindFirstFile(searchPath, &findFileData);

      if (hFind == INVALID_HANDLE_VALUE) {
         printf("pcsx2 not found!\n");
         return NULL;
      }
      
      snprintf(pcsx2_exec, MAX_PATH, "%s\\%s", pcsx2_dir, findFileData.cFileName);
      FindClose(hFind);
   #elif defined __APPLE__
      //TODO: Figure path for macOS
   #endif

   // Concat pcsx2 arguments, enclose info->path in double quotes to avoid truncation.
   const char *args[] = {" ", "-fullscreen ", "\"", info->path, "\""};

    for (size_t i = 0; i < 5; i++) {
      strncat(pcsx2_exec, args[i], strlen(args[i]));
   }

    printf("pcsx2 path: %s\n", pcsx2_exec);

   if (system(pcsx2_exec) == 0) {
      printf("libretro-pcsx2-launcher: Finished running pcsx2.\n");
      return true;
   }

   printf("libretro-pcsx2-launcher: Failed running pcsx2. Place it in the right path and try again\n");
   return false;
}

void retro_unload_game(void)
{
   // Nothing needs to happen when the game unloads.
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
   return retro_load_game(info);
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return true;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return true;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void)
{
   // Nothing.
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}
