#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#define Floor 935
#include <windows.h>
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID    CALLBACK MainLoopProc(HWND, UINT, UINT_PTR, DWORD);

HINSTANCE g_hInst;
HWND hWndMain;
LPSTR lpszClass = "[ MemoryDC Buffering (Double Buffering) ]";

#define ID_TM_MAINLOOP	1
#define ID_TM_ANIMATION 2

bool Is_Jump = FALSE;
bool Is_Idle = TRUE;

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
	SURFACEINFO	g_sfPlayer[2];
	int			nAni;
	int			nLife;
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

/*
BOOL __PutImage(HDC dcDst, int x, int y, SURFACEINFO* psInfo)
{
	return BitBlt(dcDst, x, y, psInfo->nWidth, psInfo->nHeight, psInfo->dcSurface, 0, 0, SRCCOPY);
}
*/

BOOL __PutImage(HDC dcDst, int x, int y, int nDstWidth, int nDstHeight, SURFACEINFO* psInfo)
{
	return StretchBlt(dcDst,        // ��� DC
		x, y,           // ��� ���� X, Y ��ǥ
		nDstWidth,      // �ø� �ʺ�
		nDstHeight,     // �ø� ����
		psInfo->dcSurface,  // ���� DC
		0, 0,           // ���� ���� X, Y ��ǥ
		psInfo->nWidth, // ���� �ʺ�
		psInfo->nHeight, // ���� ����
		SRCCOPY);
}

