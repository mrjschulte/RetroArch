/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
 *  Copyright (C) 2018-2019 - Brad Parker
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <boolean.h>
#include <retro_assert.h>
#include <compat/posix_string.h>
#include <string/stdstring.h>
#include <streams/interface_stream.h>
#include <streams/file_stream.h>
#include <file/file_path.h>
#include <formats/jsonsax_full.h>

#include "playlist.h"
#include "verbosity.h"
#include "configuration.h"

#ifndef PLAYLIST_ENTRIES
#define PLAYLIST_ENTRIES 6
#endif

struct playlist_entry
{
   char *path;
   char *label;
   char *core_path;
   char *core_name;
   char *db_name;
   char *crc32;
};

struct content_playlist
{
   bool modified;
   size_t size;
   size_t cap;

   char *conf_path;
   struct playlist_entry *entries;
};

typedef struct
{
   JSON_Parser parser;
   JSON_Writer writer;
   RFILE *file;
   playlist_t *playlist;
   struct playlist_entry *current_entry;
   unsigned array_depth;
   unsigned object_depth;
   char **current_entry_val;
   char *current_meta_string;
   bool in_items;
} JSONContext;

static playlist_t *playlist_cached = NULL;

typedef int (playlist_sort_fun_t)(
      const struct playlist_entry *a,
      const struct playlist_entry *b);

uint32_t playlist_get_size(playlist_t *playlist)
{
   if (!playlist)
      return 0;
   return (uint32_t)playlist->size;
}

char *playlist_get_conf_path(playlist_t *playlist)
{
   if (!playlist)
      return NULL;
   return playlist->conf_path;
}

/**
 * playlist_get_index:
 * @playlist            : Playlist handle.
 * @idx                 : Index of playlist entry.
 * @path                : Path of playlist entry.
 * @core_path           : Core path of playlist entry.
 * @core_name           : Core name of playlist entry.
 *
 * Gets values of playlist index:
 **/
void playlist_get_index(playlist_t *playlist,
      size_t idx,
      const char **path, const char **label,
      const char **core_path, const char **core_name,
      const char **crc32,
      const char **db_name)
{
   if (!playlist)
      return;

   if (path)
      *path      = playlist->entries[idx].path;
   if (label)
      *label     = playlist->entries[idx].label;
   if (core_path)
      *core_path = playlist->entries[idx].core_path;
   if (core_name)
      *core_name = playlist->entries[idx].core_name;
   if (db_name)
      *db_name   = playlist->entries[idx].db_name;
   if (crc32)
      *crc32     = playlist->entries[idx].crc32;
}

/**
 * playlist_delete_index:
 * @playlist            : Playlist handle.
 * @idx                 : Index of playlist entry.
 *
 * Delete the entry at the index:
 **/
void playlist_delete_index(playlist_t *playlist,
      size_t idx)
{
   if (!playlist)
      return;

   playlist->size     = playlist->size - 1;

   memmove(playlist->entries + idx, playlist->entries + idx + 1,
         (playlist->size - idx) * sizeof(struct playlist_entry));

   playlist->modified = true;
}

void playlist_get_index_by_path(playlist_t *playlist,
      const char *search_path,
      char **path, char **label,
      char **core_path, char **core_name,
      char **crc32,
      char **db_name)
{
   size_t i;
   if (!playlist)
      return;

   for (i = 0; i < playlist->size; i++)
   {
      if (!string_is_equal(playlist->entries[i].path, search_path))
         continue;

      if (path)
         *path      = playlist->entries[i].path;
      if (label)
         *label     = playlist->entries[i].label;
      if (core_path)
         *core_path = playlist->entries[i].core_path;
      if (core_name)
         *core_name = playlist->entries[i].core_name;
      if (db_name)
         *db_name   = playlist->entries[i].db_name;
      if (crc32)
         *crc32     = playlist->entries[i].crc32;
      break;
   }
}

bool playlist_entry_exists(playlist_t *playlist,
      const char *path,
      const char *crc32)
{
   size_t i;
   if (!playlist)
      return false;

   for (i = 0; i < playlist->size; i++)
      if (string_is_equal(playlist->entries[i].path, path))
         return true;

   return false;
}

