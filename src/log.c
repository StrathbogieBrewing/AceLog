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

static bool isSameMinute(time_t t1, time_t t2) {
  struct tm tm1;
  gmtime_r(&t1, &tm1);
  struct tm tm2;
  gmtime_r(&t2, &tm2);
  if (tm1.tm_min == tm2.tm_min)
    return true;
  return false;
}

uint32_t millisFromHour(struct timespec *ts) {
  struct tm tm;
  gmtime_r(&ts->tv_sec, &tm);
  return (uint32_t)(ts->tv_nsec / 1000000LL) +
         ((uint32_t)tm.tm_min * 60LL + (uint32_t)tm.tm_sec) * 1000LL;
}

time_t secondsToHour(time_t seconds) {
  struct tm tm;
  gmtime_r(&seconds, &tm);
  tm.tm_sec = 0;
  tm.tm_min = 0;
  return timegm(&tm);
}

void fileName(time_t t) {
  struct tm tm;
  gmtime_r(&t, &tm);
  fprintf(stdout, "%4.4lu/%2.2u/%2.2u/%2.2u.dat\n", 1900L + tm.tm_year,
          tm.tm_mon + 1, tm.tm_mday, tm.tm_hour);
}

void writeToDisk(log_t *logger) {
  if (logger->fileIndex == 0)
    return;
  struct tm tm;
  gmtime_r(&logger->fileTime, &tm);
  // path
  char directory[log_kMaxStrLen * 2];
  sprintf(directory, "%s%4.4lu/%2.2u/%2.2u", logger->basePath,
          1900L + tm.tm_year, tm.tm_mon + 1, tm.tm_mday);
  build(directory);

  // build path and file name
  char destination[log_kMaxStrLen * 4];
  sprintf(destination, "%s/%2.2u.dat", directory, tm.tm_hour);

  // write / append buffer to file
  FILE *fd = fopen(destination, "a");
  if (fd != NULL) {
    fwrite(logger->fileBuffer, 1, logger->fileIndex, fd);
    fclose(fd);
    logger->fileIndex = 0;
  }
}

void log_begin(log_t *logger, const char *logPath, int dataSize) {
  logger->dataSize = dataSize;
  logger->fileIndex = 0;
  logger->fileTime = 0;
  int length = strnlen(logPath, log_kMaxStrLen);
  strncpy(logger->basePath, logPath, log_kMaxStrLen - 1);
  if (length) {
    if (logger->basePath[length - 1] != '/')
      strncat(logger->basePath, "/", log_kMaxStrLen - 1);
  }
}

void appendToLogBuffer(log_t *logger, uint32_t time, void *data) {
  // check for space in log file Buffer
  int logRecordSize = logger->dataSize + sizeof(uint32_t);
  if (logger->fileIndex + logRecordSize >= log_kFileBufferSize) {
    logger->fileIndex = 0; // just restart on overun
    fprintf(stderr, "Error : Log File Buffer Overun\n");
  }
  // add timestamp to log file buffer
  *(uint32_t *)&logger->fileBuffer[logger->fileIndex] = htonl(time);
  // add data to log file buffer
  memcpy(&logger->fileBuffer[logger->fileIndex + sizeof(uint32_t)], data,
         logger->dataSize);
  logger->fileIndex += logRecordSize; // update the log file buffer index
}

long int log_millis(struct timespec *ts) {
  return (long int)millisFromHour(ts) +
         (long int)secondsToHour(ts->tv_sec) * 1000LL;
}

long int log_commit(log_t *logger, void *data) {
  // get millisecond timestamp for this log commit
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  time_t secondsNow = ts.tv_sec;

  if (logger->fileTime == 0)
    logger->fileTime = secondsNow;
  if (!isSameMinute(logger->fileTime, secondsNow)) {
    writeToDisk(logger);
    logger->fileTime = secondsNow;
  }
  uint32_t millisNow = millisFromHour(&ts);
  appendToLogBuffer(logger, millisNow, data);
  return log_millis(&ts);
}

void log_end(log_t *logger) {
  if (logger->fileIndex != 0) {
    writeToDisk(logger);
  }
}

bool nextHour(struct timespec *ts) {
  ts->tv_sec = secondsToHour(ts->tv_sec) + 3600LL;
  ts->tv_nsec = 0;
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  return (ts->tv_sec < now.tv_sec);
}

