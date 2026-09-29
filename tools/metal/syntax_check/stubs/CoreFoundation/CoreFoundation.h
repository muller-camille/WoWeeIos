#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef double CFTimeInterval;
typedef const void* CFTypeRef;
typedef long CFIndex;
typedef const struct __CFAllocator* CFAllocatorRef;
typedef const struct __CFArray* CFArrayRef;
typedef const struct __CFString* CFStringRef;
typedef struct { CFIndex version; void* retain; void* release; void* copyDescription; void* equal; } CFArrayCallBacks;
#ifdef __cplusplus
extern "C" {
#endif
extern const CFArrayCallBacks kCFTypeArrayCallBacks;
extern const CFAllocatorRef kCFAllocatorDefault;
CFArrayRef CFArrayCreate(CFAllocatorRef, const void**, CFIndex, const CFArrayCallBacks*);
CFTypeRef CFRetain(CFTypeRef);
void CFRelease(CFTypeRef);
#ifdef __cplusplus
}
#endif
