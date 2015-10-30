#define _CRT_SECURE_NO_WARNINGS
#pragma comment (lib, "WinMM.lib")
#include <Windows.h>
#include "resource.h"
#include <bitset>
#include <vector>
#include <random>
#include <ctime>
#include <functional>
#include <mmsystem.h>


#define BW 10
#define BH 20

#define DOWNTIME 1
#define NEWBLOCK 2
#define PAUSETIME 3

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditBlockProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutProc(HWND, UINT, WPARAM, LPARAM);
VOID CALLBACK MoveDown(HWND, UINT, UINT, DWORD);
VOID CALLBACK WhilePause(HWND, UINT, UINT, DWORD);

void SetNewBlock();
void TestEnd();

void ReadBlock();
void SaveBlock();

void DrawScreen(HDC& hdc);
void BoardInitialize();
static inline bool at(UINT16&, int&, int&);

HWND hMainWnd;
HINSTANCE g_hInst;
LPCTSTR Name = L"Tetris";



std::bitset<4> RegistryBlock[16];
unsigned int Height, Width;
std::vector<std::vector<char>> Board;
bool pause;
bool grid = false;
unsigned int difficulty;



//char Board[BH][BW];
//"Rectangle" may be replaced with bitmap
//"32" is the current size of block.



class BLOCK
{
private:
	static std::vector<std::vector<UINT16>> Blocks;
	static std::function<int()> typefind;
	static std::vector<HBITMAP> Colors;
	friend void ReadBlock();
	friend void CALLBACK MoveDown(HWND, UINT, UINT, DWORD);
	friend void DrawScreen(HDC&);
	int rotation;
	int type;
	int x, y;
public:
	void ColorGrid();
	BLOCK() :x{Width/2-1}, y{1}, rotation{ 0 }, type{ typefind() }{};
	void Move(WPARAM dir);
	void Rotate();
	bool GoDown();
	void PrintBlock(bool);
	BOOL CheckState();

};
std::vector<std::vector<UINT16>> BLOCK::Blocks;
std::function<int()> BLOCK::typefind;
std::vector<HBITMAP> BLOCK::Colors;
BLOCK* CurB = nullptr;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	g_hInst = hInstance;
	WNDCLASS WndClass;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON3));
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = Name;
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.style = CS_VREDRAW | CS_HREDRAW;

	RegisterClass(&WndClass);
	hMainWnd = CreateWindow(Name, Name, WS_OVERLAPPEDWINDOW^WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	ShowWindow(hMainWnd, nCmdShow);

	MSG Msg;
	while (GetMessage(&Msg, NULL, NULL, NULL))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam) //while - in game
{
	PAINTSTRUCT ps;
	HDC hdc;
	switch (iMsg)
	{
	case WM_CREATE:
	{

		hMainWnd = hWnd;
		if(DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hMainWnd, DlgProc) == IDCANCEL) PostQuitMessage(0);

		break;
	}
	case WM_PAINT:
	{
		hdc = BeginPaint(hMainWnd, &ps);
		DrawScreen(hdc);
		ReleaseDC(hMainWnd, hdc);
		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case ID_FILE_GAME:
			SetWindowLong(hMainWnd, GWL_WNDPROC, (LONG)WndProc);
			SetWindowText(hMainWnd, L"TETRIS");
			if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hMainWnd, DlgProc) == IDCANCEL) PostQuitMessage(0);
			InvalidateRect(hMainWnd, NULL, TRUE);
			break;
		case ID_FILE_REGISTERBLOCK:
		{
			SetWindowLong(hMainWnd, GWL_WNDPROC, (LONG)EditBlockProc);
			RECT R = { 0, 0, 32 * 5, 32 * 16 };
			AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW^WS_THICKFRAME, TRUE);
			SetWindowPos(hMainWnd, NULL, 0, 0, R.right - R.left, R.bottom - R.top, SWP_NOMOVE | SWP_SHOWWINDOW);
			SetWindowText(hMainWnd, L"EDIT BLOCK");
			InvalidateRect(hMainWnd, NULL, TRUE);
			break;
		}
		case ID_HELP_ABOUT:
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), hMainWnd, AboutProc);
			break;
		case ID_FILE_PAUSE:
			pause = !pause;
			if (pause)
			{
				KillTimer(hMainWnd, DOWNTIME);
				SetTimer(hMainWnd, PAUSETIME, 500, WhilePause);
			}
			else
			{
				KillTimer(hMainWnd, PAUSETIME);
				RECT R = { 32 + Width * 16 - 47, 32 + Height * 16 - 18, 32 + Width * 16 + 47, 32 + Height * 16 + 19 };
				InvalidateRect(hMainWnd, &R, TRUE);
				SetTimer(hMainWnd, DOWNTIME, difficulty, MoveDown);
			}
			break;
		case ID_HELP_GRIDON:
			grid = !grid;
			InvalidateRect(hMainWnd, NULL, TRUE);
			break;
		case ID_FILE_EXIT:
			PostQuitMessage(0);
			break;
		}
		break;
	}
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_LEFT:
		case VK_UP:
		case VK_RIGHT:
		case VK_DOWN:
			CurB->Move(wParam);
			break;
		case VK_SPACE:
			while (CurB->GoDown());
			break;
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}
	return 0;
}


