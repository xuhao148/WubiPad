#include "stdafx.h"
#include "WbHenkanGUI.h"
#include "WubiPad.h"
#include "wubi.h"

extern HWND henkanWindow;
extern HWND inputIndicator;
extern HWND candIndicator;
extern HWND mainBox;

char code1[] = {"qwertyuiop"};
char code2[] = {"asdfghjkl"};
char code3[] = {"xcvbnm"};

wchar_t indicatorBuf[6] = {0};
wchar_t candidateBuf[50*5] = {0};
wchar_t candidateSmallBuf[48] = {0};
int cursor = 0;
char buffer_codes[4] = {0};
char translated_codes[4] = {'_','_','_','_'};
BOOL showing_candidate = FALSE;
BOOL candidate_already_chosen = FALSE;
int pgno = 0;
int n_candidates = 0;
int n_thispage = 0;

struct s_mb_entry *head;
struct s_mb_entry *this_page;


static void drawCandidates() {
	candidateBuf[0] = 0;
	for (int i = 0; i < n_thispage; i++) {
		LPCWSTR conv = mb_getconv(&this_page[i]);
		wsprintf(candidateSmallBuf,_T("%d: %s "), i+1, conv);
		wcscat(candidateBuf,candidateSmallBuf);
	}
	wsprintf(candidateSmallBuf,_T("(%d/%d)"), pgno+1, n_candidates/5+!!(n_candidates % 5));
	wcscat(candidateBuf,candidateSmallBuf);
	SetWindowText(candIndicator, candidateBuf);
}

static void pick(int idx) {
	if (!showing_candidate || idx >= n_candidates) {
		PlaySound(_T("SystemExit"), NULL, 0);
		return;
	}
	LPCWSTR conv = mb_getconv(this_page+idx);
	SendMessage(mainBox, EM_REPLACESEL, TRUE, (LPARAM)conv);
	// cursor = 0;
	// indicatorBuf[0] = '_';
	// indicatorBuf[1] = 0;
	// SetWindowText(inputIndicator, indicatorBuf);
	candidate_already_chosen = TRUE;
	// showing_candidate = FALSE;
	// SetWindowText(candIndicator, _T(""));
}

static void henkan() {
	if (cursor <= 0) {
		PlaySound(_T("SystemExit"), NULL, 0);
		return;
	}
	if (showing_candidate) {
		PlaySound(_T("SystemExit"), NULL, 0);
		return;
	}
	for (int i = cursor; i < 4; i++) {
		buffer_codes[i] = 0;
	}
	for (int i = 0; i < 4; i++) {
		if (buffer_codes[i] >= 'a' && buffer_codes[i] <= 'z')
			translated_codes[i] = buffer_codes[i];
		else
			translated_codes[i] = '_';
	}
	head = mb_findfirst(translated_codes);
	if (head == NULL) {
		PlaySound(_T("SystemExit"), NULL, 0);
		cursor = 0;
		indicatorBuf[0] = '_';
		indicatorBuf[1] = 0;
		SetWindowText(inputIndicator, indicatorBuf);
		return;
	}
	int available = mb_getbound() - head;
	n_candidates = 0;
	while (n_candidates < available && !memcmp(head[n_candidates].code, translated_codes, 4))
		n_candidates++;
	this_page = head;
	if (n_candidates == 1 || cursor == 1) {
		//  ’¼Úã› 
		LPCWSTR conv = mb_getconv(head);
		SendMessage(mainBox, EM_REPLACESEL, TRUE, (LPARAM)conv);
		cursor = 0;
		indicatorBuf[0] = '_';
		indicatorBuf[1] = 0;
		SetWindowText(inputIndicator, indicatorBuf);
	} else {
		this_page = head;
		n_thispage = n_candidates;
		if (n_thispage > 5) n_thispage = 5;
		pgno = 0;
		drawCandidates();
		showing_candidate = TRUE;
		candidate_already_chosen = FALSE;
	}
}


