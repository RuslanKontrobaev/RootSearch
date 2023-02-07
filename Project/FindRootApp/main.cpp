#include <windows.h>
#include <math.h>
#include <stdio.h>

#define LEFT_BORDER -10
#define RIGHT_BORDER 10
#define EPSILON 0.01
/* ������ A*(x*x*x) + B*(x*x) + C*x + D  -->  3*A*(x*x) + 2*B*x + C  -->  6*A*x + 2*B */
#define A_PARAM 4
#define B_PARAM -11
#define C_PARAM -7
#define D_PARAM 41

// ����������
struct Point { double x, y; };
static Point point, root;
static int iterations, curWidthWnd, curHeightWnd, numPoints;

// ��������� �������
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
double f(double);
double df(double);
double d2f(double);
Point NewtonMethod();
void DrawChart(HDC, Point*, PAINTSTRUCT*);
void DrawCoordGrid(HDC, Point*, double, double, double, double);
Point* CalcXY();

int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInst, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
// ���������� ���������� ����������, � ����������� �������� ������ 0, ��������� ������, ����� ����������� ����
{
    TCHAR szClassName[] = L"My Class"; // ��� ������ ����
    HWND hMainWnd; // ���������� �������� ���� ���������
    MSG msg; // ����� ��������� ��������� MSG ��� ��������� � �������� ���������
    WNDCLASSEX wc; // ������ ���������, ��� ��������� � ������ ������ WNDCLASSEX
    wc.cbSize = sizeof(wc); // ������ ��������� (� ������)
    wc.style = CS_HREDRAW | CS_VREDRAW; // ����� ����
    wc.lpfnWndProc = WndProc; // ��� ������� ������� ��� ��������� ���������
    wc.lpszMenuName = NULL; // ������ �� ������ �������� ����
    wc.lpszClassName = szClassName; // ��� ������
    wc.cbWndExtra = NULL; // �������������� ��������� ����
    wc.cbClsExtra = NULL; // �������������� ��������� ������ ����
    wc.hIcon = LoadIcon(NULL, IDI_ASTERISK); // ��������� �����������
    wc.hIconSm = LoadIcon(NULL, IDI_ASTERISK); // ���������� ��������� ����������� (� ����)
    wc.hCursor = LoadCursor(NULL, IDI_WINLOGO); // ���������� �������
    wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH); // ���������� ����� ��� �������� ���� ����
    wc.hInstance = hInst; // ���������� �������� ����������
    if (!RegisterClassEx(&wc)) // ����������z ������ ����
    {
        MessageBox(NULL, L"�� ���������� ���������������� �����!", L"������", MB_OK);
        return NULL;
    }
    hMainWnd = CreateWindow
    (
        szClassName, // ��� ������ ����
        L"���������� ����� ������� �������", // ��������� ����
        WS_OVERLAPPEDWINDOW, // ������ ����������� ����
        CW_USEDEFAULT, // ������� ���� �� ��� � (����� ������� ���� ����)
        NULL, // ������� ���� �� ��� � (����� ������� ���� ����)
        CW_USEDEFAULT, // ������ ����
        NULL, // ������ ����
        (HWND)NULL, // ���������� ������������� ����
        NULL, // ��������� �� ���������� ����
        HINSTANCE(hInst), // ���������� ���������� ����������
        NULL); // ��������� �� ���. ����������
    if (!hMainWnd) 
    {
        // � ������ ������������� �������� ����
        MessageBox(NULL, L"�� ���������� ������� ����!", L"������!", MB_OK);
        return NULL;
    }
	root = NewtonMethod();
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    while (GetMessage(&msg, NULL, NULL, NULL)) // ���� ��������� � ��������� ���������, ���������� �� ��
    { 
        TranslateMessage(&msg); // ������� ���������� ����� ������� �������
        DispatchMessage(&msg); // ������� ��������� ������� ��
    }
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    HDC hDC; // ������ ���������� ���������� ������ �� ������
    PAINTSTRUCT ps; // ���������, ���������� ���������� � ���������� ������� (�������, ���� � ��)
    Point* dataPoint = CalcXY();
    switch (uMsg)
    {
    case WM_CREATE:
        break;
    case WM_PAINT:                                        
        hDC = BeginPaint(hWnd, &ps);  
        if (hDC)
        {
            DrawChart(hDC, dataPoint, &ps);
            EndPaint(hWnd, &ps);
        }
        else
        {
            MessageBox(hWnd, L"�� ������� ���������� �������!", L"������!", MB_OK);
            EndPaint(hWnd, &ps);
        }        
        break;
    case WM_SIZE:
        curWidthWnd = LOWORD(lParam); // ������ ����
        curHeightWnd = HIWORD(lParam); // ������ ����
        break;
    case WM_DESTROY:       
        PostQuitMessage(NULL);                              
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return NULL;
}

double f(double x)
{
    return ((double)A_PARAM * x * x * x) + ((double)B_PARAM * x * x) + ((double)C_PARAM * x) + D_PARAM;
}

double df(double x) 
{
    return ((double)A_PARAM * 3 * x * x) + ((double)B_PARAM * 2 * x) + C_PARAM;
}

double d2f(double x)
{
    return ((double)A_PARAM * 6 * x) + ((double)B_PARAM * 2);
}