/**
 * playlist_free_entry:
 * @entry               : Playlist entry handle.
 *
 * Frees playlist entry.
 **/
static void playlist_free_entry(struct playlist_entry *entry)
{
   if (!entry)
      return;

   if (entry->path != NULL)
      free(entry->path);
   if (entry->label != NULL)
      free(entry->label);
   if (entry->core_path != NULL)
      free(entry->core_path);
   if (entry->core_name != NULL)
      free(entry->core_name);
   if (entry->db_name != NULL)
      free(entry->db_name);
   if (entry->crc32 != NULL)
      free(entry->crc32);

   entry->path      = NULL;
   entry->label     = NULL;
   entry->core_path = NULL;
   entry->core_name = NULL;
   entry->db_name   = NULL;
   entry->crc32     = NULL;
}

void playlist_update(playlist_t *playlist, size_t idx,
      const char *path, const char *label,
      const char *core_path, const char *core_name,
      const char *crc32,
      const char *db_name)
{
   struct playlist_entry *entry = NULL;

   if (!playlist || idx > playlist->size)
      return;

   entry            = &playlist->entries[idx];

   if (path && (path != entry->path))
   {
      if (entry->path != NULL)
         free(entry->path);
      entry->path        = strdup(path);
      playlist->modified = true;
   }

   if (label && (label != entry->label))
   {
      if (entry->label != NULL)
         free(entry->label);
      entry->label       = strdup(label);
      playlist->modified = true;
   }

   if (core_path && (core_path != entry->core_path))
   {
      if (entry->core_path != NULL)
         free(entry->core_path);
      entry->core_path   = NULL;
      entry->core_path   = strdup(core_path);
      playlist->modified = true;
   }

   if (core_name && (core_name != entry->core_name))
   {
      if (entry->core_name != NULL)
         free(entry->core_name);
      entry->core_name   = strdup(core_name);
      playlist->modified = true;
   }

   if (db_name && (db_name != entry->db_name))
   {
      if (entry->db_name != NULL)
         free(entry->db_name);
      entry->db_name     = strdup(db_name);
      playlist->modified = true;
   }

   if (crc32 && (crc32 != entry->crc32))
   {
      if (entry->crc32 != NULL)
         free(entry->crc32);
      entry->crc32       = strdup(crc32);
      playlist->modified = true;
   }
}

/**
 * playlist_push:
 * @playlist        	   : Playlist handle.
 * @path                : Path of new playlist entry.
 * @core_path           : Core path of new playlist entry.
 * @core_name           : Core name of new playlist entry.
 *
 * Push entry to top of playlist.
 **/
