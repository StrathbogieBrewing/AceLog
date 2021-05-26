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

int log_read(struct timeval *tv, void* data){
  static unsigned char fileBuffer[kLogBufferSize];
  static int fileBufferIndex = 0;
  struct timeval lastTimeVal = {0};

  if(lastTimeVal.tv_sec == 0){
    lastTimeVal = *tv;
  }

  struct tm tm = *gmtime(&secondsNow);
  uint32_t millis =
      (uint32_t)tv.tv_usec / 1000L +
      (uint32_t)(tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec) * 1000LL;

    while(1);
      char directory[kDirStrLen];
      sprintf(directory, "%s%4.4lu/%2.2u/%2.2u", basePath, 1900L + tm.tm_year,
              tm.tm_mon + 1, tm.tm_mday);
      build(directory);

      // file name
      char destination[kMaxStrLen];
      sprintf(destination, "%s/%2.2u.dat", directory, tm.tm_hour);

      // write / append buffer to file
      FILE *fd = fopen(destination, "r");
      if (fd != NULL) {
        fread(fileBuffer, 1, kLogBufferSize, fd);
        fclose(fd);
        fileBufferIndex = 0;
        recordMillis = ntohl(*(uint32_t *)fileBuffer);
      }
  // cache source file and listIndex



  // read data and update time record

}


//
// // maximum log record string
// #define kMaxStrLen (1024)
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
// #define kDirStrLen (kMaxStrLen / 2)
// #define kBaseStrLen (kMaxStrLen / 4)
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
//   char destination[kMaxStrLen];
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
//   int length = strnlen(path, kMaxStrLen);
//   if (length >= kMaxStrLen)
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
// //   //   const char *ptr = bcd_parseSource(recordSource[queryCount].bcd, src);
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
// //     char fileName[kMaxStrLen];
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
// //     char source[kMaxStrLen];
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
//   char sourceName[kMaxStrLen];
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
// //     char fileName[kMaxStrLen];
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
// //             char outputString[kMaxStrLen] = {0};
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
// //               fprintf(stdout, "{\"sources\":\"%s\",\"timeseries\":[[%lld%s]",
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
// //         if ((recordMillis >= startMillis) && (recordMillis <= endMillis)) {
// //           int queryIndex = 0;
// //           while (queryIndex < queryCount) {
// //             if (fileBuffer[index].source.raw == recordSource[queryIndex].raw) {
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
// // char outputString[kMaxStrLen] = {0};
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
// // char source[kMaxStrLen];
// // char value[kMaxStrLen];
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