void BLOCK::PrintBlock(bool Show)
{
	HDC hdc = GetDC(hMainWnd);
	HDC MemDC = CreateCompatibleDC(hdc);
	HBITMAP OLDBIT = (HBITMAP)SelectObject(MemDC, BLOCK::Colors[type]); //temporarily
	if(!grid) SelectObject(hdc, GetStockObject(WHITE_PEN));
	else SelectObject(hdc, GetStockObject(BLACK_PEN));

	static HBRUSH cover = (HBRUSH)GetStockObject(WHITE_BRUSH);
	SelectObject(hdc, cover);

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (at(Blocks[type][rotation], i, j))
			{
				
				Show?
					BitBlt(hdc, (x + j) * 32, (y + i) * 32, 32, 32, MemDC, 0, 0, SRCCOPY)
					: Rectangle(hdc, (x + j) * 32, (y + i) * 32, (x + j + 1) * 32, (y + i + 1) * 32);
			}
		}
	}

	SelectObject(MemDC, OLDBIT);
	DeleteDC(MemDC);
	ReleaseDC(hMainWnd, hdc);

}
static inline bool at(UINT16& srcblock,int& i, int& j)
{
	return (srcblock >> (4 * i + j))&1;
}


void ReadBlock()
{
	FILE* temp = fopen("block.txt", "rb");
	if (temp)
	{
		UINT16 dstblock[4];
		BLOCK::Blocks.clear();
		BLOCK::Colors.clear();
		//
		for (int i = IDB_BITMAP1; i <= IDB_BITMAP7; i++)
		{
			BLOCK::Colors.push_back(LoadBitmap(g_hInst, MAKEINTRESOURCE(i)));
		}

		while (fread(&dstblock, sizeof(UINT16), 4, temp)) //reads a FULL BLOCK with rotations
		{
			BLOCK::Blocks.push_back(std::vector < UINT16 > {dstblock, dstblock + 4});
		}
		BLOCK::typefind = std::bind(std::uniform_int_distribution <unsigned int> {0, BLOCK::Blocks.size() - 1}, std::default_random_engine{ (unsigned int)time(0) });

	}
	else
	{
		FILE* newfile = fopen("block.txt", "wb");
		HRSRC hRes = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TEXT2), TEXT("TEXT"));
		DWORD dwSize = SizeofResource(GetModuleHandle(NULL), hRes); //size, in bytes ...  = 8
		HGLOBAL hGlob = LoadResource(GetModuleHandle(NULL), hRes);
		LPVOID rawData = LockResource(hGlob);
		if (fwrite(rawData, 1, dwSize, newfile) != dwSize)
			MessageBox(hMainWnd, L"ERROR : COULDN'T LOAD BLOCKS", L"ERROR", MB_OK);
		fflush(newfile);
		fclose(newfile);
		ReadBlock();
	}

}
void SaveBlock()
{
	FILE* temp = fopen("block.txt", "ab");
	UINT16 block_unit;

	for (int i =0; i < 16; i += 4)
	{
		block_unit = 0;
		for (int j = 0; j < 4; j++)
		{
			auto h = (RegistryBlock[i + j].to_ulong());
			block_unit |= (h << 4 * j);
		}
		fwrite(&block_unit, sizeof(UINT16), 1, temp);
	}
	fflush(temp);
	fclose(temp);
}
LRESULT CALLBACK EditBlockProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static bool MDOWN = false;
	PAINTSTRUCT ps;
	HDC hdc;
	switch (iMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILE_GAME:
			SetWindowLong(hMainWnd, GWL_WNDPROC, (LONG)WndProc);
			SetWindowText(hMainWnd, L"TETRIS");
			if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hMainWnd, DlgProc) == IDCANCEL) PostQuitMessage(0);
			InvalidateRect(hMainWnd, NULL, TRUE);
			break;
		case ID_FILE_REGISTERBLOCK:
		{
			SetWindowLong(hMainWnd, GWL_WNDPROC, (LONG)EditBlockProc);
			RECT R = { 0, 0, 32 * 5, 32 * 16 };
			AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW^WS_THICKFRAME, TRUE);
			SetWindowPos(hMainWnd, NULL, 0, 0, R.right - R.left, R.bottom - R.top, SWP_NOMOVE | SWP_SHOWWINDOW);
			SetWindowText(hMainWnd, L"EDIT BLOCK");
			InvalidateRect(hMainWnd, NULL, TRUE);
			break;
		}
		case ID_HELP_ABOUT:
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), hMainWnd,AboutProc);
			break;
		case ID_FILE_EXIT:
			PostQuitMessage(0);
			break;
		}
		break;
	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);
		SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));

		Rectangle(hdc, 0, 0, 32 * 5, 32*4);
		
		Rectangle(hdc, 0, 32 * 4, 32 * 5, 32 * 8);
		Rectangle(hdc, 0, 32 * 8, 32 * 5, 32 * 12);
		Rectangle(hdc, 0, 32 * 12, 32 * 5, 32 * 16);
		
		TextOut(hdc, 32 * 4,8, L"R1", 2);
		TextOut(hdc, 32 * 4, 32*4+8, L"R2", 2);
		TextOut(hdc, 32 * 4, 32*8+8, L"R3", 2);
		TextOut(hdc, 32 * 4, 32*12+8, L"R4", 2);

		for (int i = 0; i < 16; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				SelectObject(hdc, bool(RegistryBlock[i][j]) ? (HBRUSH)GetStockObject(BLACK_BRUSH) : (HBRUSH)GetStockObject(WHITE_BRUSH));
				Rectangle(hdc, j * 32, i * 32, (j + 1) * 32, (i + 1) * 32);
			}
		}
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_RETURN:
			SaveBlock();
			break;
		case VK_ESCAPE:
			for (auto &x : RegistryBlock)
			{
				x.reset();
				InvalidateRect(hWnd, NULL, TRUE);
			}
			break;
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		MDOWN = true;
		int i = HIWORD(lParam) / 32;
		int j = LOWORD(lParam) / 32;

		if (j < 4)
		{
			RegistryBlock[i][j].flip();
			hdc = GetDC(hWnd);
			SelectObject(hdc, bool(RegistryBlock[i][j]) ? (HBRUSH)GetStockObject(BLACK_BRUSH) : (HBRUSH)GetStockObject(WHITE_BRUSH));
			Rectangle(hdc, j * 32, i * 32, (j + 1) * 32, (i + 1) * 32);

			ReleaseDC(hWnd, hdc);
		}

		//HANDLE hUpdate = BeginUpdateResource(MAKEINTRESOURCE(IDR_TEXT1), FALSE);
		//bool result = UpdateResource(hUpdate, TEXT("TEXT"), MAKEINTRESOURCE(IDR_TEXT2), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), rawData, dwSize);
		break;
	}
	case WM_LBUTTONUP:
	{
		MDOWN = false;
		break;
	}
	case WM_MOUSEMOVE: //drag-drawing
	{
		if (MDOWN)
		{
			int i = HIWORD(lParam) / 32;
			int j = LOWORD(lParam) / 32;
			if (j < 4)
			{
				RegistryBlock[i][j] = 1;
				hdc = GetDC(hWnd);
				SelectObject(hdc, (HBRUSH)GetStockObject(BLACK_BRUSH));
				Rectangle(hdc, j * 32, i * 32, (j + 1) * 32, (i + 1) * 32);
				ReleaseDC(hWnd, hdc);
			}
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
		//TO-WRITE LATER.
	default:
		return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}
	return 0;
}