bool playlist_push(playlist_t *playlist,
      const char *path, const char *label,
      const char *core_path, const char *core_name,
      const char *crc32,
      const char *db_name)
{
   size_t i;
   bool core_path_empty = string_is_empty(core_path);
   bool core_name_empty = string_is_empty(core_name);

   if (core_path_empty || core_name_empty)
   {
      if (core_name_empty && !core_path_empty)
      {
         static char base_path[255] = {0};
         fill_pathname_base_noext(base_path, core_path, sizeof(base_path));
         core_name = base_path;
      }

      if (core_path_empty || core_name_empty)
      {
         RARCH_ERR("cannot push NULL or empty core name into the playlist.\n");
         return false;
      }
   }

   if (string_is_empty(path))
      path = NULL;

   if (!playlist)
      return false;

   for (i = 0; i < playlist->size; i++)
   {
      struct playlist_entry tmp;
      bool equal_path;

      equal_path = (!path && !playlist->entries[i].path) ||
         (path && playlist->entries[i].path &&
#ifdef _WIN32
          /*prevent duplicates on case-insensitive operating systems*/
          string_is_equal_noncase(path,playlist->entries[i].path)
#else
          string_is_equal(path,playlist->entries[i].path)
#endif
          );

      /* Core name can have changed while still being the same core.
       * Differentiate based on the core path only. */
      if (!equal_path)
         continue;

      if (!string_is_equal(playlist->entries[i].core_path, core_path))
         continue;

      /* If top entry, we don't want to push a new entry since
       * the top and the entry to be pushed are the same. */
      if (i == 0)
         return false;

      /* Seen it before, bump to top. */
      tmp = playlist->entries[i];
      memmove(playlist->entries + 1, playlist->entries,
            i * sizeof(struct playlist_entry));
      playlist->entries[0] = tmp;

      goto success;
   }

   if (playlist->size == playlist->cap)
   {
      struct playlist_entry *entry = &playlist->entries[playlist->cap - 1];

      if (entry)
         playlist_free_entry(entry);
      playlist->size--;
   }

   if (playlist->entries)
   {
      memmove(playlist->entries + 1, playlist->entries,
            (playlist->cap - 1) * sizeof(struct playlist_entry));

      playlist->entries[0].path         = NULL;
      playlist->entries[0].label        = NULL;
      playlist->entries[0].core_path    = NULL;
      playlist->entries[0].core_name    = NULL;
      playlist->entries[0].db_name      = NULL;
      playlist->entries[0].crc32        = NULL;
      if (!string_is_empty(path))
         playlist->entries[0].path      = strdup(path);
      if (!string_is_empty(label))
         playlist->entries[0].label     = strdup(label);
      if (!string_is_empty(core_path))
         playlist->entries[0].core_path = strdup(core_path);
      if (!string_is_empty(core_name))
         playlist->entries[0].core_name = strdup(core_name);
      if (!string_is_empty(db_name))
         playlist->entries[0].db_name   = strdup(db_name);
      if (!string_is_empty(crc32))
         playlist->entries[0].crc32     = strdup(crc32);
   }

   playlist->size++;

success:
   playlist->modified = true;

   return true;
}

static JSON_Writer_HandlerResult JSONOutputHandler(JSON_Writer writer, const char *pBytes, size_t length)
{
   JSONContext *context = (JSONContext*)JSON_Writer_GetUserData(writer);

   (void)writer; /* unused */
   return filestream_write(context->file, pBytes, length) == length ? JSON_Writer_Continue : JSON_Writer_Abort;
}

static void JSONLogError(JSONContext *pCtx)
{
   if (pCtx->parser && JSON_Parser_GetError(pCtx->parser) != JSON_Error_AbortedByHandler)
   {
      JSON_Error error = JSON_Parser_GetError(pCtx->parser);
      JSON_Location errorLocation = { 0, 0, 0 };
      (void)JSON_Parser_GetErrorLocation(pCtx->parser, &errorLocation);
      RARCH_WARN("Error: Invalid JSON at line %d, column %d (input byte %d) - %s.\n",
            (int)errorLocation.line + 1,
            (int)errorLocation.column + 1,
            (int)errorLocation.byte,
            JSON_ErrorString(error));
   }
   else if (pCtx->writer && JSON_Writer_GetError(pCtx->writer) != JSON_Error_AbortedByHandler)
   {
      RARCH_WARN("Error: could not write output - %s.\n", JSON_ErrorString(JSON_Writer_GetError(pCtx->writer)));
   }
}

