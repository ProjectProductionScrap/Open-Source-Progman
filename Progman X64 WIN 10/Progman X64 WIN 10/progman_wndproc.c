#include <windows.h>
#include <stdio.h>
#include "progman_types.h"

// Globals
HWND      g_hwndProgman = NULL;
HWND      g_hwndMDIClient = NULL;
HINSTANCE g_hInstance = NULL;
HFONT     g_hFont = NULL;
BOOL      g_fIsShell = TRUE;
BOOL      g_fSaveSettings = TRUE;

// Forward declarations
void SaveProgmanSettings(void);
BOOL Progman_OnCommand(LPARAM lParam, WPARAM wParam);

// Modern x64 Window Procedure with C-Linkage
extern "C" LRESULT CALLBACK ProgmanWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hwndProgman = hwnd;

        CLIENTCREATESTRUCT ccs = { 0 };
        HMENU hMenu = GetMenu(hwnd);

        // Submenu index 2 is the "Window" MDI menu
        ccs.hWindowMenu = GetSubMenu(hMenu, 2);
        ccs.idFirstChild = 0x1F4; // First MDI child ID (500)

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        // Create MDI Client workspace
        g_hwndMDIClient = CreateWindowExA(
            0,
            "MDICLIENT",
            NULL,
            WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
            0, 0,
            rcClient.right - rcClient.left,
            rcClient.bottom - rcClient.top,
            hwnd,
            (HMENU)0xCAC,
            g_hInstance,
            &ccs
        );

        if (!g_hwndMDIClient) {
            return -1; // Fail creation
        }
        return 0;
    }

    case WM_DESTROY: {
        if (g_fSaveSettings) {
            SaveProgmanSettings();
        }

        if (g_hFont) {
            DeleteObject(g_hFont);
        }

        WinHelpA(hwnd, "PROGMAN.HLP", HELP_QUIT, 0);
        PostQuitMessage(0);
        return 0;
    }

    case WM_SIZE: {
        if (wParam != SIZE_MINIMIZED && g_hwndMDIClient) {
            MoveWindow(g_hwndMDIClient, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        }
        return 0;
    }

    case WM_CLOSE: {
        if (g_fSaveSettings) {
            SaveProgmanSettings();
        }
        DestroyWindow(hwnd);
        return 0;
    }

    case WM_COMMAND: {
        // Check if command is handled by internal dispatcher
        if (!Progman_OnCommand(lParam, wParam)) {
            return DefFrameProcA(hwnd, g_hwndMDIClient, msg, wParam, lParam);
        }
        return 0;
    }

    case WM_INITMENU: {
        HMENU hMenu = (HMENU)wParam;
        HWND hActiveChild = (HWND)SendMessageA(g_hwndMDIClient, WM_MDIGETACTIVE, 0, 0);
        BOOL bHasActiveChild = (hActiveChild != NULL);

        // Enable/Disable menu items based on active group state
        EnableMenuItem(hMenu, 0x0065, MF_ENABLED);                                // IDM_NEW
        EnableMenuItem(hMenu, 0x0066, bHasActiveChild ? MF_ENABLED : MF_GRAYED); // IDM_OPEN
        EnableMenuItem(hMenu, 0x0068, bHasActiveChild ? MF_ENABLED : MF_GRAYED); // IDM_DELETE
        return 0;
    }

    case WM_SYSCOLORCHANGE:
    case WM_SETTINGCHANGE: {
        if (g_hwndMDIClient) {
            RedrawWindow(g_hwndMDIClient, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
        }
        return 0;
    }

    default:
        return DefFrameProcA(hwnd, g_hwndMDIClient, msg, wParam, lParam);
    }
}

// Dummy stubs for compilation
void SaveProgmanSettings(void) {}

BOOL Progman_OnCommand(LPARAM lParam, WPARAM wParam) {
    WORD wCmd = LOWORD(wParam);
    if (wCmd == 0x0069) { // IDM_EXIT
        PostQuitMessage(0);
        return TRUE;
    }
    return FALSE;
}