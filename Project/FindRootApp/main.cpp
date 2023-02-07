#include <windows.h>
#include <math.h>
#include <stdio.h>

#define LEFT_BORDER -10
#define RIGHT_BORDER 10
#define EPSILON 0.01
/* формат A*(x*x*x) + B*(x*x) + C*x + D  -->  3*A*(x*x) + 2*B*x + C  -->  6*A*x + 2*B */
#define A_PARAM 4
#define B_PARAM -11
#define C_PARAM -7
#define D_PARAM 41

// переменные
struct Point { double x, y; };
static Point point, root;
static int iterations, curWidthWnd, curHeightWnd, numPoints;

// прототипы функций
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
double f(double);
double df(double);
double d2f(double);
Point NewtonMethod();
void DrawChart(HDC, Point*, PAINTSTRUCT*);
void DrawCoordGrid(HDC, Point*, double, double, double, double);
Point* CalcXY();

int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInst, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
// дескриптор экземпляра приложения, в современных системах всегда 0, командная строка, режим отображения окна
{
    TCHAR szClassName[] = L"My Class"; // имя класса окна
    HWND hMainWnd; // дескриптор главного окна программы
    MSG msg; // создём экземпляр структуры MSG для обработки и хранения сообщений
    WNDCLASSEX wc; // создаём экземпляр, для обращения к членам класса WNDCLASSEX
    wc.cbSize = sizeof(wc); // размер структуры (в байтах)
    wc.style = CS_HREDRAW | CS_VREDRAW; // стиль окна
    wc.lpfnWndProc = WndProc; // имя оконной функции для обработки сообщений
    wc.lpszMenuName = NULL; // ссылка на строку главного меню
    wc.lpszClassName = szClassName; // имя класса
    wc.cbWndExtra = NULL; // дополнительные параметры окна
    wc.cbClsExtra = NULL; // дополнительные параметры класса окна
    wc.hIcon = LoadIcon(NULL, IDI_ASTERISK); // декриптор пиктограммы
    wc.hIconSm = LoadIcon(NULL, IDI_ASTERISK); // дескриптор маленькой пиктограммы (в трее)
    wc.hCursor = LoadCursor(NULL, IDI_WINLOGO); // дескриптор курсора
    wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH); // дескриптор кисти для закраски фона окна
    wc.hInstance = hInst; // дексриптор текущего приложения
    if (!RegisterClassEx(&wc)) // регистрациz класса окна
    {
        MessageBox(NULL, L"Не получилось зарегистрировать класс!", L"Ошибка", MB_OK);
        return NULL;
    }
    hMainWnd = CreateWindow
    (
        szClassName, // имя класса окна
        L"Нахождение корня методом Ньютона", // заголовок окна
        WS_OVERLAPPEDWINDOW, // режимы отображения окна
        CW_USEDEFAULT, // позиция окна по оси х (левый верхний угол окна)
        NULL, // позиция окна по оси у (левый верхний угол окна)
        CW_USEDEFAULT, // ширина окна
        NULL, // высота окна
        (HWND)NULL, // дескриптор родительского окна
        NULL, // указатель на дескриптор меню
        HINSTANCE(hInst), // дескриптор экземпляра приложения
        NULL); // указатель на доп. информацию
    if (!hMainWnd) 
    {
        // в случае некорректного создания окна
        MessageBox(NULL, L"Не получилось создать окно!", L"Ошибка!", MB_OK);
        return NULL;
    }
	root = NewtonMethod();
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    while (GetMessage(&msg, NULL, NULL, NULL)) // цикл получения и обработки сообщений, полученных от ОС
    { 
        TranslateMessage(&msg); // функция трансляции кодов нажатой клавиши
        DispatchMessage(&msg); // возврат сообщения обратно ОС
    }
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    HDC hDC; // создаём дескриптор ориентации текста на экране
    PAINTSTRUCT ps; // структура, содержащая информацию о клиентской области (размеры, цвет и тп)
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
            MessageBox(hWnd, L"Не удалось отрисовать объекты!", L"Ошибка!", MB_OK);
            EndPaint(hWnd, &ps);
        }        
        break;
    case WM_SIZE:
        curWidthWnd = LOWORD(lParam); // ширина окна
        curHeightWnd = HIWORD(lParam); // высота окна
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
    // если знаки функции на краях отрезка одинаковые, то здесь нет корня
    if (f(LEFT_BORDER) * f(RIGHT_BORDER) > 0)
    {
        MessageBox(NULL, L"На данном отрезке нет корня уравнения!", L"Внимание!", MB_OK | MB_ICONWARNING);
        PostQuitMessage(1);
    }
    // для выбора начальной точки проверяем f(a) * d2f(a) > 0
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
    // установка начальной точки рисования графика
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
    SelectObject(hDC, hPen);
    double X, Y;
    X = (curWidthWnd * (arrPoints[0].x - xMin)) / (xMax - xMin);
    Y = curHeightWnd - ((curHeightWnd * (arrPoints[0].y - yMin)) / (yMax - yMin));
    MoveToEx(hDC, (int)X , (int)Y, 0);
    // отрисовка графика по точкам
    for (int i = 0; i < numPoints; i++)
    {
        X = (curWidthWnd * (arrPoints[i].x - xMin)) / (xMax - xMin);
        Y = curHeightWnd - ((curHeightWnd * (arrPoints[i].y - yMin)) / (yMax - yMin));
        LineTo(hDC, (int)X, (int)Y);
    }
    DeleteObject(hPen);
    // вывод значения корня на график
    X = (curWidthWnd * (root.x - xMin)) / (xMax - xMin);
    Y = curHeightWnd - ((curHeightWnd * (root.y - yMin)) / (yMax - yMin));
    SetBkColor(hDC, RGB(255, 255, 255));
    wchar_t buffer[10];
    TextOut(hDC, (int)X, (int)Y -20, buffer, (int)swprintf(buffer, 10, L"%.3f", root.x));
    // вывод количества итераций
    TextOut(hDC, ps->rcPaint.left, ps->rcPaint.top, L"Количество итераций: ", strlen("Количество итераций: "));
    TextOut(hDC, 150, 0, buffer, swprintf(buffer, 10, L"%d", iterations));
    DeleteObject(hPen);
}

void DrawCoordGrid(HDC hDC, Point* arrPoints, double xMax, double xMin, double yMax, double yMin)
{
    const int numLinesX = 10, numLinesY = 10;
    double hX = (xMax - xMin) / numLinesX, hY = (yMax - yMin) / numLinesY; /* шаги разбиения по x и y с учётом количества линий сетки*/
    double masX[numLinesX] = { arrPoints[0].x }, masY[numLinesY] = { arrPoints[0].y };
    // заполнение массивов с учётом шага
    for (int j = 1; j < numLinesX; j++) masX[j] = masX[j - 1] + hX;
    for (int j = 1; j < numLinesY; j++) masY[j] = masY[j - 1] + hY;

    // линии по y
    wchar_t buffer[10] = { L'A' };
    SetBkColor(hDC, RGB(0, 128, 128));
    for (int x = 0, i = 0; x < curWidthWnd; x += curWidthWnd / numLinesX, i++)
    {
        MoveToEx(hDC, x, 0, NULL);
        LineTo(hDC, x, curHeightWnd);
        TextOut(hDC, (int)x + 3, curHeightWnd - 40, buffer, (int)swprintf(buffer, 10, L"%.2f", masX[i]));
    }
    // линии по x
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