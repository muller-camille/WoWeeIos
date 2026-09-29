#pragma once
typedef double CGFloat;
struct CGSize { CGFloat width, height; };
typedef struct CGSize CGSize;
struct CGPoint { CGFloat x, y; };
typedef struct CGPoint CGPoint;
struct CGRect { CGPoint origin; CGSize size; };
typedef struct CGRect CGRect;
static inline CGSize CGSizeMake(CGFloat w, CGFloat h) { CGSize s = {w, h}; return s; }
