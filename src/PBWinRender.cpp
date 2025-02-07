
// Window and render code for Windows
// MIT License, Copyright (c) 2025 Jeffrey D. Bock, except where where otherwise noted

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
