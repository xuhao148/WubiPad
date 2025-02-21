#pragma once

#define WBHK_CLASS (_T("appWBHKGUI"))

enum {
	WBKB_Q = 201,
	WBKB_W,
	WBKB_E,
	WBKB_R,
	WBKB_T,
	WBKB_Y,
	WBKB_U,
	WBKB_I,
	WBKB_O,
	WBKB_P,
	WBKB_BKSP,
	WBKB_A,
	WBKB_S,
	WBKB_D,
	WBKB_F,
	WBKB_G,
	WBKB_H,
	WBKB_J,
	WBKB_K,
	WBKB_L,
	WBKB_ENTR,
	WBKB_X,
	WBKB_C,
	WBKB_V,
	WBKB_B,
	WBKB_N,
	WBKB_M,
	WBKB_HENKAN,
	WBKB_SPC,
	WBKB_CAND1,
	WBKB_CAND2,
	WBKB_CAND3,
	WBKB_CAND4,
	WBKB_CAND5,
	WBKB_PREVCANDS,
	WBKB_NEXTCANDS,
	WBKB_PUNCT
};

ATOM WBRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK WBWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);