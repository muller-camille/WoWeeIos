#pragma once
#include <objc/objc.h>
#ifdef __cplusplus
extern "C" {
#endif
SEL sel_registerName(const char*);
id objc_lookUpClass(const char*);
Protocol* objc_getProtocol(const char*);
BOOL class_respondsToSelector(Class, SEL);
BOOL class_conformsToProtocol(Class, Protocol*);
#ifdef __cplusplus
}
#endif
