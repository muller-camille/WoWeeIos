#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>

#include <array>
#include <cstdint>
#include <functional>
#include <utility>

namespace wowee {
namespace rendering { class CameraController; }
namespace ui {

/**
 * The on-screen controls a phone needs, and nothing a desktop does.
 *
 * A touch screen already drives most of this client. SDL reports the first
 * finger as a mouse, so taps press buttons and a drag orbits the camera exactly
 * as a held left button does, and every panel, slider and action button works
 * without knowing a finger touched it. Two things a finger cannot do are hold W
 * and turn a wheel. Those are what this adds.
 *
 * It reads the finger events SDL sends alongside the mouse ones, and claims
 * only what it needs: a thumb in the lower-left corner becomes a movement
 * stick, and two fingers spreading become the camera zoom. Everything else is
 * left alone.
 *
 * The stick's own finger is also the one SDL is reporting as a held mouse
 * button, which would orbit the camera the whole time the player walked. So
 * while the stick is held the camera is told to ignore mouse motion. That is
 * the one place this has to reach outside itself.
 */
class TouchControls {
public:
    TouchControls() { buttonFinger_.fill(kNoFinger); }

    /// The stick only applies in the world. On the login and character
    /// screens every touch is left to be a mouse, but for the pinch, which
    /// is the wheel everywhere: it zooms the character preview there.
    void setInWorld(bool inWorld);
    [[nodiscard]] bool isInWorld() const { return inWorld_; }

    /// True while a thumb is on the stick, which is when the camera has to be
    /// held still.
    [[nodiscard]] bool isStickHeld() const { return stickFingerId_ != kNoFinger; }

    /// True while two fingers are down as a pinch. The finger SDL makes the
    /// mouse of is one of them, so anything that drags on the mouse - the
    /// preview's turn - holds off while this is true.
    [[nodiscard]] bool isPinching() const { return pinching_; }

    /// The camera to steer. Without one the look finger does nothing.
    void setCameraController(rendering::CameraController* camera) { camera_ = camera; }

    /// True while a finger is dragging the view, which is when the character
    /// should face where the camera does.
    [[nodiscard]] bool isSteering() const { return lookFingerId_ != kNoFinger && lookMoved_; }

    void handleEvent(const SDL_Event& event, int windowWidth, int windowHeight);

    /// Turns the mouse SDL makes of a finger into what a finger means, and
    /// says whether the event should go no further. Before anything else sees
    /// the event, on every screen:
    ///
    ///  - a quick tap is a click, sent when the finger lifts;
    ///  - a finger held still is a hover: the tooltip under it shows, and
    ///    lifting it clicks nothing;
    ///  - a finger that moves is a drag - straight away, or after a hold,
    ///    which is how an item or a spell is picked up and carried.
    ///
    /// SDL presses the button the moment the finger lands, which made every
    /// look at a tooltip a click as well: holding a spell to read it cast it.
    /// So the press is held back until the finger says which it is. Only the
    /// interface's side is filtered; the world reads SDL's own button state.
    [[nodiscard]] bool filterMouseEvent(const SDL_Event& event);

    /// A right-click where the finger last was: the button goes down on the
    /// next update and up on the one after, so the click reaches the world
    /// through the same path a mouse's does.
    void requestInteract() { interactStep_ = 2; }

    /// Applies the stick to the movement keys. Once a frame, after events.
    void update();

    /// Draws the stick where the thumb put it, the thumb buttons and the menu
    /// row. The buttons only in the world.
    void draw() const;

    /// Drops the stick and releases the keys.
    void reset();

    /// Whether an interface frame is under a point in window points. A thumb
    /// or menu button with one under it is neither drawn nor pressed, so an
    /// open bag, vendor or quest log in that corner takes its own taps.
    using InterfaceProbe = std::function<bool(float x, float y, float screenH)>;
    void setInterfaceProbe(InterfaceProbe probe) { probe_ = std::move(probe); }

    /// What a thumb button shows for action slot `slot` (0 to 5 on the main
    /// bar's current page): the spell's icon, and how much of its cooldown is
    /// left, 0 to 1. A button with no icon shows its number.
    struct SlotVisual {
        void* texture = nullptr;   // an ImTextureID
        float cooldown = 0.0f;
    };
    using SlotProvider = std::function<SlotVisual(int slot)>;
    void setSlotProvider(SlotProvider provider) { slotProvider_ = std::move(provider); }

    /// Where the stick may start, as a fraction of the window.
    static constexpr float kStickZoneWidth = 0.38f;
    static constexpr float kStickZoneTop = 0.30f;

private:
    static constexpr SDL_FingerID kNoFinger = -1;

    /// How far the thumb travels for a full deflection, in Android's density
    /// independent pixels. A radius in raw pixels is a different size on every
    /// phone: 150 of them is a third of an inch on this one, which is why the
    /// first version was unusable.
    static constexpr float kStickRadiusDp = 130.0f;

