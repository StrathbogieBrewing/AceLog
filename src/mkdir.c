#include <sys/cdefs.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

int build(char *path) {
  struct stat sb;
  mode_t numask, oumask;
  int first, last, retval;
  char *p;

  p = path;
  oumask = 0;
  retval = 0;
  if (p[0] == '/') /* Skip leading '/'. */
    ++p;
  for (first = 1, last = 0; !last; ++p) {
    if (p[0] == '\0')
      last = 1;
    else if (p[0] != '/')
      continue;
    *p = '\0';
    if (!last && p[1] == '\0')
      last = 1;
    if (first) {
      oumask = umask(0);
      numask = oumask & ~(S_IWUSR | S_IXUSR);
      (void)umask(numask);
      first = 0;
    }
    if (last)
      (void)umask(oumask);
    if (mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
      if (errno == EEXIST || errno == EISDIR) {
        if (stat(path, &sb) < 0) {
          warn("%s", path);
          retval = 1;
          break;
        } else if (!S_ISDIR(sb.st_mode)) {
          if (last)
            errno = EEXIST;
          else
            errno = ENOTDIR;
          warn("%s", path);
          retval = 1;
          break;
        }
      } else {
        warn("%s", path);
        retval = 1;
        break;
      }
    }
    if (!last)
      *p = '/';
  }
  if (!first && !last)
    (void)umask(oumask);
  return (retval);
}
