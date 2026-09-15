#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <map>
#include <rtmidi/RtMidi.h>

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QPaintEvent>
#include <QIcon>

#include "x11_helper.h"
#include "windowUI.h"
RtMidiIn *midiIn = nullptr;
QLabel *statusLabel = nullptr;
QLabel *lastNoteLabel = nullptr;
QLabel *pedalStatusLabel = nullptr;

// MAPPING ROBLOX PIANO
const std::map<std::string, std::string> fullMappings = {
    {"A#0", "ctrl+2"}, {"A#1", "ctrl+r"}, {"A#2", "^"}, {"A#3", "E"}, {"A#4", "P"}, {"A#5", "J"}, {"A#6", "B"}, {"A#7", "ctrl+g"},
    {"A0", "ctrl+1"},  {"A1", "ctrl+e"},  {"A2", "6"}, {"A3", "e"}, {"A4", "p"}, {"A5", "j"}, {"A6", "b"}, {"A7", "ctrl+f"},
    {"B0", "ctrl+3"},  {"B1", "ctrl+t"},  {"B2", "7"}, {"B3", "r"}, {"B4", "a"}, {"B5", "k"}, {"B6", "n"}, {"B7", "ctrl+h"},
    {"C#1", "ctrl+5"}, {"C#2", "!"},      {"C#3", "*"}, {"C#4", "T"}, {"C#5", "S"}, {"C#6", "L"}, {"C#7", "ctrl+y"},
    {"C1", "ctrl+4"},  {"C2", "1"},       {"C3", "8"}, {"C4", "t"}, {"C5", "s"}, {"C6", "l"}, {"C7", "m"}, {"C8", "ctrl+j"},
    {"D#1", "ctrl+7"}, {"D#2", "@"},      {"D#3", "("}, {"D#4", "Y"}, {"D#5", "D"}, {"D#6", "Z"}, {"D#7", "ctrl+i"},
    {"D1", "ctrl+6"},  {"D2", "2"},       {"D3", "9"}, {"D4", "y"}, {"D5", "d"}, {"D6", "z"}, {"D7", "ctrl+u"},
    {"E1", "ctrl+8"},  {"E2", "3"},       {"E3", "0"}, {"E4", "u"}, {"E5", "f"}, {"E6", "x"}, {"E7", "ctrl+o"},
    {"F#1", "ctrl+0"}, {"F#2", "$"},      {"F#3", "Q"}, {"F#4", "I"}, {"F#5", "G"}, {"F#6", "C"}, {"F#7", "ctrl+a"},
    {"F1", "ctrl+9"},  {"F2", "4"},       {"F3", "q"}, {"F4", "i"}, {"F5", "g"}, {"F6", "c"}, {"F7", "ctrl+p"},
    {"G#1", "ctrl+w"}, {"G#2", "%"},      {"G#3", "W"}, {"G#4", "O"}, {"G#5", "H"}, {"G#6", "V"}, {"G#7", "ctrl+d"},
    {"G1", "ctrl+q"},  {"G2", "5"},       {"G3", "w"}, {"G4", "o"}, {"G5", "h"}, {"G6", "v"}, {"G7", "ctrl+s"}
};

std::string midiNoteToName(int noteNumber) {
    const std::vector<std::string> noteNames = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    if (noteNumber < 0 || noteNumber > 127) return "";
    int octave = (noteNumber / 12) - 1;
    int index = noteNumber % 12;
    return noteNames[index] + std::to_string(octave);
}

void midiCallback(double deltatime, std::vector<unsigned char> *message, void *userData) {
    if (message->size() >= 3) {
        int status = (*message)[0];
        int data1 = (*message)[1];
        int data2 = (*message)[2];
        int channelType = status & 0xF0;

        // --- Note ON ---
        if (channelType == 0x90 && data2 > 0) {
            std::string noteName = midiNoteToName(data1);
            std::cout << "Note ON: " << data1 << " (" << noteName << ")" << std::endl;

            if (lastNoteLabel) {
                QString displayStr = QString::number(data1) + " (" + QString::fromStdString(noteName) + ")";
                QMetaObject::invokeMethod(lastNoteLabel, "setText", Qt::QueuedConnection, Q_ARG(QString, displayStr));
            }

            auto it = fullMappings.find(noteName);
            if (it != fullMappings.end()) {
                setVirtualKey(it->second, true);
            }
        } 
        // --- Note OFF ---
        else if (channelType == 0x80 || (channelType == 0x90 && data2 == 0)) {
            std::string noteName = midiNoteToName(data1);
            std::cout << "Note OFF: " << data1 << " (" << noteName << ")" << std::endl;

            auto it = fullMappings.find(noteName);
            if (it != fullMappings.end()) {
                setVirtualKey(it->second, false);
            }
        } 
        // --- Sustain Pedal (CC 64) ---
        else if (channelType == 0xB0 && data1 == 64) {
            bool pedalPressed = (data2 >= 63);
            std::cout << "Sustain Pedal: " << (pedalPressed ? "ON" : "OFF") << std::endl;
            
            if (pedalStatusLabel) {
                QString pStr = pedalPressed ? "ON" : "OFF";
                QMetaObject::invokeMethod(pedalStatusLabel, "setText", Qt::QueuedConnection, Q_ARG(QString, pStr));
                QMetaObject::invokeMethod(pedalStatusLabel, "setStyleSheet", Qt::QueuedConnection, 
                    Q_ARG(QString, pedalPressed ? "color: #ffffff; font-weight: bold;" : "color: #a1a1aa; font-weight: bold;"));
            }
            sendSpaceKey(pedalPressed);
        }
    }
}

int main(int argc, char *argv[]) {
    if (!initX11()) {
        std::cerr << "Failed to open X display!" << std::endl;
        return 1;
    }

    QApplication app(argc, argv);
    
    // Appicon
    app.setWindowIcon(QIcon(":/icon.png"));

    app.setStyleSheet(
        "QWidget {"
        "    color: #ffffff;"
        "    font-family: sans-serif;"
        "    font-size: 11px;"
        "}"
        "QLabel#AsciiLogo {"
        "    color: #ffffff;"
        "    font-family: 'Courier New', Courier, monospace;"
        "    font-size: 5px;"
        "    background-color: transparent;"
        "    padding: 2px;"
        "}"
        "QComboBox {"
        "    background-color: rgba(30, 30, 30, 180);"
        "    border: 1px solid #555555;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    color: #ffffff;"
        "}"
        "QComboBox::drop-down {"
        "    border: 0px;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #111111;"
        "    color: #ffffff;"
        "    selection-background-color: #444444;"
        "}"
        "QPushButton {"
        "    background-color: rgba(30, 30, 30, 180);"
        "    border: 1px solid #555555;"
        "    border-radius: 4px;"
        "    padding: 5px 10px;"
        "    color: #ffffff;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(60, 60, 60, 200);"
        "    border-color: #888888;"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(90, 90, 90, 200);"
        "}"
    );

    MidiApp window;
    window.show();

    int result = app.exec();

    closeX11();
    return result;
}