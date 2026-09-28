#include "ui/touch_controls.hpp"

#include "core/input.hpp"
#include "core/logger.hpp"
#include "rendering/camera_controller.hpp"

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace wowee {
namespace ui {

float TouchControls::stickRadius() const {
    if (cachedRadius_ > 0.0f) return cachedRadius_;
    // SDL3 dropped SDL_GetDisplayDPI for a content scale, which is the same
    // number this was deriving: dpi over Android's 160 baseline is 1x.
    float density = 2.0f;
    if (const float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        scale > 0.0f) {
        density = scale;
    }
    cachedRadius_ = kStickRadiusDp * std::max(density, 1.0f);
    return cachedRadius_;
}

bool TouchControls::held(float axis, float threshold, bool wasOn) {
    const float release = std::max(threshold - kReleaseHysteresis, 0.05f);
    return axis > (wasOn ? release : threshold);
}

TouchControls& touchControls() {
    static TouchControls instance;
    return instance;
}

void TouchControls::setInWorld(bool inWorld) {
    if (inWorld_ == inWorld) return;
    inWorld_ = inWorld;
    if (!inWorld_) reset();
}

void TouchControls::setMovementKeys(bool forward, bool back, bool left, bool right) const {
    auto& input = core::Input::getInstance();
    input.setVirtualKey(SDL_SCANCODE_W, forward);
    input.setVirtualKey(SDL_SCANCODE_S, back);
    // Q and E rather than A and D. With the right button up - which it always
    // is on a phone - this client turns the character on A and D and strafes on
    // Q and E. Turning meant the stick swung the view as well as the feet,
    // which is not what a stick pushed sideways should do.
    input.setVirtualKey(SDL_SCANCODE_Q, left);
    input.setVirtualKey(SDL_SCANCODE_E, right);
}

void TouchControls::handleEvent(const SDL_Event& event, int windowWidth, int windowHeight) {
    if (event.type != SDL_EVENT_FINGER_DOWN && event.type != SDL_EVENT_FINGER_MOTION &&
        event.type != SDL_EVENT_FINGER_UP && event.type != SDL_EVENT_FINGER_CANCELED) {
        return;
    }

    const float w = static_cast<float>(std::max(windowWidth, 1));
    const float h = static_cast<float>(std::max(windowHeight, 1));
    layoutW_ = w;
    layoutH_ = h;
    windowId_ = event.tfinger.windowID;
    const float x = event.tfinger.x * w;
    const float y = event.tfinger.y * h;
    const SDL_FingerID id = event.tfinger.fingerID;

    // Counted on every screen: SDL's mouse follows the finger that lands with
    // no other down, and that is the only way to know which one it is.
    const bool mouseFinger = event.type == SDL_EVENT_FINGER_DOWN && fingersDown_ == 0;
    if (event.type == SDL_EVENT_FINGER_DOWN) {
        ++fingersDown_;
    } else if (event.type == SDL_EVENT_FINGER_UP || event.type == SDL_EVENT_FINGER_CANCELED) {
        fingersDown_ = std::max(fingersDown_ - 1, 0);
        for (int i = 0; i < kButtonCount; ++i) {
            if (buttonFinger_[i] == id) pressButton(i, id, false);
        }
    }

    if (!inWorld_) {
        handleMenuPinch(event, x, y);
        return;
    }

    if (event.type == SDL_EVENT_FINGER_DOWN) {
        // A thumb button or the menu row takes the finger outright, ahead of
        // the stick and the interface: they are drawn on top of both.
        if (const int button = buttonAt(x, y, w, h); button >= 0) {
            if (mouseFinger) claimMouseFinger();
            pressButton(button, id, true);
            return;
        }
        // The interface is asked first, so a bag or an action button drawn over
        // the corner still gets the press. A stick that ate those would be
        // maddening, and the corner is only a default.
        //
        // Both interfaces: ImGui's windows answer through WantCaptureMouse, but
        // the game's own frames - the action bars, the chat, bags - are not
        // ImGui windows and only the probe knows them. Asking ImGui alone put
        // the stick under a finger pressing an action button, so nothing on
        // the bar could be dragged.
        const bool interfaceUnder = ImGui::GetIO().WantCaptureMouse || (probe_ && probe_(x, y, h));
        if (stickFingerId_ == kNoFinger && x < w * kStickZoneWidth &&
            y > h * kStickZoneTop && !interfaceUnder) {
            LOG_DEBUG("touch: stick claimed at ", x, ",", y);
            if (mouseFinger) claimMouseFinger();
            stickFingerId_ = id;
            stickOriginX_ = x;
            stickOriginY_ = y;
            stickX_ = stickY_ = 0.0f;
            return;
        }
        LOG_DEBUG("touch: finger down at ", x, ",", y, " of ", w, "x", h,
                  " uiWants=", ImGui::GetIO().WantCaptureMouse, " - not the stick");
        if (lookFingerId_ == kNoFinger) {
            lookFingerId_ = id;
            lookX_ = x;
            lookY_ = y;
            lookMoved_ = false;
            lookTravel_ = 0.0f;
        }
        if (pinchA_ == kNoFinger) {
            pinchA_ = id; pinchAX_ = x; pinchAY_ = y;
        } else if (pinchB_ == kNoFinger) {
            pinchB_ = id; pinchBX_ = x; pinchBY_ = y;
            lastPinchSpacing_ = std::hypot(pinchAX_ - pinchBX_, pinchAY_ - pinchBY_);
            pinching_ = true;
        }
        return;
    }

    if (event.type == SDL_EVENT_FINGER_UP || event.type == SDL_EVENT_FINGER_CANCELED) {
        if (id == stickFingerId_) {
            stickFingerId_ = kNoFinger;
            stickX_ = stickY_ = 0.0f;
            setMovementKeys(false, false, false, false);
        } else {
            if (id == lookFingerId_) {
                lookFingerId_ = kNoFinger;
                lookMoved_ = false;
            }
            if (id == pinchA_) { pinchA_ = kNoFinger; pinching_ = false; }
            if (id == pinchB_) { pinchB_ = kNoFinger; pinching_ = false; }
        }
        return;
    }

    // SDL_EVENT_FINGER_MOTION
    if (id == stickFingerId_) {
        const float radius = stickRadius();
        stickX_ = std::clamp((x - stickOriginX_) / radius, -1.0f, 1.0f);
        stickY_ = std::clamp((y - stickOriginY_) / radius, -1.0f, 1.0f);
        return;
    }
    // One finger dragging is a look. Two is a pinch, and the view should not
    // swing while the fingers close.
    if (id == lookFingerId_ && pinchB_ == kNoFinger) {
        const float dx = x - lookX_;
        const float dy = y - lookY_;
        lookX_ = x;
        lookY_ = y;
        // Distance travelled, not distance from where the finger went down: a
        // slow drag arrives in many small steps and would never clear a
        // per-event threshold.
        if (!lookMoved_) {
            lookTravel_ += std::hypot(dx, dy);
            if (lookTravel_ >= kLookSlopPixels) lookMoved_ = true;
        }
        if (lookMoved_ && camera_) camera_->applyLookDelta(dx, dy);
    }

    if (id == pinchA_) { pinchAX_ = x; pinchAY_ = y; }
    else if (id == pinchB_) { pinchBX_ = x; pinchBY_ = y; }
    else return;

    if (!pinching_ || pinchA_ == kNoFinger || pinchB_ == kNoFinger) return;
    sendPinchAsWheel(std::hypot(pinchAX_ - pinchBX_, pinchAY_ - pinchBY_));
}

void TouchControls::handleMenuPinch(const SDL_Event& event, float x, float y) {
    const SDL_FingerID id = event.tfinger.fingerID;
    if (event.type == SDL_EVENT_FINGER_DOWN) {
        if (pinchA_ == kNoFinger) {
            pinchA_ = id; pinchAX_ = x; pinchAY_ = y;
        } else if (pinchB_ == kNoFinger) {
            pinchB_ = id; pinchBX_ = x; pinchBY_ = y;
            lastPinchSpacing_ = std::hypot(pinchAX_ - pinchBX_, pinchAY_ - pinchBY_);
            pinching_ = true;
        }
        return;
    }
    if (event.type == SDL_EVENT_FINGER_UP || event.type == SDL_EVENT_FINGER_CANCELED) {
        if (id == pinchA_) { pinchA_ = kNoFinger; pinching_ = false; }
        if (id == pinchB_) { pinchB_ = kNoFinger; pinching_ = false; }
        return;
    }
    if (id == pinchA_) { pinchAX_ = x; pinchAY_ = y; }
    else if (id == pinchB_) { pinchBX_ = x; pinchBY_ = y; }
    else return;
    if (!pinching_ || pinchA_ == kNoFinger || pinchB_ == kNoFinger) return;
    sendPinchAsWheel(std::hypot(pinchAX_ - pinchBX_, pinchAY_ - pinchBY_));
}

void TouchControls::sendPinchAsWheel(float spacing) {
    const float moved = spacing - lastPinchSpacing_;
    if (std::abs(moved) < kPinchPixelsPerNotch) return;
    lastPinchSpacing_ = spacing;

    // Fingers spreading pulls the camera in, the way a wheel forward does. Sent
    // as a wheel event so it goes through the same clamping and the same
    // setting as a wheel, rather than reaching into the camera here.
    SDL_Event wheel{};
    wheel.type = SDL_EVENT_MOUSE_WHEEL;
    wheel.wheel.timestamp = SDL_GetTicks();
    // The window, as the other events made here carry: ImGui drops a wheel it
    // cannot place in one of its windows, which on the character screens -
    // where the pinch is ImGui's wheel rather than the camera's - was all of
    // them.
    wheel.wheel.windowID = windowId_;
    // One float axis in SDL3: the integer y and the precise one were the
    // same measurement at two precisions, and only the finer one survived.
    wheel.wheel.y = moved / kPinchPixelsPerNotch;
    SDL_PushEvent(&wheel);
}

void TouchControls::update() {
    if (press_ == Press::Pending && SDL_GetTicks() - pressTicks_ >= kLongPressMs) {
        press_ = Press::Holding;
    }
    if (interactStep_ > 0) {
        core::Input::getInstance().setVirtualMouseButton(SDL_BUTTON_RIGHT, interactStep_ == 2);
        --interactStep_;
    }
    if (!inWorld_ || stickFingerId_ == kNoFinger) {
        wasForward_ = wasBack_ = wasLeft_ = wasRight_ = false;
        setMovementKeys(false, false, false, false);
        return;
    }
    // Up is forward, sideways is a strafe: the stick moves the character and
    // never the view, which is the other thumb's job.
    //
    // Sideways asks for more of the stick than forward, and each direction lets
    // go later than it took hold, so that holding a heading does not need the
    // thumb kept still to the pixel.
    wasForward_ = held(-stickY_, kWalkDeadzone, wasForward_);
    wasBack_    = held( stickY_, kWalkDeadzone, wasBack_);
    wasLeft_    = held(-stickX_, kStrafeDeadzone, wasLeft_);
    wasRight_   = held( stickX_, kStrafeDeadzone, wasRight_);
    setMovementKeys(wasForward_, wasBack_, wasLeft_, wasRight_);
}

void TouchControls::draw() const {
    if (!inWorld_) return;
    if (layoutW_ > 0.0f && layoutH_ > 0.0f) {
        if (ImDrawList* fg = ImGui::GetForegroundDrawList()) {
            // ImGui draws in its own display units, which are the window's
            // points here; the layout is in the same.
            const auto buttons = layoutButtons(layoutW_, layoutH_);
            for (int i = 0; i < kButtonCount; ++i) {
                const TouchButton& b = buttons[i];
                const bool down = buttonFinger_[i] != kNoFinger;
                if (!down && covered(b, layoutH_)) continue;
                // Opaque enough to read over a bright scene: at a third they
                // were outlines, and an outline over grass is hard to find.
                const ImU32 fill = down ? IM_COL32(255, 210, 90, 190) : IM_COL32(12, 12, 16, 175);
                const ImU32 edge = down ? IM_COL32(255, 230, 150, 255) : IM_COL32(230, 200, 120, 200);
                const ImVec2 c(b.cx, b.cy);
                if (!b.round) {
                    const ImVec2 a(b.cx - b.halfW, b.cy - b.halfH);
                    const ImVec2 z(b.cx + b.halfW, b.cy + b.halfH);
                    fg->AddRectFilled(a, z, fill, 8.0f);
                    fg->AddRect(a, z, edge, 8.0f, 0, 2.0f);
                    drawLabel(fg, b, b.label, 1.0f);
                    continue;
                }
                fg->AddCircleFilled(c, b.halfW, fill, 48);
                // The action's own icon on slots 1 to 6, as the bar shows it,
                // with its cooldown swept over it; the number where there is
                // none to show.
                SlotVisual visual;
                if (i < 6 && slotProvider_) visual = slotProvider_(i);
                if (visual.texture) {
                    const float r = b.halfW - 3.0f;
                    fg->AddImageRounded(reinterpret_cast<ImTextureID>(visual.texture),
                                        ImVec2(b.cx - r, b.cy - r), ImVec2(b.cx + r, b.cy + r),
                                        ImVec2(0.07f, 0.07f), ImVec2(0.93f, 0.93f),
                                        down ? IM_COL32(255, 235, 180, 255) : IM_COL32_WHITE, r);
                    if (visual.cooldown > 0.0f) drawCooldown(fg, c, r, visual.cooldown);
                } else {
                    drawLabel(fg, b, b.label, i == 6 ? 0.95f : 1.35f);
                }
                fg->AddCircle(c, b.halfW, edge, 48, 3.0f);
            }
        }
    }
    if (stickFingerId_ == kNoFinger) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;
    const float radius = stickRadius();
    const ImVec2 origin(stickOriginX_, stickOriginY_);
    const ImVec2 knob(stickOriginX_ + stickX_ * radius, stickOriginY_ + stickY_ * radius);
    dl->AddCircle(origin, radius, IM_COL32(255, 255, 255, 60), 48, 3.0f);
    // The ring the thumb has to cross to start strafing, so the player can see
    // why walking straight is the easy thing to do.
    dl->AddCircle(origin, radius * kStrafeDeadzone, IM_COL32(255, 255, 255, 35), 40, 2.0f);
    dl->AddCircleFilled(knob, radius * 0.28f, IM_COL32(255, 255, 255, 95), 32);
}

void TouchControls::reset() {
    stickFingerId_ = kNoFinger;
    lookFingerId_ = kNoFinger;
    lookMoved_ = false;
    lookTravel_ = 0.0f;
    pinchA_ = pinchB_ = kNoFinger;
    pinching_ = false;
    for (int i = 0; i < kButtonCount; ++i) {
        if (buttonFinger_[i] != kNoFinger) pressButton(i, buttonFinger_[i], false);
    }
    if (interactStep_ > 0) {
        core::Input::getInstance().setVirtualMouseButton(SDL_BUTTON_RIGHT, false);
        interactStep_ = 0;
    }
    stickX_ = stickY_ = 0.0f;
    wasForward_ = wasBack_ = wasLeft_ = wasRight_ = false;
    // Everything the stick could be holding, not just the four it uses now.
    core::Input::getInstance().clearVirtualKeys();
}

std::array<TouchControls::TouchButton, TouchControls::kButtonCount>
TouchControls::layoutButtons(float w, float h) const {
    std::array<TouchButton, kButtonCount> out{};

    // The thumb cluster, clear of the main bar along the bottom: that bar is
    // 53 of the interface's 768 units tall, and its bag and menu buttons sit
    // at its right-hand end, under where a thumb rests.
    const float barHeight = h * (53.0f / 768.0f) * 1.1f;
    constexpr float kMain = 42.0f;     // slot 1, the one a thumb rests on
    constexpr float kSmall = 28.0f;    // the rest: 56 points across, over Apple's 44
    const float cx = w - 22.0f - kMain;
    const float cy = h - barHeight - 18.0f - kMain;
    out[0] = {"1", SDL_SCANCODE_1, cx, cy, kMain, kMain, true};

    // Two rings round slot 1, so that no two buttons touch. Three buttons 45
    // degrees apart need a radius of 82 to clear each other by a few points,
    // and five on one ring would need twice that - they overlapped when they
    // were squeezed onto one. Angles are measured anticlockwise from the
    // right: 180 is left of slot 1, 90 is above it.
    struct Ring { const char* label; SDL_Scancode key; float radius; float degrees; };
    constexpr float kInner = kMain + 12.0f + kSmall;          // 82
    constexpr float kOuter = kInner + 2.0f * kSmall + 10.0f;  // 148
    static const Ring kRings[] = {
        {"2", SDL_SCANCODE_2, kInner, 180.0f},
        {"3", SDL_SCANCODE_3, kInner, 135.0f},
        {"4", SDL_SCANCODE_4, kInner, 90.0f},
        {"5", SDL_SCANCODE_5, kOuter, 157.5f},
        {"6", SDL_SCANCODE_6, kOuter, 112.5f},
    };
    for (int i = 0; i < 5; ++i) {
        const float rad = kRings[i].degrees * 3.14159265f / 180.0f;
        out[1 + i] = {kRings[i].label, kRings[i].key,
                      cx + kRings[i].radius * std::cos(rad),
                      cy - kRings[i].radius * std::sin(rad), kSmall, kSmall, true};
    }
    // Jump, out on the diagonal past both rings.
    constexpr float kJump = kOuter + 62.0f;
    out[6] = {"Jump", SDL_SCANCODE_SPACE, cx - kJump * 0.7071f, cy - kJump * 0.7071f,
              kSmall + 2.0f, kSmall + 2.0f, true};

    // The menu row, across the top middle, where the default interface puts
    // nothing: the minimap has the right-hand corner and the unit frames the
    // left. Each is the key a keyboard opens that panel with.
    struct MenuEntry { const char* label; SDL_Scancode key; };
    static const MenuEntry kMenu[kMenuButtons] = {
        {"Bags", SDL_SCANCODE_B},   {"Char", SDL_SCANCODE_C},  {"Spells", SDL_SCANCODE_P},
        {"Talents", SDL_SCANCODE_N}, {"Quests", SDL_SCANCODE_L}, {"Map", SDL_SCANCODE_M},
        {"Menu", SDL_SCANCODE_ESCAPE}};
    constexpr float kMenuHalfW = 34.0f, kMenuHalfH = 17.0f, kGap = 6.0f;
    const float rowWidth = kMenuButtons * (2.0f * kMenuHalfW) + (kMenuButtons - 1) * kGap;
    float x = (w - rowWidth) * 0.5f + kMenuHalfW;
    for (int i = 0; i < kMenuButtons; ++i) {
        out[kThumbButtons + i] = {kMenu[i].label, kMenu[i].key, x, 8.0f + kMenuHalfH,
                                  kMenuHalfW, kMenuHalfH, false};
        x += 2.0f * kMenuHalfW + kGap;
    }
    return out;
}

void TouchControls::drawLabel(ImDrawList* dl, const TouchButton& b, const char* text,
                              float scale) {
    // Shrunk to fit inside the button, which "Jump" did not at full size.
    float size = ImGui::GetFontSize() * scale;
    ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    const float room = b.round ? b.halfW * 1.6f : b.halfW * 1.8f;
    if (ts.x > room) {
        size *= room / ts.x;
        ts = ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    }
    dl->AddText(ImGui::GetFont(), size, ImVec2(b.cx - ts.x * 0.5f, b.cy - ts.y * 0.5f),
                IM_COL32(255, 255, 255, 235), text);
}

void TouchControls::drawCooldown(ImDrawList* dl, ImVec2 c, float r, float remaining) {
    // A dark wedge for the part of the cooldown still to run, from twelve
    // o'clock clockwise, as the action bar's own sweep reads.
    const float frac = std::clamp(remaining, 0.0f, 1.0f);
    const int segments = std::max(3, static_cast<int>(48.0f * frac));
    const float start = -3.14159265f * 0.5f;
    const float span = 2.0f * 3.14159265f * frac;
    for (int s = 0; s < segments; ++s) {
        const float a0 = start + span * static_cast<float>(s) / segments;
        const float a1 = start + span * static_cast<float>(s + 1) / segments;
        dl->AddTriangleFilled(c, ImVec2(c.x + r * std::cos(a0), c.y + r * std::sin(a0)),
                              ImVec2(c.x + r * std::cos(a1), c.y + r * std::sin(a1)),
                              IM_COL32(0, 0, 0, 160));
    }
}

int TouchControls::buttonAt(float x, float y, float w, float h) const {
    const auto buttons = layoutButtons(w, h);
    for (int i = 0; i < kButtonCount; ++i) {
        const TouchButton& b = buttons[i];
        if (covered(b, h)) continue;
        if (b.round) {
            // A little larger than drawn: a thumb lands off-centre.
            const float reach = b.halfW + 8.0f;
            if (std::hypot(x - b.cx, y - b.cy) <= reach) return i;
        } else if (std::abs(x - b.cx) <= b.halfW + 4.0f && std::abs(y - b.cy) <= b.halfH + 4.0f) {
            return i;
        }
    }
    return -1;
}

void TouchControls::pressButton(int index, SDL_FingerID finger, bool down) {
    if (index < 0 || index >= kButtonCount) return;
    if (down == (buttonFinger_[index] != kNoFinger)) return;
    buttonFinger_[index] = down ? finger : kNoFinger;
    const auto buttons = layoutButtons(std::max(layoutW_, 1.0f), std::max(layoutH_, 1.0f));
    sendKey(buttons[index].key, down, windowId_);
}

void TouchControls::sendKey(SDL_Scancode key, bool down, SDL_WindowID window) {
    // The polled state, which the action bar and movement read...
    core::Input::getInstance().setVirtualKey(key, down);
    // ...and the event, which ImGui's key state and the interface's own
    // bindings are built from. A real key produces both.
    SDL_Event e{};
    e.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    e.key.timestamp = SDL_GetTicksNS();
    e.key.windowID = window;
    e.key.scancode = key;
    e.key.key = SDL_GetKeyFromScancode(key, SDL_KMOD_NONE, false);
    e.key.down = down;
    SDL_PushEvent(&e);
}

void TouchControls::claimMouseFinger() {
    // Its mouse press was held back like any other, so nothing reached the
    // interface; this makes sure nothing ever does.
    if (press_ == Press::Pending || press_ == Press::Holding) press_ = Press::Suppressed;
}

void TouchControls::pushButton(bool down) const {
    SDL_Event e{};
    e.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
    e.button.timestamp = SDL_GetTicksNS();
    e.button.windowID = windowId_;
    e.button.which = kSyntheticMouse;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.down = down;
    e.button.clicks = 1;
    e.button.x = pressX_;
    e.button.y = pressY_;
    SDL_PushEvent(&e);
}

void TouchControls::pushMotion(float x, float y) const {
    SDL_Event e{};
    e.type = SDL_EVENT_MOUSE_MOTION;
    e.motion.timestamp = SDL_GetTicksNS();
    e.motion.windowID = windowId_;
    e.motion.which = kSyntheticMouse;
    e.motion.x = x;
    e.motion.y = y;
    SDL_PushEvent(&e);
}

bool TouchControls::filterMouseEvent(const SDL_Event& event) {
    const bool isButton = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                          event.type == SDL_EVENT_MOUSE_BUTTON_UP;
    if (isButton) {
        if (event.button.which != SDL_TOUCH_MOUSEID || event.button.button != SDL_BUTTON_LEFT) {
            return false;
        }
        if (windowId_ == 0) windowId_ = event.button.windowID;
        if (event.button.down) {
            // Held back until the finger says what it is.
            press_ = Press::Pending;
            pressX_ = event.button.x;
            pressY_ = event.button.y;
            pressTicks_ = SDL_GetTicks();
            return true;
        }
        // The finger lifted.
        const Press was = press_;
        press_ = Press::None;
        if (was == Press::Pending && SDL_GetTicks() - pressTicks_ < kLongPressMs) {
            // A tap: the click it held back, then the release, then the
            // pointer taken off the interface so a tooltip does not stay up
            // over whatever was tapped. In that order, from the queue - this
            // event is dropped because the press has to arrive before it.
            pushButton(true);
            pushButton(false);
            pushMotion(-10000.0f, -10000.0f);
            return true;
        }
        if (was == Press::Down) return false;   // the end of a drag
        // A hold ends with the tooltip still up and nothing clicked; a
        // claimed finger ends with nothing at all.
        return true;
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION && event.motion.which == SDL_TOUCH_MOUSEID) {
        if (press_ == Press::Pending || press_ == Press::Holding) {
            const float dx = event.motion.x - pressX_;
            const float dy = event.motion.y - pressY_;
            if (dx * dx + dy * dy > kTapSlop * kTapSlop) {
                // A drag. Straight away it scrolls, turns or slides; after a
                // hold it picks up what was under the finger. Either way the
                // button goes down where the finger is now, which is where the
                // pointer will be when the press reaches the queue's head.
                pressX_ = event.motion.x;
                pressY_ = event.motion.y;
                pushButton(true);
                press_ = Press::Down;
            }
        }
        return false;   // the pointer always follows the finger
    }
    return false;
}

}  // namespace ui
}  // namespace wowee
