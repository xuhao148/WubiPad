// WubiPad.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "WubiPad.h"
#include "wubi.h"
#include "WbHenkanGUI.h"

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE			g_hInst;			// 当前实例
HWND				g_hWndMenuBar;		// 菜单栏句柄

// 此代码模块中包含的函数的前向声明:
ATOM			MyRegisterClass(HINSTANCE, LPTSTR);
BOOL			InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

// 注册表，存储码表位置
HKEY mainKey;

// File
wchar_t mbPath[256];
HANDLE mbFile;

// 主窗口：文本框，用于进行文字操作
HWND mainWindow;
HWND mainBox;

// 变换窗口
HWND henkanWindow;
HWND inputIndicator;
HWND candIndicator;

LOGFONT lfInput = {0};
LOGFONT lfCand = {0};
HFONT inputFont;
HFONT candFont;

BOOL msyhExists = FALSE;

int CALLBACK MSYHcallback(  const LOGFONT FAR* lpelf, 
							 const TEXTMETRIC FAR* lpntm,   DWORD FontType,   LPARAM lParam) {
	msyhExists = TRUE;
	return 0;
}

static void testAndGetCandFont() {
	msyhExists = FALSE;
	HDC deskDc = GetDC(NULL);
	EnumFontFamilies(deskDc, _T("微软雅黑"), MSYHcallback, NULL);
	lfCand.lfQuality = CLEARTYPE_QUALITY;
	if (msyhExists) {
		lfCand.lfHeight = -24;
		wsprintf(lfCand.lfFaceName, _T("微软雅黑"));
		lfCand.lfWeight = 300;
	} else {
		lfCand.lfHeight = -24;
		wsprintf(lfCand.lfFaceName, _T("SimSun"));
		lfCand.lfWeight = 600;
	}
	candFont = CreateFontIndirect(&lfCand);
}

static void ErrLoading(LPCWSTR err) {
	MessageBox(mainWindow,err,_T("错误"),MB_OK|MB_ICONHAND);
}

static void NullCallback(LPCWSTR err) {
	return;
}

static void pickMainBin(BOOL enforced) {
	OPENFILENAME ofn = {0};
	wchar_t buffer[128] = {0};
	wchar_t filter[] = L"码表文件(*.bin)\0*.BIN\0所有文件\0*.*\0\0";
	wchar_t title[] = L"选择码表文件";
	wchar_t ext[] = L"bin";
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = mainWindow;
	ofn.hInstance = g_hInst;
	ofn.lpstrFilter = filter;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = NULL;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = buffer;
	ofn.nMaxFile = 127;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = title;
	ofn.Flags = OFN_FILEMUSTEXIST;
	ofn.lpstrDefExt = ext;
	BOOL ret = FALSE;
	while (true) {
		ret = GetOpenFileName(&ofn);
		if (!ret) {
			if (enforced)
				exit(0);
			else {
				mb_init(mbPath, ErrLoading);
				return;
			}
		}
		int fail = mb_init(ofn.lpstrFile, ErrLoading);
		if (!fail) break;
	}
	wsprintf(mbPath, _T("%s"), ofn.lpstrFile);
	DWORD rserr = RegSetValueEx(mainKey,_T("MbPath"), NULL, REG_SZ, (LPBYTE)ofn.lpstrFile, 256);
	if (rserr != ERROR_SUCCESS) {
		MessageBox(mainWindow,_T("更新注册表失败！"),_T("错误"),MB_OK|MB_ICONHAND);
		exit(1);
	}
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR    lpCmdLine,
                   int       nCmdShow)
{
	HKEY hk;
	DWORD dispos;
	LONG ret = RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Ayachu\\WubiPad"), 0, NULL,
		REG_OPTION_NON_VOLATILE, 0, NULL, &hk, &dispos);
	if (ret != ERROR_SUCCESS) {
		MessageBox(NULL,_T("Failed to create registry entry!"), _T("Error"), MB_OK|MB_ICONHAND);
		return 0;
	} else {
		mainKey = hk;
	}


	MSG msg;

	// 执行应用程序初始化:
	if (!InitInstance(hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	DWORD bufferSize = 256;
	if (RegQueryValueEx(mainKey, _T("MbPath"), NULL, NULL, (LPBYTE)mbPath, &bufferSize) != ERROR_SUCCESS) {
		// No path in registry! Let user choose it.
		pickMainBin(TRUE);
	} else {
		int fail = mb_init(mbPath, NullCallback);
		if (fail) {
			wchar_t buf[128];
			wsprintf(buf, L"无法读取之前设置的码表(%s)。\r\n请重新设置。", mbPath);
			MessageBox(mainWindow, buf, _T("提示"), MB_OK|MB_ICONASTERISK);
			pickMainBin(TRUE);
		}
	}


	HACCEL hAccelTable;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WUBIPAD));

	// 主消息循环:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	RegCloseKey(mainKey);
	mb_finalize();

	return (int) msg.wParam;
}