static void candOp(int cmd) {
	if (cmd == WBKB_PUNCT) {
		showing_candidate = FALSE;
		candidate_already_chosen = FALSE;
		buffer_codes[0] = 'c';
		buffer_codes[1] = 'o';
		buffer_codes[2] = 'b';
		buffer_codes[3] = 'd';
		indicatorBuf[0] = 'c';
		indicatorBuf[1] = 'o';
		indicatorBuf[2] = 'b';
		indicatorBuf[3] = 'd';
		indicatorBuf[4] = '_';
		cursor = 4;
		indicatorBuf[5] = 0;
		SetWindowText(inputIndicator,indicatorBuf);
		henkan();
		candidate_already_chosen = TRUE;
		return;
	}
	if (!showing_candidate) {
		if (cmd == WBKB_PREVCANDS) {
			SendMessage(mainBox, WM_KEYDOWN, VK_LEFT, 0);
			return;
		} else if (cmd == WBKB_NEXTCANDS) {
			SendMessage(mainBox, WM_KEYDOWN, VK_RIGHT, 0);
			return;
		} else {
			PlaySound(_T("SystemExit"), NULL, 0);
			return;
		}
	}
	int pgmax = n_candidates / 5 + !!(n_candidates % 5);
	if (cmd <= WBKB_CAND5) {
		pick(cmd - WBKB_CAND1);
	} else if (cmd == WBKB_NEXTCANDS) {
		if (pgno + 1 >= pgmax) {
			PlaySound(_T("SystemExit"), NULL, 0);
			return;
		} else {
			pgno++;
			this_page += 5;
			n_thispage = n_candidates - pgno * 5;
			if (n_thispage > 5) n_thispage = 5;
			drawCandidates();
		}
	} else if (cmd == WBKB_PREVCANDS) {
		if (pgno <= 0) {
			PlaySound(_T("SystemExit"), NULL, 0);
			return;
		} else {
			pgno--;
			this_page -= 5;
			n_thispage = n_candidates - pgno * 5;
			if (n_thispage > 5) n_thispage = 5;
			drawCandidates();
		}
	}
}



static void clear_cand() {
	showing_candidate = FALSE;
	candidate_already_chosen = FALSE;
	SetWindowText(candIndicator, _T(""));
	cursor = 0;
	buffer_codes[cursor] = 0;
	indicatorBuf[0] = '_';
	indicatorBuf[1] = '\0';
	SetWindowText(inputIndicator,indicatorBuf);
}

static void del() {
	if (showing_candidate) {
		if (!candidate_already_chosen) {
			showing_candidate = FALSE;
			SetWindowText(candIndicator, _T(""));
		} else {
			clear_cand();
			SendMessage(mainBox, WM_CHAR, VK_BACK, 0);
			return;
		}
	}
	if (cursor <= 0) {
		SendMessage(mainBox, WM_CHAR, VK_BACK, 0);
		return;
	}
	cursor--;
	buffer_codes[cursor] = 0;
	for (int i = 0; i < cursor; i++) {
		indicatorBuf[i] = buffer_codes[i];
	}
	indicatorBuf[cursor] = L'_';
	indicatorBuf[cursor+1] = 0;
	SetWindowText(inputIndicator,indicatorBuf);
}

static void type(char code) {
	if (showing_candidate) {
		if (candidate_already_chosen) {
			clear_cand();
		} else {
			pick(0);
			clear_cand();
		}
	}
	if (cursor >= 4) {
		PlaySound(_T("SystemExit"), NULL, 0);
		return;
	}
	buffer_codes[cursor] = code;
	cursor++;
	for (int i = 0; i < cursor; i++) {
		indicatorBuf[i] = buffer_codes[i];
	}
	indicatorBuf[cursor] = L'_';
	indicatorBuf[cursor+1] = 0;
	SetWindowText(inputIndicator,indicatorBuf);
	if (cursor >= 4)
		henkan();
}



LRESULT CALLBACK WBWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
	switch (message) {
		case WM_CTLCOLORSTATIC:
			if ((HWND)lParam == inputIndicator)
				return (COLOR_WINDOW+1);
			else
				return DefWindowProc(hWnd, message, wParam, lParam);
			break;
		case WM_COMMAND:
			{
				int msgid  = HIWORD(wParam);
				if (msgid == BN_CLICKED) {
					int idButton = LOWORD(wParam);
					if (idButton >= WBKB_Q && idButton <= WBKB_P) {
						type(code1[idButton - WBKB_Q]);
					} else if (idButton >= WBKB_A && idButton <= WBKB_L) {
						type(code2[idButton - WBKB_A]);
					} else if (idButton >= WBKB_X && idButton <= WBKB_M) {
						type(code3[idButton - WBKB_X]);
					} else if (idButton == WBKB_BKSP) {
						del();
					} else if (idButton == WBKB_HENKAN) {
						henkan();
					} else if (idButton >= WBKB_CAND1) {
						candOp(idButton);
					} else if (idButton == WBKB_ENTR) {
						if (cursor > 0) {
							if (!showing_candidate)
								henkan();
							else if (candidate_already_chosen)
								SendMessage(mainBox, WM_CHAR, VK_RETURN, 0);
							else
								pick(0);
						}
						else
							SendMessage(mainBox, WM_CHAR, VK_RETURN, 0);
					} else if (idButton == WBKB_SPC) {
						if (showing_candidate && n_thispage >= 1)
							pick(0);
						else if (cursor > 0)
							henkan();
						else
							SendMessage(mainBox, WM_CHAR, VK_SPACE, 0);
					}
				}
			}
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	
}

ATOM WBRegisterClass(HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WBWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WUBIPAD));
	wc.hCursor       = 0;
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = WBHK_CLASS;

	return RegisterClass(&wc);
}