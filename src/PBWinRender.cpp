
#include "PBWinRender.h"

HWND PBInitWinRender (LONG width, LONG height) {

    HWND hwnd = NULL;

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = TEXT("PBWinClass");
    RegisterClass(&wc);

    RECT rect = { 0, 0, width, height };
    AdjustWindowRect( &rect, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd = CreateWindow(
        wc.lpszClassName, TEXT("PInball"), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (hwnd != NULL) return (hwnd);
    else return (NULL); 
}
