#ifndef BINARY_STARTUP_H
#define BINARY_STARTUP_H

/* Emit the internal-linker mainCRTStartup object for --build. */
int binary_write_program_startup_object(const char *path, int profile_runtime,
                                        int stack_trace_init,
                                        int main_wants_argc_argv);

#endif /* BINARY_STARTUP_H */
