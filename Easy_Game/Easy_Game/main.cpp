#define Floor 935
#define ID_TM_MAINLOOP	1
#define ID_TM_ANIMATION 2

#include <windows.h>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID    CALLBACK MainLoopProc(HWND, UINT, UINT_PTR, DWORD);

HINSTANCE g_hInst;
HWND hWndMain;
LPSTR lpszClass = "Easy_Game";

typedef struct SURFACEINFOtag
{
	HDC			dcSurface;		
	HBITMAP		hBmp;			
	HBITMAP		hOldBmp;		
	int			nWidth;
	int			nHeight;
	COLORREF	crColorKey;		
} SURFACEINFO;

int g_nFrame = 0;					
int g_nMonIdx = 0;

typedef struct OBJ_MONtag
{
	SURFACEINFO	g_sf_Idle_Player[2];

	SURFACEINFO	g_sf_Left_Idle_Player[2];
	SURFACEINFO	g_sf_Right_Idle_Player[2];

	SURFACEINFO	g_sf_Left_Jump_Player[1];
	SURFACEINFO	g_sf_Right_Jump_Player[1];

	int			nAni;

	bool Is_Jump = FALSE;
	bool Is_Fall = FALSE;
	bool Is_Idle = TRUE;
} OBJ_MON;

SURFACEINFO g_sfBack;
SURFACEINFO g_sfBG;
OBJ_MON		g_objPlayer;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance
	, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW/*WS_SYSMENU | WS_CAPTION */ , //WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
		NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	hWndMain = hWnd;

	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return Message.wParam;
}

BOOL __CreateBackBuffer(HDC dcScreen, int nWidth, int nHeight, int nBPP, SURFACEINFO* psInfo)
{
	psInfo->dcSurface = CreateCompatibleDC(dcScreen);
	psInfo->hBmp = CreateCompatibleBitmap(dcScreen, nWidth, nHeight);
	psInfo->hOldBmp = (HBITMAP)SelectObject(psInfo->dcSurface, psInfo->hBmp);
	PatBlt(psInfo->dcSurface, 0, 0, nWidth, nHeight, PATCOPY);
	psInfo->nWidth = nWidth;
	psInfo->nHeight = nHeight;

	return TRUE;
}

void __LoadSurface(HDC dcScreen, SURFACEINFO* psInfo)
{
	BITMAP  bit;

	psInfo->dcSurface = CreateCompatibleDC(dcScreen);
	psInfo->hOldBmp = (HBITMAP)SelectObject(psInfo->dcSurface, psInfo->hBmp);

	GetObject(psInfo->hBmp, sizeof(BITMAP), &bit);
	psInfo->nWidth = bit.bmWidth;
	psInfo->nHeight = bit.bmHeight;
}

void __ReleaseSurface(SURFACEINFO* psInfo)
{
	SelectObject(psInfo->dcSurface, psInfo->hOldBmp);
	DeleteDC(psInfo->dcSurface);
	DeleteObject(psInfo->hBmp);
}

BOOL __PutImage(HDC dcDst, int x, int y, int nDstWidth, int nDstHeight, SURFACEINFO* psInfo)
{
	return StretchBlt(dcDst,        // 대상 DC
		x, y,           // 대상 시작 X, Y 좌표
		nDstWidth,      // 늘릴 너비
		nDstHeight,     // 늘릴 높이
		psInfo->dcSurface,  // 원본 DC
		0, 0,           // 원본 시작 X, Y 좌표
		psInfo->nWidth, // 원본 너비
		psInfo->nHeight, // 원본 높이
		SRCCOPY);
}

BOOL __PutImageBlend(HDC dcDst, int x, int y, SURFACEINFO* psInfo, int nAlpha)
{
	BLENDFUNCTION bf;

	//// BLENDFUNCTION 초기화
	bf.BlendOp = AC_SRC_OVER;	//
	bf.BlendFlags = 0;				//			
	bf.AlphaFormat = 0;				//
	bf.SourceConstantAlpha = nAlpha; // 0 - 255

	//// 알파 블렌드
	return 	AlphaBlend(dcDst,
		x, y, psInfo->nWidth, psInfo->nHeight,
		psInfo->dcSurface,
		0, 0, psInfo->nWidth, psInfo->nHeight,
		bf);
}

BOOL __PutSprite(HDC dcDst, int x, int y, int nDstWidth, int nDstHeight, SURFACEINFO* psInfo)
{
	return TransparentBlt(dcDst,
		x, y, psInfo->nWidth, psInfo->nHeight,
		psInfo->dcSurface,
		0, 0, psInfo->nWidth, psInfo->nHeight,
		psInfo->crColorKey);
}

