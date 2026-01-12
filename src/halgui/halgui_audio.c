/*
 * HalGUI Audio Module Implementation
 * Uses Windows MCI for audio playback
 */

#include "halgui_audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

/* Maximum number of audio players */
#define MAX_AUDIO_PLAYERS 16

/* Audio player structure */
struct HalAudioPlayer {
    int id;                     /* Unique ID for MCI */
    char* filePath;             /* Current file path */
    char alias[32];             /* MCI alias */
    HalAudioState state;        /* Current state */
    int volume;                 /* Volume 0-100 */
    bool muted;                 /* Mute state */
    bool loop;                  /* Loop mode */
    int duration;               /* Duration in ms */
    HalAudioCallback callback;  /* Event callback */
    void* userData;             /* User data for callback */
    bool loaded;                /* File loaded flag */
};

/* Global state */
static bool g_audioInitialized = false;
static int g_nextPlayerId = 1;
static HalAudioPlayer* g_players[MAX_AUDIO_PLAYERS] = {0};
static int g_playerCount = 0;

/* Helper: Send MCI command */
static bool mci_send(const char* command, char* result, int resultSize) {
    MCIERROR err = mciSendStringA(command, result, resultSize, NULL);
    if (err != 0) {
        char errorText[256];
        mciGetErrorStringA(err, errorText, sizeof(errorText));
        printf("[Audio] MCI Error: %s (command: %s)\n", errorText, command);
        return false;
    }
    return true;
}

/* Helper: Send MCI command (no result) */
static bool mci_cmd(const char* command) {
    return mci_send(command, NULL, 0);
}

/* Initialize audio system */
bool hal_audio_init(void) {
    if (g_audioInitialized) return true;
    
    /* Initialize COM for some audio codecs */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    
    g_audioInitialized = true;
    printf("[Audio] Audio system initialized\n");
    return true;
}

/* Shutdown audio system */
void hal_audio_shutdown(void) {
    if (!g_audioInitialized) return;
    
    /* Destroy all players */
    for (int i = 0; i < MAX_AUDIO_PLAYERS; i++) {
        if (g_players[i]) {
            hal_audio_destroy(g_players[i]);
        }
    }
    
    CoUninitialize();
    g_audioInitialized = false;
    printf("[Audio] Audio system shutdown\n");
}

/* Create audio player */
HalAudioPlayer* hal_audio_create(void) {
    if (!g_audioInitialized) {
        hal_audio_init();
    }
    
    if (g_playerCount >= MAX_AUDIO_PLAYERS) {
        printf("[Audio] Maximum number of players reached\n");
        return NULL;
    }
    
    HalAudioPlayer* player = (HalAudioPlayer*)calloc(1, sizeof(HalAudioPlayer));
    if (!player) return NULL;
    
    player->id = g_nextPlayerId++;
    player->volume = 100;
    player->state = HAL_AUDIO_STOPPED;
    snprintf(player->alias, sizeof(player->alias), "halplayer%d", player->id);
    
    /* Add to global list */
    for (int i = 0; i < MAX_AUDIO_PLAYERS; i++) {
        if (!g_players[i]) {
            g_players[i] = player;
            g_playerCount++;
            break;
        }
    }
    
    printf("[Audio] Created player %d\n", player->id);
    return player;
}

/* Destroy audio player */
void hal_audio_destroy(HalAudioPlayer* player) {
    if (!player) return;
    
    /* Stop and close */
    hal_audio_unload(player);
    
    /* Remove from global list */
    for (int i = 0; i < MAX_AUDIO_PLAYERS; i++) {
        if (g_players[i] == player) {
            g_players[i] = NULL;
            g_playerCount--;
            break;
        }
    }
    
    if (player->filePath) free(player->filePath);
    free(player);
}

/* Load audio file */
bool hal_audio_load(HalAudioPlayer* player, const char* filePath) {
    if (!player || !filePath) return false;
    
    /* Unload previous */
    hal_audio_unload(player);
    
    /* Store file path */
    player->filePath = strdup(filePath);
    
    /* Open file with MCI */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias %s", filePath, player->alias);
    
    if (!mci_cmd(cmd)) {
        /* Try as waveaudio */
        snprintf(cmd, sizeof(cmd), "open \"%s\" type waveaudio alias %s", filePath, player->alias);
        if (!mci_cmd(cmd)) {
            /* Try auto-detect */
            snprintf(cmd, sizeof(cmd), "open \"%s\" alias %s", filePath, player->alias);
            if (!mci_cmd(cmd)) {
                printf("[Audio] Failed to open: %s\n", filePath);
                free(player->filePath);
                player->filePath = NULL;
                return false;
            }
        }
    }
    
    /* Set time format to milliseconds */
    snprintf(cmd, sizeof(cmd), "set %s time format milliseconds", player->alias);
    mci_cmd(cmd);
    
    /* Get duration */
    char result[64];
    snprintf(cmd, sizeof(cmd), "status %s length", player->alias);
    if (mci_send(cmd, result, sizeof(result))) {
        player->duration = atoi(result);
    }
    
    /* Set initial volume */
    hal_audio_set_volume(player, player->volume);
    
    player->loaded = true;
    player->state = HAL_AUDIO_STOPPED;
    
    printf("[Audio] Loaded: %s (duration: %d ms)\n", filePath, player->duration);
    return true;
}