void DrawScreen(HDC& hdc)
{
	SelectObject(hdc, GetStockObject(BLACK_BRUSH));
	if(!grid)SelectObject(hdc, GetStockObject(WHITE_PEN));
	else SelectObject(hdc, GetStockObject(BLACK_PEN));

	HDC MemDC = CreateCompatibleDC(hdc);
	HBITMAP OLDBIT = (HBITMAP)SelectObject(MemDC, BLOCK::Colors[0]);
 //temporarily

	for (int i = 0; i <=Height+1; i++) //including walls
	{
		for (int j = 0; j <=Width+1; j++)
		{
			if (Board[i][j] == -1)
			{
				SelectObject(hdc, GetStockObject(BLACK_BRUSH));
				Rectangle(hdc, j * 32, i * 32, (j + 1) * 32, (i + 1) * 32);
			}
			else if (Board[i][j] == 0)
			{
				SelectObject(hdc, GetStockObject(WHITE_BRUSH));
				Rectangle(hdc, j * 32, i * 32, (j + 1) * 32, (i + 1) * 32);
			}
			else
			{
				SelectObject(MemDC, BLOCK::Colors[Board[i][j]-1]);
				BitBlt(hdc, j * 32, i * 32, 32, 32, MemDC, 0, 0, SRCCOPY);
			}
			
		}
	}
	CurB->PrintBlock(true);
	SelectObject(MemDC, OLDBIT);
	DeleteDC(MemDC);
}

