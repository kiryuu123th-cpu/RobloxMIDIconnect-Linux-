#include "x11_helper.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>

static Display *display = nullptr;
// キーの押しっぱなし状態を保持するマップ
static std::map<KeyCode, bool> keyStateMap;

bool initX11() {
    display = XOpenDisplay(NULL);
    if (!display) {
        std::cerr << "X11: Failed to open display" << std::endl;
        return false;
    }

    // 1. オートリピート（キー長押しによる自動連打）を無効化
    // これにより、X11が勝手にKeyRelease/KeyPressの連打を生成する誤作動を防ぎます
    XAutoRepeatOff(display);
    XFlush(display);

    std::cout << "X11 initialized (Auto-Repeat disabled)." << std::endl;
    return true;
}

void closeX11() {
    if (display) {
        // 残っている全キーを安全に解放
        for (auto const& [code, isPressed] : keyStateMap) {
            if (isPressed) {
                XTestFakeKeyEvent(display, code, False, CurrentTime);
            }
        }
        keyStateMap.clear();

        // 2. アプリ終了時にオートリピートを元の状態（有効）に戻す
        XAutoRepeatOn(display);
        XFlush(display);

        XCloseDisplay(display);
        display = nullptr;
        std::cout << "X11 closed (Auto-Repeat restored)." << std::endl;
    }
}

// キー文字列から KeyCode と Shift判定を取得するヘルパー
static KeyCode parseKeyToCode(const std::string& keyStr, bool& needShift) {
    needShift = false;
    KeySym sym = NoSymbol;

    if (keyStr.length() == 1) {
        char c = keyStr[0];
        if (c >= 'A' && c <= 'Z') {
            needShift = true;
            sym = XStringToKeysym(keyStr.c_str());
        } else {
            // 記号類のShift判定 (!, @, #, ^ など)
            switch (c) {
                case '!': sym = XK_exclam; needShift = true; break;
                case '@': sym = XK_at; needShift = true; break;
                case '#': sym = XK_numbersign; needShift = true; break;
                case '$': sym = XK_dollar; needShift = true; break;
                case '%': sym = XK_percent; needShift = true; break;
                case '^': sym = XK_asciicircum; needShift = true; break;
                case '&': sym = XK_ampersand; needShift = true; break;
                case '*': sym = XK_asterisk; needShift = true; break;
                case '(': sym = XK_parenleft; needShift = true; break;
                case ')': sym = XK_parenright; needShift = true; break;
                default:
                    sym = XStringToKeysym(keyStr.c_str());
                    break;
            }
        }
    } else if (keyStr == "ctrl" || keyStr == "Control_L") {
        sym = XK_Control_L;
    } else if (keyStr == "space" || keyStr == "Space") {
        sym = XK_space;
    } else {
        sym = XStringToKeysym(keyStr.c_str());
    }

    if (sym == NoSymbol) return 0;
    return XKeysymToKeycode(display, sym);
}

void setVirtualKey(const std::string& keyCombo, bool press) {
    if (!display) return;

    // "ctrl+2" や "ctrl+g" のような複合キーの分解
    std::vector<std::string> keys;
    std::stringstream ss(keyCombo);
    std::string item;
    while (std::getline(ss, item, '+')) {
        keys.push_back(item);
    }

    KeyCode ctrlCode = XKeysymToKeycode(display, XK_Control_L);
    KeyCode shiftCode = XKeysymToKeycode(display, XK_Shift_L);

    bool hasCtrl = false;
    std::string mainKeyStr = "";

    for (const auto& k : keys) {
        if (k == "ctrl") hasCtrl = true;
        else mainKeyStr = k;
    }

    bool needShift = false;
    KeyCode mainCode = parseKeyToCode(mainKeyStr, needShift);

    if (press) {
        // --- KEY PRESS 処理 ---
        
        // 1. 修飾キー (Ctrl / Shift) の適用
        if (hasCtrl) {
            XTestFakeKeyEvent(display, ctrlCode, True, CurrentTime);
            keyStateMap[ctrlCode] = true;
        }
        if (needShift) {
            XTestFakeKeyEvent(display, shiftCode, True, CurrentTime);
            keyStateMap[shiftCode] = true;
        }

        // 短いディレイを入れて修飾キーが認識されてからメインキーを押す（誤作動防止）
        if (hasCtrl || needShift) {
            XFlush(display);
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }

        // 2. メインキーの送出
        if (mainCode != 0) {
            XTestFakeKeyEvent(display, mainCode, True, CurrentTime);
            keyStateMap[mainCode] = true;
        }

    } else {
        // --- KEY RELEASE 処理 ---

        // 1. メインキーの解放
        if (mainCode != 0 && keyStateMap[mainCode]) {
            XTestFakeKeyEvent(display, mainCode, False, CurrentTime);
            keyStateMap[mainCode] = false;
        }

        // 2. 修飾キー (Ctrl / Shift) の解放
        if (hasCtrl && keyStateMap[ctrlCode]) {
            XTestFakeKeyEvent(display, ctrlCode, False, CurrentTime);
            keyStateMap[ctrlCode] = false;
        }
        if (needShift && keyStateMap[shiftCode]) {
            XTestFakeKeyEvent(display, shiftCode, False, CurrentTime);
            keyStateMap[shiftCode] = false;
        }
    }

    XFlush(display);
}

void sendSpaceKey(bool press) {
    if (!display) return;
    KeyCode spaceCode = XKeysymToKeycode(display, XK_space);
    if (spaceCode != 0) {
        XTestFakeKeyEvent(display, spaceCode, press ? True : False, CurrentTime);
        keyStateMap[spaceCode] = press;
        XFlush(display);
    }
}