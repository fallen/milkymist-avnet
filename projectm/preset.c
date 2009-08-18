/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2004 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#include "win32-dirent.h"
#else
#include <dirent.h>
#endif /** WIN32 */
#include <time.h>

#include "projectM.h"

#include "common.h"
#include "fatal.h"

#include "preset_types.h"
#include "preset.h"

#include "parser.h"

#include "expr_types.h"
#include "eval.h"

#include "splaytree_types.h"
#include "splaytree.h"
#include "tree_types.h"

#include "per_frame_eqn_types.h"
#include "per_frame_eqn.h"

#include "per_pixel_eqn_types.h"
#include "per_pixel_eqn.h"

#include "init_cond_types.h"
#include "init_cond.h"

#include "param_types.h"
#include "param.h"

#include "func_types.h"
#include "func.h"

#include "custom_wave_types.h"
#include "custom_wave.h"

#include "custom_shape_types.h"
#include "custom_shape.h"

#include "idle_preset.h"

/* The maximum number of preset names loaded into buffer */
#define MAX_PRESETS_IN_DIR 50000
extern int per_frame_eqn_count;
extern int per_frame_init_eqn_count;
//extern int custom_per_frame_eqn_count;

extern splaytree_t * builtin_param_tree;

preset_t * idle_preset = NULL;
FILE * write_stream = NULL;

int get_preset_path(char ** preset_path_ptr, char * filepath, char * filename);
preset_t * load_preset(char * pathname);
int is_valid_extension(char * name);
int load_preset_file(char * pathname, preset_t * preset);
int close_preset(preset_t * preset);

int write_preset_name(FILE * fs);
int write_per_pixel_equations(FILE * fs);
int write_per_frame_equations(FILE * fs);
int write_per_frame_init_equations(FILE * fs);
int write_init_conditions(FILE * fs);
void load_init_cond(param_t * param);
void load_init_conditions();
void write_init(init_cond_t * init_cond);
int init_idle_preset();
int destroy_idle_preset();
void load_custom_wave_init_conditions();
void load_custom_wave_init(custom_wave_t * custom_wave);

void load_custom_shape_init_conditions();
void load_custom_shape_init(custom_shape_t * custom_shape);

/* Loads a specific preset by absolute path */
int loadPresetByFile(char * filename) {

  preset_t * new_preset;

  /* Finally, load the preset using its actual path */
  if ((new_preset = load_preset(filename)) == NULL) {
#ifdef PRESET_DEBUG
        printf("loadPresetByFile: failed to load preset!\n");
#endif
        return PROJECTM_ERROR;
  }

  /* Closes a preset currently loaded, if any */
  if ((PM->active_preset != NULL) && (PM->active_preset != idle_preset))
    close_preset(PM->active_preset);

  /* Sets active preset global pointer */
  PM->active_preset = new_preset;

  /* Reinitialize engine variables */
  projectM_resetengine( PM);


  /* Add any missing initial conditions */
  load_init_conditions();
  
  /* Add any missing initial conditions for each wave */
  load_custom_wave_init_conditions();

 /* Add any missing initial conditions for each wave */
  load_custom_shape_init_conditions();

  /* Need to do this once for menu */
  evalInitConditions();
  //  evalPerFrameInitEquations();
  return PROJECTM_SUCCESS;

}

