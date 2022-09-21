#pragma once

#include "Win32Window.h"

#include "../Core/Events/EventManager.h"
#include "../Core/Events/KeyboardEvent.h"

#include "../NewRenderer/Camera.h"

#include "../imgui/backends/imgui_impl_win32.h"

#include "Engine.h"

#include <windowsx.h>
#include <winuser.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
      HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Farlor {
Window<PlatformType::Win32>::Window(Engine &engine)
    : m_engine(engine)
{
}

bool Window<PlatformType::Win32>::Initialize(bool fullscreen)
{
    m_fullscreen = fullscreen;

    // Set the main window
    WNDCLASSEXA windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEXA);
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = GetModuleHandle(0);
    windowClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    windowClass.hIconSm = windowClass.hIcon;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    windowClass.lpszClassName = "WindowClass";
    windowClass.cbWndExtra = sizeof(Window<PlatformType::Win32> *);

    RegisterClassEx(&windowClass);

    DEVMODE screenSettings = { 0 };
    int posX, posY;

    if (m_fullscreen) {
        m_clientWidth = GetSystemMetrics(SM_CXSCREEN);
        m_clientHeight = GetSystemMetrics(SM_CYSCREEN);

        // In fullscreen mode, the client is the full size of the window
        m_windowWidth = m_clientWidth;
        m_windowHeight = m_clientHeight;

        screenSettings.dmSize = sizeof(screenSettings);
        screenSettings.dmPelsWidth = (unsigned long)m_clientWidth;
        screenSettings.dmPelsHeight = (unsigned long)m_clientHeight;
        screenSettings.dmBitsPerPel = 32;
        screenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN);

        posX = posY = 0;
    } else {
        m_clientWidth = static_cast<unsigned int>(800 * 1.5);
        m_clientHeight = static_cast<unsigned int>(600 * 1.5);

        RECT wr = { 0, 0, (LONG)m_clientWidth, (LONG)m_clientHeight };
        AdjustWindowRect(&wr, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_CAPTION, FALSE);

        m_windowWidth = wr.right - wr.left;
        m_windowHeight = wr.bottom - wr.top;

        posX = (GetSystemMetrics(SM_CXSCREEN) - m_windowWidth) / 2;
        posY = (GetSystemMetrics(SM_CYSCREEN) - m_windowHeight) / 2;
    }

    m_windowHandle = CreateWindowExA(WS_EX_APPWINDOW, "WindowClass", "Window Title",
          WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_CAPTION, posX, posY, m_windowWidth,
          m_windowHeight, 0, 0, GetModuleHandle(0), (LPVOID)this);
    SetWindowLongPtrA(m_windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    //// Lets go ahead and add raw input controller support
    //RAWINPUTDEVICE rid = { 0 };
    //// https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/hid-architecture#hid-clients-supported-in-windows
    //// 0x01 is for game controllers
    //rid.usUsagePage = 0x01;
    //rid.usUsage = 0x05;
    //rid.dwFlags = RIDEV_INPUTSINK;
    //rid.hwndTarget = m_windowHandle;
    //// Register for input events from controller
    //RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));

    m_isRunning = true;
    return true;
}

void Window<PlatformType::Win32>::Shutdown() { }

void Window<PlatformType::Win32>::ShowGameWindow()
{
    ShowWindow(m_windowHandle, SW_SHOW);
    SetForegroundWindow(m_windowHandle);
    SetFocus(m_windowHandle);
}

void Window<PlatformType::Win32>::ShowGameWindowCursor() { ShowCursor(false); }

void Window<PlatformType::Win32>::SetWindowTitleText(std::string windowText)
{
    SetWindowTextA(m_windowHandle, windowText.c_str());
}

Farlor::Vector2 Window<PlatformType::Win32>::GetWindowCenter() const
{
    RECT windowRect;
    GetWindowRect(m_windowHandle, &windowRect);
    Vector2 center { static_cast<float>(windowRect.right + windowRect.left) / 2,
        static_cast<float>(windowRect.top + windowRect.bottom) / 2 };
    return center;
}

