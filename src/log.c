#include <ctype.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "log.h"
#include "mkdir.h"

// maximum log record string
#define kMaxStrLen (1024)

// maximum log file buffer size
#define kLogBufferSize (1048576)

static unsigned char logBuffer[kLogBufferSize];
static long logBufferIndex = 0;
static time_t logBufferTime = 0;
static int logDataSize = 0;
static int logRecordSize = 0;

#define kDirStrLen (kMaxStrLen / 2)
#define kBaseStrLen (kMaxStrLen / 4)

static char basePath[kBaseStrLen] = {0};

static bool isSameMinute(time_t t1, time_t t2) {
  struct tm tm1 = *gmtime(&t1);
  struct tm tm2 = *gmtime(&t2);
  if (tm1.tm_min == tm2.tm_min)
    return true;
  return false;
}

void writeToDisk() {
  if (logBufferIndex == 0)
    return;
  struct tm tm = *gmtime(&logBufferTime);
  // path
  char directory[kDirStrLen];
  sprintf(directory, "%s%4.4lu/%2.2u/%2.2u", basePath, 1900L + tm.tm_year,
          tm.tm_mon + 1, tm.tm_mday);
  build(directory);

  // file name
  char destination[kMaxStrLen];
  sprintf(destination, "%s/%2.2u.dat", directory, tm.tm_hour);

  // write / append buffer to file
  FILE *fd = fopen(destination, "a");
  if (fd != NULL) {
    fwrite(logBuffer, 1, logBufferIndex, fd);
    fclose(fd);
    logBufferIndex = 0;
  }
}

void log_begin(const char *logPath, int dataSize) {
  logDataSize = dataSize;
  logRecordSize = dataSize + sizeof(uint32_t);
  logBufferIndex = 0;
  logBufferTime = 0;
  int length = strnlen(logPath, kMaxStrLen);
  strncpy(basePath, logPath, kBaseStrLen - 1);
  if (length) {
    if (basePath[length - 1] != '/')
      strncat(basePath, "/", kBaseStrLen - 1);
  }
}

static inline void appendToLogBuffer(unsigned char *logRecord) {
  memcpy(&logBuffer[logBufferIndex], logRecord, logRecordSize);
  logBufferIndex += logRecordSize;
  if (logBufferIndex >= (kLogBufferSize - logRecordSize)) {
    logBufferIndex = 0;
    fprintf(stderr, "Error : Log Buffer Overun\n");
  }
}

void log_commit(void *data) {
  // get millisecond timestamp for this log commit
  struct timeval tv;
  gettimeofday(&tv, NULL);
  time_t secondsNow = tv.tv_sec;
  struct tm tm = *gmtime(&secondsNow);
  uint32_t millisNow =
      (uint32_t)tv.tv_usec / 1000L +
      (uint32_t)(tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec) * 1000LL;
  if (logBufferTime == 0)
    logBufferTime = secondsNow;
  if (!isSameMinute(logBufferTime, secondsNow)) {
    writeToDisk();
    logBufferTime = secondsNow;
  }
  unsigned char
      logRecord[logRecordSize]; // add timestamp to frame and append to log
  *(uint32_t *)logRecord = htonl(millisNow);
  memcpy(logRecord + sizeof(uint32_t), data, logDataSize);
  appendToLogBuffer(logRecord);
}

void log_end(void) {
  if (logBufferIndex != 0) {
    writeToDisk();
  }
}
