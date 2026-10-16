#include "framework.h"
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
void LoadEmbeddedGroups(HWND hwndMDI);
LRESULT CALLBACK GroupWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Modern x64 Window Procedure with C-Linkage
extern "C" LRESULT CALLBACK ProgmanWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hwndProgman = hwnd;
        g_hInstance = ((LPCREATESTRUCT)lParam)->hInstance;

        // Register MDI Group Child Window Class
        WNDCLASS wc = { 0 };
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = GroupWndProc;
        wc.hInstance = g_hInstance;
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = TEXT("PMGroupClass");
        RegisterClass(&wc);

        CLIENTCREATESTRUCT ccs = { 0 };
        HMENU hMenu = GetMenu(hwnd);

        // Submenu index 2 is the "Window" MDI menu
        ccs.hWindowMenu = GetSubMenu(hMenu, 2);
        ccs.idFirstChild = 0x1F4; // First MDI child ID (500)

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        // Create MDI Client workspace
        g_hwndMDIClient = CreateWindowEx(
            0,
            TEXT("MDICLIENT"),
            NULL,
            WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
            0, 0,
            rcClient.right - rcClient.left,
            rcClient.bottom - rcClient.top,
            hwnd,
            (HMENU)(UINT_PTR)0xCAC,
            g_hInstance,
            &ccs
        );

        if (!g_hwndMDIClient) {
            return -1; // Fail creation
        }

        // Load embedded GRP resources and populate MDI workspace
        LoadEmbeddedGroups(g_hwndMDIClient);

        return 0;
    }

    case WM_DESTROY: {
        if (g_fSaveSettings) {
            SaveProgmanSettings();
        }

        if (g_hFont) {
            DeleteObject(g_hFont);
        }

        WinHelp(hwnd, TEXT("PROGMAN.HLP"), HELP_QUIT, 0);
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
        if (!Progman_OnCommand(lParam, wParam)) {
            return DefFrameProc(hwnd, g_hwndMDIClient, msg, wParam, lParam);
        }
        return 0;
    }

    case WM_INITMENU: {
        HMENU hMenu = (HMENU)wParam;
        HWND hActiveChild = (HWND)SendMessage(g_hwndMDIClient, WM_MDIGETACTIVE, 0, 0);
        BOOL bHasActiveChild = (hActiveChild != NULL);

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
        return DefFrameProc(hwnd, g_hwndMDIClient, msg, wParam, lParam);
    }
}

// Loads binary GRP file resources into MDI child windows
void LoadEmbeddedGroups(HWND hwndMDI) {
    int resourceIds[] = { 101, 102, 103, 104 };

    for (int resId : resourceIds) {
        HRSRC hRes = FindResource(g_hInstance, MAKEINTRESOURCE(resId), TEXT("GRP_DATA"));
        if (!hRes) continue;

        HGLOBAL hMem = LoadResource(g_hInstance, hRes);
        if (!hMem) continue;

        const BYTE* pData = (const BYTE*)LockResource(hMem);
        if (!pData) continue;

        PPROGMAN_GROUP pGroup = (PPROGMAN_GROUP)pData;

        // Verify "PMCC" magic bytes
        if (memcmp(pGroup->magic, "PMCC", 4) != 0) {
            continue;
        }

        // Extract Title String from header offset
        const char* szTitleA = (const char*)(pData + pGroup->pName);
        WCHAR szTitleW[256] = { 0 };
        MultiByteToWideChar(CP_ACP, 0, szTitleA, -1, szTitleW, 256);

        // Spawn MDI Child for the GRP file
        MDICREATESTRUCT mcs = { 0 };
        mcs.szClass = TEXT("PMGroupClass");
        mcs.szTitle = szTitleW;
        mcs.hOwner = g_hInstance;
        mcs.x = (pGroup->rcLeft > 0) ? pGroup->rcLeft : CW_USEDEFAULT;
        mcs.y = (pGroup->rcTop > 0) ? pGroup->rcTop : CW_USEDEFAULT;
        mcs.cx = (pGroup->rcRight - pGroup->rcLeft > 100) ? (pGroup->rcRight - pGroup->rcLeft) : 320;
        mcs.cy = (pGroup->rcBottom - pGroup->rcTop > 100) ? (pGroup->rcBottom - pGroup->rcTop) : 240;
        mcs.lParam = (LPARAM)pGroup;

        SendMessage(hwndMDI, WM_MDICREATE, 0, (LPARAM)&mcs);
    }
}

// Window Procedure for MDI Child Group Windows
LRESULT CALLBACK GroupWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        LPMDICREATESTRUCT pmcs = (LPMDICREATESTRUCT)pcs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pmcs->lParam);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        PPROGMAN_GROUP pGroup = (PPROGMAN_GROUP)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (pGroup) {
            const BYTE* pBase = (const BYTE*)pGroup;
            SetBkMode(hdc, TRANSPARENT);

            // Iterate items inside the group file
            for (WORD i = 0; i < pGroup->cItems; i++) {
                WORD offset = pGroup->rgiItems[i];
                if (offset == 0) continue;

                PPROGMAN_ITEM pItem = (PPROGMAN_ITEM)(pBase + offset);
                const char* szItemNameA = (const char*)(pBase + pItem->pName);

                WCHAR szItemNameW[128] = { 0 };
                MultiByteToWideChar(CP_ACP, 0, szItemNameA, -1, szItemNameW, 128);

                // Draw standard application icon placeholder and title text
                HICON hIcon = LoadIcon(NULL, IDI_APPLICATION);
                DrawIcon(hdc, pItem->x, pItem->y, hIcon);

                RECT rcText = { pItem->x - 20, pItem->y + 34, pItem->x + 52, pItem->y + 70 };
                DrawText(hdc, szItemNameW, -1, &rcText, DT_CENTER | DT_WORDBREAK);
            }
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    default:
        return DefMDIChildProc(hwnd, msg, wParam, lParam);
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