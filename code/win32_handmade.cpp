#include <windows.h>
#include <stdint.h>
#include <Xinput.h>

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
typedef int32 bool32;

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

// XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_var x_input_get_state *XInputGetState_ = XInputGetStateStub;

// XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_var x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

global_var bool Running = false;
global_var win32_offscreen_buffer GlobalBackbuffer;

internal_func void Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    
    if (XInputLibrary)
    {
        XInputGetState_ = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState_ = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

internal_func win32_window_dimension Win32GetWindowDimensions(HWND Window)
{
    win32_window_dimension Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    return(Result);
}

internal_func void RenderPosGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    int Pitch = Buffer->Width * Buffer->BytesPerPixel;
    uint8 *Row = (uint8 *)Buffer->Memory;
    for (int Y = 0; Y < Buffer->Height; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0; X < Buffer->Width; ++X)
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
    win32_offscreen_buffer *Buffer,
    int X, int Y, int Width, int Height)
{
    StretchDIBits(
        DeviceContext,
        //X, Y, Width, Height,
        //X, Y, Width, Height,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
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

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = wParam;
            bool WasDown = ((lParam & (1 << 30)) != 0);
            bool IsDown = ((lParam & (1 << 31)) == 0);
            if (WasDown != IsDown)
            {
                if (VKCode == 'W')
                {
                    
                }
                else if (VKCode == 'S')
                {
                    
                }
                else if (VKCode == 'A')
                {
                    
                }
                else if (VKCode == 'D')
                {
                    
                }
                else if (VKCode == 'Q')
                {
                    
                }
                else if (VKCode == 'E')
                {
                    
                }
                else if (VKCode == VK_UP)
                {
                    
                }
                else if (VKCode == VK_DOWN)
                {
                    
                }
                else if (VKCode == VK_LEFT)
                {
                    
                }
                else if (VKCode == VK_RIGHT)
                {
                    
                }
                else if (VKCode == VK_ESCAPE)
                {
                    OutputDebugStringA("Escape: ");
                    if(IsDown)
                    {
                        OutputDebugStringA("is down.\n");
                    }
                    if(WasDown)
                    {
                        OutputDebugStringA("was down.\n");
                    }
                }
                else if (VKCode == VK_SPACE)
                {
                    
                }
            }

            bool32 AltKeyDown = ((lParam & (1 << 29)));
            if ((VKCode == VK_F4) && AltKeyDown)
            {
                Running = false;
            }
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
            
            win32_window_dimension Dimension = Win32GetWindowDimensions(hwnd);
            Win32DisplayBufferInWindow(DeviceContex, Dimension.Width, Dimension.Height, &GlobalBackbuffer, X, Y, Width, Height);
            
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
    Win32LoadXInput();
    WNDCLASSA WindowClass = {};

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

            int XOffset = 0;
            int YOffset = 0;

            Running = true;
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
                    DispatchMessageA(&Message);
                }

                for (int ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
                {
                    XINPUT_STATE ContollerState;
                    if (XInputGetState(ControllerIndex, &ContollerState) == ERROR_SUCCESS)
                    {
                        // controller is plugged in
                        XINPUT_GAMEPAD *Pad = &ContollerState.Gamepad;

                        bool DPadUp = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        bool DPadDown = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        bool DPadLeft = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        bool DPadRight = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        bool PadStart = Pad->wButtons & XINPUT_GAMEPAD_START;
                        bool PadBack = Pad->wButtons & XINPUT_GAMEPAD_BACK;
                        bool PadLeftShoulder = Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        bool PadRightShoulder = Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        bool PadA = Pad->wButtons & XINPUT_GAMEPAD_A;
                        bool PadB = Pad->wButtons & XINPUT_GAMEPAD_B;
                        bool PadX = Pad->wButtons & XINPUT_GAMEPAD_X;
                        bool PadY = Pad->wButtons & XINPUT_GAMEPAD_Y;

                        int16 LStickX = Pad->sThumbLX;
                        int16 LStickY = Pad->sThumbLY;

                        XOffset += LStickX >> 12;
                        YOffset += LStickY >> 12;
                    }
                    else 
                    {
                        // controller is not available
                    }
                }

                XINPUT_VIBRATION Vibration;
                Vibration.wLeftMotorSpeed = 5000;
                Vibration.wRightMotorSpeed = 5000;
				XInputSetState(0, &Vibration);

                RenderPosGradient(&GlobalBackbuffer, XOffset, YOffset);
                win32_window_dimension Dimension = Win32GetWindowDimensions(WindowHandle);
                Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackbuffer, 0, 0, Dimension.Width, Dimension.Height);
            }

            ReleaseDC(WindowHandle, DeviceContext);
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