#include <iostream>
#include <Windows.h>
#include <vector>
#include <thread>
//SERVER SHIT
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")


#define BUFFER_SIZE 44100 * 2
#define SERVER_IP "127.0.0.1"
#define PORT 8080

void ProcessAudioData(char* buffer, int size, SOCKET serverSocket) {
    send(serverSocket, buffer, size, 0);
}

void CALLBACK WaveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WIM_DATA) {
        WAVEHDR* waveHdr = reinterpret_cast<WAVEHDR*>(dwParam1);
        SOCKET serverSocket = dwInstance;

        ProcessAudioData(waveHdr->lpData, waveHdr->dwBytesRecorded, serverSocket);
        waveInAddBuffer(hwi, waveHdr, sizeof(WAVEHDR)); // Requeue the buffer for further data
    }
}

void CaptureAndSendAudio(SOCKET serverSocket) {


    int sampleRate = 44100;
    HWAVEIN hWaveIn;
    WAVEFORMATEX pFormat;
    pFormat.wFormatTag = WAVE_FORMAT_PCM;     // simple, uncompressed format
    pFormat.nChannels = 1;                    //  1=mono, 2=stereo
    pFormat.nSamplesPerSec = sampleRate;      // 44100
    pFormat.nAvgBytesPerSec = sampleRate * 2;   // = nSamplesPerSec * n.Channels *    wBitsPerSample/8
    pFormat.nBlockAlign = 2;                  // = n.Channels * wBitsPerSample/8
    pFormat.wBitsPerSample = 16;              //  16 for high quality, 8 for telephone-grade
    pFormat.cbSize = 0;

 
    MMRESULT result = waveInOpen(&hWaveIn, WAVE_MAPPER, &pFormat, reinterpret_cast<DWORD_PTR>(&WaveInProc), serverSocket, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to open wave input device!" << std::endl;
        return;
    }

    char buffer[BUFFER_SIZE];
    WAVEHDR WaveInHdr;
    WaveInHdr.lpData = reinterpret_cast<LPSTR>(buffer);
    WaveInHdr.dwBufferLength = BUFFER_SIZE;
    WaveInHdr.dwBytesRecorded = 0;
    WaveInHdr.dwUser = 0L;
    WaveInHdr.dwFlags = 0L;
    WaveInHdr.dwLoops = 0L;
    waveInPrepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));

    // Insert a wave input buffer
    waveInAddBuffer(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
    // Commence sampling input
    waveInStart(hWaveIn);
    // Keep the capturing thread running
    while (true) {
        Sleep(100); // Adjust the sleep duration as needed
    }

    waveInClose(hWaveIn);
}

int main() {
    WSADATA wsaData;
    WSADATA wsa;
    SOCKET client_socket;
    struct sockaddr_in server;


    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    // Create socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_port = htons(PORT);

    // Connect to server
    if (connect(client_socket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return 1;
    }


  
    std::thread audioThread(CaptureAndSendAudio, client_socket);
    audioThread.detach(); // Detach the thread to let it run independently

    while (true) {
       //lloop
    }

    closesocket(client_socket);
    WSACleanup();

    return 0;
}