/* Unload current audio */
void hal_audio_unload(HalAudioPlayer* player) {
    if (!player || !player->loaded) return;
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "close %s", player->alias);
    mci_cmd(cmd);
    
    if (player->filePath) {
        free(player->filePath);
        player->filePath = NULL;
    }
    
    player->loaded = false;
    player->state = HAL_AUDIO_STOPPED;
    player->duration = 0;
}

/* Play */
bool hal_audio_play(HalAudioPlayer* player) {
    if (!player || !player->loaded) return false;
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "play %s from 0", player->alias);
    
    if (!mci_cmd(cmd)) return false;
    
    player->state = HAL_AUDIO_PLAYING;
    
    if (player->callback) {
        player->callback(player, HAL_AUDIO_EVENT_PLAY, player->userData);
    }
    
    return true;
}

/* Pause */
bool hal_audio_pause(HalAudioPlayer* player) {
    if (!player || !player->loaded) return false;
    if (player->state != HAL_AUDIO_PLAYING) return false;
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pause %s", player->alias);
    
    if (!mci_cmd(cmd)) return false;
    
    player->state = HAL_AUDIO_PAUSED;
    
    if (player->callback) {
        player->callback(player, HAL_AUDIO_EVENT_PAUSE, player->userData);
    }
    
    return true;
}

/* Stop */
bool hal_audio_stop(HalAudioPlayer* player) {
    if (!player || !player->loaded) return false;
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "stop %s", player->alias);
    mci_cmd(cmd);
    
    /* Seek to beginning */
    snprintf(cmd, sizeof(cmd), "seek %s to start", player->alias);
    mci_cmd(cmd);
    
    player->state = HAL_AUDIO_STOPPED;
    
    if (player->callback) {
        player->callback(player, HAL_AUDIO_EVENT_STOP, player->userData);
    }
    
    return true;
}

/* Resume */
bool hal_audio_resume(HalAudioPlayer* player) {
    if (!player || !player->loaded) return false;
    if (player->state != HAL_AUDIO_PAUSED) return false;
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "resume %s", player->alias);
    
    /* MCI doesn't have resume, use play */
    if (!mci_cmd(cmd)) {
        snprintf(cmd, sizeof(cmd), "play %s", player->alias);
        if (!mci_cmd(cmd)) return false;
    }
    
    player->state = HAL_AUDIO_PLAYING;
    
    if (player->callback) {
        player->callback(player, HAL_AUDIO_EVENT_PLAY, player->userData);
    }
    
    return true;
}

/* Seek */
bool hal_audio_seek(HalAudioPlayer* player, int positionMs) {
    if (!player || !player->loaded) return false;
    
    if (positionMs < 0) positionMs = 0;
    if (positionMs > player->duration) positionMs = player->duration;
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "seek %s to %d", player->alias, positionMs);
    
    if (!mci_cmd(cmd)) return false;
    
    /* If was playing, continue playing */
    if (player->state == HAL_AUDIO_PLAYING) {
        snprintf(cmd, sizeof(cmd), "play %s", player->alias);
        mci_cmd(cmd);
    }
    
    return true;
}

/* Get volume */
int hal_audio_get_volume(HalAudioPlayer* player) {
    if (!player) return 0;
    return player->muted ? 0 : player->volume;
}

/* Set volume */
bool hal_audio_set_volume(HalAudioPlayer* player, int volume) {
    if (!player) return false;
    
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    player->volume = volume;
    
    if (player->loaded && !player->muted) {
        /* MCI volume is 0-1000 */
        int mciVolume = volume * 10;
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "setaudio %s volume to %d", player->alias, mciVolume);
        mci_cmd(cmd);
    }
    
    return true;
}

/* Is muted */
bool hal_audio_is_muted(HalAudioPlayer* player) {
    if (!player) return false;
    return player->muted;
}