Point NewtonMethod()
{
    iterations = 0;
    double x0 = RIGHT_BORDER, xn;
    // ���� ����� ������� �� ����� ������� ����������, �� ����� ��� �����
    if (f(LEFT_BORDER) * f(RIGHT_BORDER) > 0)
    {
        MessageBox(NULL, L"�� ������ ������� ��� ����� ���������!", L"��������!", MB_OK | MB_ICONWARNING);
        PostQuitMessage(1);
    }
    // ��� ������ ��������� ����� ��������� f(a) * d2f(a) > 0
    if ((f(LEFT_BORDER) * d2f(LEFT_BORDER)) > 0) x0 = LEFT_BORDER;
    xn = x0 - (f(x0) / df(x0));
    iterations++;
    while (fabs(x0 - xn) > EPSILON && iterations < 10)
    {
        x0 = xn;
        xn = x0 - (f(x0) / df(x0));
        iterations++;
    }
    point.x = xn;
    point.y = f(xn);
    return point;
}

void DrawChart(HDC hDC, Point* arrPoints, PAINTSTRUCT* ps)
{
    double xMax = arrPoints[0].x, xMin = arrPoints[0].x, yMax = arrPoints[0].y, yMin = arrPoints[0].y;
    for (int i = 0; i < numPoints; i++)
    {
        if (arrPoints[i].x > xMax) xMax = arrPoints[i].x;
        if (arrPoints[i].y > yMax) yMax = arrPoints[i].y;
        if (arrPoints[i].x < xMin) xMin = arrPoints[i].x;
        if (arrPoints[i].y < yMin) yMin = arrPoints[i].y;
    }
    DrawCoordGrid(hDC, arrPoints, xMax, xMin, yMax, yMin);
    // ��������� ��������� ����� ��������� �������
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
    SelectObject(hDC, hPen);
    double X, Y;
    X = (curWidthWnd * (arrPoints[0].x - xMin)) / (xMax - xMin);
    Y = curHeightWnd - ((curHeightWnd * (arrPoints[0].y - yMin)) / (yMax - yMin));
    MoveToEx(hDC, (int)X , (int)Y, 0);
    // ��������� ������� �� ������
    for (int i = 0; i < numPoints; i++)
    {
        X = (curWidthWnd * (arrPoints[i].x - xMin)) / (xMax - xMin);
        Y = curHeightWnd - ((curHeightWnd * (arrPoints[i].y - yMin)) / (yMax - yMin));
        LineTo(hDC, (int)X, (int)Y);
    }
    DeleteObject(hPen);
    // ����� �������� ����� �� ������
    X = (curWidthWnd * (root.x - xMin)) / (xMax - xMin);
    Y = curHeightWnd - ((curHeightWnd * (root.y - yMin)) / (yMax - yMin));
    SetBkColor(hDC, RGB(255, 255, 255));
    wchar_t buffer[10];
    TextOut(hDC, (int)X, (int)Y -20, buffer, (int)swprintf(buffer, 10, L"%.3f", root.x));
    // ����� ���������� ��������
    TextOut(hDC, ps->rcPaint.left, ps->rcPaint.top, L"���������� ��������: ", strlen("���������� ��������: "));
    TextOut(hDC, 150, 0, buffer, swprintf(buffer, 10, L"%d", iterations));
    DeleteObject(hPen);
}

void DrawCoordGrid(HDC hDC, Point* arrPoints, double xMax, double xMin, double yMax, double yMin)
{
    const int numLinesX = 10, numLinesY = 10;
    double hX = (xMax - xMin) / numLinesX, hY = (yMax - yMin) / numLinesY; /* ���� ��������� �� x � y � ������ ���������� ����� �����*/
    double masX[numLinesX] = { arrPoints[0].x }, masY[numLinesY] = { arrPoints[0].y };
    // ���������� �������� � ������ ����
    for (int j = 1; j < numLinesX; j++) masX[j] = masX[j - 1] + hX;
    for (int j = 1; j < numLinesY; j++) masY[j] = masY[j - 1] + hY;

    // ����� �� y
    wchar_t buffer[10] = { L'A' };
    SetBkColor(hDC, RGB(0, 128, 128));
    for (int x = 0, i = 0; x < curWidthWnd; x += curWidthWnd / numLinesX, i++)
    {
        MoveToEx(hDC, x, 0, NULL);
        LineTo(hDC, x, curHeightWnd);
        TextOut(hDC, (int)x + 3, curHeightWnd - 40, buffer, (int)swprintf(buffer, 10, L"%.2f", masX[i]));
    }
    // ����� �� x
    SetBkColor(hDC, RGB(0, 128, 256));
    for (int y = 0, i = numLinesY - 1; y < curHeightWnd; y += curHeightWnd / numLinesY, i--)
    {
        MoveToEx(hDC, 0, y, NULL);
        LineTo(hDC, curWidthWnd, y);
        TextOut(hDC, 0, (int)y + 40, buffer, (int)swprintf(buffer, 10, L"%.2f", masY[i]));
    }
}

Point* CalcXY()
{
    numPoints = (((RIGHT_BORDER) - (LEFT_BORDER))) * 10;
    Point* arrPoints = new Point[numPoints];
    double x = LEFT_BORDER, delta = 0.1;
    for (int i = 0; i < numPoints; i++, x += delta)
    {
        arrPoints[i].x = x;
        arrPoints[i].y = f(x);
    }
    return arrPoints;
}