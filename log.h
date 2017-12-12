#pragma once
void set_log_mode(int set_enabled_log, int set_enabled_log_error);
void log_stdout(char *format, ...);
void log_error(char *format, ...);
void log_perror(char *message);