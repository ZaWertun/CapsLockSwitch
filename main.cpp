#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinUser.h>
#include <strsafe.h>
#include <shellapi.h>

#include "resource.h"

#define CLASS_NAME  L"CapsLock Switch"
#define WINDOW_NAME L"CapsLock Switch Window"

HWND TakeActiveWindow()
{
    HWND hWnd = GetForegroundWindow();
    HWND hWndParent;
    while ((hWndParent = GetParent(hWnd)))
    {
        hWnd = hWndParent;
    }
    return hWnd;
}

void SwitchInputLocale(bool forward = true)
{
    if (HWND hWnd = TakeActiveWindow())
    {
        PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, forward ? INPUTLANGCHANGE_FORWARD : INPUTLANGCHANGE_BACKWARD, NULL);
    }
}

LRESULT CALLBACK HookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
    bool preventDefault = false;
    if (nCode == HC_ACTION)
    {
        switch (wParam)
        {
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
                PKBDLLHOOKSTRUCT hookStruct = (PKBDLLHOOKSTRUCT)lParam;
                bool isShiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;
                if (hookStruct->vkCode == VK_CAPITAL && !isShiftPressed)
                {
                    if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
                    {
                        SwitchInputLocale();
                    }
                    preventDefault = true;
                }
        }
    }
    return preventDefault ? 1 : CallNextHookEx(NULL, nCode, wParam, lParam);
}

HMENU hMenu = NULL;
const int IDM_EXIT = 100;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static NOTIFYICONDATA niData = { sizeof(niData) };
    switch (uMsg)
    {
        case WM_CREATE:
        {
            niData.cbSize = sizeof(niData);
            niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            niData.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
            niData.hWnd   = hWnd;
            StringCchCopy(niData.szTip, ARRAYSIZE(niData.szTip), CLASS_NAME);
            niData.uCallbackMessage = WM_USER + 1;
            Shell_NotifyIcon(NIM_ADD, &niData);
            return 0;
        }
        case WM_DESTROY:
        {
            Shell_NotifyIcon(NIM_DELETE, &niData);
            DestroyIcon(niData.hIcon);
            DestroyMenu(hMenu);
            PostQuitMessage(0);
            return 0;
        }
        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDM_EXIT)
            {
                DestroyWindow(hWnd);
            }
            break;
        }
        case WM_USER + 1:
        {
            WORD cmd = LOWORD(lParam);
            if (cmd == WM_RBUTTONUP && hMenu == NULL)
            {
				POINT pt;
				GetCursorPos(&pt);
                SetForegroundWindow(hWnd);
				hMenu = CreatePopupMenu();
				InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_EXIT, L"Exit");
				if (TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL))
				{
					SendMessage(hWnd, WM_COMMAND, cmd, 0);
				}
            }
			else if (cmd != WM_MOUSEMOVE && hMenu != NULL)
			{
				DestroyMenu(hMenu);
				hMenu = NULL;
				SendMessage(hWnd, WM_CANCELMODE, 0, 0);
			}
            break;
        }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR lpCmdLine, _In_ INT nCmdShow)
{
    if (FindWindow(CLASS_NAME, WINDOW_NAME))
    {
        MessageBox(NULL, L"Already started!", L"Error", MB_ICONINFORMATION);
        return 0;
    }

    HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0);
    if (!hook)
    {
        MessageBox(NULL, L"Failed to install hook!", L"Error", MB_ICONERROR);
        return 1;
    }

    WNDCLASSEX wx = { sizeof(wx) };
    wx.lpfnWndProc = WndProc;
    wx.hInstance   = hInstance;
    wx.lpszClassName = CLASS_NAME;
    RegisterClassEx(&wx);
    if (!CreateWindowEx(0, CLASS_NAME, WINDOW_NAME, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL))
    {
        MessageBox(NULL, L"Failed to install hook!", L"Error", MB_ICONERROR);
        return 2;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hook);
    return 0;
}
