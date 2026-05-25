#ifndef METTLE_TRACY_BUILD_H
#define METTLE_TRACY_BUILD_H

#include <stddef.h>

typedef struct {
  char *helpers_object;
  char *client_object;
  int use_gxx_link;
} TracyBuildArtifacts;

typedef struct {
  const char *tracy_directory;
  const char *stdlib_directory;
  const char *executable_filename;
} TracyBuildRequest;

int tracy_directory_is_valid(const char *directory);
char *tracy_resolve_directory(const TracyBuildRequest *request, char **error_out);
int tracy_save_directory_config(const char *directory, char **error_out);
int tracy_build_support_objects(const TracyBuildRequest *request,
                                const char *tracy_directory,
                                TracyBuildArtifacts *artifacts,
                                char **error_out);
void tracy_free_artifacts(TracyBuildArtifacts *artifacts);

#endif
