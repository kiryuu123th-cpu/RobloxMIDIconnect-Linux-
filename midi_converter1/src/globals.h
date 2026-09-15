#pragma once

#include <QLabel>
#include <vector>
#include <RtMidi.h>

// Global objects used by both UI (windowUI.cpp) and system logic (main.cpp)
extern RtMidiIn *midiIn;
extern QLabel *statusLabel;
extern QLabel *lastNoteLabel;
extern QLabel *pedalStatusLabel;

// Callback used by RtMidiIn
void midiCallback(double deltatime, std::vector<unsigned char> *message, void *userData);

// Functions used by UI (declared elsewhere)
void setVirtualKey(const std::string &key, bool press);
void sendSpaceKey(bool press);

