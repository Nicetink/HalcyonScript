/*
 * HalcyonScript Compiler
 * Developer: KAInaps
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <io.h>
#include <fcntl.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "lexer.h"
#include "parser.h"
#include "runtime.h"
#include "project.h"

/* Attach to parent console or create new one for output */
static void init_console(void) {
    // Set UTF-8 code page for proper Russian/Unicode text output
    SetConsoleOutputCP(65001);  // CP_UTF8
    SetConsoleCP(65001);
    
    // Set locale for proper UTF-8 handling
    setlocale(LC_ALL, ".UTF-8");
    
    // Disable buffering for immediate output
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

static void show_help(void) {
    printf("\n%s ver_%s (HalcyonScript %s)\n", HALCYON_RT_NAME, HALCYON_VERSION, HALCYON_VERSION);
    printf("Developer: KAInaps\n\n");
    printf("USAGE:\n");
    printf("  halcyon <file.hcs>        Run a script\n");
    printf("  halcyon <project.halproj> Run a project\n");
    printf("  halcyon run <file>        Run a script or project\n");
    printf("  halcyon build             Build project to executable\n");
    printf("  halcyon check <file>      Check syntax\n");
    printf("  halcyon new <name>        Create new project\n");
    printf("  halcyon help              Show help\n");
    printf("  halcyon version           Show version\n\n");
    printf("BUILD COMMAND:\n");
    printf("  halcyon build             Build current directory project\n");
    printf("  halcyon build <project>   Build specified .halproj\n\n");
    printf("  Output: dist/<AppName>.exe + runtime files\n\n");
    printf("PROJECT FILE (.halproj):\n");
    printf("  [project]\n");
    printf("  name = \"MyApp\"\n");
    printf("  version = \"1.0.0\"\n");
    printf("  entry = \"main.hcs\"\n\n");
    printf("  [files]\n");
    printf("  main.hcs\n");
    printf("  ui/window.hcs\n\n");
    printf("IMPORT SYNTAX:\n");
    printf("  import \"file.hcs\"\n");
    printf("  import \"utils/helpers.hcs\"\n\n");
    fflush(stdout);
}

static void show_version(void) {
    printf("%s ver_%s\n", HALCYON_RT_NAME, HALCYON_VERSION);
    printf("HalcyonScript Language: %s\n", HALCYON_VERSION);
    printf("Developer: KAInaps\n");
    printf("Platform: Windows\n");
    fflush(stdout);
}

static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { 
        fprintf(stderr, "Error: Cannot open %s\n", path); 
        fflush(stderr);
        return NULL; 
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* c = (char*)malloc(sz + 1);
    fread(c, 1, sz, f);
    c[sz] = '\0';
    fclose(f);
    return c;
}

/* Track imported files to avoid circular imports */
static char* g_imported_files[256];
static int g_imported_count = 0;
static HcsProject* g_current_project = NULL;
static char* g_current_file = NULL;

static bool is_already_imported(const char* path) {
    for (int i = 0; i < g_imported_count; i++) {
        if (strcmp(g_imported_files[i], path) == 0) return true;
    }
    return false;
}

static void mark_imported(const char* path) {
    if (g_imported_count < 256) {
        g_imported_files[g_imported_count++] = strdup(path);
    }
}

static void clear_imports(void) {
    for (int i = 0; i < g_imported_count; i++) {
        free(g_imported_files[i]);
    }
    g_imported_count = 0;
}

/* Process imports in AST and execute them */
static void process_imports(HcsRuntime* rt, HcsAstNode* prog);

/* Parse and execute a single file */
static HcsAstNode* parse_file(const char* path) {
    char* src = read_file(path);
    if (!src) return NULL;
    
    HcsLexer* lex = lexer_create(src);
    int tc;
    HcsToken** toks = lexer_tokenize(lex, &tc);
    lexer_free(lex);
    
    HcsParser* par = parser_create(toks, tc);
    HcsAstNode* prog = parser_parse(par);
    parser_free(par);
    
    lexer_free_tokens(toks, tc);
    free(src);
    
    return prog;
}

/* Execute import statement */
static void execute_import(HcsRuntime* rt, HcsAstNode* import_node) {
    const char* import_path = import_node->data.import_stmt.path;
    
    /* Resolve import path */
    char* full_path = project_resolve_import(g_current_project, import_path, g_current_file);
    if (!full_path) {
        fprintf(stderr, "Error: Cannot find import file: %s\n", import_path);
        return;
    }
    
    /* Check for circular import */
    if (is_already_imported(full_path)) {
        free(full_path);
        return;
    }
    mark_imported(full_path);
    
    printf("Importing: %s\n", full_path);
    fflush(stdout);
    
    /* Save current file context */
    char* prev_file = g_current_file;
    g_current_file = full_path;
    
    /* Parse and execute imported file */
    HcsAstNode* prog = parse_file(full_path);
    if (prog) {
        /* Process nested imports first */
        process_imports(rt, prog);
        
        /* Execute the imported code */
        runtime_execute(rt, prog);
        ast_free(prog);
    }
    
    /* Restore file context */
    g_current_file = prev_file;
    free(full_path);
}

/* Process all imports in a program */
static void process_imports(HcsRuntime* rt, HcsAstNode* prog) {
    if (!prog || prog->type != HCS_AST_PROGRAM) return;
    
    for (int i = 0; i < prog->data.program.statements.count; i++) {
        HcsAstNode* stmt = prog->data.program.statements.items[i];
        if (stmt && stmt->type == HCS_AST_IMPORT) {
            execute_import(rt, stmt);
        }
    }
}

static int run_script(const char* path) {
    /* Initialize import tracking */
    clear_imports();
    mark_imported(path);
    g_current_file = (char*)path;
    
    HcsAstNode* prog = parse_file(path);
    if (!prog) return 1;
    
    HcsRuntime* rt = runtime_create();
    
    /* Process imports first */
    process_imports(rt, prog);
    
    /* Execute main program */
    gui_execute_program(rt, prog);
    
    runtime_free(rt);
    ast_free(prog);
    clear_imports();
    g_current_file = NULL;
    
    return 0;
}

/* Run a project file */
static int run_project(const char* path) {
    HcsProject* proj = project_load(path);
    if (!proj) return 1;
    
    g_current_project = proj;
    
    /* Get entry point path */
    char* entry_path = project_get_file_path(proj, proj->entry_point);
    if (!entry_path) {
        fprintf(stderr, "Error: Cannot find entry point: %s\n", proj->entry_point);
        project_free(proj);
        return 1;
    }
    
    printf("Running: %s\n", entry_path);
    fflush(stdout);
    
    int result = run_script(entry_path);
    
    free(entry_path);
    project_free(proj);
    g_current_project = NULL;
    
    return result;
}

/* Create new project */
static int create_project(const char* name) {
    char proj_file[512];
    snprintf(proj_file, sizeof(proj_file), "%s.halproj", name);
    
    /* Check if already exists */
    FILE* f = fopen(proj_file, "r");
    if (f) {
        fclose(f);
        fprintf(stderr, "Error: Project %s already exists\n", proj_file);
        return 1;
    }
    
    /* Create project */
    HcsProject* proj = project_create();
    free(proj->name);
    proj->name = strdup(name);
    
    /* Add default entry file */
    proj->files[proj->file_count++] = strdup("main.hcs");
    
    /* Save project file */
    if (!project_save(proj, proj_file)) {
        fprintf(stderr, "Error: Cannot create project file\n");
        project_free(proj);
        return 1;
    }
    
    /* Create main.hcs template */
    char main_file[512];
    snprintf(main_file, sizeof(main_file), "main.hcs");
    f = fopen(main_file, "w");
    if (f) {
        fprintf(f, "# %s - Main Entry Point\n", name);
        fprintf(f, "# Created with HalcyonScript\n\n");
        fprintf(f, "HalGUI.init()\n");
        fprintf(f, "HalGUI.setTheme(\"dark\")\n\n");
        fprintf(f, "create window app \"%s\" 800 600\n\n", name);
        fprintf(f, "create label title \"Welcome to %s\" x:20 y:20 width:760 height:40\n\n", name);
        fprintf(f, "HalGUI.run()\n");
        fclose(f);
    }
    
    printf("Created project: %s\n", proj_file);
    printf("Created entry point: main.hcs\n");
    printf("\nRun with: halcyon %s\n", proj_file);
    fflush(stdout);
    
    project_free(proj);
    return 0;
}

static int check_script(const char* path) {
    char* src = read_file(path);
    if (!src) return 1;
    
    HcsLexer* lex = lexer_create(src);
    int tc;
    HcsToken** toks = lexer_tokenize(lex, &tc);
    lexer_free(lex);
    printf("Tokens: %d\n", tc);
    fflush(stdout);
    
    HcsParser* par = parser_create(toks, tc);
    HcsAstNode* prog = parser_parse(par);
    parser_free(par);
    
    if (prog && prog->type == HCS_AST_PROGRAM) {
        printf("OK: %d statements parsed successfully.\n", prog->data.program.statements.count);
        fflush(stdout);
    }
    
    ast_free(prog);
    lexer_free_tokens(toks, tc);
    free(src);
    return 0;
}

/* Find .halproj file in current directory */
static char* find_project_file(void) {
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA("*.halproj", &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    char* result = strdup(fd.cFileName);
    FindClose(hFind);
    return result;
}

/* Get directory of current executable */
static char* get_exe_directory(void) {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* last_sep = strrchr(path, '\\');
    if (last_sep) *last_sep = '\0';
    return strdup(path);
}

/* Copy file */
static bool copy_file_to(const char* src, const char* dst) {
    return CopyFileA(src, dst, FALSE) != 0;
}

/* Create directory recursively */
static void create_dir_recursive(const char* path) {
    char tmp[MAX_PATH];
    strncpy(tmp, path, MAX_PATH - 1);
    tmp[MAX_PATH - 1] = '\0';
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '\\' || *p == '/') {
            *p = '\0';
            CreateDirectoryA(tmp, NULL);
            *p = '\\';
        }
    }
    CreateDirectoryA(tmp, NULL);
}

