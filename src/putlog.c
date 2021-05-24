#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <time.h>

#include "log.h"
#include "tinux.h"

#define kProgramName (0)
#define kSerialDevice (1)
#define kLogPath (2)

#define kBufferSize (256)

static volatile int keepRunning = 1;
void intHandler(int dummy) { keepRunning = 0; }

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stdout, "Usage: %s <serial device> <log path>\n",
            argv[kProgramName]);
    return EXIT_FAILURE;
  }
  if (tinux_open(argv[kSerialDevice]) == -1) {
    return EXIT_FAILURE;
  }
  signal(SIGINT, intHandler);
  signal(SIGTERM, intHandler);
  log_begin(argv[kLogPath], sizeof(tinframe_t));
  unsigned char sequence = 0;
  while (keepRunning) {
    tinframe_t rxFrame;
    int result = tinux_read(&rxFrame);
    if (result == tinux_kOK) {
      log_commit(&rxFrame);

      char str[kBufferSize] = {0};
      sprintf(str + strlen(str), "RX: ");
      int found = 0;
      int index = 0;
      while (index < sizeof(tinframe_t)) {
        sprintf(str + strlen(str), "0x%2.2X  ",
                ((unsigned char *)(&rxFrame))[index]);
        index++;
      }
      sprintf(str + strlen(str), "\n");
      fprintf(stdout, "%s", str);
    }
  }
  // teardown ports
  tinux_close();
  log_end();
  fprintf(stdout, "\nExit %s\n", argv[0]);

  return EXIT_SUCCESS;
}

// char str[kBufferSize] = {0};
// sprintf(str + strlen(str), "msg : 0x%2.2X\tseq : %u  \t",
//         rxFrame.data[MSG_ID], rxFrame.data[MSG_SEQ]);
// int found = 0;
// int index = 0;
// while (index < msgCount) {
//   int value;
//   int format = msg_unpack(&rxFrame, &msgNames[index].pack, &value);
//   if (format != MSG_NULL) {
//     if (found)
//       sprintf(str + strlen(str), ", \t");
//     found++;
//     char valueBuffer[kBufferSize] = {0};
//     msg_format(&rxFrame, &msgNames[index].pack, valueBuffer);
//     sprintf(str + strlen(str), "%s=%s", msgNames[index].name, valueBuffer);
//   }
//   index++;
// }
// sprintf(str + strlen(str), "\n");
// fprintf(stdout, "%s", str);
// if(found == 0){
//   messageErrorCount++;
//   logError();
// }
