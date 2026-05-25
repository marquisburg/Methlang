/**
 * tracy_helpers.c - Tracy profiler bridge for Mettle std/tracy.
 *
 * Build without Tracy (no-op stubs, safe to link for non-profiled builds):
 *   cl /c stdlib/tracy_helpers.c
 *   gcc -c stdlib/tracy_helpers.c -o tracy_helpers.o
 *
 * Build with Tracy instrumentation:
 *   cl /c /DTRACY_ENABLE /I <tracy>/public stdlib/tracy_helpers.c
 *   g++ -c /DTRACY_ENABLE -I<tracy>/public <tracy>/public/TracyClient.cpp -o TracyClient.o
 *
 * Link both objects when building the Mettle executable (see std/tracy.mettle).
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct mettle_tracy_zone {
  uint32_t id;
  int32_t active;
} mettle_tracy_zone_t;

#ifdef TRACY_ENABLE

#include "tracy/TracyC.h"

#ifndef TRACY_CALLSTACK
#define METTLE_TRACY_CALLSTACK 0
#else
#define METTLE_TRACY_CALLSTACK TRACY_CALLSTACK
#endif

static mettle_tracy_zone_t mettle_tracy_zone_from_ctx(TracyCZoneCtx ctx) {
  mettle_tracy_zone_t out;
  memcpy(&out, &ctx, sizeof(out));
  return out;
}

static mettle_tracy_zone_t mettle_tracy_zone_inactive(void) {
  mettle_tracy_zone_t out = {0, 0};
  return out;
}

void mettle_tracy_zone_begin(uint32_t line, const char *file, int64_t file_len,
                             const char *function, int64_t function_len,
                             const char *name, int64_t name_len, uint32_t color,
                             int32_t active, mettle_tracy_zone_t *out) {
  if (!out) {
    return;
  }
  if (!active) {
    *out = mettle_tracy_zone_inactive();
    return;
  }

  const char *zone_name = name;
  size_t zone_name_len = 0;
  if (name != NULL && name_len > 0) {
    zone_name_len = (size_t)name_len;
  } else {
    zone_name = NULL;
  }

  uint64_t srcloc = ___tracy_alloc_srcloc_name(
      line, file, (size_t)file_len, function, (size_t)function_len, zone_name,
      zone_name_len, color);
  TracyCZoneCtx ctx =
      ___tracy_emit_zone_begin_alloc(srcloc, active);
  *out = mettle_tracy_zone_from_ctx(ctx);
}

void mettle_tracy_zone_end(mettle_tracy_zone_t zone) {
  if (!zone.active) {
    return;
  }
  TracyCZoneCtx ctx;
  memcpy(&ctx, &zone, sizeof(ctx));
  ___tracy_emit_zone_end(ctx);
}

void mettle_tracy_zone_text(mettle_tracy_zone_t zone, const char *text,
                            int64_t text_len) {
  if (!zone.active || text == NULL || text_len <= 0) {
    return;
  }
  TracyCZoneCtx ctx;
  memcpy(&ctx, &zone, sizeof(ctx));
  ___tracy_emit_zone_text(ctx, text, (size_t)text_len);
}

void mettle_tracy_zone_name(mettle_tracy_zone_t zone, const char *text,
                            int64_t text_len) {
  if (!zone.active || text == NULL || text_len <= 0) {
    return;
  }
  TracyCZoneCtx ctx;
  memcpy(&ctx, &zone, sizeof(ctx));
  ___tracy_emit_zone_name(ctx, text, (size_t)text_len);
}

void mettle_tracy_zone_color(mettle_tracy_zone_t zone, uint32_t color) {
  if (!zone.active) {
    return;
  }
  TracyCZoneCtx ctx;
  memcpy(&ctx, &zone, sizeof(ctx));
  ___tracy_emit_zone_color(ctx, color);
}

void mettle_tracy_zone_value(mettle_tracy_zone_t zone, uint64_t value) {
  if (!zone.active) {
    return;
  }
  TracyCZoneCtx ctx;
  memcpy(&ctx, &zone, sizeof(ctx));
  ___tracy_emit_zone_value(ctx, value);
}

void mettle_tracy_frame_mark(void) { ___tracy_emit_frame_mark(NULL); }

void mettle_tracy_frame_mark_named(const char *name) {
  ___tracy_emit_frame_mark(name);
}

void mettle_tracy_frame_mark_start(const char *name) {
  ___tracy_emit_frame_mark_start(name);
}

void mettle_tracy_frame_mark_end(const char *name) {
  ___tracy_emit_frame_mark_end(name);
}

void mettle_tracy_set_thread_name(const char *name) {
  if (name != NULL) {
    ___tracy_set_thread_name(name);
  }
}

void mettle_tracy_plot(const char *name, double value) {
  if (name != NULL) {
    ___tracy_emit_plot(name, value);
  }
}

void mettle_tracy_plot_float(const char *name, float value) {
  if (name != NULL) {
    ___tracy_emit_plot_float(name, value);
  }
}

void mettle_tracy_plot_int(const char *name, int64_t value) {
  if (name != NULL) {
    ___tracy_emit_plot_int(name, value);
  }
}

void mettle_tracy_plot_config(const char *name, int32_t type, int32_t step,
                              int32_t fill, uint32_t color) {
  if (name != NULL) {
    ___tracy_emit_plot_config(name, type, step, fill, color);
  }
}

void mettle_tracy_message(const char *text, int64_t text_len) {
  if (text == NULL || text_len <= 0) {
    return;
  }
  ___tracy_emit_logString(TracyMessageSeverityInfo, 0, METTLE_TRACY_CALLSTACK,
                          (size_t)text_len, text);
}

void mettle_tracy_message_colored(const char *text, int64_t text_len,
                                  uint32_t color) {
  if (text == NULL || text_len <= 0) {
    return;
  }
  ___tracy_emit_logString(TracyMessageSeverityInfo, (int32_t)color,
                          METTLE_TRACY_CALLSTACK, (size_t)text_len, text);
}

void mettle_tracy_message_literal(const char *text) {
  if (text != NULL) {
    ___tracy_emit_logStringL(TracyMessageSeverityInfo, 0,
                             METTLE_TRACY_CALLSTACK, text);
  }
}

void mettle_tracy_app_info(const char *text, int64_t text_len) {
  if (text == NULL || text_len <= 0) {
    return;
  }
  ___tracy_emit_message_appinfo(text, (size_t)text_len);
}

void mettle_tracy_alloc(const void *ptr, int64_t size) {
  if (ptr == NULL) {
    return;
  }
  ___tracy_emit_memory_alloc_callstack(ptr, (size_t)size, METTLE_TRACY_CALLSTACK,
                                       0);
}

void mettle_tracy_free(const void *ptr) {
  if (ptr == NULL) {
    return;
  }
  ___tracy_emit_memory_free_callstack(ptr, METTLE_TRACY_CALLSTACK, 0);
}

int32_t mettle_tracy_connected(void) { return ___tracy_connected(); }

int32_t mettle_tracy_started(void) {
#ifdef TRACY_MANUAL_LIFETIME
  return ___tracy_profiler_started();
#else
  return 1;
#endif
}

void mettle_tracy_startup(void) {
#ifdef TRACY_MANUAL_LIFETIME
  ___tracy_startup_profiler();
#endif
}

void mettle_tracy_shutdown(void) {
#ifdef TRACY_MANUAL_LIFETIME
  ___tracy_shutdown_profiler();
#endif
}

#else /* !TRACY_ENABLE */