BOOL __CompleteBlt(HDC dcScreen, SURFACEINFO* psInfo)
{
	BitBlt(dcScreen, 0, 0, g_sfBack.nWidth, g_sfBack.nHeight, psInfo->dcSurface, 0, 0, SRCCOPY);
	return TRUE;
}

HBITMAP __MakeDDBFromDIB(HDC hdc, char* Path)
{
	HANDLE  hFile;
	DWORD   FileSize, dwRead;
	HBITMAP hBit;
	BITMAPFILEHEADER* fh;
	BITMAPINFO* ih;
	BITMAPINFOHEADER* iheader;
	LPBYTE			 lpMemBlock;

	//// 파일을 연다
	hFile = CreateFile(Path, GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	FileSize = GetFileSize(hFile, NULL);

	lpMemBlock = (LPBYTE)malloc(FileSize);

	if (!lpMemBlock)
	{
		CloseHandle(hFile);
		return NULL;
	}

	fh = (BITMAPFILEHEADER*)lpMemBlock;
	ReadFile(hFile, lpMemBlock, FileSize, &dwRead, NULL);
	CloseHandle(hFile);

	ih = (BITMAPINFO*)(lpMemBlock + sizeof(BITMAPFILEHEADER));
	iheader = (BITMAPINFOHEADER*)(lpMemBlock + sizeof(BITMAPFILEHEADER));

	//// DDB로 변환한다.
	hBit = CreateDIBitmap(hdc,						// hdc
		iheader,					// BITMAPINFOHEADER 헤더
		CBM_INIT,					// 0 또는 CBM_INIT ( 초기화 )
		lpMemBlock + fh->bfOffBits,	// 래스터 어드래스
		ih,							// BITMAPINFO 헤더
		DIB_RGB_COLORS);

	free(lpMemBlock);

	return hBit;
}

void __SetImgSurface(SURFACEINFO* psfInfo)
{
	::memset(psfInfo, 0, sizeof(SURFACEINFO));
}

void __SetSprSurface(SURFACEINFO* psfInfo, COLORREF crColorKey)
{
	::memset(psfInfo, 0, sizeof(SURFACEINFO));
	psfInfo->crColorKey = crColorKey;
}

void __Init(HDC dcScreen)
{
	int i;
	char strName[24];

	__CreateBackBuffer(dcScreen, 1920, 1080, 32, &g_sfBack);

	__SetImgSurface(&g_sfBG);
	g_sfBG.hBmp = __MakeDDBFromDIB(dcScreen, "map.bmp");
	__LoadSurface(dcScreen, &g_sfBG);

	for (i = 0; i < 2; i++)
	{
		__SetImgSurface(&(g_objPlayer.g_sf_Right_Idle_Player[i]));
		wsprintf(strName, "Player_Right_Idle_%d.bmp", i + 1);
		g_objPlayer.g_sf_Idle_Player[i].hBmp = __MakeDDBFromDIB(dcScreen, strName);
		__LoadSurface(dcScreen, &(g_objPlayer.g_sf_Idle_Player[i]));
	}

	for (i = 0; i < 2; i++)
	{
		__SetImgSurface(&(g_objPlayer.g_sf_Left_Idle_Player[i]));
		wsprintf(strName, "Player_Left_Idle_%d.bmp", i + 1);
		g_objPlayer.g_sf_Idle_Player[i].hBmp = __MakeDDBFromDIB(dcScreen, strName);
		__LoadSurface(dcScreen, &(g_objPlayer.g_sf_Idle_Player[i]));
	}

	__SetImgSurface(&(g_objPlayer.g_sf_Right_Jump_Player[0]));
	wsprintf(strName, "Player_Right_Jump.bmp");
	g_objPlayer.g_sf_Right_Jump_Player[0].hBmp = __MakeDDBFromDIB(dcScreen, strName);
	__LoadSurface(dcScreen, &(g_objPlayer.g_sf_Right_Jump_Player[0]));

	__SetImgSurface(&(g_objPlayer.g_sf_Left_Jump_Player[0]));
	wsprintf(strName, "Player_Left_Jump.bmp");
	g_objPlayer.g_sf_Left_Jump_Player[0].hBmp = __MakeDDBFromDIB(dcScreen, strName);
	__LoadSurface(dcScreen, &(g_objPlayer.g_sf_Left_Jump_Player[0]));
}

void __DestroyAll()
{
	int i;

	for (i = 0; i < 2; i++)
	{
		__ReleaseSurface(&(g_objPlayer.g_sf_Idle_Player[i]));
	}

	__ReleaseSurface(&g_sfBG);
	__ReleaseSurface(&g_sfBack);
}

static float x = 100;
static float y = 100;

static float f_Temp_x = 0;
static float f_Temp_y = 0;

void CALLBACK MainLoopProc(HWND hWnd, UINT message, UINT_PTR iTimerID, DWORD dwTime)
{
	HDC   dcScreen;
	BOOL  bRval;

	//// 연산부

	

	if (f_Temp_y < y)
	{
		g_objPlayer.Is_Fall = TRUE;
		g_objPlayer.Is_Idle = FALSE;
		g_objPlayer.Is_Jump = TRUE;
	}
	else if (f_Temp_y > y)
	{
		g_objPlayer.Is_Fall = FALSE;
		g_objPlayer.Is_Idle = FALSE;
		g_objPlayer.Is_Jump = TRUE;
	}
	else if (f_Temp_y == y)
	{
		g_objPlayer.Is_Fall = FALSE;
		g_objPlayer.Is_Idle = TRUE;
		g_objPlayer.Is_Jump = FALSE;
	}

	f_Temp_x = x;
	f_Temp_y = y;

	
	//// 출력부
	dcScreen = GetDC(hWnd);
	{
		//// 배경
		__PutImage(g_sfBack.dcSurface, 0, 0, g_sfBack.nWidth, g_sfBack.nHeight, &g_sfBG);

		// 오브젝트 및 기타 인터페이스창
		bRval = __PutSprite(g_sfBack.dcSurface, x, y , 100, 100, &(g_objPlayer.g_sf_Idle_Player[g_objPlayer.nAni]));
		if (!bRval)	::OutputDebugString("__PutSprite - fail");

		if (g_objPlayer.Is_Fall == TRUE)
		{
			TextOut(g_sfBack.dcSurface, 10, 10, "Is_Fall = TRUE", strlen("Is_Fall = TRUE"));
		}
		else if (g_objPlayer.Is_Fall == FALSE)
		{
			TextOut(g_sfBack.dcSurface, 10, 10, "Is_Fall = FALSE", strlen("Is_Fall = FALSE"));
		}

		if (g_objPlayer.Is_Idle == TRUE)
		{
			TextOut(g_sfBack.dcSurface, 10, 30, "Is_Idle = TRUE", strlen("Is_Idle = TRUE"));
		}
		else if (g_objPlayer.Is_Idle == FALSE)
		{
			TextOut(g_sfBack.dcSurface, 10, 30, "Is_Idle = FALSE", strlen("Is_Idle = FALSE"));
		}

		if (g_objPlayer.Is_Jump == TRUE)
		{
			TextOut(g_sfBack.dcSurface, 10, 50, "Is_Jump = TRUE", strlen("Is_Jump = TRUE"));
		}
		else if (g_objPlayer.Is_Jump == FALSE)
		{
			TextOut(g_sfBack.dcSurface, 10, 50, "Is_Jump = FALSE", strlen("Is_Jump = FALSE"));
		}

		//// 출력 완료
		__CompleteBlt(dcScreen, &g_sfBack);
	}
	ReleaseDC(hWnd, dcScreen);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC dcScreen;

	switch (iMessage)
	{
	case WM_CREATE:
		//// 표면 생성 및 기타
		dcScreen = GetDC(hWnd);
		__Init(dcScreen);
		ReleaseDC(hWnd, dcScreen);

		////
		g_objPlayer.nAni = 0;

		////
		::Sleep(100);

		//// 타이머 생성		
		// 60 FPS ( 1000ms / 60fps = 16.6ms )
		// 정밀도 NT : 10ms ( 100 fps )
		//        98 : 55ms (  18 fps )
		SetTimer(hWnd, ID_TM_MAINLOOP, 16, MainLoopProc);
		SetTimer(hWnd, ID_TM_ANIMATION, 160, NULL);

		return 0;

	case WM_SETFOCUS:
		::OutputDebugString("WM_SETFOCUS");
		return 0;

	case WM_TIMER:
		if (wParam == ID_TM_ANIMATION)
		{
			//// 애니메이션 인덱스
			g_objPlayer.nAni++;
			if (g_objPlayer.nAni >= 2) g_objPlayer.nAni = 0;
		}
		return 0;

	case WM_DESTROY:
		KillTimer(hWnd, ID_TM_MAINLOOP);
		KillTimer(hWnd, ID_TM_ANIMATION);
		__DestroyAll();
		PostQuitMessage(0);
		return 0;
	}

	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}