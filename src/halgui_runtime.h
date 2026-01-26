/*
 * HalcyonScript - HalGUI Runtime Integration Header
 */

#ifndef HALGUI_RUNTIME_H
#define HALGUI_RUNTIME_H

#include "runtime.h"
#include "ast.h"

/* Initialize HalGUI runtime */
void halgui_runtime_init(HcsRuntime* rt);

/* Shutdown HalGUI runtime */
void halgui_runtime_shutdown(void);

/* Create window using HalGUI */
void halgui_create_window(HcsRuntime* rt, HcsAstNode* node);

/* Create control using HalGUI */
void halgui_create_control(HcsRuntime* rt, HcsAstNode* node);

/* Set property on HalGUI widget */
void halgui_set_property(HcsRuntime* rt, HcsAstNode* node);

/* Get property from HalGUI widget */
void halgui_get_property(HcsRuntime* rt, HcsAstNode* node);

/* Register event handler */
void halgui_register_handler(HcsAstNode* node);

/* Run HalGUI event loop */
void halgui_run(void);

/* Set theme */
void halgui_set_theme(const char* theme_name);

/* Dialogs */
HcsValue* halgui_dialog_message(HcsRuntime* rt, const char* title, const char* message, int buttons, int icon);
HcsValue* halgui_dialog_open_file(HcsRuntime* rt, const char* title, const char* filter);
HcsValue* halgui_dialog_save_file(HcsRuntime* rt, const char* title, const char* filter, const char* default_name);

/* Process periodic updates (audio, animations, etc.) */
void halgui_process_updates(void);

/* Layout system */
void halgui_set_layout(const char* panelName, const char* layoutType);
void halgui_set_gap(const char* panelName, int gap);
void halgui_set_align(const char* widgetName, const char* horizontal, const char* vertical);
void halgui_set_widget_flex(const char* widgetName, float flex);
void halgui_set_widget_margin(const char* widgetName, int top, int right, int bottom, int left);
void halgui_apply_layout(const char* panelName);

#endif /* HALGUI_RUNTIME_H */
