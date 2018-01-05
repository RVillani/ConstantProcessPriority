// ConstantProcessPriority.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "ConstantProcessPriority.h"
#include "PriorityClassControl.h"
#include <shellapi.h>
#include <windowsx.h>
#include "SettingsManager.h"

#define MAX_LOADSTRING 100
#define WM_UPDATESTATUS WM_USER
#define TRAY_ICON_ID 0xFE10
#define TRAY_ICON_MESSAGE WM_APP
#define PROCESS_NAMES_LENGTH 1024

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
WCHAR szNextStatusMessage[250];					// Status message to set after WM_UPDATESTATUS
bool bMinimizedToTray = false;

SettingsManager settingsFile;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

void SetupDialog(HWND hWnd);
void UpdateDialog(HWND hWnd);
void UpdateSettings(HWND hWnd);
void SaveSettingsToFile();

void MinimizeToTray(HWND hWnd);
void RestoreFromTray(HWND hWnd);
void RemoveTrayIcon(HWND hWnd);
BOOL ShowContextMenu(HWND hWnd, LPARAM lParam);

void SetStatusMessage(HWND hWnd, const wchar_t *Message, UINT clearTimer = 6000);
void AddStatusMessage(const wchar_t *Message);

// Priority class alternatives
wchar_t PriorityNames[][16] = {
	TEXT("Low"),
	TEXT("Below Normal"),
	TEXT("Normal"),
	TEXT("Above Normal"),
	TEXT("High"),
	TEXT("Realtime")
};

PriorityClassControl ProcessesControl;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
	
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);

	//-- Windows Win32 window stuff -----------------------------------------//

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDS_MAINWINDOWCLASS, szWindowClass, MAX_LOADSTRING);

	// Registers the window class
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = DLGWINDOWEXTRA;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CONSTANTPROCESSPRIORITY));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_MENUBAR + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_CONSTANTPROCESSPRIORITY));

	if (!RegisterClassExW(&wcex)) return GetLastError();

	// Perform application initialization:
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateDialogParam(
		nullptr,
		MAKEINTRESOURCE(IDD_MAINDIALOG),
		nullptr,
		(DLGPROC)WndProc,
		0x12345678
	);

	if (!hWnd)
	{
		return GetLastError();
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);


	//-- Processes control --------------------------------------------------//

	ProcessesControl.MessagesCallback = &AddStatusMessage;
	int argc;
	wchar_t **argv = CommandLineToArgvW(lpCmdLine, &argc);

	// try loading settings from settings file
	bool bLoadedPrioritySettings, bLoadedMinimizedState;
	settingsFile.LoadSettings(bLoadedPrioritySettings, bLoadedMinimizedState);

	// if there are command-line arguments, use them. They're priority
	if (argc >= 3 && lstrcmpW(argv[0], L"-set") == 0)
	{
		ProcessesControl.Setup(argv[1], argv[2]);
		UpdateDialog(hWnd);
		SetStatusMessage(hWnd, L"Settings set from command line");
		SaveSettingsToFile();
	}
	else if (bLoadedPrioritySettings)
	{
		ProcessesControl.Setup(settingsFile.Names.c_str(), settingsFile.Priority);

		UpdateDialog(hWnd);
		SetStatusMessage(hWnd, L"Settings loaded from file");
	}

	// loaded minimized settings?
	if (bLoadedMinimizedState && settingsFile.bStartMinimized)
	{
		MinimizeToTray(hWnd);
	}


	//-- Main message loop --------------------------------------------------//
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		// reduce processor usage when on tray
		if (bMinimizedToTray)
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	return (int)msg.wParam;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
    {
	case TRAY_ICON_MESSAGE:
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
			RestoreFromTray(hWnd);
			break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			if (!ShowContextMenu(hWnd, lParam))
				return DefWindowProc(hWnd, message, wParam, lParam);
			break;
		}
		break;
	case WM_UPDATESTATUS:
		SetStatusMessage(hWnd, szNextStatusMessage);
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_SET:
			UpdateSettings(hWnd);
			break;
		case ID_POPUP_RESTOREWINDOW:
			RestoreFromTray(hWnd);
			break;
		case ID_POPUP_EXIT:
			RemoveTrayIcon(hWnd);
			DestroyWindow(hWnd);
			break;
		}
		break;
	case WM_INITDIALOG:
		SetupDialog(hWnd);
		break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_SYSCOMMAND:
		switch (wParam & 0xFFF0)
		{
		case SC_MINIMIZE:
			MinimizeToTray(hWnd);
			settingsFile.bStartMinimized = true;
			settingsFile.SaveSettings();
			return 0;
			break;
		}
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Creates the combo box's options, set it to "Above Normal" and clears the status message
void SetupDialog(HWND hWnd)
{
	// Empty message
	SetStatusMessage(hWnd, L"");

	// Load alternatives into the combo box and select highest one by default
	TCHAR A[16];
	memset(&A, 0, sizeof(A));
	HWND hComboBox = GetDlgItem(hWnd, IDC_COMBO_PRIORITY);

	for (UINT i = 0; i < sizeof(PriorityNames) / sizeof(A); ++i)
	{
		wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)PriorityNames[i]);
		SendMessage(hComboBox, (UINT)CB_INSERTSTRING, (WPARAM)i, (LPARAM)A);
	}
	SendMessage(hComboBox, CB_SETCURSEL, (WPARAM)2, (LPARAM)0);
}

