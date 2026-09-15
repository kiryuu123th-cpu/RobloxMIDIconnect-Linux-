#ifndef X11_HELPER_H
#define X11_HELPER_H

#include <string>

// X11ディスプレイの初期化と終了（オートリピート制御を含む）
bool initX11();
void closeX11();

// キーの長押し/解放を制御する関数
void setVirtualKey(const std::string& keyCombo, bool press);

// サステインペダル（Spaceキー）の長押し/解放制御関数
void sendSpaceKey(bool press);

#endif // X11_HELPER_H