int init_idle_preset() {

  preset_t * preset;
    /* Initialize idle preset struct */
  if ((preset = (preset_t*)wipemalloc(sizeof(preset_t))) == NULL)
    return PROJECTM_FAILURE;


  strncpy(preset->name, "idlepreset", strlen("idlepreset"));

  /* Initialize equation trees */
  preset->init_cond_tree = create_splaytree(compare_string, copy_string, free_string);
  preset->user_param_tree = create_splaytree(compare_string, copy_string, free_string);
  preset->per_frame_eqn_tree = create_splaytree(compare_int, copy_int, free_int);
  preset->per_pixel_eqn_tree = create_splaytree(compare_int, copy_int, free_int);
  preset->per_frame_init_eqn_tree = create_splaytree(compare_string, copy_string, free_string);
  preset->custom_wave_tree = create_splaytree(compare_int, copy_int, free_int);
  preset->custom_shape_tree = create_splaytree(compare_int, copy_int, free_int);

  /* Set file path to dummy name */
  strncpy(preset->file_path, "IDLE PRESET", MAX_PATH_SIZE-1);

  /* Set initial index values */
  preset->per_pixel_eqn_string_index = 0;
  preset->per_frame_eqn_string_index = 0;
  preset->per_frame_init_eqn_string_index = 0;
  memset(preset->per_pixel_flag, 0, sizeof(int)*NUM_OPS);

  /* Clear string buffers */
  memset(preset->per_pixel_eqn_string_buffer, 0, STRING_BUFFER_SIZE);
  memset(preset->per_frame_eqn_string_buffer, 0, STRING_BUFFER_SIZE);
  memset(preset->per_frame_init_eqn_string_buffer, 0, STRING_BUFFER_SIZE);

  idle_preset = preset;

  return PROJECTM_SUCCESS;
}

int destroy_idle_preset() {

  return close_preset(idle_preset);

}

/* initPresetLoader: initializes the preset
   loading library. this should be done before
   any parsing */
int initPresetLoader() {

  /* Initializes the builtin parameter database */
  init_builtin_param_db();

  /* Initializes the builtin function database */
  init_builtin_func_db();

  /* Initializes all infix operators */
  init_infix_ops();

  /* Set the seed to the current time in seconds */
  srand(time(NULL));

  /* Initialize the 'idle' preset */
  init_idle_preset();



  projectM_resetengine( PM);

//  PM->active_preset = idle_preset;
   
    switchToIdlePreset();
  load_init_conditions();

  /* Done */
#ifdef PRESET_DEBUG
    printf("initPresetLoader: finished\n");
#endif
  return PROJECTM_SUCCESS;
}

/* Sort of experimental code here. This switches
   to a hard coded preset. Useful if preset directory
   was not properly loaded, or a preset fails to parse */

void switchToIdlePreset() {

    if ( idle_preset == NULL ) {
        return;
      }

  /* Idle Preset already activated */
  if (PM->active_preset == idle_preset)
    return;


  /* Close active preset */
  if (PM->active_preset != NULL)
    close_preset(PM->active_preset);

  /* Sets global PM->active_preset pointer */
  PM->active_preset = idle_preset;

  /* Reinitialize the engine variables to sane defaults */
  projectM_resetengine( PM);

  /* Add any missing initial conditions */
  load_init_conditions();

  /* Need to evaluate the initial conditions once */
  evalInitConditions();

}

/* destroyPresetLoader: closes the preset
   loading library. This should be done when
   projectM does cleanup */

int destroyPresetLoader() {

  if ((PM->active_preset != NULL) && (PM->active_preset != idle_preset)) {
        close_preset(PM->active_preset);
  }

  PM->active_preset = NULL;

  destroy_idle_preset();
  destroy_builtin_param_db();
  destroy_builtin_func_db();
  destroy_infix_ops();

  return PROJECTM_SUCCESS;

}

/* load_preset_file: private function that loads a specific preset denoted
   by the given pathname */
int load_preset_file(char * pathname, preset_t * preset) {
  FILE * fs;
  int retval;
    int lineno;

  if (pathname == NULL)
          return PROJECTM_FAILURE;
  if (preset == NULL)
          return PROJECTM_FAILURE;

  /* Open the file corresponding to pathname */
  if ((fs = fopen(pathname, "rb")) == 0) {
#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf( debugFile,"load_preset_file: loading of file %s failed!\n", pathname);
      }
#endif
    return PROJECTM_ERROR;
  }

#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf( debugFile,"load_preset_file: file stream \"%s\" opened successfully\n", pathname);
      }