//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
//  注释:
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WUBIPAD));
	wc.hCursor       = 0;
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;

	return RegisterClass(&wc);
}

const static wchar_t *kbdLine1[] = {L"Q",L"W",L"E",L"R",L"T",L"Y",L"U",L"I",L"O",L"P",L"BKSP"};
const static wchar_t *kbdLine2[] = {L"A",L"S",L"D",L"F",L"G",L"H",L"J",L"K",L"L",L"RETN"};
const static wchar_t *kbdLine3[] = {L"X",L"C",L"V",L"B",L"N",L"M",L"变换",L"空格"};
const static wchar_t *kbdLine4[] = {L"候选1",L"候选2",L"候选3",L"候选4",L"候选5",L"←",L"→",L"标点"};

static void createRowOfKeyAt(HWND parent, int x, int y, int cx, int cy, int baseid, const wchar_t **labels, int len) {
	for (int i = 0; i < len; i++) {
		CreateWindow(_T("BUTTON"), labels[i], WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, x, y, cx, cy, parent, (HMENU)(baseid+i), g_hInst, NULL);
		x += cx;
	}
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;
    TCHAR szTitle[MAX_LOADSTRING];		// 标题栏文本
    TCHAR szWindowClass[MAX_LOADSTRING];	// 主窗口类名

    g_hInst = hInstance; // 将实例句柄存储在全局变量中

    // 在应用程序初始化期间，应调用一次 SHInitExtraControls 以初始化
    // 所有设备特定控件，例如，CAPEDIT 和 SIPPREF。
    SHInitExtraControls();

    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING); 
    LoadString(hInstance, IDC_WUBIPAD, szWindowClass, MAX_LOADSTRING);

    //如果它已经在运行，则将焦点置于窗口上，然后退出
    hWnd = FindWindow(szWindowClass, szTitle);	
    if (hWnd) 
    {
        // 将焦点置于最前面的子窗口
        // “| 0x00000001”用于将所有附属窗口置于前台并
        // 激活这些窗口。
        SetForegroundWindow((HWND)((ULONG) hWnd | 0x00000001));
        return 0;
    } 

    if (!MyRegisterClass(hInstance, szWindowClass))
    {
    	return FALSE;
    }

	if (!WBRegisterClass(hInstance)) {
		return FALSE;
	}

    hWnd = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }

	mainWindow = hWnd;

	mainBox = CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL, WS_CHILD|WS_VISIBLE|ES_MULTILINE|WS_VSCROLL, 0, 0, 32, 32, mainWindow, NULL, g_hInst, NULL);

    // 使用 CW_USEDEFAULT 创建主窗口时，将不会考虑菜单栏的高度(如果创建了一个
    // 菜单栏)。因此，我们要在创建窗口后调整其大小
    // 如果菜单栏存在
    if (g_hWndMenuBar)
    {
        RECT rc;
        RECT rcMenuBar;

        GetWindowRect(hWnd, &rc);
        GetWindowRect(g_hWndMenuBar, &rcMenuBar);
        rc.bottom -= (rcMenuBar.bottom - rcMenuBar.top);
		
        MoveWindow(hWnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, FALSE);
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

	henkanWindow = CreateWindowEx(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,WBHK_CLASS, _T("五笔输入面板"), WS_VISIBLE|WS_CAPTION, 64, 64, 720, 320, NULL, NULL, hInstance, NULL);
	createRowOfKeyAt(henkanWindow, 6, 32+20, 64, 64, WBKB_Q, kbdLine1, 11);
	createRowOfKeyAt(henkanWindow, 6+30, 32+64+20, 64, 64, WBKB_A, kbdLine2, 10);
	createRowOfKeyAt(henkanWindow, 6+30+30+30+30, 32+64+64+20, 64, 64, WBKB_X, kbdLine3,8);
	createRowOfKeyAt(henkanWindow, 8, 32+64+64+64+4+20, (720-12)/8, 36, WBKB_CAND1, kbdLine4,8);
	inputIndicator = CreateWindowEx(WS_EX_STATICEDGE, _T("STATIC"), _T("_"), WS_CHILD|WS_VISIBLE, 6, 6, 96, 32, henkanWindow, NULL, g_hInst, NULL);
	candIndicator = CreateWindowEx(WS_EX_STATICEDGE, _T("STATIC"), _T(""), WS_CHILD|WS_VISIBLE, 6+96+10, 6, 720-6-6-96-10, 32, henkanWindow, NULL, g_hInst, NULL);
	
	lfInput.lfPitchAndFamily = FIXED_PITCH;
	lfInput.lfHeight = -24;
	lfInput.lfQuality = CLEARTYPE_QUALITY;
	wsprintf(lfInput.lfFaceName,_T("Courier New"));
	inputFont = CreateFontIndirect(&lfInput);
	SendMessage(inputIndicator, WM_SETFONT, (WPARAM)inputFont, MAKELPARAM(TRUE,0));

	testAndGetCandFont();
	SendMessage(candIndicator, WM_SETFONT, (WPARAM)candFont, MAKELPARAM(TRUE,0));
	SendMessage(mainBox, WM_SETFONT, (WPARAM)candFont, MAKELPARAM(TRUE,0));

	
	ShowWindow(henkanWindow, SW_SHOW);
	UpdateWindow(henkanWindow);

    return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: 处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    static SHACTIVATEINFO s_sai;
	
    switch (message) 
    {
        case WM_COMMAND:
            wmId    = LOWORD(wParam); 
            wmEvent = HIWORD(wParam); 
            // 分析菜单选择:
            switch (wmId)
            {
                case IDM_HELP_ABOUT:
                    DialogBox(g_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, About);
                    break;
                case IDM_OK:
                    SendMessage (hWnd, WM_CLOSE, 0, 0);				
                    break;
				case IDM_SHOWPAD:
					BringWindowToTop(henkanWindow);
					break;
				case IDM_CLEARTEXT:
					SetWindowText(mainBox, _T(""));
					break;
				case IDM_COPYTEXT:
					{
						int len = GetWindowTextLength(mainBox);
						if (!OpenClipboard(NULL)) {
							MessageBox(mainWindow, _T("无法打开剪贴板！"),_T("错误"),MB_OK|MB_ICONHAND);
							break;
						}
						EmptyClipboard();
						if (len == 0) {
							CloseClipboard();
							break;
						}
						HLOCAL hChunk = LocalAlloc(LPTR, (len+1)*2);
						if (hChunk == NULL) 
						{ 
							MessageBox(mainWindow, _T("分配内存（LocalAlloc）失败！"),_T("错误"),MB_OK|MB_ICONHAND);
							CloseClipboard(); 
							break;
						}
						GetWindowText(mainBox, (LPWSTR)hChunk, len+1);
						SetClipboardData(CF_UNICODETEXT, hChunk);
						CloseClipboard();
					}
					break;
				case IDM_CHANGEMB:
					pickMainBin(FALSE);
					break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_CREATE:
            SHMENUBARINFO mbi;

            memset(&mbi, 0, sizeof(SHMENUBARINFO));
            mbi.cbSize     = sizeof(SHMENUBARINFO);
            mbi.hwndParent = hWnd;
            mbi.nToolBarId = IDR_MENU;
            mbi.hInstRes   = g_hInst;

            if (!SHCreateMenuBar(&mbi)) 
            {
                g_hWndMenuBar = NULL;
            }
            else
            {
                g_hWndMenuBar = mbi.hwndMB;
            }

            // 初始化外壳程序激活信息结构
            memset(&s_sai, 0, sizeof (s_sai));
            s_sai.cbSize = sizeof (s_sai);
            break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            
            // TODO: 在此添加任意绘图代码...
            
            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
            CommandBar_Destroy(g_hWndMenuBar);
            PostQuitMessage(0);
            break;

        case WM_ACTIVATE:
            // 向外壳程序通知我们的激活消息
            SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
            break;
        case WM_SETTINGCHANGE:
            SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
            break;
		case WM_SIZE: {
			int cx = LOWORD(lParam);
			int cy = HIWORD(lParam);
			MoveWindow(mainBox, 0, 0, cx, cy, TRUE);
		}

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            {
                // 创建一个“完成”按钮并调整其大小。
                SHINITDLGINFO shidi;
                shidi.dwMask = SHIDIM_FLAGS;
                shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN | SHIDIF_EMPTYMENU;
                shidi.hDlg = hDlg;
                SHInitDialog(&shidi);
            }
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, message);
            return TRUE;

    }
    return (INT_PTR)FALSE;
}