void mettle_tracy_zone_begin(uint32_t line, const char *file, int64_t file_len,
                             const char *function, int64_t function_len,
                             const char *name, int64_t name_len, uint32_t color,
                             int32_t active, mettle_tracy_zone_t *out) {
  (void)line;
  (void)file;
  (void)file_len;
  (void)function;
  (void)function_len;
  (void)name;
  (void)name_len;
  (void)color;
  (void)active;
  if (out) {
    out->id = 0;
    out->active = 0;
  }
}

void mettle_tracy_zone_end(mettle_tracy_zone_t zone) {
  (void)zone;
}

void mettle_tracy_zone_text(mettle_tracy_zone_t zone, const char *text,
                            int64_t text_len) {
  (void)zone;
  (void)text;
  (void)text_len;
}

void mettle_tracy_zone_name(mettle_tracy_zone_t zone, const char *text,
                            int64_t text_len) {
  (void)zone;
  (void)text;
  (void)text_len;
}

void mettle_tracy_zone_color(mettle_tracy_zone_t zone, uint32_t color) {
  (void)zone;
  (void)color;
}

void mettle_tracy_zone_value(mettle_tracy_zone_t zone, uint64_t value) {
  (void)zone;
  (void)value;
}

void mettle_tracy_frame_mark(void) {}
void mettle_tracy_frame_mark_named(const char *name) { (void)name; }
void mettle_tracy_frame_mark_start(const char *name) { (void)name; }
void mettle_tracy_frame_mark_end(const char *name) { (void)name; }
void mettle_tracy_set_thread_name(const char *name) { (void)name; }
void mettle_tracy_plot(const char *name, double value) {
  (void)name;
  (void)value;
}
void mettle_tracy_plot_float(const char *name, float value) {
  (void)name;
  (void)value;
}
void mettle_tracy_plot_int(const char *name, int64_t value) {
  (void)name;
  (void)value;
}
void mettle_tracy_plot_config(const char *name, int32_t type, int32_t step,
                              int32_t fill, uint32_t color) {
  (void)name;
  (void)type;
  (void)step;
  (void)fill;
  (void)color;
}
void mettle_tracy_message(const char *text, int64_t text_len) {
  (void)text;
  (void)text_len;
}
void mettle_tracy_message_colored(const char *text, int64_t text_len,
                                  uint32_t color) {
  (void)text;
  (void)text_len;
  (void)color;
}
void mettle_tracy_message_literal(const char *text) { (void)text; }
void mettle_tracy_app_info(const char *text, int64_t text_len) {
  (void)text;
  (void)text_len;
}
void mettle_tracy_alloc(const void *ptr, int64_t size) {
  (void)ptr;
  (void)size;
}
void mettle_tracy_free(const void *ptr) { (void)ptr; }
int32_t mettle_tracy_connected(void) { return 0; }
int32_t mettle_tracy_started(void) { return 0; }
void mettle_tracy_startup(void) {}
void mettle_tracy_shutdown(void) {}

#endif