// read hourly data log file into buffer, return bytes read
int getBuffer(log_t *logger, time_t fileTime) {
  struct tm tm;
  char filePath[log_kMaxStrLen * 2];
  gmtime_r(&fileTime, &tm);
  sprintf(filePath, "%s%4.4lu/%2.2u/%2.2u/%2.2u.dat", logger->basePath,
          1900L + tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour);
  logger->fileTime = secondsToHour(fileTime);
  logger->fileIndex = 0;
  FILE *fd = fopen(filePath, "r");
  if (fd != NULL) {
    logger->fileSize = fread(logger->fileBuffer, 1, log_kFileBufferSize, fd);
    fclose(fd);
  } else {
    logger->fileSize = 0;
  }
  // fprintf(stdout, "getBuffer %s %d\n", filePath, logger->fileSize);
  return logger->fileSize;
}

int getFileBuffer(log_t *logger, struct timespec *ts) {
  int bytesRead = 0;
  while ((bytesRead = getBuffer(logger, ts->tv_sec)) == 0) {
    if (nextHour(ts) == false) {
      return 0;
    }
  }
  return bytesRead;
}

// read the log record later than tv and earlier than now
int log_read(log_t *logger, struct timespec *ts, void *data) {
  // fileName(ts->tv_sec);
  // usleep(10000);

  if (logger->fileTime != secondsToHour(ts->tv_sec)) {
    if (getFileBuffer(logger, ts) == 0) { // get new file
      return 0;
    }

    // fprintf(stdout, "New file opened 1\n");
  }

  while (millisFromHour(ts) >=
         ntohl(*(uint32_t *)&logger->fileBuffer[logger->fileIndex])) {
    logger->fileIndex += logger->dataSize + sizeof(uint32_t);
    if (logger->fileIndex >= logger->fileSize) {
      if (nextHour(ts) == false) {
        return 0;
      }
      if (getFileBuffer(logger, ts) == 0) { // get new file
        return 0;
      }

      // fprintf(stdout, "New file opened 2\n");
    }
  }

  uint32_t recordMillisFromHour =
      ntohl(*(uint32_t *)&logger->fileBuffer[logger->fileIndex]);
  time_t recordSecondsToHour = secondsToHour(ts->tv_sec);

  ts->tv_sec = recordSecondsToHour + ((long int)recordMillisFromHour / 1000LL);
  ts->tv_nsec = ((long int)recordMillisFromHour % 1000LL) * 1000000LL;

  memcpy(data, &logger->fileBuffer[logger->fileIndex] + sizeof(uint32_t),
         logger->dataSize); // copy the new log data
  return logger->dataSize;
}

// if (logger->fileIndex + logger->dataSize + sizeof(uint32_t) >=
//     logger->fileSize) {
//   logger->fileTime = 0; // if end of log, flag to read next log file
// }

// else {
// uint32_t requestedMillis = millisFromHour(tv);
// uint32_t recordMillis =
//     ntohl(*(uint32_t *)&logger->fileBuffer[logger->fileIndex]);
// }

// uint32_t requestedMillis = millisFromHour(tv);
// uint32_t recordMillis =
//     ntohl(*(uint32_t *)&logger->fileBuffer[logger->fileIndex]);
//
// if (logger->fileIndex != 0) {
//   // same file
//   if (requestedMillis == recordMillis) {
//     // same record
//     logger->fileIndex += logger->dataSize + sizeof(uint32_t);
//   }
// }
//
// uint32_t millisRequested = millisFromHour(tv);
//
// uint32_t recordMillis = ntohl(*(uint32_t
// *)&logger->fileBuffer[logger->fileIndex]));
//
// while (millisRequested >
//        ntohl(*(uint32_t *)&logger->fileBuffer[logger->fileIndex])) {
//   logger->fileIndex += logger->dataSize + sizeof(uint32_t);
//   if (logger->fileIndex >= logger->fileSize) {
//     millisRequested = 0;
//     do {
//       if (nextHour(tv) == false) {
//         return 0;
//       }
//     } while (getBuffer(logger, tv) == 0);
//   }
// }

