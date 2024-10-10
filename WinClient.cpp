#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <Ws2tcpip.h> //InetPtonW
#include <cstdio>
#include <commctrl.h>
#include <wchar.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")

#define MAX_LOADSTRING 102
#define WM_SOCKET (WM_USER + 1)

// Global variables
HINSTANCE hInst;
WCHAR szTitle[] = L"Chat Client";      // Changed to wide character string
WCHAR szWindowClass[] = L"ChatWindow"; // Changed to wide character string
SOCKET clientSocket;
HWND hEditInput;
HWND hEditOutput;
HWND hEditID;
HWND hEditChannel;
HWND hEditIP;

// Window procedure function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Create IP address input box
        CreateWindowExW(0, L"STATIC", L"IP:",
                        WS_CHILD | WS_VISIBLE,
                        10, 10, 80, 25, hwnd, nullptr, hInst, nullptr);
        hEditIP = CreateWindowExW(0, L"EDIT", L"",
                                  WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                  100, 10, 200, 25, hwnd, nullptr, hInst, nullptr);

        // Create ID input box
        CreateWindowExW(0, L"STATIC", L"ID:",
                        WS_CHILD | WS_VISIBLE,
                        10, 40, 80, 25, hwnd, nullptr, hInst, nullptr);
        hEditID = CreateWindowExW(0, L"EDIT", L"",
                                  WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                  100, 40, 200, 25, hwnd, nullptr, hInst, nullptr);

        // Create channel input box
        CreateWindowExW(0, L"STATIC", L"Channel:",
                        WS_CHILD | WS_VISIBLE,
                        10, 70, 100, 25, hwnd, nullptr, hInst, nullptr);
        hEditChannel = CreateWindowExW(0, L"EDIT", L"",
                                       WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                       100, 70, 200, 25, hwnd, nullptr, hInst, nullptr);

        // Create input box
        hEditInput = CreateWindowExW(0, L"EDIT", L"",
                                     WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_MULTILINE,
                                     10, 100, 370, 50, hwnd, nullptr, hInst, nullptr);
        // Create send button
        HWND hButtonSend = CreateWindowExW(0, L"BUTTON", L"Send",
                                           WS_CHILD | WS_VISIBLE | WS_DISABLED,
                                           390, 100, 80, 50, hwnd, (HMENU)1, hInst, nullptr);

        // Create output box
        hEditOutput = CreateWindowExW(0, L"EDIT", L"",
                                      WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_MULTILINE | ES_READONLY,
                                      10, 160, 460, 230, hwnd, nullptr, hInst, nullptr);
        // Create connect button
        CreateWindowExW(0, L"BUTTON", L"Connect",
                        WS_CHILD | WS_VISIBLE,
                        390, 40, 80, 50, hwnd, (HMENU)2, hInst, nullptr);
        break;
    }
    case WM_ERASEBKGND:
    {
        // Set the window background color to white
        HDC hdc = (HDC)wParam;
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));

        RECT rect = {0, 0, 460, 450};
        FillRect(hdc, &rect, hBrush);

        DeleteObject(hBrush);
        return 1;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == 1)
        {
            // Send button was clicked
            wchar_t buffer[256];
            GetWindowTextW(hEditInput, buffer, sizeof(buffer) / sizeof(buffer[0]));

            // Prepend "You: " to the sent message
            wchar_t message[256];
            swprintf(message, sizeof(message) / sizeof(message[0]), L"You: %s", buffer);

            // Send message (convert wide string to multibyte string)
            char mbBuffer[256];
            wcstombs(mbBuffer, buffer, sizeof(mbBuffer));

            if (send(clientSocket, mbBuffer, strlen(mbBuffer), 0) == SOCKET_ERROR)
            {
                MessageBoxW(hwnd, L"Failed to send message!", L"Error", MB_OK);
            }
            else
            {
                // Display the sent message in the output box
                SendMessageW(hEditOutput, EM_REPLACESEL, TRUE, (LPARAM)message);
                SendMessageW(hEditOutput, EM_REPLACESEL, TRUE, (LPARAM)L"\r\n");
                SetWindowTextW(hEditInput, L""); // Clear input box
            }
        }
        else if (LOWORD(wParam) == 2)
        {
            // Connect button was clicked
            wchar_t idBuffer[256];
            GetWindowTextW(hEditID, idBuffer, sizeof(idBuffer) / sizeof(idBuffer[0]));

            wchar_t channelBuffer[256];
            GetWindowTextW(hEditChannel, channelBuffer, sizeof(channelBuffer) / sizeof(channelBuffer[0]));

            wchar_t ipBuffer[256];
            GetWindowTextW(hEditIP, ipBuffer, sizeof(ipBuffer) / sizeof(ipBuffer[0]));

            if (wcslen(idBuffer) == 0)
            {
                MessageBoxW(hwnd, L"Please enter your ID!", L"Warning", MB_OK);
                return 0;
            }
            if (wcslen(channelBuffer) == 0)
            {
                MessageBoxW(hwnd, L"Please enter a channel number!", L"Warning", MB_OK);
                return 0;
            }
            if (wcslen(ipBuffer) == 0)
            {
                MessageBoxW(hwnd, L"Please enter an IP address!", L"Warning", MB_OK);
                return 0;
            }

            // Convert wide char IP to multibyte
            clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            struct sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(9527);

            // Using InetPtonW for wide char IP conversion
            if (InetPtonW(AF_INET, ipBuffer, &serverAddr.sin_addr) != 1)
            {
                MessageBoxW(hwnd, L"Invalid IP address!", L"Error", MB_OK);
                return 1;
            }

            if (connect(clientSocket, (SOCKADDR *)&serverAddr, sizeof(serverAddr)) != 0)
            {
                // Add detailed error handling
                int errorCode = WSAGetLastError();
                wchar_t errorMessage[256];
                swprintf(errorMessage, sizeof(errorMessage) / sizeof(errorMessage[0]), L"Failed to connect to server! Error code: %d", errorCode);
                MessageBoxW(nullptr, errorMessage, L"Error", MB_OK);
                return 1;
            }

            // Enable the send button and notify the user
            EnableWindow(GetDlgItem(hwnd, 1), TRUE);
            MessageBoxW(hwnd, L"Connected to server! You can start chatting now.", L"Info", MB_OK);

            // Send ID and channel to server (convert wide char to multibyte)
            char mbID[256];
            wcstombs(mbID, idBuffer, sizeof(mbID));
            send(clientSocket, mbID, strlen(mbID), 0);

            // Ensure a newline or delimiter is sent between ID and channel
            send(clientSocket, "\n", 1, 0); // Add this line to separate ID and channel

            char mbChannel[256];
            wcstombs(mbChannel, channelBuffer, sizeof(mbChannel));
            send(clientSocket, mbChannel, strlen(mbChannel), 0);

            WSAAsyncSelect(clientSocket, hwnd, WM_SOCKET, FD_READ);
        }
        break;
    }
    case WM_SOCKET:
    {
        if (WSAGETSELECTEVENT(lParam) == FD_READ)
        {
            char buffer[256];
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived > 0)
            {
                buffer[bytesReceived] = '\0'; // Null-terminate the received string

                // Convert the multibyte string to wide character string
                wchar_t wBuffer[256];
                MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wBuffer, sizeof(wBuffer) / sizeof(wchar_t));

                // Display the received message in the output box
                SendMessageW(hEditOutput, EM_REPLACESEL, TRUE, (LPARAM)wBuffer);
                SendMessageW(hEditOutput, EM_REPLACESEL, TRUE, (LPARAM)L"\r\n");
            }
            else
            {
                // Connection closed by server
                closesocket(clientSocket);
                MessageBoxW(hwnd, L"Connection closed by server!", L"Info", MB_OK);
                EnableWindow(GetDlgItem(hwnd, 1), FALSE);
            }
        }
        break;
    }

    case WM_DESTROY:
        closesocket(clientSocket);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// WinMain function
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    hInst = hInstance;

    // Register the window class
    WNDCLASSEXW wc = {sizeof(WNDCLASSEX)};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = szWindowClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);

    // Create the window
    HWND hwnd = CreateWindowExW(0, szWindowClass, szTitle,
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 450,
                                nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);

    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    WSACleanup();
    return (int)msg.wParam;
}