/* Set mute */
bool hal_audio_set_mute(HalAudioPlayer* player, bool mute) {
    if (!player) return false;
    
    player->muted = mute;
    
    if (player->loaded) {
        int mciVolume = mute ? 0 : (player->volume * 10);
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "setaudio %s volume to %d", player->alias, mciVolume);
        mci_cmd(cmd);
    }
    
    return true;
}

/* Get state */
HalAudioState hal_audio_get_state(HalAudioPlayer* player) {
    if (!player) return HAL_AUDIO_STOPPED;
    return player->state;
}

/* Get position */
int hal_audio_get_position(HalAudioPlayer* player) {
    if (!player || !player->loaded) return 0;
    
    char cmd[256];
    char result[64];
    snprintf(cmd, sizeof(cmd), "status %s position", player->alias);
    
    if (mci_send(cmd, result, sizeof(result))) {
        return atoi(result);
    }
    
    return 0;
}

/* Get duration */
int hal_audio_get_duration(HalAudioPlayer* player) {
    if (!player) return 0;
    return player->duration;
}

/* Get audio info */
HalAudioInfo* hal_audio_get_info(HalAudioPlayer* player) {
    if (!player || !player->loaded) return NULL;
    
    HalAudioInfo* info = (HalAudioInfo*)calloc(1, sizeof(HalAudioInfo));
    if (!info) return NULL;
    
    info->durationMs = player->duration;
    info->positionMs = hal_audio_get_position(player);
    
    /* Try to get more info from MCI */
    char cmd[256];
    char result[256];
    
    /* Channels */
    snprintf(cmd, sizeof(cmd), "status %s channels", player->alias);
    if (mci_send(cmd, result, sizeof(result))) {
        info->channels = atoi(result);
    }
    
    /* Sample rate */
    snprintf(cmd, sizeof(cmd), "status %s samplespersec", player->alias);
    if (mci_send(cmd, result, sizeof(result))) {
        info->sampleRate = atoi(result);
    }
    
    /* Bit rate */
    snprintf(cmd, sizeof(cmd), "status %s bitspersample", player->alias);
    if (mci_send(cmd, result, sizeof(result))) {
        info->bitRate = atoi(result);
    }
    
    return info;
}

/* Free audio info */
void hal_audio_free_info(HalAudioInfo* info) {
    if (!info) return;
    if (info->title) free(info->title);
    if (info->artist) free(info->artist);
    if (info->album) free(info->album);
    free(info);
}

/* Set loop */
bool hal_audio_set_loop(HalAudioPlayer* player, bool loop) {
    if (!player) return false;
    player->loop = loop;
    return true;
}

/* Is looping */
bool hal_audio_is_looping(HalAudioPlayer* player) {
    if (!player) return false;
    return player->loop;
}

/* Set callback */
void hal_audio_set_callback(HalAudioPlayer* player, HalAudioCallback callback, void* userData) {
    if (!player) return;
    player->callback = callback;
    player->userData = userData;
}

/* Check if file is supported */
bool hal_audio_is_supported(const char* filePath) {
    if (!filePath) return false;
    
    const char* ext = strrchr(filePath, '.');
    if (!ext) return false;
    
    /* Supported formats */
    const char* supported[] = {
        ".mp3", ".wav", ".wma", ".mid", ".midi", ".aif", ".aiff", ".au", ".snd", NULL
    };
    
    for (int i = 0; supported[i]; i++) {
        if (_stricmp(ext, supported[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

/* Get supported formats */
const char* hal_audio_get_formats(void) {
    return "MP3, WAV, WMA, MIDI, AIF, AU";
}

/* Process audio events */
void hal_audio_process_events(void) {
    for (int i = 0; i < MAX_AUDIO_PLAYERS; i++) {
        HalAudioPlayer* player = g_players[i];
        if (!player || !player->loaded) continue;
        
        if (player->state == HAL_AUDIO_PLAYING) {
            /* Check if playback ended */
            char cmd[256];
            char result[64];
            snprintf(cmd, sizeof(cmd), "status %s mode", player->alias);
            
            if (mci_send(cmd, result, sizeof(result))) {
                if (strcmp(result, "stopped") == 0) {
                    /* Playback ended */
                    if (player->loop) {
                        /* Restart */
                        hal_audio_play(player);
                    } else {
                        player->state = HAL_AUDIO_STOPPED;
                        if (player->callback) {
                            player->callback(player, HAL_AUDIO_EVENT_END, player->userData);
                        }
                    }
                }
            }
        }
    }
}
