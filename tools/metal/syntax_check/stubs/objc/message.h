#pragma once
#include <objc/objc.h>
#ifdef __cplusplus
extern "C" {
#endif
void objc_msgSend(void);
void objc_msgSend_stret(void);
void objc_msgSend_fpret(void);
#ifdef __cplusplus
}
#endif
