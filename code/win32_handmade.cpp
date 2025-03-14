#include <windows.h>
#include <stdint.h>

#define internal_func static
#define local_persist static
#define global_var static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel = 4;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

global_var bool Running = false;
global_var win32_offscreen_buffer GlobalBackbuffer;

win32_window_dimension Win32DisplayBufferInWindow(HWND Window)
{
    win32_window_dimension Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    return(Result);
}

internal_func void RenderPosGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{
    int Pitch = Buffer.Width * Buffer.BytesPerPixel;
    uint8 *Row = (uint8 *)Buffer.Memory;
    for (int Y = 0; Y < Buffer.Height; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0; X < Buffer.Width; ++X)
        {
            uint8 Blue = X + XOffset;
            uint8 Green = Y + YOffset;

            *Pixel++ = ((Green << 8) | Blue);
        }
        Row += Pitch;
    }
}

internal_func void ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    Buffer->Info.bmiHeader.biSizeImage = 0;
    Buffer->Info.bmiHeader.biXPelsPerMeter = 0;
    Buffer->Info.bmiHeader.biYPelsPerMeter = 0;
    Buffer->Info.bmiHeader.biClrUsed = 0;
    Buffer->Info.bmiHeader.biClrImportant = 0;


    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal_func void Win32DisplayBufferInWindow(
    HDC DeviceContext, 
    int WindowWidth, int WindowHeight, 
    win32_offscreen_buffer Buffer,
    int X, int Y, int Width, int Height)
{
    StretchDIBits(
        DeviceContext,
        //X, Y, Width, Height,
        //X, Y, Width, Height,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer.Width, Buffer.Height,
        Buffer.Memory, &Buffer.Info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam)
{
    LRESULT Result = 0;

    switch (uMsg)
    {
    case WM_SIZE:
        {
            
        } break;

    case WM_DESTROY:
        {
            Running = false;
        } break;
        
    case WM_CLOSE:
        {
            Running = false;
        } break;

    case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
    case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContex = BeginPaint(hwnd, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            
            win32_window_dimension Dimension = Win32DisplayBufferInWindow(hwnd);
            Win32DisplayBufferInWindow(DeviceContex, Dimension.Width, Dimension.Height, GlobalBackbuffer, X, Y, Width, Height);
            
            EndPaint(hwnd, &Paint);
        } break;

    default:
        {
            Result = DefWindowProc(hwnd, uMsg, wParam, lParam);
        } break;
    }
    return(Result);
}

int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd)
{
    WNDCLASS WindowClass = {};

    ResizeDIBSection(&GlobalBackbuffer, 1288, 720);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = hInstance;
    //WindowClass.hIcon = ;
    WindowClass.lpszClassName = "HandmadeEngineWindowClass";

    if(RegisterClass(&WindowClass))
    {
        HWND WindowHandle = CreateWindowEx(
            0,
            WindowClass.lpszClassName,
            "HandmadeEngine",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE|CS_OWNDC,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            hInstance,
            0
        );
    
        if (WindowHandle)
        {
            HDC DeviceContext = GetDC(WindowHandle);

            Running = true;
            int XOffset = 0;
            int YOffset = 0;
            while(Running)
            {
                MSG Message;
                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        Running = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                RenderPosGradient(GlobalBackbuffer, XOffset, YOffset);

                
                win32_window_dimension Dimension = Win32DisplayBufferInWindow(WindowHandle);
                Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer, 0, 0, Dimension.Width, Dimension.Height);
                ReleaseDC(WindowHandle, DeviceContext);

                ++XOffset;
            }
        }
        else
        {
            // TODO - logging
        }
    }
    else
    {
        // TODO - logging
    }
    
    return(0);
}