void BoardInitialize()
{
	Board.clear();
	Board.push_back(std::vector<char>(Width + 2, -1));
	for (int i = 1; i <=Height; i++)
	{
		Board.push_back(std::vector<char>(Width+2,0));
		Board.back()[0] = Board.back()[Width + 1] = -1;
	}
	Board.push_back(std::vector<char>(Width + 2, -1));

	SetNewBlock();
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg, IDC_RADIO1, BM_SETCHECK, BST_CHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"Easy");
		SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"Medium");
		SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"Hard");
		SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, 0, 0);
		SetDlgItemInt(hDlg, IDC_EDIT1, Width?Width:10, FALSE);
		SetDlgItemInt(hDlg, IDC_EDIT2, Height?Height:20, FALSE);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			BOOL checkw, checkh;
			Width = GetDlgItemInt(hDlg, IDC_EDIT1, &checkw, FALSE);
			Height = GetDlgItemInt(hDlg, IDC_EDIT2, &checkh, FALSE);
			int d = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
			difficulty = 500 - (d * 200);
			if (checkw&&checkh)
			{
				if (SendDlgItemMessage(hDlg, IDC_RADIO2, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					PostMessage(hMainWnd, WM_COMMAND, MAKEWPARAM(ID_FILE_REGISTERBLOCK, 0), 0);
				}
				else
				{
					RECT R{ 0, 0, (Width + 2) * 32, (Height + 2) * 32 }; //including borders
					AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW^WS_THICKFRAME, TRUE);
					SetWindowPos(hMainWnd, NULL, 0, 0, R.right - R.left, R.bottom - R.top, SWP_NOMOVE);
					ReadBlock();
					BoardInitialize();
				}
				EndDialog(hDlg, IDOK);
			}
			else EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		case IDCANCEL:;
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}	
	}
	return FALSE;
}

BOOL CALLBACK AboutProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_INITDIALOG:
		KillTimer(hMainWnd, DOWNTIME);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hDlg, IDOK);
			return TRUE;
		}
	case WM_DESTROY:
		SetTimer(hMainWnd, DOWNTIME, difficulty, MoveDown);
		return TRUE;
	}
	return FALSE;
}

