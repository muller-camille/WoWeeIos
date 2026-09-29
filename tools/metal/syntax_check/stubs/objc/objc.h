#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <math.h>
typedef struct objc_class* Class;
struct objc_object { Class isa; };
typedef struct objc_object* id;
typedef struct objc_selector* SEL;
typedef id (*IMP)(id, SEL, ...);
typedef bool BOOL;
#define YES true
#define NO false
typedef struct objc_object Protocol;