// **************************************************************** //
// // maximum log record string
// #define log_kMaxStrLen (1024)
//
// // maximum log file buffer size
// #define kFileBufferSize (1048576)
//
// #define kMaxQueryCount (16)
//
// #include "msg_solar.h"
// msg_name_t msgNames[] = MSG_NAMES;
// #define msgCount (sizeof(msgNames) / sizeof(msg_name_t))
//
// typedef struct {
//   uint32_t timestamp;
//   tinbus_frame_t frame;
// } log_t;
//
// static log_t fileBuffer[kFileBufferSize];
// static long fileBufferIndex = 0;
// static time_t fileBufferTime = 0;
//
// #define kDirStrLen (log_kMaxStrLen / 2)
// #define kBaseStrLen (log_kMaxStrLen / 4)
//
// static char basePath[kBaseStrLen] = {0};
//
// static bool isSameMinute(time_t t1, time_t t2) {
//   struct tm tm1 = *gmtime(&t1);
//   struct tm tm2 = *gmtime(&t2);
//   if (tm1.tm_min == tm2.tm_min)
//     return true;
//   return false;
// }
//
// bool log_writeBuffer(time_t tv_secs) {
//   if (fileBufferIndex == 0)
//     return false;
//   struct tm tm = *gmtime(&tv_secs);
//   // path
//   char directory[kDirStrLen];
//   sprintf(directory, "%s%4.4lu/%2.2u/%2.2u", basePath, 1900L + tm.tm_year,
//           tm.tm_mon + 1, tm.tm_mday);
//   build(directory);
//
//   // file name
//   char destination[log_kMaxStrLen];
//   sprintf(destination, "%s/%2.2u.dat", directory, tm.tm_hour);
//
//   // write / append buffer to file
//   FILE *fd = fopen(destination, "a");
//   if (fd != NULL) {
//     fwrite(fileBuffer, sizeof(log_t), fileBufferIndex, fd);
//     fclose(fd);
//     printf("Appended log to file %s\n", destination);
//     fileBufferIndex = 0;
//     return true;
//   }
//   return false;
// }
//
// bool log_initialise(const char *path) {
//   fileBufferIndex = 0;
//   fileBufferTime = 0;
//   int length = strnlen(path, log_kMaxStrLen);
//   if (length >= log_kMaxStrLen)
//     return false;
//   strncpy(basePath, path, kBaseStrLen - 1);
//   if (length) {
//     if (basePath[length - 1] != '/')
//       strncat(basePath, "/", kBaseStrLen - 1);
//   }
//   return true;
// }
//
// static inline void appendFileBuffer(log_t *log) {
//   memcpy(&fileBuffer[fileBufferIndex], log, sizeof(log_t));
//   if (++fileBufferIndex >= kFileBufferSize)
//     fileBufferIndex--;
// }
//
// bool log_commit(tinbus_frame_t* frame) {
//   // sanity check
//   if (frame == NULL)
//     return false;
//
//   // get millisecond timestamp for this log commit
//   struct timeval tv;
//   gettimeofday(&tv, NULL);
//   time_t secondsNow = tv.tv_sec;
//   struct tm tm = *gmtime(&secondsNow);
//   uint32_t millisNow = (uint32_t)tv.tv_usec / 1000L +
//                        (uint32_t)(tm.tm_min * 60 + tm.tm_sec) * 1000LL;
//
//   if (fileBufferTime == 0)
//     fileBufferTime = secondsNow;
//
//   if (!isSameMinute(fileBufferTime, secondsNow)) {
//     // save buffer to file every minute
//     log_writeBuffer(fileBufferTime);
//     fileBufferTime = secondsNow;
//   }
//
//   // add timestamp into frame and append to log
//   log_t log;
//   log.timestamp = htonl(millisNow);
//   memcpy(&log.frame, frame, tinbus_kFrameSize);
//   appendFileBuffer(&log);
//   return true;
// }
//
// #define kMaxSourceCount (256)
//
// bool log_readSourceList(time_t startSecond, time_t endSecond) {
//   long long startMillis = (long long)startSecond * 1000LL;
//   long long endMillis = (long long)endSecond * 1000LL;
//   return true;
// }
// //
// //   union bcd_u sourceList[kMaxSourceCount];
// //   int sourceCount = 0;
// //
// //   // const char *src = source;
// //   // int queryCount = 0;
// //   // while (queryCount < kMaxQueryCount) {
// //   //   const char *ptr = bcd_parseSource(recordSource[queryCount].bcd,
// src);
// //   //   if (ptr == src)
// //   //     break;
// //   //   queryCount++;
// //   //   if ((*ptr == '\0') || (*ptr != ','))
// //   //     break;
// //   //   src = ptr + 1;
// //   // }
// //   // if (queryCount == 0)
// //   //   return false;
// //
// //   int outputCount = 0;
// //   time_t fileSeconds = startSecond;
// //   while (fileSeconds <= endSecond) {
// //     struct tm tm = *gmtime(&fileSeconds);
// //     char fileName[log_kMaxStrLen];
// //     sprintf(fileName, "%s%4.4u/%2.2u/%2.2u/%2.2u.dat", basePath,
// //             1900L + tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour);
// //     tm.tm_min = 0;
// //     tm.tm_sec = 0;
// //     fileSeconds = timegm(&tm);
// //     long long fileMillis = (long long)fileSeconds * 1000LL;
// //
// //     FILE *fd = fopen(fileName, "r");
// //     if (fd != NULL) {
// //       int count = fread(fileBuffer, sizeof(log_t), kFileBufferSize, fd);
// //       fclose(fd);
// //       int index = 0;
// //       union bcd_u recordValue[kMaxQueryCount];
// //       uint32_t dataAvailable = 0;
// //       long long recordMillis = 0;
// //
// //       while (index < count) {
// //         // add to source list
// //         int listIndex = 0;
// //         while (true){
// //           if(fileBuffer[index].source.raw == sourceList[listIndex].raw){
// //             break;
// //           }
// //           if(listIndex++ >= sourceCount){
// //             sourceList[sourceCount++].raw = fileBuffer[index].source.raw;
// //             break;
// //           }
// //         }
// //         index++;
// //       }
// //     }
// //     fileSeconds += 3600L; // advance to next hourly file
// //   }
// //
// //   if(sourceCount){
// //     int listIndex = 0;
// //     char source[log_kMaxStrLen];
// //     bcd_printSource(source, sourceList[listIndex++].bcd);
// //     fprintf(stdout, "{\"sources\":[\"%s\"", source);
// //     while (listIndex < sourceCount){
// //       bcd_printSource(source, sourceList[listIndex++].bcd);
// //       fprintf(stdout, ",\"%s\"", source);
// //     }
// //     fprintf(stdout, "]}");
// //   } else {
// //     fprintf(stdout, "{\"sources\":[]}");
// //   }
// //
// //   return true;
// // }
// //
//
// msg_name_t* msgFind(const char* name){
//   int i = msgCount;
//   while (i) {
//     i--;
//     if(strcmp(msgNames[i], name) == 0){
//       return &msgNames[i];
//     }
//   }
//   return NULL;
// }
//
//
// bool log_read(const char *source, time_t startSecond, time_t endSecond) {
//   long long startMillis = (long long)startSecond * 1000LL;
//   long long endMillis = (long long)endSecond * 1000LL;
//
//   msg_name_t* msgSource[kMaxQueryCount];
//
//   char sourceName[log_kMaxStrLen];
//   int sourceLength = 0;
//   int queryCount = 0;
//
//
//   char* ptrStart = source;
//
//   char* ptrEnd = source;
//
//   while(*ptrEnd){
//
//     if((*ptrEnd == '\0') || (*ptrEnd == ',')){
//
//       msgSource[queryCount] = msgFind(ptrStart);
//       if(msgSource[queryCount] != NULL){
//         queryCount++;
//       }
//     }
//
//     if(*ptrStart == ','){
//
//     }
//
//
//   }
//
//
//
//
//   while (queryCount < kMaxQueryCount) {
//     const char* ptr = src;
//     while(*ptr++){
//       if(*ptr == ','){
//         *ptr = '\0';
//         msgSource[queryCount] = msgFind(src);
//         if(msgSource[queryCount] != NULL){
//           queryCount++;
//         }
//         src = ptr + 1;
//       } else if(*ptr == '\0'){
//         msgSource[queryCount] = msgFind(src);
//         if(msgSource[queryCount] != NULL){
//           queryCount++;
//         }
//         src = ptr;
//       }
//     }
//   }
//
//       if (queryCount == 0)
//         return false;
//
//
//       const char *ptr = bcd_parseSource(recordSource[queryCount].bcd, src);
//       if (ptr == src)
//         break;
//       queryCount++;
//       if ((*ptr == '\0') || (*ptr != ','))
//         break;
//       src = ptr + 1;
//     }
//
//     if (queryCount == 0)
//       return false;
//
//   return true;
// }
//
// //   union bcd_u recordSource[kMaxQueryCount];
// //   const char *src = source;
// //   int queryCount = 0;
// //   while (queryCount < kMaxQueryCount) {
// //     const char *ptr = bcd_parseSource(recordSource[queryCount].bcd, src);
// //     if (ptr == src)
// //       break;
// //     queryCount++;
// //     if ((*ptr == '\0') || (*ptr != ','))
// //       break;
// //     src = ptr + 1;
// //   }
// //   if (queryCount == 0)
// //     return false;
// //
// //   int outputCount = 0;
// //   time_t fileSeconds = startSecond;
// //   while (fileSeconds <= endSecond) {
// //     struct tm tm = *gmtime(&fileSeconds);
// //     char fileName[log_kMaxStrLen];
// //     sprintf(fileName, "%s%4.4u/%2.2u/%2.2u/%2.2u.dat", basePath,
// //             1900L + tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour);
// //     tm.tm_min = 0;
// //     tm.tm_sec = 0;
// //     fileSeconds = timegm(&tm);
// //     long long fileMillis = (long long)fileSeconds * 1000LL;
// //
// //     FILE *fd = fopen(fileName, "r");
// //     if (fd != NULL) {
// //       // printf("Opened %s\n", fileName);
// //       int count = fread(fileBuffer, sizeof(log_t), kFileBufferSize, fd);
// //       fclose(fd);
// //       int index = 0;
// //       union bcd_u recordValue[kMaxQueryCount];
// //       uint32_t dataAvailable = 0;
// //       long long recordMillis = 0;
// //
// //       while (index < count) {
// //         if (fileBuffer[index].source.raw == kTimeSource.raw){
// //           if (dataAvailable != 0) {
// //             char outputString[log_kMaxStrLen] = {0};
// //             int queryIndex = 0;
// //             while (queryIndex < queryCount) {
// //               if (dataAvailable & (1 << queryIndex)) {
// //                 char valueString[8 + 1];
// //                 bcd_printNumber(valueString, recordValue[queryIndex].bcd);
// //                 strcat(outputString, ",");
// //                 strcat(outputString, valueString);
// //               } else {
// //                 strcat(outputString, ",null");
// //               }
// //               queryIndex++;
// //             }
// //             dataAvailable = 0;
// //             if (outputCount == 0) {
// //               outputCount++;
// //               fprintf(stdout,
// "{\"sources\":\"%s\",\"timeseries\":[[%lld%s]",
// //                       source, recordMillis, outputString);
// //             } else {
// //               fprintf(stdout, ",[%lld%s]", recordMillis, outputString);
// //             }
// //           }
// //           // update time stamp
// //           uint32_t timeStamp = ntohl(fileBuffer[index].value.raw);
// //           recordMillis = (long long)(timeStamp) + fileMillis;
// //         }
// //
// //         // harvest requested data entries
// //         if ((recordMillis >= startMillis) && (recordMillis <= endMillis))
// {
// //           int queryIndex = 0;
// //           while (queryIndex < queryCount) {
// //             if (fileBuffer[index].source.raw ==
// recordSource[queryIndex].raw) {
// //               recordValue[queryIndex].raw = fileBuffer[index].value.raw;
// //               dataAvailable |= (1 << queryIndex);
// //             }
// //             queryIndex++;
// //           }
// //         }
// //         index++;
// //       }
// //     }
// //     fileSeconds += 3600L; // advance to next hourly file
// //   }
// //   if (outputCount)
// //     fprintf(stdout, "]}");
// //   else
// //     fprintf(stdout, "{\"source\":\"%s\",\"timeseries\":[]}", source);
// //   return true;
// // }
//
// // char outputString[log_kMaxStrLen] = {0};
// //     containsData = true;
// //     char valueString[8 + 1];
// //     bcd_printNumber(valueString, fileBuffer[index].value.bcd);
// //     strcat(outputString, ",");
// //     strcat(outputString, valueString);
// //     // fprintf(stdout, ",%s", valueString);
// //   } else {
// //     strcat(outputString, ",null");
// //     // fprintf(stdout, ",null");
// //   }
// // }
// // if (containsData){
// //
// //   if (outputCount == 0) {
// //     outputCount++;
// //     // fprintf(stdout, "{\"source\":\"%s\",\"timeseries\":[",
// //     source); fprintf(stdout,
// //     "{\"sources\":\"%s\",\"timeseries\":[[%lld%s]", source,
// //     recordMillis, outputString);
// //   }
// //   else
// //   //   fprintf(stdout, ",");
// //     fprintf(stdout, ",[%lld%s]", recordMillis, outputString);
// //   // fprintf(stdout, "%s]", outputString);
// // }
//
// // }
//
// bool log_terminate(void) {
//   if (fileBufferIndex != 0) {
//     log_writeBuffer(fileBufferTime);
//   }
//   return true;
// };
//
// // #define kMaxSourcesQuery (16)
// // char timeString[8];
// // bcd_printNumber(timeString, timeStamp.bcd);
//
// // // parse sources into array of bcd_t
// // union bcd_u recordSource[kMaxSourcesQuery];
// // const char *ptr = sources;
// // int index = 0;
// // while (*ptr) {
// //   if (index >= kMaxSourcesQuery)
// //     return false;
// //   const char *p = bcd_parseSource(recordSource[index].bcd, ptr);
// //   if (*p == ',')
// //     ptr = p + 1;
// //   else if (p == ptr)
// //     return false;
// //   else
// //     ptr = p;
// //   index++;
// // }
// // if (index == 0)
// //   return false;
//
// // union bcd_u timeRecord;
// // bcd_parseSource(timeRecord.bcd, "time");
//
// // char source[log_kMaxStrLen];
// // char value[log_kMaxStrLen];
// //
// // bcd_printSource(source, log.source);
// // bcd_printNumber(value, log.value);
// //
// // printf("Entry : \t%s\t%s\n", source, value);
//
// // uint32_t millisNow = (tv.tv_usec / 1000L); // + (tm.tm_sec * 1000L);
//
// // static uint32_t parseID(char **idPointer) {
// //   if (isalnum(**idPointer)) {
// //     uint32_t id = (uint16_t)(**idPointer) << 8;
// //     (*idPointer)++;
// //     if (isalnum(**idPointer)) {
// //       id += **idPointer;
// //       (*idPointer)++;
// //       if (**idPointer == '=') {
// //         (*idPointer)++;
// //         return id;
// //       }
// //     }
// //   }
// //   *idPointer = NULL;
// //   return 0;
// // }
// //
// // static float parseValue(char **valuePointer) {
// //   char *ptr = *valuePointer;
// //   if ((isdigit(*ptr)) || (*ptr == '-')) {
// //     ptr++;
// //     while (1) {
// //       if ((isdigit(*ptr)) || (*ptr == '.')) {
// //         ptr++;
// //       } else if ((*ptr == ',') || (*ptr == '\0') || (*ptr == '\n') ||
// //                  (*ptr == '\r')) {
// //         *ptr = '\0';
// //         ptr++;
// //         float value = atof(*valuePointer);
// //         *valuePointer = ptr;
// //         return value;
// //       } else
// //         break;
// //     }
// //   }
// //   *valuePointer = NULL;
// //   return 0;
// // }
//
// // typedef struct {
// //   unsigned char[4] value;
// //   unsigned char[4] source;
// //   // unsigned char[4] millis;
// // } log_t;
//
// //! Convert 32-bit float from host to network byte order
// // static inline float htonf(float f) {
// //   uint32_t val = hton32(*(const uint32_t *)(&f));
// //   return *((float *)(&val));
// // }
// //
// // float htonf(float val) {
// //   uint32_t rep;
// //   memcpy(&rep, &val, sizeof rep);
// //   rep = htonl(rep);
// //   memcpy(&val, &rep, sizeof rep);
// //   return val;
// // }
// //
// // #define ntohf(x) htonf((x))
//
// // typedef struct {
// //   float value;
// //   uint32_t micros;
// //   uint32_t source;
// // } logRecord_t;
//
// // typedef struct {
// //   float value;
// //   unsigned int source : 16;
// //   unsigned int millis : 16;
// // } logRecord_t;