/* Display the settings from ProcessesControl object.
Used to display settings when set by other means than the dialog/user input */
void UpdateDialog(HWND hWnd)
{
	std::wstring Names; int Priority;
	ProcessesControl.GetSettings(Names, Priority);
	SetDlgItemText(hWnd, IDC_EDIT_NAMES, Names.c_str());
	SendMessage(GetDlgItem(hWnd, IDC_COMBO_PRIORITY), CB_SETCURSEL, (WPARAM)Priority, (LPARAM)0);
}

// Called to update the worker thread's settings with the dialog settings
void UpdateSettings(HWND hWnd)
{
	wchar_t Names[PROCESS_NAMES_LENGTH];
	GetDlgItemText(hWnd, IDC_EDIT_NAMES, Names, PROCESS_NAMES_LENGTH);
	int Priority = (int)SendMessage(GetDlgItem(hWnd, IDC_COMBO_PRIORITY), CB_GETCURSEL, 0, 0);

	ProcessesControl.Setup(Names, Priority);
	SetStatusMessage(hWnd, L"Settings updated", 3000);
	SaveSettingsToFile();
}

// Store current ProcessesControl settings on a settings file
void SaveSettingsToFile()
{
	ProcessesControl.GetSettings(settingsFile.Names, settingsFile.Priority);
	settingsFile.SaveSettings();
}

//-- TRAY ICON --------------------------------------------------------------//

// Hides the main window and display an icon on the tray area
void MinimizeToTray(HWND hWnd)
{
	NOTIFYICONDATA niData;
	memset(&niData, 0, sizeof(NOTIFYICONDATA));
	niData.cbSize = NOTIFYICONDATA_V1_SIZE;
	niData.uID = TRAY_ICON_ID;
	niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	wcscpy_s(niData.szTip, L"Constant Process Priority");
	niData.hIcon = (HICON)LoadImage(GetModuleHandle(NULL),
			MAKEINTRESOURCE(IDI_CONSTANTPROCESSPRIORITY),
		IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR);
	niData.hWnd = hWnd;
	niData.uCallbackMessage = TRAY_ICON_MESSAGE;
	
	Shell_NotifyIcon(NIM_ADD, &niData);
	ShowWindow(hWnd, SW_HIDE);
	DestroyIcon(niData.hIcon);

	SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN);

	bMinimizedToTray = true;
}

// Destroys the tray area icon and restores the main window
void RestoreFromTray(HWND hWnd)
{
	RemoveTrayIcon(hWnd);
	ShowWindow(hWnd, SW_RESTORE);
	bMinimizedToTray = false;

	SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_END);

	settingsFile.bStartMinimized = false;
	settingsFile.SaveSettings();
}

// Destroy the tray area icon
void RemoveTrayIcon(HWND hWnd)
{
	NOTIFYICONDATA niData;
	memset(&niData, 0, sizeof(NOTIFYICONDATA));
	niData.cbSize = NOTIFYICONDATA_V1_SIZE;
	niData.uID = TRAY_ICON_ID;
	niData.hWnd = hWnd;

	Shell_NotifyIcon(NIM_DELETE, &niData);
}

// Display context menu for the tray icon
BOOL ShowContextMenu(HWND hWnd, LPARAM lParam)
{
	POINT pt;
	//ClientToScreen(hWnd, &pt);
	GetCursorPos(&pt);

	HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_POPUPMENU));
	if (!hMenu) return FALSE;

	hMenu = GetSubMenu(hMenu, 0);
	if (!hMenu) return FALSE;

	SetForegroundWindow(hWnd);
	TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
	DestroyMenu(hMenu);

	return TRUE;
}


//-- STATUS MESSAGES --------------------------------------------------------//

void CALLBACK ClearStatusTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	SetDlgItemText(hwnd, IDC_STATIC_MESSAGES, L"");
}

void SetStatusMessage(HWND hWnd, const wchar_t *Message, UINT clearTimer)
{
	SetDlgItemText(hWnd, IDC_STATIC_MESSAGES, Message);
	SetTimer(hWnd, 1, clearTimer, &ClearStatusTimerProc);
}

void AddStatusMessage(const wchar_t *Message)
{
	wcscpy_s(szNextStatusMessage, Message);
	SendMessage(GetActiveWindow(), WM_UPDATESTATUS, 0, 0);
}