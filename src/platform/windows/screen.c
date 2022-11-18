#include "windows.h"

#define TRANSPARENT_COLOR RGB(0,0,1)
#define AcquireMutex(mtx) assert(WaitForSingleObject(mtx, INFINITE) == WAIT_OBJECT_0)

static DWORD ui_thread_id;
static HANDLE mtx;

struct box {
	RECT rect;
	COLORREF color;
};

struct screen {
	/* Position in the virtual screen space */
	int x;
	int y;
	int w;
	int h;
	
	struct hint hints[4096];
	struct box boxes[128];

	size_t nboxes;
	size_t nhints;

	HANDLE mtx;
	HWND overlay;
	HDC dc;
};

static COLORREF hint_bgcol;
static COLORREF hint_fgcol;

static struct screen screens[16];
static size_t nscreens = 0;

static void draw_hints(struct screen *scr)
{
	size_t i;

	HBRUSH bgbrush = CreateSolidBrush(hint_bgcol);

	SetBkColor(scr->dc, hint_bgcol);
	SetTextColor(scr->dc, hint_fgcol);


	//TODO: font should fill the box.

	for (i = 0; i < scr->nhints; i++) {
		RECT rect;
		wchar_t label[64];
		struct hint *h = &scr->hints[i];

		rect.left = h->x;
		rect.top = h->y;
		rect.right = h->x+h->w;
		rect.bottom = h->y+h->h;

		FillRect(scr->dc, &rect, bgbrush);
		//FIXME
		//mbstowcs (label, h->label, sizeof label / sizeof label[0] - 1);
		//label[sizeof label / sizeof label[0] - 1] = 0;

		DrawText(scr->dc, h->label, -1, &rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
	}

	DeleteObject(bgbrush);
}

static void clear(struct screen *scr)
{
	RECT rect;
	static HBRUSH br = 0;

	if (!br)
		br = CreateSolidBrush(TRANSPARENT_COLOR);

	rect.left = scr->x;
	rect.top = scr->y;
	rect.right = scr->x+scr->w;
	rect.bottom = scr->y+scr->h;

	FillRect(scr->dc,  &rect, br);
}


static void redraw(struct screen *scr)
{
	size_t i;

	clear(scr);

	for (i = 0; i < scr->nboxes; i++) {
		HBRUSH brush = CreateSolidBrush(scr->boxes[i].color); //TODO: optimize
		FillRect(scr->dc, &scr->boxes[i].rect, brush);
		DeleteObject(brush);
	}

	draw_hints(scr);
}

LRESULT CALLBACK OverlayRedrawProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_PAINT) {
		size_t i;
		for (i = 0; i < nscreens; i++) {
			if (screens[i].overlay == hWnd)
				redraw(&screens[i]);
		}
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

static HWND create_overlay(int x, int y, int w, int h)
{
	static WNDCLASS wc = {0};
	static int init = 0;

	const char CLASS_NAME[] = "warpd";

	HINSTANCE hInstance = GetModuleHandle(NULL);

	if (!init) {
		wc.lpfnWndProc = OverlayRedrawProc; // Window callback function
		wc.hInstance = hInstance;
		wc.lpszClassName = CLASS_NAME;

		RegisterClass(&wc);
	}

	HWND wnd = CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST |
				      WS_EX_LAYERED, // Optional window styles.
				  CLASS_NAME,	     // Window class
				  "",		     // Window text
				  WS_POPUP,	     // Window style

				  // Size and position
				  x, y, w, h,

				  NULL,	     // Parent window
				  NULL,	     // Menu
				  hInstance, // Instance handle
				  NULL	     // Additional application data
	);

	assert(wnd);

	SetLayeredWindowAttributes(wnd, TRANSPARENT_COLOR, 0, LWA_COLORKEY);
	return wnd;
}

static BOOL CALLBACK screenCallback(HMONITOR mon, HDC hdc, LPRECT dim, LPARAM lParam)
{
AcquireMutex(mtx);

    assert(nscreens < sizeof screens / sizeof screens[0]);

    struct screen *scr = &screens[nscreens++];

    scr->x = dim->top;
    scr->y = dim->left;
    scr->h = dim->bottom - dim->top;
    scr->w = dim->right - dim->left;

    scr->nboxes = 0;
    scr->nhints = 0;

    scr->overlay = create_overlay(scr->x, scr->y, scr->w, scr->h);
    scr->dc = GetDC(scr->overlay);


    ShowWindow(scr->overlay, SW_SHOW);

ReleaseMutex(mtx);

    return TRUE;
}

/* Main draw loop. */

static DWORD WINAPI uithread(void *arg)
{
	EnumDisplayMonitors(0, 0, screenCallback, 0);

	while (1) {
		MSG msg;
		GetMessage(&msg, 0, 0, 0);

AcquireMutex(mtx);

		DispatchMessage(&msg);

		if (msg.message == WM_USER)
			redraw((struct screen *)msg.lParam);

ReleaseMutex(mtx);
	}
}

/* internal screen API */

void wn_screen_redraw(struct screen *scr)
{
	PostThreadMessage(ui_thread_id, WM_USER, 0, (LPARAM)scr);
}

void wn_screen_set_hints(struct screen *scr, struct hint *hints, size_t nhints)
{
AcquireMutex(mtx);

	assert(nhints < sizeof scr->hints / sizeof scr->hints[0]);
	memcpy(scr->hints, hints, sizeof(struct hint) * nhints);
	scr->nhints = nhints;

ReleaseMutex(mtx);
}

void wn_screen_get_dimensions(struct screen *scr, int *xoff, int *yoff, int *w, int *h)
{
	if (xoff) *xoff = scr->x;
	if (yoff) *yoff = scr->y;
	if (w) *w = scr->w;
	if (h) *h = scr->h;
}

void wn_screen_add_box(struct screen *scr, int x, int y, int w, int h, COLORREF color)
{
AcquireMutex(mtx);

	assert(scr->nboxes < sizeof scr->boxes / sizeof scr->boxes[0]);

	struct box *box = &scr->boxes[scr->nboxes];

	box->rect.top = y;
	box->rect.left = x;
	box->rect.right = x + w;
	box->rect.bottom = y + h;

	box->color = color;
	scr->nboxes++;

ReleaseMutex(mtx);
}

void wn_screen_clear(struct screen *scr)
{
AcquireMutex(mtx);

	scr->nboxes = 0;
	scr->nhints = 0;

ReleaseMutex(mtx);
}

void wn_screen_set_hintinfo(COLORREF _hint_bgcol, COLORREF _hint_fgcol)
{
	hint_bgcol = _hint_bgcol;
	hint_fgcol = _hint_fgcol;
}

void wn_init_screen()
{
	mtx = CreateMutex(0, 0, NULL);

	CreateThread(0, 0, uithread, 0, 0, &ui_thread_id);
	Sleep(200); //FIXME
}

struct screen *wn_get_screen_at(int x, int y)
{
	size_t i;

	for (i = 0; i < nscreens; i++) {
		if (
			x >= screens[i].x && x <= screens[i].x + screens[i].w &&
			y >= screens[i].y && y <= screens[i].y + screens[i].h
		)
			return &screens[i];
	}

	return NULL;
}