void playlist_write_file(playlist_t *playlist)
{
   size_t i;
   RFILE *file = NULL;
   settings_t *settings = config_get_ptr();

   if (!playlist || !playlist->modified)
      return;

   file = filestream_open(playlist->conf_path,
         RETRO_VFS_FILE_ACCESS_WRITE, RETRO_VFS_FILE_ACCESS_HINT_NONE);

   if (!file)
   {
      RARCH_ERR("Failed to write to playlist file: %s\n", playlist->conf_path);
      return;
   }

   if (settings->bools.playlist_use_old_format)
   {
      for (i = 0; i < playlist->size; i++)
         filestream_printf(file, "%s\n%s\n%s\n%s\n%s\n%s\n",
               playlist->entries[i].path    ? playlist->entries[i].path    : "",
               playlist->entries[i].label   ? playlist->entries[i].label   : "",
               playlist->entries[i].core_path,
               playlist->entries[i].core_name,
               playlist->entries[i].crc32   ? playlist->entries[i].crc32   : "",
               playlist->entries[i].db_name ? playlist->entries[i].db_name : ""
               );
   }
   else
   {
      JSONContext context = {0};
      context.writer = JSON_Writer_Create(NULL);
      context.file = file;

      if (!context.writer)
      {
         RARCH_ERR("Failed to create JSON writer\n");
         goto end;
      }

      JSON_Writer_SetOutputEncoding(context.writer, JSON_UTF8);
      JSON_Writer_SetOutputHandler(context.writer, &JSONOutputHandler);
      JSON_Writer_SetUserData(context.writer, &context);

      JSON_Writer_WriteStartObject(context.writer);
      JSON_Writer_WriteNewLine(context.writer);
      JSON_Writer_WriteSpace(context.writer, 2);
      JSON_Writer_WriteString(context.writer, "version", strlen("version"), JSON_UTF8);
      JSON_Writer_WriteColon(context.writer);
      JSON_Writer_WriteSpace(context.writer, 1);
      JSON_Writer_WriteString(context.writer, "1.0", strlen("1.0"), JSON_UTF8);
      JSON_Writer_WriteComma(context.writer);
      JSON_Writer_WriteNewLine(context.writer);
      JSON_Writer_WriteSpace(context.writer, 2);
      JSON_Writer_WriteString(context.writer, "items", strlen("items"), JSON_UTF8);
      JSON_Writer_WriteColon(context.writer);
      JSON_Writer_WriteSpace(context.writer, 1);
      JSON_Writer_WriteStartArray(context.writer);
      JSON_Writer_WriteNewLine(context.writer);

      for (i = 0; i < playlist->size; i++)
      {
         JSON_Writer_WriteSpace(context.writer, 4);
         JSON_Writer_WriteStartObject(context.writer);

         JSON_Writer_WriteNewLine(context.writer);
         JSON_Writer_WriteSpace(context.writer, 6);
         JSON_Writer_WriteString(context.writer, "path", strlen("path"), JSON_UTF8);
         JSON_Writer_WriteColon(context.writer);
         JSON_Writer_WriteSpace(context.writer, 1);
         JSON_Writer_WriteString(context.writer, playlist->entries[i].path ? playlist->entries[i].path : "", playlist->entries[i].path ? strlen(playlist->entries[i].path) : 0, JSON_UTF8);
         JSON_Writer_WriteComma(context.writer);

         JSON_Writer_WriteNewLine(context.writer);
         JSON_Writer_WriteSpace(context.writer, 6);
         JSON_Writer_WriteString(context.writer, "label", strlen("label"), JSON_UTF8);
         JSON_Writer_WriteColon(context.writer);
         JSON_Writer_WriteSpace(context.writer, 1);
         JSON_Writer_WriteString(context.writer, playlist->entries[i].label ? playlist->entries[i].label : "", playlist->entries[i].label ? strlen(playlist->entries[i].label) : 0, JSON_UTF8);
         JSON_Writer_WriteComma(context.writer);

         JSON_Writer_WriteNewLine(context.writer);
         JSON_Writer_WriteSpace(context.writer, 6);
         JSON_Writer_WriteString(context.writer, "core_path", strlen("core_path"), JSON_UTF8);
         JSON_Writer_WriteColon(context.writer);
         JSON_Writer_WriteSpace(context.writer, 1);
         JSON_Writer_WriteString(context.writer, playlist->entries[i].core_path, strlen(playlist->entries[i].core_path), JSON_UTF8);
         JSON_Writer_WriteComma(context.writer);

         JSON_Writer_WriteNewLine(context.writer);
         JSON_Writer_WriteSpace(context.writer, 6);
         JSON_Writer_WriteString(context.writer, "core_name", strlen("core_name"), JSON_UTF8);
         JSON_Writer_WriteColon(context.writer);
         JSON_Writer_WriteSpace(context.writer, 1);
         JSON_Writer_WriteString(context.writer, playlist->entries[i].core_name, strlen(playlist->entries[i].core_name), JSON_UTF8);
         JSON_Writer_WriteComma(context.writer);

         JSON_Writer_WriteNewLine(context.writer);
         JSON_Writer_WriteSpace(context.writer, 6);
         JSON_Writer_WriteString(context.writer, "crc32", strlen("crc32"), JSON_UTF8);
         JSON_Writer_WriteColon(context.writer);
         JSON_Writer_WriteSpace(context.writer, 1);
         JSON_Writer_WriteString(context.writer, playlist->entries[i].crc32 ? playlist->entries[i].crc32 : "", playlist->entries[i].crc32 ? strlen(playlist->entries[i].crc32) : 0, JSON_UTF8);
         JSON_Writer_WriteComma(context.writer);

         JSON_Writer_WriteNewLine(context.writer);
         JSON_Writer_WriteSpace(context.writer, 6);
         JSON_Writer_WriteString(context.writer, "db_name", strlen("db_name"), JSON_UTF8);
         JSON_Writer_WriteColon(context.writer);
         JSON_Writer_WriteSpace(context.writer, 1);
         JSON_Writer_WriteString(context.writer, playlist->entries[i].db_name ? playlist->entries[i].db_name : "", playlist->entries[i].db_name ? strlen(playlist->entries[i].db_name) : 0, JSON_UTF8);
         JSON_Writer_WriteNewLine(context.writer);

         JSON_Writer_WriteSpace(context.writer, 4);
         JSON_Writer_WriteEndObject(context.writer);

         if (i < playlist->size - 1)
            JSON_Writer_WriteComma(context.writer);

         JSON_Writer_WriteNewLine(context.writer);
      }

      JSON_Writer_WriteSpace(context.writer, 2);
      JSON_Writer_WriteEndArray(context.writer);
      JSON_Writer_WriteNewLine(context.writer);
      JSON_Writer_WriteEndObject(context.writer);
      JSON_Writer_WriteNewLine(context.writer);
      JSON_Writer_Free(context.writer);
   }

   playlist->modified = false;

   RARCH_LOG("Written to playlist file: %s\n", playlist->conf_path);
end:
   filestream_close(file);
}