BOOL BLOCK::CheckState()
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (at(Blocks[type][rotation], i, j) && Board[y + i][x + j]) return FALSE;
		}
	}
	return TRUE;
}

VOID CALLBACK MoveDown(HWND hWnd, UINT iMsg, UINT CallerId, DWORD dwTime)
{
	CurB->PrintBlock(false);
	CurB->GoDown();
	CurB->PrintBlock(true);
}

VOID SetNewBlock()
{
	--difficulty;
	//1) test end
	//2) set new block
	delete CurB;
	CurB = new BLOCK();
	if (CurB->CheckState())
	{
		SetTimer(hMainWnd, DOWNTIME, difficulty, MoveDown);
		SendMessage(hMainWnd, WM_TIMER, DOWNTIME, (LPARAM)MoveDown);
	}
	else
	{
		MessageBox(hMainWnd, L"FAIL!", L"GAME OVER", MB_OK);
	}

}

void TestEnd()
{
	for (int i = 1; i <= Height; i++)
	{
		int j;
		for (j = 1; j <= Width; j++)
		{
			if (!Board[i][j]) break; //Board[i][j] = 0(empty)
		}

		if (j == Width + 1)
		{
			PlaySound(MAKEINTRESOURCE(IDR_WAVE3), g_hInst, SND_RESOURCE | SND_ASYNC);
			for (int ti = i; ti > 1; ti--)
			{
				for (int tj = 1; tj <= BW; tj++)
				{
					Board[ti][tj] = Board[ti - 1][tj];
				}
			}
			for (int tj = 1; tj <= BW; tj++) Board[1][tj] = 0;

		}
	}
	InvalidateRect(hMainWnd, NULL, FALSE);

	SetNewBlock();
}

void BLOCK::Move(WPARAM dir)
{
	PrintBlock(false);
	switch (dir)
	{
	case VK_LEFT:
		PlaySound(MAKEINTRESOURCE(IDR_WAVE4), g_hInst, SND_RESOURCE | SND_ASYNC);
		--x; if (!CheckState()) ++x; break;
	case VK_RIGHT:
		PlaySound(MAKEINTRESOURCE(IDR_WAVE4), g_hInst, SND_RESOURCE | SND_ASYNC);
		++x; if (!CheckState()) --x; break;
	case VK_UP:
		PlaySound(MAKEINTRESOURCE(IDR_WAVE5), g_hInst, SND_RESOURCE | SND_ASYNC);
		Rotate(); break;
	case VK_DOWN:
		PlaySound(MAKEINTRESOURCE(IDR_WAVE2), g_hInst, SND_RESOURCE | SND_ASYNC);
		GoDown(); break;
	}
	PrintBlock(true);
}

bool BLOCK::GoDown()
{
	++y;
	if (!CheckState()) //reached ground
	{
		--y;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				if (at(BLOCK::Blocks[type][rotation], i, j))
					Board[y + i][x + j] = type + 1;
			}
		}

		KillTimer(hMainWnd,DOWNTIME);
		TestEnd();
		return FALSE;
	}
	return TRUE;
}

void BLOCK::Rotate()
{
	rotation = (rotation + 1) % 4;
	if (!CheckState())
		rotation = (rotation + 3) % 4;
}

void CALLBACK WhilePause(HWND hWnd, UINT iMsg, UINT CallerID, DWORD dwTime)
{
	static bool display = true;
	HDC hdc = GetDC(hWnd);
	HBITMAP PauseBit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP8));
	HDC MemDC = CreateCompatibleDC(hdc);
	HBITMAP OldBit = (HBITMAP)SelectObject(MemDC, PauseBit);
	RECT R = { 32 + Width * 16 - 47, 32 + Height * 16 - 18, 32 + Width*16 + 47, 32 + Height*16 + 19 };
	if (display = !display) BitBlt(hdc, 32 + Width * 16 - 47, 32 + Height * 16 - 18, 94,37, MemDC, 0, 0, SRCCOPY);
	else InvalidateRect(hMainWnd, &R, TRUE);
}