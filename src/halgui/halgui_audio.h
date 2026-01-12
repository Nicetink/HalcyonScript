/*
 * HalGUI Audio Module
 * Audio playback support for HalcyonScript
 * Uses Windows MCI (Media Control Interface)
 */

#ifndef HALGUI_AUDIO_H
#define HALGUI_AUDIO_H

#include <windows.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Audio player handle */
typedef struct HalAudioPlayer HalAudioPlayer;

/* Audio state */
typedef enum {
    HAL_AUDIO_STOPPED,
    HAL_AUDIO_PLAYING,
    HAL_AUDIO_PAUSED
} HalAudioState;

/* Audio event types */
typedef enum {
    HAL_AUDIO_EVENT_PLAY,
    HAL_AUDIO_EVENT_PAUSE,
    HAL_AUDIO_EVENT_STOP,
    HAL_AUDIO_EVENT_END,
    HAL_AUDIO_EVENT_ERROR,
    HAL_AUDIO_EVENT_POSITION
} HalAudioEventType;

/* Audio event callback */
typedef void (*HalAudioCallback)(HalAudioPlayer* player, HalAudioEventType event, void* userData);

/* Audio info */
typedef struct {
    int durationMs;         /* Total duration in milliseconds */
    int positionMs;         /* Current position in milliseconds */
    int channels;           /* Number of channels (1=mono, 2=stereo) */
    int sampleRate;         /* Sample rate in Hz */
    int bitRate;            /* Bit rate in kbps */
    char* title;            /* Track title (from metadata) */
    char* artist;           /* Artist name (from metadata) */
    char* album;            /* Album name (from metadata) */
} HalAudioInfo;

/* ============================================
   Audio API
   ============================================ */

/* Initialize audio system */
bool hal_audio_init(void);

/* Shutdown audio system */
void hal_audio_shutdown(void);

/* Create audio player */
HalAudioPlayer* hal_audio_create(void);

/* Destroy audio player */
void hal_audio_destroy(HalAudioPlayer* player);

/* Load audio file (MP3, WAV, WMA, etc.) */
bool hal_audio_load(HalAudioPlayer* player, const char* filePath);

/* Unload current audio */
void hal_audio_unload(HalAudioPlayer* player);

/* Playback control */
bool hal_audio_play(HalAudioPlayer* player);
bool hal_audio_pause(HalAudioPlayer* player);
bool hal_audio_stop(HalAudioPlayer* player);
bool hal_audio_resume(HalAudioPlayer* player);

/* Seek to position (milliseconds) */
bool hal_audio_seek(HalAudioPlayer* player, int positionMs);

/* Get/Set volume (0-100) */
int hal_audio_get_volume(HalAudioPlayer* player);
bool hal_audio_set_volume(HalAudioPlayer* player, int volume);

/* Get/Set mute */
bool hal_audio_is_muted(HalAudioPlayer* player);
bool hal_audio_set_mute(HalAudioPlayer* player, bool mute);

/* Get current state */
HalAudioState hal_audio_get_state(HalAudioPlayer* player);

/* Get current position (milliseconds) */
int hal_audio_get_position(HalAudioPlayer* player);

/* Get duration (milliseconds) */
int hal_audio_get_duration(HalAudioPlayer* player);

/* Get audio info */
HalAudioInfo* hal_audio_get_info(HalAudioPlayer* player);
void hal_audio_free_info(HalAudioInfo* info);

/* Set loop mode */
bool hal_audio_set_loop(HalAudioPlayer* player, bool loop);
bool hal_audio_is_looping(HalAudioPlayer* player);

/* Set event callback */
void hal_audio_set_callback(HalAudioPlayer* player, HalAudioCallback callback, void* userData);

/* Check if file is supported */
bool hal_audio_is_supported(const char* filePath);

/* Get supported formats */
const char* hal_audio_get_formats(void);

/* Process audio events (call in main loop) */
void hal_audio_process_events(void);

#ifdef __cplusplus
}
#endif

#endif /* HALGUI_AUDIO_H */