/**
 * playlist_free:
 * @playlist            : Playlist handle.
 *
 * Frees playlist handle.
 */
void playlist_free(playlist_t *playlist)
{
   size_t i;

   if (!playlist)
      return;

   if (playlist->conf_path != NULL)
      free(playlist->conf_path);

   playlist->conf_path = NULL;

   for (i = 0; i < playlist->size; i++)
   {
      struct playlist_entry *entry = &playlist->entries[i];

      if (entry)
         playlist_free_entry(entry);
   }

   free(playlist->entries);
   playlist->entries = NULL;

   free(playlist);
}

/**
 * playlist_clear:
 * @playlist        	   : Playlist handle.
 *
 * Clears all playlist entries in playlist.
 **/
void playlist_clear(playlist_t *playlist)
{
   size_t i;
   if (!playlist)
      return;

   for (i = 0; i < playlist->size; i++)
   {
      struct playlist_entry *entry = &playlist->entries[i];

      if (entry)
         playlist_free_entry(entry);
   }
   playlist->size = 0;
}

/**
 * playlist_size:
 * @playlist        	   : Playlist handle.
 *
 * Gets size of playlist.
 * Returns: size of playlist.
 **/
size_t playlist_size(playlist_t *playlist)
{
   if (!playlist)
      return 0;
   return playlist->size;
}

static JSON_Parser_HandlerResult JSONStartArrayHandler(JSON_Parser parser)
{
   JSONContext *pCtx = (JSONContext*)JSON_Parser_GetUserData(parser);

   pCtx->array_depth++;

   if (pCtx->object_depth == 1)
   {
      if (string_is_equal(pCtx->current_meta_string, "items") && pCtx->array_depth == 1)
      {
         pCtx->in_items = true;
      }
   }

   return JSON_Parser_Continue;
}

static JSON_Parser_HandlerResult JSONEndArrayHandler(JSON_Parser parser)
{
   JSONContext *pCtx = (JSONContext*)JSON_Parser_GetUserData(parser);

   retro_assert(pCtx->array_depth > 0);

   pCtx->array_depth--;

   if (pCtx->object_depth == 1)
   {
      if (pCtx->in_items && string_is_equal(pCtx->current_meta_string, "items") && pCtx->array_depth == 0)
      {
         free(pCtx->current_meta_string);
         pCtx->current_meta_string = NULL;
         pCtx->in_items = false;
      }
   }

   return JSON_Parser_Continue;
}