/* Create scripts bundle file for embedding */
static bool create_scripts_bundle(HcsProject* proj, const char* bundle_path) {
    FILE* bundle = fopen(bundle_path, "wb");
    if (!bundle) return false;
    
    /* Bundle format: [filename_len:4][filename][content_len:4][content]... */
    
    /* Helper to add file to bundle */
    #define ADD_FILE_TO_BUNDLE(file_path, rel_name) do { \
        FILE* src = fopen(file_path, "rb"); \
        if (src) { \
            fseek(src, 0, SEEK_END); \
            DWORD content_len = (DWORD)ftell(src); \
            fseek(src, 0, SEEK_SET); \
            char* content = (char*)malloc(content_len); \
            fread(content, 1, content_len, src); \
            fclose(src); \
            DWORD name_len = (DWORD)strlen(rel_name); \
            fwrite(&name_len, 4, 1, bundle); \
            fwrite(rel_name, 1, name_len, bundle); \
            fwrite(&content_len, 4, 1, bundle); \
            fwrite(content, 1, content_len, bundle); \
            free(content); \
        } \
    } while(0)
    
    /* Add all project files */
    for (int i = 0; i < proj->file_count; i++) {
        char* src_path = project_get_file_path(proj, proj->files[i]);
        if (src_path) {
            /* Normalize path separators for bundle */
            char normalized[MAX_PATH];
            strncpy(normalized, proj->files[i], MAX_PATH - 1);
            normalized[MAX_PATH - 1] = '\0';
            for (char* p = normalized; *p; p++) {
                if (*p == '/') *p = '\\';
            }
            ADD_FILE_TO_BUNDLE(src_path, normalized);
            free(src_path);
        }
    }
    
    /* Add entry point if not in files list */
    bool entry_in_list = false;
    for (int i = 0; i < proj->file_count; i++) {
        if (strcmp(proj->files[i], proj->entry_point) == 0) {
            entry_in_list = true;
            break;
        }
    }
    if (!entry_in_list) {
        char* src_path = project_get_file_path(proj, proj->entry_point);
        if (src_path) {
            char normalized[MAX_PATH];
            strncpy(normalized, proj->entry_point, MAX_PATH - 1);
            normalized[MAX_PATH - 1] = '\0';
            for (char* p = normalized; *p; p++) {
                if (*p == '/') *p = '\\';
            }
            ADD_FILE_TO_BUNDLE(src_path, normalized);
            free(src_path);
        }
    }
    
    #undef ADD_FILE_TO_BUNDLE
    
    fclose(bundle);
    return true;
}

