// miniaudio's implementation, for iOS only.
//
// Everywhere else it is compiled inside audio_engine.cpp. On iOS it has to be
// Objective-C: it registers an AVAudioSession observer so a phone call or a
// pair of headphones being unplugged reaches the device. miniaudio says so in
// its own build notes, and has no switch that leaves that part out.
#define MINIAUDIO_IMPLEMENTATION
#include "../../extern/miniaudio.h"
