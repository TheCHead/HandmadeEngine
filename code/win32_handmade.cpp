#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>

#define internal_func static
#define local_persist static
#define global_var static
#define Pi32 3.14159265359f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

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

// DirectSound
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

global_var bool32 GlobalRunning = false;
global_var win32_offscreen_buffer GlobalBackbuffer;
global_var LPDIRECTSOUNDBUFFER GlobalDSSecondaryBuffer;

struct win32_sound_output
{
    int SamplesPerSecond;
    int ToneHz;
    int ToneVolume;
    uint32 RunningSampleIndex;
    int WavePeriod;
    int BytesPerSample;
    int SecondaryBufferSize;
    //bool32 SoundIsPlaying;
};

internal_func void Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary)
    {
        // TODO: Diagnostic
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
    if (!XInputLibrary)
    {
        // TODO: Diagnostic
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    
    if (XInputLibrary)
    {
        XInputGetState_ = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState_ = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
    else
    {
        // TODO: Diagnostic
    }
}

internal_func void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    // Load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if (DSoundLibrary)
    {
        // Get a DirectSound object
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                // Create a primary buffer
                DSBUFFERDESC BufferDesc = {};
                BufferDesc.dwSize = sizeof(BufferDesc);
                BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesc, &PrimaryBuffer, 0)))
                {
                    if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                    {
                        OutputDebugStringA("Primary Buffer Set\n");
                    }
                    else
                    {
                        //TODO: Diagnostic
                    }
                }
                else
                {
                    //TODO: Diagnostic
                }
            }
            else
            {
                //TODO: Diagnostic
            }

            // Create a secondary buffer to write to
            DSBUFFERDESC BufferDesc = {};
            BufferDesc.dwSize = sizeof(BufferDesc);
            BufferDesc.dwFlags = 0;
            BufferDesc.dwBufferBytes = BufferSize;
            BufferDesc.lpwfxFormat = &WaveFormat;
            
            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesc, &GlobalDSSecondaryBuffer, 0)))
            {
                OutputDebugStringA("Secondary Buffer Set\n");
            }
            else
            {
                //TODO: Diagnostic
            }
        }
        else
        {
            // TODO: Diagnostic
        }
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
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
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
            GlobalRunning = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = wParam;
            bool32 WasDown = ((lParam & (1 << 30)) != 0);
            bool32 IsDown = ((lParam & (1 << 31)) == 0);
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
                GlobalRunning = false;
            }
        } break;

        case WM_CLOSE:
        {
            GlobalRunning = false;
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
            Result = DefWindowProcA(hwnd, uMsg, wParam, lParam);
        } break;
    }
    return(Result);
}

void Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    
    if (SUCCEEDED(GlobalDSSecondaryBuffer->Lock(
        ByteToLock, BytesToWrite,
        &Region1, &Region1Size,
        &Region2, &Region2Size,
        0 )))
    {
        // Assert RegionSize is valid
        int16 *SampleOut = (int16 *)Region1;
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
        {
            real32 t = 2.0 * Pi32 * (real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput->WavePeriod;
            real32 SineValue = sinf(t);
            int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;
            ++SoundOutput->RunningSampleIndex;
        }
    
        SampleOut = (int16 *)Region2;
        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
        {
            real32 t = 2.0 * Pi32 * (real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput->WavePeriod;
            real32 SineValue = sinf(t);
            int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;
            ++SoundOutput->RunningSampleIndex;
        }

        GlobalDSSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
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

    if(RegisterClassA(&WindowClass))
    {
        HWND WindowHandle = CreateWindowExA(
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

            // Graphics test
            int XOffset = 0;
            int YOffset = 0;

            win32_sound_output SoundOutput = {};
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.ToneHz = 256;
            SoundOutput.ToneVolume = 3000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
            SoundOutput.BytesPerSample = sizeof(int16)*2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;

            // Initializing DirectSound only after Window initialization
            Win32InitDSound(WindowHandle, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.SecondaryBufferSize);
            GlobalDSSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);        

            GlobalRunning = true;
            while(GlobalRunning)
            {
                MSG Message;
                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
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

                        bool32 DPadUp = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        bool32 DPadDown = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        bool32 DPadLeft = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        bool32 DPadRight = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        bool32 PadStart = Pad->wButtons & XINPUT_GAMEPAD_START;
                        bool32 PadBack = Pad->wButtons & XINPUT_GAMEPAD_BACK;
                        bool32 PadLeftShoulder = Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        bool32 PadRightShoulder = Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        bool32 PadA = Pad->wButtons & XINPUT_GAMEPAD_A;
                        bool32 PadB = Pad->wButtons & XINPUT_GAMEPAD_B;
                        bool32 PadX = Pad->wButtons & XINPUT_GAMEPAD_X;
                        bool32 PadY = Pad->wButtons & XINPUT_GAMEPAD_Y;

                        int16 LStickX = Pad->sThumbLX;
                        int16 LStickY = Pad->sThumbLY;

                        XOffset += LStickX / 4096;
                        YOffset += LStickY / 4096;
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

                // Direct Sound output test
                DWORD PlayCursor;
                DWORD WriteCursor;
                if (SUCCEEDED(GlobalDSSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    DWORD ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
                    DWORD BytesToWrite = 0;

                    if (ByteToLock > PlayCursor)
                    {
                        BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
                        BytesToWrite += PlayCursor;
                    }
                    else
                    {
                        BytesToWrite = PlayCursor - ByteToLock;
                    }

                    Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);
                }   
                
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