void Window<PlatformType::Win32>::SetCursorPos(uint32_t x, uint32_t y)
{
    // ::SetCursorPos(x, y);
}

void Window<PlatformType::Win32>::ProcessSystemMessages()
{
    MSG msg = { 0 };

    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

struct OutputData {
    std::array<uint8_t, 96> buffer;
    HANDLE file = INVALID_HANDLE_VALUE;
    OVERLAPPED overlapped = { 0 };
};

uint32_t MakeReflectedCRC32(uint8_t *data, uint32_t byteCount)
{
    static const uint32_t crcTable[] = { 0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419,
        0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148,
        0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0,
        0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8,
        0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
        0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF,
        0xABD13D59, 0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
        0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87,
        0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
        0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E,
        0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162,
        0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6,
        0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D,
        0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5,
        0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525,
        0x206F85B3, 0xB966D409, 0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
        0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C,
        0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
        0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 0xF00F9344,
        0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
        0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43,
        0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767,
        0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3,
        0xA867DF55, 0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
        0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92,
        0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226,
        0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82,
        0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
        0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1,
        0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
        0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661,
        0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
        0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, 0xBDBDF21C, 0xCABAC28A, 0x53B39330,
        0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8,
        0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };
    uint32_t crc = (uint32_t)~0;
    while (byteCount--) {
        crc = (crc >> 8) ^ crcTable[(uint8_t)crc ^ *data];
        ++data;
    }
    return ~crc;
}

// We need to read out bytes per the HID format
// https://www.psdevwiki.com/ps4/DS4-USB
void UpdateDualshock4(uint8_t rawData[], DWORD byteCount, WCHAR *deviceName, OutputData &outputData)
{
    const DWORD usbInputByteCount = 64;
    const DWORD bluetoothInputByteCount = 547;

    int offset = 0;

    // Error out, we dont support bluetooth dualshock controller 4
    if (byteCount == bluetoothInputByteCount) {
        std::cout << "Error: We dont supprot bluetooth dualshock controlelr 4 atm" << std::endl;
        return;
    }

    uint8_t leftStickX = rawData[offset + 1];
    uint8_t leftStickY = rawData[offset + 2];
    uint8_t rightStickX = rawData[offset + 3];
    uint8_t rightStickY = rawData[offset + 4];

    // Get the dpad
    // Four LSB of data[5] are the dpad values
    uint8_t dpad = 0b00001111 & rawData[offset + 5];
    if (1 & (rawData[offset + 5] >> 4))
        printf("Square ");
    if (1 & (rawData[offset + 5] >> 5))
        printf("X ");
    if (1 & (rawData[offset + 5] >> 6))
        printf("O ");
    if (1 & (rawData[offset + 5] >> 7))
        printf("Triangle ");


    if (1 & (rawData[offset + 6] >> 0))
        printf("L1 ");
    if (1 & (rawData[offset + 6] >> 1))
        printf("R1 ");
    if (1 & (rawData[offset + 6] >> 2))
        printf("L2 ");
    if (1 & (rawData[offset + 6] >> 3))
        printf("R2 ");
    if (1 & (rawData[offset + 6] >> 4))
        printf("Share ");
    if (1 & (rawData[offset + 6] >> 5))
        printf("Options ");
    if (1 & (rawData[offset + 6] >> 6))
        printf("L3 ");
    if (1 & (rawData[offset + 6] >> 7))
        printf("R3 ");

    if (1 & (rawData[offset + 7] >> 0))
        printf("PS ");
    if (1 & (rawData[offset + 7] >> 1))
        printf("TouchPad ");

    // Botyh triggers signal 0 for not pressed, and 0xFF for pressed
    uint8_t leftTrigger = rawData[offset + 8];
    uint8_t rightTrigger = rawData[offset + 9];
    printf("DS4 - LX:%3d LY:%3d RX:%3d RY:%3d LT:%3d RT:%3d Dpad:%1d ", leftStickX, leftStickY,
          rightStickX, rightStickY, leftTrigger, rightTrigger, dpad);

    uint8_t battery = rawData[offset + 12];

    // There are gyro data values which can be read in, but lets not
    int16_t touch1X = ((rawData[offset + 37] & 0x0F) << 8) | rawData[offset + 36];
    int16_t touch1Y = ((rawData[offset + 37]) >> 4) | (rawData[offset + 38] << 4);
    int16_t touch2X = ((rawData[offset + 41] & 0x0F) << 8) | rawData[offset + 40];
    int16_t touch2Y = ((rawData[offset + 41]) >> 4) | (rawData[offset + 42] << 4);
    printf("Battery:%3d Touch1X:%4d Touch1Y:%4d Touch2X:%4d Touch2Y:%4d ", battery, touch1X,
          touch1Y, touch2X, touch2Y);

    printf("Buttons: ");
    printf("\n");

    // Ouput force-feedback and LED color
    int headerSize = 0;
    int outputByteCount = 0;
    if (byteCount == usbInputByteCount) {
        outputByteCount = 32;
        outputData.buffer[0] = 0x05;
        outputData.buffer[1] = 0xFF;
    }
    if (byteCount == bluetoothInputByteCount) {
        outputByteCount = 78;
        outputData.buffer[0] = 0xA2;  // Header - Bluetooth HID report type: data/output
        outputData.buffer[1] = 0x11;
        outputData.buffer[2] = 0XC0;
        outputData.buffer[4] = 0x07;
        headerSize = 1;
    }

    uint8_t lightRumble = rightTrigger;
    uint8_t heavyRumble = leftTrigger;
    uint8_t ledRed = (uint8_t)(touch1X * 255 / 2000);
    uint8_t ledGreen = (uint8_t)(touch1Y * 255 / 1000);
    uint8_t ledBlue = (uint8_t)(touch2Y * 255 / 1000);

    outputData.buffer[4 + offset + headerSize] = lightRumble;
    outputData.buffer[5 + offset + headerSize] = heavyRumble;
    outputData.buffer[6 + offset + headerSize] = ledRed;
    outputData.buffer[7 + offset + headerSize] = ledGreen;
    outputData.buffer[8 + offset + headerSize] = ledBlue;

    if (byteCount == bluetoothInputByteCount) {
        uint32_t crc = MakeReflectedCRC32(outputData.buffer.data(), 75);
        memcpy(outputData.buffer.data() + 75, &crc, sizeof(crc));
    }

    DWORD bytesTransferred = 0;
    bool finishedLastOutput
          = GetOverlappedResult(outputData.file, &outputData.overlapped, &bytesTransferred, false);
    if (finishedLastOutput) {
        if (outputData.file) {
            CloseHandle(outputData.file);
        }
        outputData.file = CreateFileW(deviceName, GENERIC_READ | GENERIC_WRITE,
              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
        if (outputData.file != INVALID_HANDLE_VALUE) {
            WriteFile(outputData.file, (void *)(outputData.buffer.data() + offset + headerSize),
                  outputByteCount, 0, &outputData.overlapped);
            CloseHandle(outputData.file);
        }
    }
}

bool IsDualshock4(RID_DEVICE_INFO_HID info)
{
    const DWORD sonyVendorID = 0x054C;
    const DWORD ds4Gen1ProductID = 0x05C4;
    const DWORD ds4Gen2ProductID = 0x09CC;

    return info.dwVendorId == sonyVendorID
          && (info.dwProductId == ds4Gen1ProductID || info.dwProductId == ds4Gen2ProductID);
}

void UpdateRawInput(LPARAM lParam, OutputData &outputData)
{
    UINT size = 0;
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
    RAWINPUT *input = (RAWINPUT *)malloc(size);
    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, input, &size, sizeof(RAWINPUTHEADER)) > 0) {
        RID_DEVICE_INFO deviceInfo;
        UINT deviceInfoSize = sizeof(deviceInfo);
        bool gotInfo = GetRawInputDeviceInfo(
                             input->header.hDevice, RIDI_DEVICEINFO, &deviceInfo, &deviceInfoSize)
              > 0;

        WCHAR deviceName[1024] = { 0 };
        UINT deviceNameLength = sizeof(deviceName) / sizeof(*deviceName);
        bool gotName = GetRawInputDeviceInfoW(
                             input->header.hDevice, RIDI_DEVICENAME, deviceName, &deviceNameLength)
              > 0;

        if (gotInfo && gotName) {
            if (IsDualshock4(deviceInfo.hid)) {
                std::cout << "Got dualshock 4 event" << std::endl;
                UpdateDualshock4(
                      input->data.hid.bRawData, input->data.hid.dwSizeHid, deviceName, outputData);
            } else {
                std::cout << "Other event" << std::endl;
            }
        }
    }
    free(input);
}