/* Build project to executable */
static int build_project(const char* proj_path) {
    /* Load project */
    HcsProject* proj = project_load(proj_path);
    if (!proj) {
        fprintf(stderr, "Error: Cannot load project file: %s\n", proj_path);
        return 1;
    }
    
    printf("\n========================================\n");
    printf("Building: %s v%s\n", proj->name, proj->version);
    printf("========================================\n\n");
    fflush(stdout);
    
    /* Create output directory */
    char dist_dir[512];  /* Increased buffer size to avoid truncation warnings */
    snprintf(dist_dir, sizeof(dist_dir), "%s\\dist", proj->project_dir);
    create_dir_recursive(dist_dir);
    
    /* Create temp build directory */
    char build_dir[512];  /* Increased buffer size */
    snprintf(build_dir, sizeof(build_dir), "%s\\build_temp", proj->project_dir);
    create_dir_recursive(build_dir);
    
    /* Get HalcyonRT.exe location (for launcher source) */
    char* exe_dir = get_exe_directory();
    char launcher_src[512];  /* Increased buffer size */
    snprintf(launcher_src, sizeof(launcher_src), "%s\\launcher\\launcher.c", exe_dir);
    
    /* Check if we can build launcher (need gcc and windres) */
    bool can_build_launcher = false;
    
    /* Test if gcc is available */
    FILE* test_gcc = _popen("gcc --version 2>nul", "r");
    if (test_gcc) {
        char buf[256];
        if (fgets(buf, sizeof(buf), test_gcc) && strstr(buf, "gcc")) {
            can_build_launcher = true;
        }
        _pclose(test_gcc);
    }
    
    if (can_build_launcher) {
        /* Check if launcher source exists */
        if (GetFileAttributesA(launcher_src) == INVALID_FILE_ATTRIBUTES) {
            can_build_launcher = false;
        }
    }
    
    printf("[1/4] Preparing scripts...\n");
    fflush(stdout);
    
    /* Create app.halproj for embedding */
    char config_path[1024];  /* Increased buffer size to avoid truncation */
    snprintf(config_path, sizeof(config_path), "%s\\app.halproj", build_dir);
    
    FILE* f = fopen(config_path, "w");
    if (f) {
        char normalized_entry[MAX_PATH];
        strncpy(normalized_entry, proj->entry_point, MAX_PATH - 1);
        normalized_entry[MAX_PATH - 1] = '\0';
        for (char* p = normalized_entry; *p; p++) {
            if (*p == '/') *p = '\\';
        }
        
        fprintf(f, "[project]\n");
        fprintf(f, "name = \"%s\"\n", proj->name);
        fprintf(f, "version = \"%s\"\n", proj->version);
        fprintf(f, "entry = \"scripts\\%s\"\n", normalized_entry);
        fprintf(f, "\n[files]\n");
        for (int i = 0; i < proj->file_count; i++) {
            char normalized_file[MAX_PATH];
            strncpy(normalized_file, proj->files[i], MAX_PATH - 1);
            normalized_file[MAX_PATH - 1] = '\0';
            for (char* p = normalized_file; *p; p++) {
                if (*p == '/') *p = '\\';
            }
            fprintf(f, "scripts\\%s\n", normalized_file);
        }
        fclose(f);
        printf("  + app.halproj\n");
    }
    
    printf("[2/4] Creating scripts bundle...\n");
    fflush(stdout);
    
    /* Create scripts bundle */
    char bundle_path[1024];  /* Increased buffer size to avoid truncation */
    snprintf(bundle_path, sizeof(bundle_path), "%s\\scripts.bundle", build_dir);
    
    if (!create_scripts_bundle(proj, bundle_path)) {
        fprintf(stderr, "Error: Failed to create scripts bundle\n");
        free(exe_dir);
        project_free(proj);
        return 1;
    }
    
    int file_count = proj->file_count;
    printf("  + %d script(s) bundled\n", file_count);
    fflush(stdout);
    
    if (can_build_launcher) {
        printf("[3/4] Compiling launcher...\n");
        fflush(stdout);
        
        /* Copy halcyon.ico to build directory */
        char icon_src[1024];  /* Increased buffer size to avoid truncation */
        char icon_dst[1024];  /* Increased buffer size to avoid truncation */
        snprintf(icon_src, sizeof(icon_src), "%s\\logo\\halcyon.ico", exe_dir);
        snprintf(icon_dst, sizeof(icon_dst), "%s\\halcyon.ico", build_dir);
        
        /* Try to copy icon from logo directory */
        bool has_icon = false;
        if (copy_file_to(icon_src, icon_dst)) {
            has_icon = true;
            printf("  + halcyon.ico (default icon)\n");
        } else {
            /* Try alternative location - same directory as exe */
            snprintf(icon_src, sizeof(icon_src), "%s\\halcyon.ico", exe_dir);
            if (copy_file_to(icon_src, icon_dst)) {
                has_icon = true;
                printf("  + halcyon.ico (default icon)\n");
            }
        }
        
        /* Check if project has custom icon */
        if (proj->icon) {
            char* custom_icon = project_get_file_path(proj, proj->icon);
            if (custom_icon && copy_file_to(custom_icon, icon_dst)) {
                has_icon = true;
                printf("  + %s (custom icon)\n", proj->icon);
            }
            free(custom_icon);
        }
        
        /* Create resource file */
        char rc_path[1024];  /* Increased buffer size to avoid truncation */
        snprintf(rc_path, sizeof(rc_path), "%s\\launcher.rc", build_dir);
        
        f = fopen(rc_path, "w");
        if (f) {
            fprintf(f, "/* Auto-generated resource file */\n");
            fprintf(f, "#define IDR_APP_CONFIG 100\n");
            fprintf(f, "#define IDR_SCRIPTS 101\n");
            fprintf(f, "#define IDI_APPICON 1\n\n");
            if (has_icon) {
                fprintf(f, "/* Application icon - HalcyonScript logo */\n");
                fprintf(f, "IDI_APPICON ICON \"halcyon.ico\"\n\n");
            }
            fprintf(f, "IDR_APP_CONFIG HALCYON_DATA \"%s\"\n", "app.halproj");
            fprintf(f, "IDR_SCRIPTS HALCYON_DATA \"%s\"\n", "scripts.bundle");
            fclose(f);
        }
        
        /* Compile resources */
        char cmd[MAX_PATH * 4];
        char res_obj[1024];  /* Increased buffer size to avoid truncation */
        snprintf(res_obj, sizeof(res_obj), "%s\\launcher_res.o", build_dir);
        
        /* Change to build directory for windres */
        char old_dir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, old_dir);
        SetCurrentDirectoryA(build_dir);
        
        int rc_result = system("windres launcher.rc -o launcher_res.o");
        
        if (rc_result != 0) {
            SetCurrentDirectoryA(old_dir);
            printf("  ! Resource compilation failed, using fallback method\n");
            can_build_launcher = false;
        } else {
            /* Compile launcher - output to build_temp first, then copy */
            char temp_exe[MAX_PATH];
            snprintf(temp_exe, sizeof(temp_exe), "%s.exe", proj->name);
            
            snprintf(cmd, sizeof(cmd), "gcc -O2 -mwindows -o \"%s\" \"%s\" launcher_res.o -lshlwapi 2>&1",
                     temp_exe, launcher_src);
            
            int gcc_result = system(cmd);
            
            SetCurrentDirectoryA(old_dir);
            
            if (gcc_result != 0) {
                printf("  ! Launcher compilation failed, using fallback method\n");
                can_build_launcher = false;
            } else {
                /* Copy compiled exe to dist */
                char src_exe[1024];  /* Increased buffer size to avoid truncation */
                char dst_exe[1024];  /* Increased buffer size to avoid truncation */
                snprintf(src_exe, sizeof(src_exe), "%s\\%s.exe", build_dir, proj->name);
                snprintf(dst_exe, sizeof(dst_exe), "%s\\%s.exe", dist_dir, proj->name);
                
                if (copy_file_to(src_exe, dst_exe)) {
                    printf("  + %s.exe (standalone launcher)\n", proj->name);
                } else {
                    printf("  ! Failed to copy launcher, using fallback method\n");
                    can_build_launcher = false;
                }
            }
        }
    }
    
    if (!can_build_launcher) {
        printf("[3/4] Creating portable package...\n");
        fflush(stdout);
        
        /* Fallback: create portable package with runtime + scripts */
        
        /* Create scripts directory */
        char scripts_dir[1024];  /* Increased buffer size to avoid truncation */
        snprintf(scripts_dir, sizeof(scripts_dir), "%s\\scripts", dist_dir);
        create_dir_recursive(scripts_dir);
        
        /* Copy HalcyonRT.exe as runtime */
        char halcyon_exe[1024];  /* Increased buffer size to avoid truncation */
        snprintf(halcyon_exe, sizeof(halcyon_exe), "%s\\HalcyonRT.exe", exe_dir);
        
        char runtime_path[1024];  /* Increased buffer size to avoid truncation */
        snprintf(runtime_path, sizeof(runtime_path), "%s\\halcyon_runtime.exe", dist_dir);
        
        if (!copy_file_to(halcyon_exe, runtime_path)) {
            fprintf(stderr, "Error: Cannot copy runtime from %s\n", halcyon_exe);
            free(exe_dir);
            project_free(proj);
            return 1;
        }
        printf("  + halcyon_runtime.exe\n");
        
        /* Copy halcyon.ico for window icons */
        char icon_src[1024];  /* Increased buffer size to avoid truncation */
        char icon_dst[1024];  /* Increased buffer size to avoid truncation */
        snprintf(icon_src, sizeof(icon_src), "%s\\logo\\halcyon.ico", exe_dir);
        snprintf(icon_dst, sizeof(icon_dst), "%s\\halcyon.ico", dist_dir);
        
        if (copy_file_to(icon_src, icon_dst)) {
            printf("  + halcyon.ico (default icon)\n");
        } else {
            /* Try alternative location */
            snprintf(icon_src, sizeof(icon_src), "%s\\halcyon.ico", exe_dir);
            if (copy_file_to(icon_src, icon_dst)) {
                printf("  + halcyon.ico (default icon)\n");
            }
        }
        
        /* Copy all script files */
        for (int i = 0; i < proj->file_count; i++) {
            char* src_path = project_get_file_path(proj, proj->files[i]);
            if (!src_path) continue;
            
            char normalized_file[MAX_PATH];
            strncpy(normalized_file, proj->files[i], MAX_PATH - 1);
            normalized_file[MAX_PATH - 1] = '\0';
            for (char* p = normalized_file; *p; p++) {
                if (*p == '/') *p = '\\';
            }
            
            char dst_path[2048];  /* Increased buffer size to avoid truncation (scripts_dir + normalized_file) */
            snprintf(dst_path, sizeof(dst_path), "%s\\%s", scripts_dir, normalized_file);
            
            char* last_sep = strrchr(dst_path, '\\');
            if (last_sep) {
                *last_sep = '\0';
                create_dir_recursive(dst_path);
                *last_sep = '\\';
            }
            
            if (copy_file_to(src_path, dst_path)) {
                printf("  + scripts\\%s\n", normalized_file);
            }
            free(src_path);
        }
        
        /* Copy entry point if not in files list */
        bool entry_copied = false;
        for (int i = 0; i < proj->file_count; i++) {
            if (strcmp(proj->files[i], proj->entry_point) == 0) {
                entry_copied = true;
                break;
            }
        }
        if (!entry_copied) {
            char* src_path = project_get_file_path(proj, proj->entry_point);
            if (src_path) {
                char normalized_entry[MAX_PATH];
                strncpy(normalized_entry, proj->entry_point, MAX_PATH - 1);
                normalized_entry[MAX_PATH - 1] = '\0';
                for (char* p = normalized_entry; *p; p++) {
                    if (*p == '/') *p = '\\';
                }
                
                char dst_path[2048];  /* Increased buffer size to avoid truncation (scripts_dir + normalized_entry) */
                snprintf(dst_path, sizeof(dst_path), "%s\\%s", scripts_dir, normalized_entry);
                
                char* last_sep = strrchr(dst_path, '\\');
                if (last_sep) {
                    *last_sep = '\0';
                    create_dir_recursive(dst_path);
                    *last_sep = '\\';
                }
                
                if (copy_file_to(src_path, dst_path)) {
                    printf("  + scripts\\%s (entry)\n", proj->entry_point);
                }
                free(src_path);
            }
        }
        
        /* Copy app.halproj to dist */
        char dist_config[1024];  /* Increased buffer size to avoid truncation */
        snprintf(dist_config, sizeof(dist_config), "%s\\app.halproj", dist_dir);
        copy_file_to(config_path, dist_config);
        printf("  + app.halproj\n");
        
        /* Create main exe (copy of runtime) */
        char exe_path[1024];  /* Increased buffer size to avoid truncation */
        snprintf(exe_path, sizeof(exe_path), "%s\\%s.exe", dist_dir, proj->name);
        copy_file_to(runtime_path, exe_path);
        printf("  + %s.exe\n", proj->name);
    }
    
    printf("[4/4] Cleaning up...\n");
    fflush(stdout);
    
    /* Clean up build temp directory */
    char del_cmd[1024];  /* Increased buffer size to avoid truncation */
    snprintf(del_cmd, sizeof(del_cmd), "rmdir /s /q \"%s\" 2>nul", build_dir);
    system(del_cmd);
    
    printf("\n========================================\n");
    printf("Build complete!\n");
    printf("========================================\n\n");
    printf("Output: %s\n\n", dist_dir);
    
    if (can_build_launcher) {
        printf("Created standalone launcher:\n");
        printf("  %s.exe - Run this file (requires HalcyonScript installed)\n\n", proj->name);
        printf("Note: HalcyonScript must be installed on target system.\n");
        printf("The app will extract scripts to temp folder and run them.\n");
    } else {
        printf("Created portable package:\n");
        printf("  %s.exe              - Main application\n", proj->name);
        printf("  halcyon_runtime.exe - Runtime engine\n");
        printf("  app.halproj         - Configuration\n");
        printf("  scripts/            - Application scripts\n\n");
        printf("Note: Distribute the entire dist folder.\n");
    }
    
    printf("\nTo run: %s\\%s.exe\n\n", dist_dir, proj->name);
    fflush(stdout);
    
    free(exe_dir);
    project_free(proj);
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow) {
    (void)hInst; (void)hPrev; (void)cmdShow;
    
    int argc;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    /* Convert to char** */
    char** argv = (char**)malloc(sizeof(char*) * argc);
    for (int i = 0; i < argc; i++) {
        int len = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, NULL, 0, NULL, NULL);
        argv[i] = (char*)malloc(len);
        WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, argv[i], len, NULL, NULL);
    }
    LocalFree(argvW);
    
    /* Initialize console for output */
    init_console();
    setlocale(LC_ALL, "");
    
    int result = 0;
    
    if (argc < 2) {
        /* Try to find app.halproj in current directory (for built apps) */
        FILE* f = fopen("app.halproj", "r");
        if (f) {
            fclose(f);
            result = run_project("app.halproj");
            goto cleanup;
        }
        
        /* Try to find any .halproj in current directory */
        char* proj_file = find_project_file();
        if (proj_file) {
            result = run_project(proj_file);
            free(proj_file);
            goto cleanup;
        }
        
        show_help();
        goto cleanup;
    }
    
    const char* cmd = argv[1];
    
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        show_help();
        goto cleanup;
    }
    
    if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0 || strcmp(cmd, "-v") == 0) {
        show_version();
        goto cleanup;
    }
    
    if (strcmp(cmd, "run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: No file specified\n");
            fflush(stderr);
            result = 1;
            goto cleanup;
        }
        if (is_project_file(argv[2])) {
            result = run_project(argv[2]);
        } else {
            result = run_script(argv[2]);
        }
        goto cleanup;
    }
    
    if (strcmp(cmd, "new") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: No project name specified\n");
            fflush(stderr);
            result = 1;
            goto cleanup;
        }
        result = create_project(argv[2]);
        goto cleanup;
    }
    
    if (strcmp(cmd, "check") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: No file specified\n");
            fflush(stderr);
            result = 1;
            goto cleanup;
        }
        result = check_script(argv[2]);
        goto cleanup;
    }
    
    if (strcmp(cmd, "build") == 0) {
        char* proj_file = NULL;
        
        if (argc >= 3) {
            /* Build specified project */
            proj_file = strdup(argv[2]);
        } else {
            /* Find .halproj in current directory */
            proj_file = find_project_file();
        }
        
        if (!proj_file) {
            fprintf(stderr, "Error: No .halproj file found in current directory\n");
            fprintf(stderr, "Usage: halcyon build <project.halproj>\n");
            fprintf(stderr, "Or create a project first: halcyon new <name>\n");
            fflush(stderr);
            result = 1;
            goto cleanup;
        }
        
        if (!is_project_file(proj_file)) {
            fprintf(stderr, "Error: %s is not a .halproj file\n", proj_file);
            fprintf(stderr, "Build command requires a .halproj project file\n");
            fflush(stderr);
            free(proj_file);
            result = 1;
            goto cleanup;
        }
        
        result = build_project(proj_file);
        free(proj_file);
        goto cleanup;
    }
    
    /* Try to run as script or project file */
    FILE* f = fopen(cmd, "r");
    if (f) {
        fclose(f);
        if (is_project_file(cmd)) {
            result = run_project(cmd);
        } else {
            result = run_script(cmd);
        }
        goto cleanup;
    }
    
    fprintf(stderr, "Error: Unknown command or file not found: %s\n", cmd);
    fprintf(stderr, "Use 'halcyon help' for usage.\n");
    fflush(stderr);
    result = 1;

cleanup:
    for (int i = 0; i < argc; i++) free(argv[i]);
    free(argv);
    return result;
}
