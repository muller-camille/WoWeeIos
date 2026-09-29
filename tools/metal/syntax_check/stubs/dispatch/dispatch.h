#pragma once
#include <stdint.h>
typedef struct dispatch_queue_s* dispatch_queue_t;
typedef struct dispatch_data_s* dispatch_data_t;
typedef struct dispatch_semaphore_s* dispatch_semaphore_t;
typedef uint64_t dispatch_time_t;
#define DISPATCH_TIME_NOW (0ull)
#define DISPATCH_TIME_FOREVER (~0ull)
#define NSEC_PER_MSEC 1000000ull
#define NSEC_PER_SEC 1000000000ull
#ifdef __cplusplus
extern "C" {
#endif
dispatch_semaphore_t dispatch_semaphore_create(long);
long dispatch_semaphore_wait(dispatch_semaphore_t, dispatch_time_t);
long dispatch_semaphore_signal(dispatch_semaphore_t);
dispatch_time_t dispatch_time(dispatch_time_t, int64_t);
void dispatch_release(void*);
#ifdef __cplusplus
}
#endif
