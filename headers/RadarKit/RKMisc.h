//
//  RKMisc.h
//  RadarKit
//
//  Technically these functions are not part of the RadarKit framework. They can be
//  used independently outside of RadarKit.
//
//  Created by Boonleng Cheong on 11/4/15.
//
//

#ifndef __RadarKit_Misc__
#define __RadarKit_Misc__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <dirent.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <resolv.h>

#ifdef __MACH__
#include <mach/mach.h>
#include <mach/clock.h>
#endif

#define RKErrnoString(A)  \
(errno == EAGAIN       ? "EAGAIN"       : \
(errno == EBADF        ? "EBADF"        : \
(errno == EFAULT       ? "EFAULT"       : \
(errno == EINTR        ? "EINTR"        : \
(errno == EINVAL       ? "EINVAL"       : \
(errno == ECONNREFUSED ? "ECONNREFUSED" : \
(errno == EHOSTDOWN    ? "EHOSTDOWN"    : \
(errno == EHOSTUNREACH ? "EHOSTUNREACH" : \
(errno == EACCES       ? "EACCES"       : \
(errno == EIO          ? "EIO"          : "OTHERS"))))))))))

#define POSIX_MEMALIGN_CHECK(x)        if (x) { RKLog("Could not allocate memory.\n"); exit(EXIT_FAILURE); }

#define MAKE_FUNCTION_NAME(x) \
RKName x; \
sprintf(x, "%s()", __FUNCTION__);

#define SHOW_FUNCTION_NAME \
int _fn_len = strlen(__FUNCTION__); \
char _fn_str[RKNameLength]; \
memset(_fn_str, '=', _fn_len); \
sprintf(_fn_str + _fn_len, "\n%s\n", __FUNCTION__); \
memset(_fn_str + 2 * _fn_len + 2, '=', _fn_len); \
_fn_str[3 * _fn_len + 2] = '\0'; \
printf("%s\n", _fn_str);

#define SHOW_SIZE(x) \
printf(RKDeepPinkColor "sizeof" RKNoColor "(" RKSkyBlueColor #x RKNoColor ") = " RKLimeColor "%s" RKNoColor "\n", \
RKUIntegerToCommaStyleString(sizeof(x)));

#define SHOW_SIZE_SIMD(x) \
printf(RKDeepPinkColor "sizeof" RKNoColor "(" RKSkyBlueColor #x RKNoColor ") = " RKLimeColor "%s" RKNoColor \
"   " RKOrangeColor "SIMDAlign" RKNoColor " = " RKPurpleColor "%s" RKNoColor "\n", \
RKUIntegerToCommaStyleString(sizeof(x)), \
sizeof(x) % RKSIMDAlignSize == 0 ? "True" : "False");

#if defined(__APPLE__)

#define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"

typedef struct cpu_set {
    uint32_t    count;
} cpu_set_t;

static inline void
CPU_ZERO(cpu_set_t *cs) { cs->count = 0; }

static inline void
CPU_SET(int num, cpu_set_t *cs) { cs->count |= (1 << num); }

static inline int
CPU_ISSET(int num, cpu_set_t *cs) { return (cs->count & (1 << num)); }

int pthread_setaffinity_np(pthread_t thread, size_t cpu_size, cpu_set_t *cpu_set);

#endif

#define RKMiscStringLength  1024

enum RKJSONObjectType {
    RKJSONObjectTypeUnknown,
    RKJSONObjectTypePlain,
    RKJSONObjectTypeString,
    RKJSONObjectTypeArray,
    RKJSONObjectTypeObject
};

char *RKGetColor(void);
char *RKGetColorOfIndex(const int);
char *RKGetBackgroundColor(void);
char *RKGetBackgroundColorOfIndex(const int);
char *RKGetBackgroundColorOfCubeIndex(const int);

char *RKExtractJSON(char *ks, uint8_t *type, char *key, char *value);
char *RKGetValueOfKey(const char *string, const char *key);
void RKReplaceAllValuesOfKey(char *string, const char *key, int value);
void RKReplaceEnumOfKey(char *string, const char *key, int value);
void RKReviseLogicalValues(char *);

char *RKUIntegerToCommaStyleString(const unsigned long long);
char *RKIntegerToCommaStyleString(const long long);
char *RKIntegerToHexStyleString(const long long);
char *RKFloatToCommaStyleString(const double);

char *RKNow(void);
double RKTimevalDiff(const struct timeval minuend, const struct timeval subtrahend);
double RKTimespecDiff(const struct timespec minuend, const struct timespec subtrahend);
void RKUTCTime(struct timespec *);

bool RKFilenameExists(const char *);
void RKPreparePath(const char *);
long RKCountFilesInPath(const char *);
char *RKFolderOfFilename(const char *);
char *RKFileExtension(const char *);
char *RKLastPartOfPath(const char *);
char *RKLastTwoPartsOfPath(const char *);
char *RKLastNPartsOfPath(const char *, const int n);
char *RKPathStringByExpandingTilde(const char *);
void RKReplaceFileExtension(char *filename, const char *pattern, const char *replacement);

char *RKSignalString(const int);

int RKStripTail(char *);
int RKIndentCopy(char *dst, char *src, const int width);
int RKStringCenterized(char *dst, const char *src, const int width);
char *RKNextNoneWhite(const char *);
char *RKLastLine(const char *);

float RKMinDiff(const float minuend, const float subtrahend);
float RKUMinDiff(const float minuend, const float subtrahend);
bool RKAngularCrossOver(const float a1, const float a2, const float crossover);

long RKGetCPUIndex(void);
long RKGetMemoryUsage(void);

char *RKCountryFromPosition(const double latitude, const double longitude);

char *RKStripEscapeSequence(char *);
int RKMergeColumns(char *text, char *left, char *right);

#endif /* rk_misc_h */
