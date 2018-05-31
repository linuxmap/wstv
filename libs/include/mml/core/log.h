#ifndef _MML_LOG_H_
#define _MML_LOG_H_

#include <stdio.h>
#include <time.h>

#ifdef WIN32
;;
#else
#include <sys/time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define MMLLOGT(fmt, ...) \
    do { \
        time_t t0; \
        time(&t0); \
        printf("[TIME]: [%ld] " fmt, \
                t0, \
                ##__VA_ARGS__); \
    } while (0);
#else
#define MMLLOGT(fmt, ...) \
    do { \
        struct timeval tv0; \
        gettimeofday(&tv0, 0); \
        printf("[TIME]: [%ld.%06ld] " fmt, \
                tv0.tv_sec, tv0.tv_usec, \
                ##__VA_ARGS__); \
    } while (0);
#endif

#define MMLLOGV(fmt, ...) \
    do { \
        printf("[VERBOSE]: [%s] [%d] " fmt, \
                __FUNCTION__, __LINE__, \
                ##__VA_ARGS__); \
    } while (0);

#define MMLLOGD(fmt, ...) \
    do { \
        printf("[DEBUG]: [%s] [%d] " fmt, \
                __FUNCTION__, __LINE__, \
                ##__VA_ARGS__); \
    } while (0);

#define MMLLOGI(fmt, ...) \
    do { \
        printf("[INFO]: [%s] [%d] " fmt, \
                __FUNCTION__, __LINE__, \
                ##__VA_ARGS__); \
    } while (0);

#define MMLLOGW(fmt, ...) \
    do { \
        printf("[WARN]: [%s] [%d] " fmt, \
                __FUNCTION__, __LINE__, \
                ##__VA_ARGS__); \
    } while (0);

#define MMLLOGE(fmt, ...) \
    do { \
        printf("[ERROR]: [%s] [%d] " fmt, \
                __FUNCTION__, __LINE__, \
                ##__VA_ARGS__); \
    } while (0);

#define MMLLOGN(fmt, ...) \
    do { \
        printf(fmt, ##__VA_ARGS__); \
    } while (0);

#ifdef __cplusplus
}
#endif

#endif /* ifndef _MML_LOG_H_ */
