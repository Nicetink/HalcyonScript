/*
 * HalcyonScript Project System Implementation
 * 
 * .halproj file format (simple key=value):
 * 
 * name = MyProject
 * version = 1.0.0
 * author = Developer
 * entry = main.hcs
 * 
 * [files]
 * main.hcs
 * ui/window.hcs
 * utils/helpers.hcs
 * 
 * [include]
 * lib/
 * modules/
 */

#include "project.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEP '\\'
#else
#include <unistd.h>
#define PATH_SEP '/'
#endif

/* Helper: trim whitespace */
static char* trim(char* str) {
    while (isspace(*str)) str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = 0;
    return str;
}

/* Helper: duplicate string */
static char* str_dup(const char* s) {
    if (!s) return NULL;
    char* d = malloc(strlen(s) + 1);
    strcpy(d, s);
    return d;
}

/* Helper: get directory from path */
static char* get_directory(const char* path) {
    char* dir = str_dup(path);
    char* last_sep = strrchr(dir, PATH_SEP);
    if (!last_sep) last_sep = strrchr(dir, '/');
    if (last_sep) {
        *last_sep = '\0';
    } else {
        dir[0] = '.';
        dir[1] = '\0';
    }
    return dir;
}

/* Helper: join paths */
static char* join_path(const char* dir, const char* file) {
    size_t len = strlen(dir) + strlen(file) + 2;
    char* result = malloc(len);
    snprintf(result, len, "%s%c%s", dir, PATH_SEP, file);
    return result;
}

HcsProject* project_create(void) {
    HcsProject* proj = calloc(1, sizeof(HcsProject));
    proj->name = str_dup("Untitled");
    proj->version = str_dup("1.0.0");
    proj->entry_point = str_dup("main.hcs");
    proj->target = str_dup("windows");
    proj->debug = true;
    return proj;
}

void project_free(HcsProject* proj) {
    if (!proj) return;
    
    free(proj->name);
    free(proj->version);
    free(proj->author);
    free(proj->description);
    free(proj->entry_point);
    free(proj->output);
    free(proj->icon);
    free(proj->project_dir);
    free(proj->target);
    
    for (int i = 0; i < proj->file_count; i++) {
        free(proj->files[i]);
    }
    
    for (int i = 0; i < proj->include_dir_count; i++) {
        free(proj->include_dirs[i]);
    }
    
    free(proj);
}

HcsProject* project_load(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open project file: %s\n", path);
        return NULL;
    }
    
    HcsProject* proj = project_create();
    proj->project_dir = get_directory(path);
    
    char line[1024];
    enum { SECTION_MAIN, SECTION_FILES, SECTION_INCLUDE } section = SECTION_MAIN;
    
    while (fgets(line, sizeof(line), f)) {
        char* trimmed = trim(line);
        
        /* Skip empty lines and comments */
        if (trimmed[0] == '\0' || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }
        
        /* Check for section headers */
        if (trimmed[0] == '[') {
            if (strncmp(trimmed, "[files]", 7) == 0) {
                section = SECTION_FILES;
            } else if (strncmp(trimmed, "[include]", 9) == 0) {
                section = SECTION_INCLUDE;
            } else if (strncmp(trimmed, "[project]", 9) == 0) {
                section = SECTION_MAIN;
            }
            continue;
        }
        
        /* Parse based on section */
        if (section == SECTION_MAIN) {
            /* Key = Value format */
            char* eq = strchr(trimmed, '=');
            if (eq) {
                *eq = '\0';
                char* key = trim(trimmed);
                char* value = trim(eq + 1);
                
                /* Remove quotes if present */
                if (value[0] == '"') {
                    value++;
                    char* end_quote = strchr(value, '"');
                    if (end_quote) *end_quote = '\0';
                }
                
                if (strcmp(key, "name") == 0) {
                    free(proj->name);
                    proj->name = str_dup(value);
                } else if (strcmp(key, "version") == 0) {
                    free(proj->version);
                    proj->version = str_dup(value);
                } else if (strcmp(key, "author") == 0) {
                    free(proj->author);
                    proj->author = str_dup(value);
                } else if (strcmp(key, "description") == 0) {
                    free(proj->description);
                    proj->description = str_dup(value);
                } else if (strcmp(key, "entry") == 0) {
                    free(proj->entry_point);
                    proj->entry_point = str_dup(value);
                } else if (strcmp(key, "output") == 0) {
                    free(proj->output);
                    proj->output = str_dup(value);
                } else if (strcmp(key, "icon") == 0) {
                    free(proj->icon);
                    proj->icon = str_dup(value);
                } else if (strcmp(key, "target") == 0) {
                    free(proj->target);
                    proj->target = str_dup(value);
                } else if (strcmp(key, "debug") == 0) {
                    proj->debug = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
                } else if (strcmp(key, "optimize") == 0) {
                    proj->optimize = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
                }
            }
        } else if (section == SECTION_FILES) {
            /* File path */
            if (proj->file_count < HCS_MAX_PROJECT_FILES) {
                proj->files[proj->file_count++] = str_dup(trimmed);
            }
        } else if (section == SECTION_INCLUDE) {
            /* Include directory */
            if (proj->include_dir_count < 32) {
                proj->include_dirs[proj->include_dir_count++] = str_dup(trimmed);
            }
        }
    }
    
    fclose(f);
    
    /* If no files specified, add entry point */
    if (proj->file_count == 0 && proj->entry_point) {
        proj->files[proj->file_count++] = str_dup(proj->entry_point);
    }
    
    printf("Loaded project: %s v%s\n", proj->name, proj->version);
    printf("  Entry: %s\n", proj->entry_point);
    printf("  Files: %d\n", proj->file_count);
    fflush(stdout);
    
    return proj;
}

