
// Window and render code for Windows
// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "PBWinRender.h"

HWND PBInitWinRender (long width, long height) {

    HWND hwnd = NULL;

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = TEXT("PBWinClass");
    RegisterClass(&wc);

    RECT rect = { 0, 0, width, height };
    AdjustWindowRect( &rect, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd = CreateWindow(
        wc.lpszClassName, TEXT("PInball Simulator"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (hwnd != NULL) return (hwnd);
    else return (NULL); 
}