static JSON_Parser_HandlerResult JSONStartObjectHandler(JSON_Parser parser)
{
   JSONContext *pCtx = (JSONContext*)JSON_Parser_GetUserData(parser);

   pCtx->object_depth++;

   if (pCtx->in_items && pCtx->object_depth == 2)
   {
      if (pCtx->array_depth == 1)
      {
         if (pCtx->playlist->size < pCtx->playlist->cap)
         {
            pCtx->current_entry = &pCtx->playlist->entries[pCtx->playlist->size];
         }
         else
         {
            /* hit max item limit */
            return JSON_Parser_Abort;
         }
      }
   }

   return JSON_Parser_Continue;
}

static JSON_Parser_HandlerResult JSONEndObjectHandler(JSON_Parser parser)
{
   JSONContext *pCtx = (JSONContext*)JSON_Parser_GetUserData(parser);

   if (pCtx->in_items && pCtx->object_depth == 2)
   {
      if (pCtx->array_depth == 1)
      {
         pCtx->playlist->size++;
      }
   }

   retro_assert(pCtx->object_depth > 0);

   pCtx->object_depth--;

   return JSON_Parser_Continue;
}

static JSON_Parser_HandlerResult JSONStringHandler(JSON_Parser parser, char *pValue, size_t length, JSON_StringAttributes attributes)
{
   JSONContext *pCtx = (JSONContext*)JSON_Parser_GetUserData(parser);
   (void)attributes; /* unused */

   if (pCtx->in_items && pCtx->object_depth == 2)
   {
      if (pCtx->array_depth == 1)
      {
         if (pCtx->current_entry_val && length && !string_is_empty(pValue))
         {
            *pCtx->current_entry_val = strdup(pValue);
         }
         else
         {
            /* must be a value for an unknown member we aren't tracking, skip it */
         }
      }
   }
   else if (pCtx->object_depth == 1)
   {
      if (pCtx->array_depth == 0)
      {
         if (pCtx->current_meta_string && length && !string_is_empty(pValue))
         {
            /* handle any top-level playlist metadata here */
            /*RARCH_LOG("found meta: %s = %s\n", pCtx->current_meta_string, pValue);*/

            free(pCtx->current_meta_string);
         }
      }
   }

   pCtx->current_entry_val = NULL;

   return JSON_Parser_Continue;
}

static JSON_Parser_HandlerResult JSONObjectMemberHandler(JSON_Parser parser, char *pValue, size_t length, JSON_StringAttributes attributes)
{
   JSONContext *pCtx = (JSONContext*)JSON_Parser_GetUserData(parser);
   (void)attributes; /* unused */

   if (pCtx->in_items && pCtx->object_depth == 2)
   {
      if (pCtx->array_depth == 1)
      {
         if (pCtx->current_entry_val)
         {
            /* something went wrong */
            RARCH_WARN("JSON parsing failed at line %d.\n", __LINE__);
            return JSON_Parser_Abort;
         }

         if (length)
         {
            if (string_is_equal(pValue, "path"))
               pCtx->current_entry_val = &pCtx->current_entry->path;
            else if (string_is_equal(pValue, "label"))
               pCtx->current_entry_val = &pCtx->current_entry->label;
            else if (string_is_equal(pValue, "core_path"))
               pCtx->current_entry_val = &pCtx->current_entry->core_path;
            else if (string_is_equal(pValue, "core_name"))
               pCtx->current_entry_val = &pCtx->current_entry->core_name;
            else if (string_is_equal(pValue, "crc32"))
               pCtx->current_entry_val = &pCtx->current_entry->crc32;
            else if (string_is_equal(pValue, "db_name"))
               pCtx->current_entry_val = &pCtx->current_entry->db_name;
            else
            {
               /* ignore unknown members */
            }
         }
      }
   }
   else if (pCtx->object_depth == 1)
   {
      if (pCtx->array_depth == 0)
      {
         if (length)
         {
            pCtx->current_meta_string = strdup(pValue);
         }
      }
   }

   return JSON_Parser_Continue;
}