bool project_save(HcsProject* proj, const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return false;
    
    fprintf(f, "# HalcyonScript Project\n\n");
    fprintf(f, "[project]\n");
    fprintf(f, "name = \"%s\"\n", proj->name ? proj->name : "Untitled");
    fprintf(f, "version = \"%s\"\n", proj->version ? proj->version : "1.0.0");
    if (proj->author) fprintf(f, "author = \"%s\"\n", proj->author);
    if (proj->description) fprintf(f, "description = \"%s\"\n", proj->description);
    fprintf(f, "entry = \"%s\"\n", proj->entry_point ? proj->entry_point : "main.hcs");
    if (proj->output) fprintf(f, "output = \"%s\"\n", proj->output);
    if (proj->icon) fprintf(f, "icon = \"%s\"\n", proj->icon);
    fprintf(f, "target = \"%s\"\n", proj->target ? proj->target : "windows");
    fprintf(f, "debug = %s\n", proj->debug ? "true" : "false");
    fprintf(f, "optimize = %s\n", proj->optimize ? "true" : "false");
    
    if (proj->file_count > 0) {
        fprintf(f, "\n[files]\n");
        for (int i = 0; i < proj->file_count; i++) {
            fprintf(f, "%s\n", proj->files[i]);
        }
    }
    
    if (proj->include_dir_count > 0) {
        fprintf(f, "\n[include]\n");
        for (int i = 0; i < proj->include_dir_count; i++) {
            fprintf(f, "%s\n", proj->include_dirs[i]);
        }
    }
    
    fclose(f);
    return true;
}

char* project_get_file_path(HcsProject* proj, const char* relative_path) {
    if (!proj || !relative_path) return NULL;
    
    /* If absolute path, return as-is */
    if (relative_path[0] == '/' || (relative_path[1] == ':')) {
        return str_dup(relative_path);
    }
    
    return join_path(proj->project_dir, relative_path);
}

bool is_project_file(const char* path) {
    if (!path) return false;
    const char* ext = strrchr(path, '.');
    return ext && strcmp(ext, ".halproj") == 0;
}

char* project_resolve_import(HcsProject* proj, const char* import_path, const char* current_file) {
    /* Try relative to current file first */
    if (current_file) {
        char* current_dir = get_directory(current_file);
        char* full_path = join_path(current_dir, import_path);
        free(current_dir);
        
        FILE* f = fopen(full_path, "r");
        if (f) {
            fclose(f);
            return full_path;
        }
        free(full_path);
    }
    
    /* Try relative to project directory */
    if (proj && proj->project_dir) {
        char* full_path = join_path(proj->project_dir, import_path);
        FILE* f = fopen(full_path, "r");
        if (f) {
            fclose(f);
            return full_path;
        }
        free(full_path);
        
        /* Try include directories */
        for (int i = 0; i < proj->include_dir_count; i++) {
            char* inc_dir = join_path(proj->project_dir, proj->include_dirs[i]);
            full_path = join_path(inc_dir, import_path);
            free(inc_dir);
            
            f = fopen(full_path, "r");
            if (f) {
                fclose(f);
                return full_path;
            }
            free(full_path);
        }
    }
    
    /* Try as absolute path */
    FILE* f = fopen(import_path, "r");
    if (f) {
        fclose(f);
        return str_dup(import_path);
    }
    
    return NULL;
}
