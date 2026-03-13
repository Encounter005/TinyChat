#ifndef ANIMATIONTIMING_H
#define ANIMATIONTIMING_H

// Unified animation rhythm table for TinyChat desktop UI.
// Keep all interactive motion durations in one place to
// ensure consistent feel across dialogs and chat views.
namespace UiAnim {
constexpr int kInputFocusMs = 360;
constexpr int kPanelSlideMs = 800;
constexpr int kBubbleHeightMs = 700;
constexpr int kBubbleWidthMs = 500;
constexpr int kPageFadeMs = 600;
constexpr int kListItemFadeMs = 600;
}

#endif