static bool playlist_read_file(
      playlist_t *playlist, const char *path)
{
   unsigned i;
   bool new_format = true;
   RFILE *file = filestream_open(path,
         RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);

   /* If playlist file does not exist,
    * create an empty playlist instead.
    */
   if (!file)
      return true;

   /* Detect format of playlist */
   {
      char buf[16] = {0};
      int64_t bytes_read = filestream_read(file, buf, 15);

      /* Empty playlist file */
      if (bytes_read == 0)
      {
         filestream_close(file);
         return true;
      }

      filestream_seek(file, 0, SEEK_SET);

      if (bytes_read == 15)
      {
         if (string_is_equal(buf, "{\n  \"version\": "))
         {
            /* new playlist format detected */
            /*RARCH_LOG("New playlist format detected.\n");*/
            new_format = true;
         }
         else
         {
            /* old playlist format detected */
            /*RARCH_LOG("Old playlist format detected.\n");*/
            new_format = false;
         }
      }
      else
      {
         /* corrupt playlist? */
         RARCH_ERR("Could not detect playlist format.\n");
      }
   }

   if (new_format)
   {
      JSONContext context = {0};
      context.parser = JSON_Parser_Create(NULL);
      context.file = file;
      context.playlist = playlist;

      if (!context.parser)
      {
         RARCH_ERR("Failed to create JSON parser\n");
         goto end;
      }

      /*JSON_Parser_SetTrackObjectMembers(context.parser, JSON_True);*/
      JSON_Parser_SetAllowBOM(context.parser, JSON_True);
      JSON_Parser_SetAllowComments(context.parser, JSON_True);
      JSON_Parser_SetAllowSpecialNumbers(context.parser, JSON_True);
      JSON_Parser_SetAllowHexNumbers(context.parser, JSON_True);
      JSON_Parser_SetAllowUnescapedControlCharacters(context.parser, JSON_True);
      JSON_Parser_SetReplaceInvalidEncodingSequences(context.parser, JSON_True);

      /*JSON_Parser_SetNullHandler(context.parser, &JSONNullHandler);
      JSON_Parser_SetBooleanHandler(context.parser, &JSONBooleanHandler);
      JSON_Parser_SetNumberHandler(context.parser, &JSONNumberHandler);
      JSON_Parser_SetSpecialNumberHandler(context.parser, &JSONSpecialNumberHandler);
      JSON_Parser_SetArrayItemHandler(context.parser, &JSONArrayItemHandler);*/

      JSON_Parser_SetStringHandler(context.parser, &JSONStringHandler);
      JSON_Parser_SetStartObjectHandler(context.parser, &JSONStartObjectHandler);
      JSON_Parser_SetEndObjectHandler(context.parser, &JSONEndObjectHandler);
      JSON_Parser_SetObjectMemberHandler(context.parser, &JSONObjectMemberHandler);
      JSON_Parser_SetStartArrayHandler(context.parser, &JSONStartArrayHandler);
      JSON_Parser_SetEndArrayHandler(context.parser, &JSONEndArrayHandler);
      JSON_Parser_SetUserData(context.parser, &context);

      while (!filestream_eof(file))
      {
         char chunk[4096] = {0};
         int64_t length = filestream_read(file, chunk, sizeof(chunk));

         if (!length && !filestream_eof(file))
         {
            RARCH_WARN("Could not read JSON input.\n");
            JSON_Parser_Free(context.parser);
            goto end;
         }

         if (!JSON_Parser_Parse(context.parser, chunk, length, JSON_False))
         {
            RARCH_WARN("Error parsing chunk:\n---snip---\n%s\n---snip---\n", chunk);
            JSONLogError(&context);
            JSON_Parser_Free(context.parser);
            goto end;
         }
      }

      if (!JSON_Parser_Parse(context.parser, NULL, 0, JSON_True))
      {
         RARCH_WARN("Error parsing JSON.\n");
         JSONLogError(&context);
         JSON_Parser_Free(context.parser);
         goto end;
      }

      JSON_Parser_Free(context.parser);
   }
   else
   {
      char buf[PLAYLIST_ENTRIES][1024] = {{0}};

      for (i = 0; i < PLAYLIST_ENTRIES; i++)
         buf[i][0] = '\0';

      for (playlist->size = 0; playlist->size < playlist->cap; )
      {
         unsigned i;
         struct playlist_entry *entry     = NULL;
         for (i = 0; i < PLAYLIST_ENTRIES; i++)
         {
            char *last  = NULL;
            *buf[i]     = '\0';

            if (!filestream_gets(file, buf[i], sizeof(buf[i])))
               goto end;

            /* Read playlist entry and terminate string with NUL character
             * regardless of Windows or Unix line endings
             */
            if((last = strrchr(buf[i], '\r')))
               *last = '\0';
            else if((last = strrchr(buf[i], '\n')))
               *last = '\0';
         }

         entry = &playlist->entries[playlist->size];

         if (!*buf[2] || !*buf[3])
            continue;

         if (*buf[0])
            entry->path      = strdup(buf[0]);
         if (*buf[1])
            entry->label     = strdup(buf[1]);

         entry->core_path    = strdup(buf[2]);
         entry->core_name    = strdup(buf[3]);
         if (*buf[4])
            entry->crc32     = strdup(buf[4]);
         if (*buf[5])
            entry->db_name   = strdup(buf[5]);
         playlist->size++;
      }
   }

end:
   filestream_close(file);
   return true;
}

