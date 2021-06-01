#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// maximum log record string
#define log_kMaxStrLen (1024)

// maximum log file buffer size
#define log_kFileBufferSize (1048576)

typedef struct {
  int fileSize;
  int fileIndex;
  time_t fileTime;
  int dataSize;
  char basePath[log_kMaxStrLen];
  unsigned char fileBuffer[log_kFileBufferSize];
} log_t;

void log_begin(log_t *logger, const char *logPath, int dataSize);
long int log_commit(log_t *logger, void* data);
long int log_millis(struct timespec *ts);
int log_read(log_t *logger, struct timespec *ts, void *data);
void log_end(log_t *logger);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