#endif

  /* Parse any comments */
  if (parse_top_comment(fs) < 0) {
#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: no left bracket found...\n");
      }
#endif
    fclose(fs);
    return PROJECTM_FAILURE;
  }

  /* Parse the preset name and a left bracket */
  if (parse_preset_name(fs, preset->name) < 0) {
#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: loading of preset name in file \"%s\" failed\n", pathname);
      }
#endif
    fclose(fs);
    return PROJECTM_ERROR;
  }

#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: preset \"%s\" parsed\n", preset->name);
      }
#endif

  /* Parse each line until end of file */
    lineno = 0;
#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: beginning line parsing...\n");
      }
#endif
  while ((retval = parse_line(fs, preset)) != EOF) {
    if (retval == PROJECTM_PARSE_ERROR) {
#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: parse error in file \"%s\": line %d\n", pathname,lineno);
      }
#endif
    }
    lineno++;
  }

#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: finished line parsing successfully\n");
        fflush( debugFile );
      }
#endif

  /* Now the preset has been loaded.
     Evaluation calls can be made at appropiate
     times in the frame loop */

  fclose(fs);

#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: file \"%s\" closed, preset ready\n", pathname);
      }
#endif
  return PROJECTM_SUCCESS;

}

void evalInitConditions() {
  splay_traverse(eval_init_cond, PM->active_preset->init_cond_tree);
  splay_traverse(eval_init_cond, PM->active_preset->per_frame_init_eqn_tree);
}

void evalPerFrameEquations() {
  splay_traverse(eval_per_frame_eqn, PM->active_preset->per_frame_eqn_tree);
}

void evalPerFrameInitEquations() {
  //printf("evalPerFrameInitEquations: per frame init unimplemented!\n");
  //  splay_traverse(eval_per_frame_eqn, PM->active_preset->per_frame_init_eqn_tree);
}

/* Returns nonzero if string 'name' contains .milk or
   (the better) .prjm extension. Not a very strong function currently */
int is_valid_extension(char * name) {

#ifdef PRESET_DEBUG
                printf("is_valid_extension: scanning string \"%s\"...", name);
                fflush(stdout);
#endif

        if (strstr(name, MILKDROP_FILE_EXTENSION)) {
#if defined(PRESET_DEBUG) && defined(DEBUG)
            if ( debugFile != NULL ) {
                            fprintf( debugFile, "\".milk\" extension found in string [true]\n");
              }
#endif
                        return TRUE;
        }

        if (strstr(name, PROJECTM_FILE_EXTENSION)) {
#if defined(PRESET_DEBUG) && defined(DEBUG)
            if ( debugFile != NULL ) {
                        printf("\".prjm\" extension found in string [true]\n");
              }
#endif
                        return TRUE;
        }

#ifdef PRESET_DEBUG
        if (PRESET_DEBUG > 1) printf("no valid extension found [false]\n");
#endif
        return FALSE;
}

/* Private function to close a preset file */
int close_preset(preset_t * preset) {

#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf( debugFile, "close_preset(): in\n" );
        fflush( debugFile );
      }
#endif

  if (preset == NULL)
    return PROJECTM_FAILURE;

  splay_traverse(free_init_cond, preset->init_cond_tree);
  destroy_splaytree(preset->init_cond_tree);

  splay_traverse(free_init_cond, preset->per_frame_init_eqn_tree);
  destroy_splaytree(preset->per_frame_init_eqn_tree);

  splay_traverse(free_per_pixel_eqn, preset->per_pixel_eqn_tree);
  destroy_splaytree(preset->per_pixel_eqn_tree);

  splay_traverse(free_per_frame_eqn, preset->per_frame_eqn_tree);
  destroy_splaytree(preset->per_frame_eqn_tree);

  splay_traverse(free_param, preset->user_param_tree);
  destroy_splaytree(preset->user_param_tree);

  splay_traverse(free_custom_wave, preset->custom_wave_tree);
  destroy_splaytree(preset->custom_wave_tree);

  splay_traverse(free_custom_shape, preset->custom_shape_tree);
  destroy_splaytree(preset->custom_shape_tree);

  free(preset);
  preset = NULL;