LRESULT CALLBACK Window<PlatformType::Win32>::WndProc(
      HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Window<PlatformType::Win32> *pWindow
          = reinterpret_cast<Window<PlatformType::Win32> *>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    // It is possible that we get passed a few messages initially, before the window is actually created.
    // Thus, we need to explicitly handle this.
    // TODO: Revisit to see if there is a better approach
    if (pWindow) {
        return pWindow->RealWndProc(hwnd, message, wParam, lParam);
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK Window<PlatformType::Win32>::RealWndProc(
      HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam)) {
        return true;
    }

    ImGuiIO &io = ImGui::GetIO();

    switch (message) {
        // Windows Messages
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;

        case WM_SIZE: {
            WindowEvent *pEvent = new WindowEvent();
            pEvent->type = WindowEventType::WindowResizeEvent;
            pEvent->WindowEventArgs.windowResizeEventArgs.newWidth = (uint32_t)LOWORD(lParam);
            pEvent->WindowEventArgs.windowResizeEventArgs.newHeight = (uint32_t)HIWORD(lParam);
            WindowEventManager(pEvent);
        } break;

        case WM_CREATE: {
            return 0;
        } break;

        case WM_INPUT: {
            OutputData outputData;
            UpdateRawInput(lParam, outputData);
        } break;

        case WM_LBUTTONDOWN: {
            if (!io.WantCaptureMouse) {
                WindowEvent *pEvent = new WindowEvent();
                pEvent->type = WindowEventType::MouseButtonEvent;
                pEvent->WindowEventArgs.mouseButtonEventArgs.button
                      = Farlor::WindowMouseButtons::Left;
                pEvent->WindowEventArgs.mouseButtonEventArgs.buttonState
                      = WindowMouseButtonState::Down;
                WindowEventManager(pEvent);
            }
        } break;

        case WM_LBUTTONUP: {
            if (!io.WantCaptureMouse) {
                WindowEvent *pEvent = new WindowEvent();
                pEvent->type = WindowEventType::MouseButtonEvent;
                pEvent->WindowEventArgs.mouseButtonEventArgs.button
                      = Farlor::WindowMouseButtons::Left;
                pEvent->WindowEventArgs.mouseButtonEventArgs.buttonState
                      = WindowMouseButtonState::Up;
                WindowEventManager(pEvent);
            }
        } break;

        case WM_RBUTTONDOWN: {
            if (!io.WantCaptureMouse) {
                // Able to use same ptr, callback responsible for deleting event memory
                WindowEvent *pEvent = new WindowEvent();
                pEvent->type = WindowEventType::MouseButtonEvent;
                pEvent->WindowEventArgs.mouseButtonEventArgs.button
                      = Farlor::WindowMouseButtons::Right;
                pEvent->WindowEventArgs.mouseButtonEventArgs.buttonState
                      = WindowMouseButtonState::Down;
                WindowEventManager(pEvent);
            }
        } break;

        case WM_RBUTTONUP: {
            if (!io.WantCaptureMouse) {
                // Able to use same ptr, callback responsible for deleting event memory
                WindowEvent *pEvent = new WindowEvent();
                pEvent->type = WindowEventType::MouseButtonEvent;
                pEvent->WindowEventArgs.mouseButtonEventArgs.button
                      = Farlor::WindowMouseButtons::Right;
                pEvent->WindowEventArgs.mouseButtonEventArgs.buttonState
                      = WindowMouseButtonState::Up;
                WindowEventManager(pEvent);
            }
        } break;

        case WM_MOUSEMOVE: {
            int newX = GET_X_LPARAM(lParam);
            int newY = GET_Y_LPARAM(lParam);

            if (!io.WantCaptureMouse) {
                WindowEvent *pEvent = new WindowEvent();

                POINT p;
                p.x = newX;
                p.y = newY;
                ClientToScreen(m_windowHandle, &p);

                pEvent->type = WindowEventType::MouseMoveEvent;
                pEvent->WindowEventArgs.mouseMoveEventArgs.xPos = p.x;
                pEvent->WindowEventArgs.mouseMoveEventArgs.yPos = p.y;
                WindowEventManager(pEvent);
            }
        } break;

        // Keyboard Messages
        case WM_KEYDOWN: {
            if (!io.WantCaptureKeyboard && !io.WantCaptureMouse) {
                // cout << "WM_KEYDOWN" << endl;

#define ADD_KEY_DOWN(Win32_Key, WKB)                                                               \
    case Win32_Key: {                                                                              \
        WindowEvent *pEvent = new WindowEvent();                                                   \
        pEvent->type = WindowEventType::KeyboardButtonEvent;                                       \
        pEvent->WindowEventArgs.keyboardButtonEventArgs.button                                     \
              = Farlor::WindowKeyboardButton::WKB;                                                 \
        pEvent->WindowEventArgs.mouseButtonEventArgs.buttonState = WindowMouseButtonState::Down;   \
        WindowEventManager(pEvent);                                                                \
    } break;

#define ADD_KEY_DOWN(Win32_Key, WKB)                                                               \
    case Win32_Key: {                                                                              \
        WindowEvent *pEvent = new WindowEvent();                                                   \
        pEvent->type = WindowEventType::KeyboardButtonEvent;                                       \
        pEvent->WindowEventArgs.keyboardButtonEventArgs.button                                     \
              = Farlor::WindowKeyboardButton::WKB;                                                 \
        pEvent->WindowEventArgs.mouseButtonEventArgs.buttonState = WindowMouseButtonState::Down;   \
        WindowEventManager(pEvent);                                                                \
    } break;

                switch (wParam) {
                    ADD_KEY_DOWN(VK_ESCAPE, Escape)
                    ADD_KEY_DOWN(VK_SPACE, Space)
                    ADD_KEY_DOWN(VK_CONTROL, Ctrl)
                    ADD_KEY_DOWN(VK_SHIFT, Shift)

                    ADD_KEY_DOWN('A', A);
                    ADD_KEY_DOWN('B', B);
                    ADD_KEY_DOWN('C', C);
                    ADD_KEY_DOWN('D', D);
                    ADD_KEY_DOWN('E', E);
                    ADD_KEY_DOWN('F', F);
                    ADD_KEY_DOWN('G', G);
                    ADD_KEY_DOWN('H', H);
                    ADD_KEY_DOWN('I', I);
                    ADD_KEY_DOWN('J', J);
                    ADD_KEY_DOWN('K', K);
                    ADD_KEY_DOWN('L', L);
                    ADD_KEY_DOWN('M', M);
                    ADD_KEY_DOWN('N', N);
                    ADD_KEY_DOWN('O', O);
                    ADD_KEY_DOWN('P', P);
                    ADD_KEY_DOWN('Q', Q);
                    ADD_KEY_DOWN('R', R);
                    ADD_KEY_DOWN('S', S);
                    ADD_KEY_DOWN('T', T);
                    ADD_KEY_DOWN('U', U);
                    ADD_KEY_DOWN('V', V);
                    ADD_KEY_DOWN('W', W);
                    ADD_KEY_DOWN('X', X);
                    ADD_KEY_DOWN('Y', Y);
                    ADD_KEY_DOWN('Z', Z);
#undef ADD_KEY_DOWN

                    default: {
                    } break;
                }
            }
        } break;

#define ADD_KEY_UP(Win32_Key, WKB)                                                                 \
    case Win32_Key: {                                                                              \
        WindowEvent *pEvent = new WindowEvent();                                                   \
        pEvent->type = WindowEventType::KeyboardButtonEvent;                                       \
        pEvent->WindowEventArgs.keyboardButtonEventArgs.button                                     \
              = Farlor::WindowKeyboardButton::WKB;                                                 \
        pEvent->WindowEventArgs.mouseButtonEventArgs.buttonState = WindowMouseButtonState::Up;     \
        WindowEventManager(pEvent);                                                                \
    } break;

        case WM_KEYUP: {
            if (!io.WantCaptureKeyboard && !io.WantCaptureMouse) {
                // cout << "WM_KEYDOWN" << endl;
                switch (wParam) {
                    ADD_KEY_UP(VK_ESCAPE, Escape)
                    ADD_KEY_UP(VK_SPACE, Space)
                    ADD_KEY_UP(VK_CONTROL, Ctrl)
                    ADD_KEY_UP(VK_SHIFT, Shift)

                    ADD_KEY_UP('A', A);
                    ADD_KEY_UP('B', B);
                    ADD_KEY_UP('C', C);
                    ADD_KEY_UP('D', D);
                    ADD_KEY_UP('E', E);
                    ADD_KEY_UP('F', F);
                    ADD_KEY_UP('G', G);
                    ADD_KEY_UP('H', H);
                    ADD_KEY_UP('I', I);
                    ADD_KEY_UP('J', J);
                    ADD_KEY_UP('K', K);
                    ADD_KEY_UP('L', L);
                    ADD_KEY_UP('M', M);
                    ADD_KEY_UP('N', N);
                    ADD_KEY_UP('O', O);
                    ADD_KEY_UP('P', P);
                    ADD_KEY_UP('Q', Q);
                    ADD_KEY_UP('R', R);
                    ADD_KEY_UP('S', S);
                    ADD_KEY_UP('T', T);
                    ADD_KEY_UP('U', U);
                    ADD_KEY_UP('V', V);
                    ADD_KEY_UP('W', W);
                    ADD_KEY_UP('X', X);
                    ADD_KEY_UP('Y', Y);
                    ADD_KEY_UP('Z', Z);
#undef ADD_KEY_UP

                    default: {
                    } break;
                }
            }
        } break;

        default: {
        } break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

void Window<PlatformType::Win32>::WindowEventManager(WindowEvent *pEvent)
{
    if (!pEvent) {
        return;
    }

    switch (pEvent->type) {
            // Window Events
        case WindowEventType::WindowClosedEvent: {
            // We want to exit the app!!
            m_engine.TriggerShutdown();
        } break;

        case WindowEventType::WindowResizeEvent: {
        } break;

        // Mouse Events
        // Calculate the delta movement then reset the cursor position
        case WindowEventType::MouseMoveEvent: {
            // NOTE: MUST be signed
            int32_t deltaX = static_cast<int32_t>(pEvent->WindowEventArgs.mouseMoveEventArgs.xPos);
            int32_t deltaY = static_cast<int32_t>(pEvent->WindowEventArgs.mouseMoveEventArgs.yPos);

            m_engine.AccessInputStateManager().SetMousePosition(
                  static_cast<int32_t>(pEvent->WindowEventArgs.mouseMoveEventArgs.xPos),
                  static_cast<int32_t>(pEvent->WindowEventArgs.mouseMoveEventArgs.yPos));
        } break;

        case WindowEventType::MouseButtonEvent: {
            bool state = (pEvent->WindowEventArgs.mouseButtonEventArgs.buttonState
                               == WindowMouseButtonState::Down)
                  ? true
                  : false;

            switch (pEvent->WindowEventArgs.mouseButtonEventArgs.button) {
#define ADD_MOUSE_CASE(Button)                                                                     \
    case WindowMouseButtons::##Button: {                                                           \
        m_engine.AccessInputStateManager().SetMouseState(MouseButtons::##Button, state);           \
    } break;

                // Special
                ADD_MOUSE_CASE(Left);
                ADD_MOUSE_CASE(Right);
                ADD_MOUSE_CASE(Center);

                default: {
                } break;
            };
        } break;

        // Keyboard Events
        case Farlor::WindowEventType::KeyboardButtonEvent: {
            bool state = (pEvent->WindowEventArgs.keyboardButtonEventArgs.buttonState
                               == Farlor::WindowKeyboardButtonState::Down)
                  ? true
                  : false;

            switch (pEvent->WindowEventArgs.keyboardButtonEventArgs.button) {
#define ADD_KEYBOARD_CASE(Key)                                                                     \
    case Farlor::WindowKeyboardButton::##Key: {                                                    \
        m_engine.AccessInputStateManager().SetKeyboardState(                                       \
              Farlor::KeyboardButtons::##Key, state);                                              \
    } break;

                // Special
                ADD_KEYBOARD_CASE(Backspace);
                ADD_KEYBOARD_CASE(Tab);
                ADD_KEYBOARD_CASE(Return);
                ADD_KEYBOARD_CASE(Shift);
                ADD_KEYBOARD_CASE(Ctrl);
                ADD_KEYBOARD_CASE(Alt);
                ADD_KEYBOARD_CASE(Space);
                ADD_KEYBOARD_CASE(Escape);

                // Arrow Keys
                ADD_KEYBOARD_CASE(LeftArrow);
                ADD_KEYBOARD_CASE(UpArrow);
                ADD_KEYBOARD_CASE(RightArrow);
                ADD_KEYBOARD_CASE(DownArrow);

                // Numbers
                ADD_KEYBOARD_CASE(Zero);
                ADD_KEYBOARD_CASE(One);
                ADD_KEYBOARD_CASE(Two);
                ADD_KEYBOARD_CASE(Three);
                ADD_KEYBOARD_CASE(Four);
                ADD_KEYBOARD_CASE(Five);
                ADD_KEYBOARD_CASE(Six);
                ADD_KEYBOARD_CASE(Seven);
                ADD_KEYBOARD_CASE(Eight);
                ADD_KEYBOARD_CASE(Nine);

                // Alphabet
                ADD_KEYBOARD_CASE(A);
                ADD_KEYBOARD_CASE(B);
                ADD_KEYBOARD_CASE(C);
                ADD_KEYBOARD_CASE(D);
                ADD_KEYBOARD_CASE(E);
                ADD_KEYBOARD_CASE(F);
                ADD_KEYBOARD_CASE(G);
                ADD_KEYBOARD_CASE(H);
                ADD_KEYBOARD_CASE(I);
                ADD_KEYBOARD_CASE(J);
                ADD_KEYBOARD_CASE(K);
                ADD_KEYBOARD_CASE(L);
                ADD_KEYBOARD_CASE(M);
                ADD_KEYBOARD_CASE(N);
                ADD_KEYBOARD_CASE(O);
                ADD_KEYBOARD_CASE(P);
                ADD_KEYBOARD_CASE(Q);
                ADD_KEYBOARD_CASE(R);
                ADD_KEYBOARD_CASE(S);
                ADD_KEYBOARD_CASE(T);
                ADD_KEYBOARD_CASE(U);
                ADD_KEYBOARD_CASE(V);
                ADD_KEYBOARD_CASE(W);
                ADD_KEYBOARD_CASE(X);
                ADD_KEYBOARD_CASE(Y);
                ADD_KEYBOARD_CASE(Z);
                ADD_KEYBOARD_CASE(Tilda);
                default: {
                } break;
            };
        } break;
    };
    // NOTE: This requires the same c-runtime to be linked.
    delete pEvent;
};
}
