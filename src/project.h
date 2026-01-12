/*
 * HalcyonScript Project System
 * 
 * Supports .halproj project files for multi-file projects
 */

#ifndef HCS_PROJECT_H
#define HCS_PROJECT_H

#include <stdbool.h>

#define HCS_MAX_PROJECT_FILES 256
#define HCS_MAX_PATH_LEN 512

/* Project configuration */
typedef struct {
    char* name;              /* Project name */
    char* version;           /* Project version */
    char* author;            /* Author name */
    char* description;       /* Project description */
    char* entry_point;       /* Main entry file (e.g., "main.hcs") */
    char* output;            /* Output executable name */
    char* icon;              /* Application icon path */
    
    /* Source files */
    char* files[HCS_MAX_PROJECT_FILES];
    int file_count;
    
    /* Include directories */
    char* include_dirs[32];
    int include_dir_count;
    
    /* Project directory (where .halproj is located) */
    char* project_dir;
    
    /* Build settings */
    bool debug;
    bool optimize;
    char* target;            /* "windows", "console", etc. */
} HcsProject;

/* Create empty project */
HcsProject* project_create(void);

/* Free project */
void project_free(HcsProject* proj);

/* Load project from .halproj file */
HcsProject* project_load(const char* path);

/* Save project to .halproj file */
bool project_save(HcsProject* proj, const char* path);

/* Get full path to a project file */
char* project_get_file_path(HcsProject* proj, const char* relative_path);

/* Check if file is a project file */
bool is_project_file(const char* path);

/* Import system - load and merge another .hcs file */
char* project_resolve_import(HcsProject* proj, const char* import_path, const char* current_file);

#endif /* HCS_PROJECT_H */
