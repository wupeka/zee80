#if defined(__MINGW32__)
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <sys/stat.h>

char path[4096];
const char *getSnapshotFileLocation() {
  char *p = getenv("USERPROFILE");
  strcpy(path, p);
  strcat(path, "\\AppData\\Local\\Puszka Pandory\\pandora.sav");
  for (char *p = &path[1]; *p; p++) {
    if (*p == '\\') {
      *p = '\0';
      if (mkdir(path) != 0) {
        if (errno != EEXIST) {
          return "pandora.sav";
        }
      }
      *p = '\\';
    }
  }
  return path;
}
#elif defined(__APPLE__)
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

char path[4096];
const char *getSnapshotFileLocation() {
  char *p = getenv("HOME");
  strcpy(path, p);
  strcat(path, "/Library/Application Support/Puszka Pandory/pandora.sav");

  for (char *p = &path[1]; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(path, S_IRWXU) != 0) {
        if (errno != EEXIST) {
          return "pandora.sav";
        }
      }
      *p = '/';
    }
  }
  return path;
}

#else /* linux et al. */
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

char path[4096];
const char *getSnapshotFileLocation() {
  char *xdg_data_home = getenv("XDG_DATA_HOME");
  if (xdg_data_home == nullptr) {
    xdg_data_home = getenv("HOME");
    strcpy(path, xdg_data_home);
    strcat(path, "/.local/share/PuszkaPandory/pandora.sav");
  } else {
    strcpy(path, xdg_data_home);
    strcat(path, "/PuszkaPandory/pandora.sav");
  }
  for (char *p = &path[1]; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(path, S_IRWXU) != 0) {
        if (errno != EEXIST) {
          return "pandora.sav";
        }
      }
      *p = '/';
    }
  }
  return path;
}

#endif