#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf( debugFile, "close_preset(): out\n" );
        fflush( debugFile );
      }
#endif

  return PROJECTM_SUCCESS;

}

void reloadPerPixel(char *s, preset_t * preset) {

  FILE * fs;
  int slen;
  char c;
  int i;

  if (s == NULL)
    return;

  if (preset == NULL)
    return;

  /* Clear previous per pixel equations */
  splay_traverse(free_per_pixel_eqn, preset->per_pixel_eqn_tree);
  destroy_splaytree(preset->per_pixel_eqn_tree);
  preset->per_pixel_eqn_tree = create_splaytree(compare_int, copy_int, free_int);

  /* Convert string to a stream */
#if !defined(MACOS) && !defined(WIN32)
  fs = fmemopen (s, strlen(s), "r");

  while ((c = fgetc(fs)) != EOF) {
    ungetc(c, fs);
    parse_per_pixel_eqn(fs, preset);
  }

  fclose(fs);
#else
printf( "reloadPerPixel()\n" );
#endif

  /* Clear string space */
  memset(preset->per_pixel_eqn_string_buffer, 0, STRING_BUFFER_SIZE);

  /* Compute length of string */
  slen = strlen(s);

  /* Copy new string into buffer */
  strncpy(preset->per_pixel_eqn_string_buffer, s, slen);

  /* Yet again no bounds checking */
  preset->per_pixel_eqn_string_index = slen;

  /* Finished */

  return;
}

/* Obviously unwritten */
void reloadPerFrameInit(char *s, preset_t * preset) {

}

void reloadPerFrame(char * s, preset_t * preset) {

  FILE * fs;
  int slen;
  char c;
  int eqn_count = 1;
  per_frame_eqn_t * per_frame;

  if (s == NULL)
    return;

  if (preset == NULL)
    return;

  /* Clear previous per frame equations */
  splay_traverse(free_per_frame_eqn, preset->per_frame_eqn_tree);
  destroy_splaytree(preset->per_frame_eqn_tree);
  preset->per_frame_eqn_tree = create_splaytree(compare_int, copy_int, free_int);

  /* Convert string to a stream */
#if !defined(MACOS) && !defined(WIN32)
  fs = fmemopen (s, strlen(s), "r");

  while ((c = fgetc(fs)) != EOF) {
    ungetc(c, fs);
    if ((per_frame = parse_per_frame_eqn(fs, eqn_count, preset)) != NULL) {
      splay_insert(per_frame, &eqn_count, preset->per_frame_eqn_tree);
      eqn_count++;
    }
  }

  fclose(fs);
#else
printf( "reloadPerFrame()\n" );
#endif

  /* Clear string space */
  memset(preset->per_frame_eqn_string_buffer, 0, STRING_BUFFER_SIZE);

  /* Compute length of string */
  slen = strlen(s);

  /* Copy new string into buffer */
  strncpy(preset->per_frame_eqn_string_buffer, s, slen);

  /* Yet again no bounds checking */
  preset->per_frame_eqn_string_index = slen;

  /* Finished */
  printf("reloadPerFrame: %d eqns parsed succesfully\n", eqn_count-1);
  return;

}