BOOL __PutImageBlend(HDC dcDst, int x, int y, SURFACEINFO* psInfo, int nAlpha)
{
	BLENDFUNCTION bf;

	//// BLENDFUNCTION �ʱ�ȭ
	bf.BlendOp = AC_SRC_OVER;	//
	bf.BlendFlags = 0;				//			
	bf.AlphaFormat = 0;				//
	bf.SourceConstantAlpha = nAlpha; // 0 - 255

	//// ���� ����
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

/*
BOOL __PutSprite(HDC dcDst, int x, int y, int nDstWidth, int nDstHeight, SURFACEINFO* psInfo)
{
	HDC hdc;
	HBITMAP hBmp, hBmpOld;

	hdc = CreateCompatibleDC(dcDst);
	hBmp = CreateCompatibleBitmap(dcDst, nDstWidth, nDstHeight);
	hBmpOld = (HBITMAP)SelectObject(hdc, hBmp);

	 StretchBlt(dcDst,        // ��� DC
		x, y,           // ��� ���� X, Y ��ǥ
		nDstWidth,      // �ø� �ʺ�
		nDstHeight,     // �ø� ����
		psInfo->dcSurface,  // ���� DC
		0, 0,           // ���� ���� X, Y ��ǥ
		psInfo->nWidth, // ���� �ʺ�
		psInfo->nHeight, // ���� ����
		SRCCOPY);

	BOOL bResult = TransparentBlt(
		dcDst, x, y, nDstWidth, nDstHeight,
		hdc, 0, 0, nDstWidth, nDstHeight,
		psInfo->crColorKey
	);

	SelectObject(hdc, hBmpOld);
	DeleteObject(hBmp);
	DeleteDC(hdc);

	return bResult;
}
*/

/*
BOOL __PutSprite(HDC dcDst, int x, int y, int nDstWidth, int nDstHeight, SURFACEINFO* psInfo, SURFACEINFO* psMask)
{
	HDC hdcTemp = CreateCompatibleDC(dcDst);
	HBITMAP hBmpOld = (HBITMAP)SelectObject(hdcTemp, psInfo->hBmp);

	// 1. ����ũ ��Ʈ�� ���� (AND ����)
	BitBlt(dcDst, x, y, psInfo->nWidth * 2, psInfo->nHeight * 2,
		psMask->dcSurface, 0, 0, SRCAND);

	// 2. ĳ���� ��Ʈ�� ���� (OR ����)
	BitBlt(dcDst, x, y, psInfo->nWidth * 2, psInfo->nHeight * 2,
		psInfo->dcSurface, 0, 0, SRCPAINT);

	StretchBlt(dcDst,        // ��� DC
		x, y,           // ��� ���� X, Y ��ǥ
		nDstWidth,      // �ø� �ʺ�
		nDstHeight,     // �ø� ����
		psMask->dcSurface,  // ���� DC
		0, 0,           // ���� ���� X, Y ��ǥ
		psInfo->nWidth, // ���� �ʺ�
		psInfo->nHeight, // ���� ����
		SRCAND);

	StretchBlt(dcDst,        // ��� DC
		x, y,           // ��� ���� X, Y ��ǥ
		nDstWidth,      // �ø� �ʺ�
		nDstHeight,     // �ø� ����
		psInfo->dcSurface,  // ���� DC
		0, 0,           // ���� ���� X, Y ��ǥ
		psInfo->nWidth, // ���� �ʺ�
		psInfo->nHeight, // ���� ����
		SRCPAINT);

	SelectObject(hdcTemp, hBmpOld);
	DeleteDC(hdcTemp);

	return TRUE;
}
*/

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

	//// ������ ����
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

	//// DDB�� ��ȯ�Ѵ�.
	hBit = CreateDIBitmap(hdc,						// hdc
		iheader,					// BITMAPINFOHEADER ���
		CBM_INIT,					// 0 �Ǵ� CBM_INIT ( �ʱ�ȭ )
		lpMemBlock + fh->bfOffBits,	// ������ ��巡��
		ih,							// BITMAPINFO ���
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

	////
	__CreateBackBuffer(dcScreen, 1920, 1080, 32, &g_sfBack);

	////
	__SetImgSurface(&g_sfBG);
	g_sfBG.hBmp = __MakeDDBFromDIB(dcScreen, "map.bmp");
	__LoadSurface(dcScreen, &g_sfBG);

	if (Is_Idle == TRUE)
	{
		for (i = 0; i < 2; i++)
		{
			__SetImgSurface(&(g_objPlayer.g_sfPlayer[i]));
			wsprintf(strName, "Player_Idle_%d.bmp", i + 1);
			g_objPlayer.g_sfPlayer[i].hBmp = __MakeDDBFromDIB(dcScreen, strName);
			__LoadSurface(dcScreen, &(g_objPlayer.g_sfPlayer[i]));
		}
	}

	if (Is_Jump == TRUE)
	{
		__SetImgSurface(&(g_objPlayer.g_sfPlayer[0]));
		wsprintf(strName, "Player_Jump.bmp");
		g_objPlayer.g_sfPlayer[0].hBmp = __MakeDDBFromDIB(dcScreen, strName);
		__LoadSurface(dcScreen, &(g_objPlayer.g_sfPlayer[0]));
	}

	/*
	__SetImgSurface(&(g_objPlayer.g_sfPlayer[1]));
	wsprintf(strName, "Player_Idle_%d.bmp", 2);
	g_objPlayer.g_sfPlayer[1].hBmp = __MakeDDBFromDIB(dcScreen, strName);
	__LoadSurface(dcScreen, &(g_objPlayer.g_sfPlayer[1]));
	*/
}

void __DestroyAll()
{
	int i;

	for (i = 0; i < 2; i++)
	{
		__ReleaseSurface(&(g_objPlayer.g_sfPlayer[i]));
	}

	__ReleaseSurface(&g_sfBG);
	__ReleaseSurface(&g_sfBack);
}

static int x = 100;
static int y = 100;

void CALLBACK MainLoopProc(HWND hWnd, UINT message, UINT_PTR iTimerID, DWORD dwTime)
{
	char  strBuff[24];
	HDC   dcScreen;
	BOOL  bRval;

	//�����
	//�߷�
	if (y >= Floor)
	{
		y = Floor;
	}
	else
	{
		y = y + 5;
	}

	if (GetAsyncKeyState(VK_UP) & 0x8000 && GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		if (y == Floor)
		{
			Is_Idle = FALSE;
			Is_Jump = TRUE;

			y -= 200;
			x -= 10;
		}
		else
		{
			x -= 10;
		}
	}
	if (GetAsyncKeyState(VK_UP) & 0x8000 && GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		if (y == Floor)
		{
			Is_Idle = FALSE;
			Is_Jump = TRUE;

			y -= 200;
			x += 10;
		}
		else
		{
			x -= 10;
		}
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		x += 10;
	}

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		x -= 10;
	}

	//// ��º�
	dcScreen = GetDC(hWnd);
	{
		//// ���
		//__PutImage(g_sfBack.dcSurface, 0, nBgY, &g_sfBG);
		__PutImage(g_sfBack.dcSurface, 0, 0, g_sfBack.nWidth, g_sfBack.nHeight, &g_sfBG);

		//__PutImageBlend(g_sfBack.dcSurface, 0, 0, &g_sfBG, 128);

		// ������Ʈ �� ��Ÿ �������̽�â
		bRval = __PutSprite(g_sfBack.dcSurface, x, y , 100, 100, &(g_objPlayer.g_sfPlayer[g_objPlayer.nAni]));
		//bRval = __PutSprite(g_sfBack.dcSurface, 100, 100, &(g_objPlayer.g_sfPlayer[0/*g_objPlayer.nAni*/], &(g_sfPlayerMask[0]));
		if (!bRval)	::OutputDebugString("__PutSprite - fail");
	
		::wsprintf(strBuff, "Frame %d", ++g_nFrame);
		::TextOut(g_sfBack.dcSurface, 10, 10, strBuff, strlen(strBuff));

		//// ��� �Ϸ�
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
		//// ǥ�� ���� �� ��Ÿ
		dcScreen = GetDC(hWnd);
		__Init(dcScreen);
		ReleaseDC(hWnd, dcScreen);

		////
		g_objPlayer.nAni = 0;

		////
		::Sleep(100);

		//// Ÿ�̸� ����		
		// 60 FPS ( 1000ms / 60fps = 16.6ms )
		// ���е� NT : 10ms ( 100 fps )
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
			//// �ִϸ��̼� �ε���
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