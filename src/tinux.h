#ifndef TINUX_H
#define TINUX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tinframe.h"

enum{
tinux_kOK = 0,
tinux_kReadNoData,
tinux_kReadCRCError,
tinux_kReadOverunError,
};

int tinux_open(char *port);
int tinux_read(tinframe_t *rxFrame);
void tinux_close(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // TINUX_H