preset_t * load_preset(char * pathname) {

  preset_t * preset;
  int i;

  //printf( "loading preset from '%s'\n", pathname );

  /* Initialize preset struct */
  if ((preset = (preset_t*)wipemalloc(sizeof(preset_t))) == NULL)
    return NULL;

  /* Initialize equation trees */
  preset->init_cond_tree = create_splaytree(compare_string, copy_string, free_string);
  preset->user_param_tree = create_splaytree(compare_string, copy_string, free_string);
  preset->per_frame_eqn_tree = create_splaytree(compare_int, copy_int, free_int);
  preset->per_pixel_eqn_tree = create_splaytree(compare_int, copy_int, free_int);
  preset->per_frame_init_eqn_tree = create_splaytree(compare_string, copy_string, free_string);
  preset->custom_wave_tree = create_splaytree(compare_int, copy_int, free_int);
  preset->custom_shape_tree = create_splaytree(compare_int, copy_int, free_int);

  memset(preset->per_pixel_flag, 0, sizeof(int)*NUM_OPS);

  /* Copy file path */
  if ( pathname == NULL ) {
    close_preset( preset );
    return NULL;
  }
  strncpy(preset->file_path, pathname, MAX_PATH_SIZE-1);

  /* Set initial index values */
  preset->per_pixel_eqn_string_index = 0;
  preset->per_frame_eqn_string_index = 0;
  preset->per_frame_init_eqn_string_index = 0;


  /* Clear string buffers */
  memset(preset->per_pixel_eqn_string_buffer, 0, STRING_BUFFER_SIZE);
  memset(preset->per_frame_eqn_string_buffer, 0, STRING_BUFFER_SIZE);
  memset(preset->per_frame_init_eqn_string_buffer, 0, STRING_BUFFER_SIZE);


  if (load_preset_file(pathname, preset) < 0) {
#ifdef PRESET_DEBUG
        if (PRESET_DEBUG) printf("load_preset: failed to load file \"%s\"\n", pathname);
#endif
        close_preset(preset);
        return NULL;
  }

  /* It's kind of ugly to reset these values here. Should definitely be placed in the parser somewhere */
  per_frame_eqn_count = 0;
  per_frame_init_eqn_count = 0;

  /* Finished, return new preset */
  return preset;
}

void savePreset(char * filename) {

  FILE * fs;

  if (filename == NULL)
    return;

  /* Open the file corresponding to pathname */
  if ((fs = fopen(filename, "w+")) == 0) {
#ifdef PRESET_DEBUG
    if (PRESET_DEBUG) printf("savePreset: failed to create filename \"%s\"!\n", filename);
#endif
    return;
  }

  write_stream = fs;

  if (write_preset_name(fs) < 0) {
    write_stream = NULL;
    fclose(fs);
    return;
  }

  if (write_init_conditions(fs) < 0) {
    write_stream = NULL;
    fclose(fs);
    return;
  }

  if (write_per_frame_init_equations(fs) < 0) {
    write_stream = NULL;
    fclose(fs);
    return;
  }

  if (write_per_frame_equations(fs) < 0) {
    write_stream = NULL;
    fclose(fs);
    return;
  }

  if (write_per_pixel_equations(fs) < 0) {
    write_stream = NULL;
    fclose(fs);
    return;
  }

  write_stream = NULL;
  fclose(fs);

}

int write_preset_name(FILE * fs) {

  char s[256];
  int len;

  memset(s, 0, 256);

  if (fs == NULL)
    return PROJECTM_FAILURE;

  /* Format the preset name in a string */
  sprintf(s, "[%s]\n", PM->active_preset->name);

  len = strlen(s);

  /* Write preset name to file stream */
  if (fwrite(s, 1, len, fs) != len)
    return PROJECTM_FAILURE;

  return PROJECTM_SUCCESS;

}

int write_init_conditions(FILE * fs) {

  if (fs == NULL)
    return PROJECTM_FAILURE;
  if (PM->active_preset == NULL)
    return PROJECTM_FAILURE;


  splay_traverse(write_init, PM->active_preset->init_cond_tree);

  return PROJECTM_SUCCESS;
}