void playlist_free_cached(void)
{
   playlist_free(playlist_cached);
   playlist_cached = NULL;
}

playlist_t *playlist_get_cached(void)
{
   if (playlist_cached)
      return playlist_cached;
   return NULL;
}

bool playlist_init_cached(const char *path, size_t size)
{
   playlist_t *playlist = playlist_init(path, size);
   if (!playlist)
      return false;

   playlist_cached      = playlist;
   return true;
}

/**
 * playlist_init:
 * @path            	   : Path to playlist contents file.
 * @size                : Maximum capacity of playlist size.
 *
 * Creates and initializes a playlist.
 *
 * Returns: handle to new playlist if successful, otherwise NULL
 **/
playlist_t *playlist_init(const char *path, size_t size)
{
   struct playlist_entry *entries = NULL;
   playlist_t           *playlist = (playlist_t*)malloc(sizeof(*playlist));
   if (!playlist)
      return NULL;

   entries = (struct playlist_entry*)calloc(size, sizeof(*entries));
   if (!entries)
   {
      free(playlist);
      return NULL;
   }

   playlist->modified  = false;
   playlist->size      = 0;
   playlist->cap       = size;
   playlist->conf_path = strdup(path);
   playlist->entries   = entries;

   playlist_read_file(playlist, path);

   return playlist;
}

static int playlist_qsort_func(const struct playlist_entry *a,
      const struct playlist_entry *b)
{
   const char *a_label = a ? a->label : NULL;
   const char *b_label = b ? b->label : NULL;

   if (!a_label || !b_label)
      return 0;

   return strcasecmp(a_label, b_label);
}

void playlist_qsort(playlist_t *playlist)
{
   qsort(playlist->entries, playlist->size,
         sizeof(struct playlist_entry),
         (int (*)(const void *, const void *))playlist_qsort_func);
}

void command_playlist_push_write(
      playlist_t *playlist,
      const char *path,
      const char *label,
      const char *core_path,
      const char *core_name)
{
   if (!playlist)
      return;

   if (playlist_push(
         playlist,
         path,
         label,
         core_path,
         core_name,
         NULL,
         NULL
         ))
      playlist_write_file(playlist);
}

void command_playlist_update_write(
      playlist_t *plist,
      size_t idx,
      const char *path,
      const char *label,
      const char *core_path,
      const char *core_display_name,
      const char *crc32,
      const char *db_name)
{
   playlist_t *playlist = plist ? plist : playlist_get_cached();

   if (!playlist)
      return;

   playlist_update(
         playlist,
         idx,
         path,
         label,
         core_path,
         core_display_name,
         crc32,
         db_name);

   playlist_write_file(playlist);
}