    /// Below this the stick reads as centred, so a resting thumb does not walk.
    /// Sideways takes more deflection than forward, so that walking straight
    /// ahead does not need a steady hand.
    static constexpr float kWalkDeadzone = 0.30f;
    static constexpr float kStrafeDeadzone = 0.45f;
    /// How far back inside the deadzone a direction has to fall before it lets
    /// go, so a thumb resting on the edge does not stutter.
    static constexpr float kReleaseHysteresis = 0.10f;
    /// What a pinch has to cover before it is worth a notch of the wheel.
    static constexpr float kPinchPixelsPerNotch = 90.0f;
    /// How far a look finger travels before it counts as a drag rather than a
    /// tap, so that selecting a target does not also swing the view.
    static constexpr float kLookSlopPixels = 16.0f;

    void setMovementKeys(bool forward, bool back, bool left, bool right) const;
    /// Full deflection in real pixels, from the display's density.
    [[nodiscard]] float stickRadius() const;
    /// Held with hysteresis: a direction already on lets go later than it came on.
    [[nodiscard]] static bool held(float axis, float threshold, bool wasOn);

    bool inWorld_ = false;

    SDL_FingerID stickFingerId_ = kNoFinger;
    float stickOriginX_ = 0.0f, stickOriginY_ = 0.0f;
    float stickX_ = 0.0f, stickY_ = 0.0f;   // -1..1

    // Pinch: the two fingers that are not the stick.
    rendering::CameraController* camera_ = nullptr;

    // The finger steering the view. Not the stick's, and not the second of a
    // pinch.
    SDL_FingerID lookFingerId_ = kNoFinger;
    float lookX_ = 0.0f, lookY_ = 0.0f;
    bool lookMoved_ = false;
    float lookTravel_ = 0.0f;

    // --- Thumb buttons and the menu row -----------------------------------
    //
    // Large targets for a thumb: the first six action slots and jump around
    // the lower-right corner, and a row along the top for the panels a
    // keyboard opens with a letter. Each presses its key the way a keyboard
    // does, in both places a key is read: the polled state the action bar
    // reads, and the events the interface's bindings and ImGui read.
    struct TouchButton {
        const char* label = "";
        SDL_Scancode key = SDL_SCANCODE_UNKNOWN;
        float cx = 0.0f, cy = 0.0f;   // centre, in window points
        float halfW = 0.0f, halfH = 0.0f;
        bool round = true;
    };
    static constexpr int kThumbButtons = 7;
    static constexpr int kMenuButtons = 7;
    static constexpr int kButtonCount = kThumbButtons + kMenuButtons;
    std::array<TouchButton, kButtonCount> layoutButtons(float w, float h) const;
    static void drawLabel(ImDrawList* dl, const TouchButton& b, const char* text, float scale);
    static void drawCooldown(ImDrawList* dl, ImVec2 c, float r, float remaining);
    /// The button under a point, or -1.
    int buttonAt(float x, float y, float w, float h) const;
    void pressButton(int index, SDL_FingerID finger, bool down);
    /// Off the world: nothing but the pinch.
    void handleMenuPinch(const SDL_Event& event, float x, float y);
    /// A pinch that has moved far enough, sent on as the wheel it stands for.
    void sendPinchAsWheel(float spacing);
    static void sendKey(SDL_Scancode key, bool down, SDL_WindowID window);

    // Which finger holds which button, so the key comes up with the finger.
    std::array<SDL_FingerID, kButtonCount> buttonFinger_{};
    InterfaceProbe probe_;
    SlotProvider slotProvider_;
    /// A button the interface is covering: not drawn, not pressed.
    [[nodiscard]] bool covered(const TouchButton& b, float h) const {
        return probe_ && probe_(b.cx, b.cy, h);
    }
    float layoutW_ = 0.0f, layoutH_ = 0.0f;
    SDL_WindowID windowId_ = 0;

    // --- The finger's mouse ----------------------------------------------
    enum class Press { None, Pending, Holding, Down, Suppressed };
    Press press_ = Press::None;
    float pressX_ = 0.0f, pressY_ = 0.0f;
    uint64_t pressTicks_ = 0;
    /// Fingers on the glass, for knowing which one SDL's mouse follows: the
    /// one that lands when no other is down.
    int fingersDown_ = 0;
    /// A hold long enough to be a hover rather than a tap.
    static constexpr uint64_t kLongPressMs = 450;
    /// How far a finger may wander and still be a tap, in window points.
    static constexpr float kTapSlop = 10.0f;
    /// SDL's id for a mouse made of a finger is filtered; these are the
    /// events this sends in its place, told apart by a different id.
    static constexpr SDL_MouseID kSyntheticMouse = 0x57575745u;
    void pushButton(bool down) const;
    void pushMotion(float x, float y) const;
    /// The finger SDL's mouse follows was taken for the stick or a button.
    void claimMouseFinger();

    SDL_FingerID pinchA_ = kNoFinger, pinchB_ = kNoFinger;
    float pinchAX_ = 0.0f, pinchAY_ = 0.0f;
    float pinchBX_ = 0.0f, pinchBY_ = 0.0f;
    float lastPinchSpacing_ = 0.0f;
    bool pinching_ = false;

    // What the stick asked for last frame, for the hysteresis.
    mutable bool wasForward_ = false, wasBack_ = false, wasLeft_ = false, wasRight_ = false;

    mutable float cachedRadius_ = 0.0f;

    // 2: press the virtual right button on the next update; 1: release it.
    int interactStep_ = 0;
};

/// The one the client uses. One screen, one pair of thumbs.
TouchControls& touchControls();

}  // namespace ui
}  // namespace wowee