void write_init(init_cond_t * init_cond) {

  char s[512];
  int len;

  if (write_stream == NULL)
    return;

  memset(s, 0, 512);

  if (init_cond->param->type == P_TYPE_BOOL)
    sprintf(s, "%s=%d\n", init_cond->param->name, init_cond->init_val.bool_val);

  else if (init_cond->param->type == P_TYPE_INT)
    sprintf(s, "%s=%d\n", init_cond->param->name, init_cond->init_val.int_val);

  else if (init_cond->param->type == P_TYPE_DOUBLE)
    sprintf(s, "%s=%f\n", init_cond->param->name, init_cond->init_val.double_val);

  else { printf("write_init: unknown parameter type!\n"); return; }

  len = strlen(s);

  if ((fwrite(s, 1, len, write_stream)) != len)
    printf("write_init: failed writing to file stream! Out of disk space?\n");

}


int write_per_frame_init_equations(FILE * fs) {

  int len;

  if (fs == NULL)
    return PROJECTM_FAILURE;
  if (PM->active_preset == NULL)
    return PROJECTM_FAILURE;

  len = strlen(PM->active_preset->per_frame_init_eqn_string_buffer);

  if (fwrite(PM->active_preset->per_frame_init_eqn_string_buffer, 1, len, fs) != len)
    return PROJECTM_FAILURE;

  return PROJECTM_SUCCESS;
}


int write_per_frame_equations(FILE * fs) {

  int len;

  if (fs == NULL)
    return PROJECTM_FAILURE;
  if (PM->active_preset == NULL)
    return PROJECTM_FAILURE;

  len = strlen(PM->active_preset->per_frame_eqn_string_buffer);

  if (fwrite(PM->active_preset->per_frame_eqn_string_buffer, 1, len, fs) != len)
    return PROJECTM_FAILURE;

  return PROJECTM_SUCCESS;
}


int write_per_pixel_equations(FILE * fs) {

  int len;

  if (fs == NULL)
    return PROJECTM_FAILURE;
  if (PM->active_preset == NULL)
    return PROJECTM_FAILURE;

  len = strlen(PM->active_preset->per_pixel_eqn_string_buffer);

  if (fwrite(PM->active_preset->per_pixel_eqn_string_buffer, 1, len, fs) != len)
    return PROJECTM_FAILURE;

  return PROJECTM_SUCCESS;
}


void load_init_conditions() {

  splay_traverse(load_init_cond, builtin_param_tree);


}

void load_init_cond(param_t * param) {

  init_cond_t * init_cond;
  value_t init_val;

  /* Don't count read only parameters as initial conditions */
  if (param->flags & P_FLAG_READONLY)
    return;

  /* If initial condition was not defined by the preset file, force a default one
     with the following code */
  if ((init_cond = splay_find(param->name, PM->active_preset->init_cond_tree)) == NULL) {

    /* Make sure initial condition does not exist in the set of per frame initial equations */
    if ((init_cond = splay_find(param->name, PM->active_preset->per_frame_init_eqn_tree)) != NULL)
      return;

    if (param->type == P_TYPE_BOOL)
      init_val.bool_val = 0;

    else if (param->type == P_TYPE_INT)
      init_val.int_val = *(int*)param->engine_val;

    else if (param->type == P_TYPE_DOUBLE)
      init_val.double_val = *(double*)param->engine_val;

    /* Create new initial condition */
    if ((init_cond = new_init_cond(param, init_val)) == NULL)
      return;

    /* Insert the initial condition into this presets tree */
    if (splay_insert(init_cond, init_cond->param->name, PM->active_preset->init_cond_tree) < 0) {
      free_init_cond(init_cond);
      return;
    }

  }

}

void load_custom_wave_init_conditions() {

  splay_traverse(load_custom_wave_init, PM->active_preset->custom_wave_tree);

}

void load_custom_wave_init(custom_wave_t * custom_wave) {

  load_unspecified_init_conds(custom_wave);

}


void load_custom_shape_init_conditions() {

  splay_traverse(load_custom_shape_init, PM->active_preset->custom_shape_tree);

}

void load_custom_shape_init(custom_shape_t * custom_shape) {

  load_unspecified_init_conds_shape(custom_shape);

}