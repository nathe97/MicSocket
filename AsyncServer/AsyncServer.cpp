#include <iostream>
#include <Windows.h>
#include <vector>
#include <thread>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 44100 * 2

void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WOM_DONE) {


        WAVEHDR* waveHdr = reinterpret_cast<WAVEHDR*>(dwParam1);
        MMRESULT resul = waveOutUnprepareHeader(hwo, waveHdr, sizeof(WAVEHDR));
        if (resul != MMSYSERR_NOERROR) {
            std::cerr << "Failed to open wave waveOutUnprepareHeader device!" << std::endl;
            return;
        }
        // delete[] waveHdr->lpData;
        // delete waveHdr;

    }

}

void PlayAudioData(char* buffer, int size, HWAVEOUT hWaveOut) {



    WAVEHDR* waveHdr = new WAVEHDR;
    waveHdr->lpData = reinterpret_cast<LPSTR>(buffer);
    waveHdr->dwBufferLength = BUFFER_SIZE;
    waveHdr->dwBytesRecorded = BUFFER_SIZE;
    waveHdr->dwUser = 0L;
    waveHdr->dwFlags = 0L;
    waveHdr->dwLoops = 0L;
    MMRESULT result = waveOutPrepareHeader(hWaveOut, waveHdr, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to open wave waveOutPrepareHeader device!" << std::endl;

    }
    MMRESULT rr = waveOutWrite(hWaveOut, waveHdr, BUFFER_SIZE);
    if (rr != MMSYSERR_NOERROR) {
        std::cerr << "Failed to open wave waveOutPrepareHeader device!" << std::endl;

    }


}

void ReceiveAndPlayAudio(SOCKET serverSocket) {

    int sampleRate = 44100;
    WAVEFORMATEX pFormat;
    pFormat.wFormatTag = WAVE_FORMAT_PCM;     // simple, uncompressed format
    pFormat.nChannels = 1;                    //  1=mono, 2=stereo
    pFormat.nSamplesPerSec = sampleRate;      // 44100
    pFormat.nAvgBytesPerSec = sampleRate * 2;   // = nSamplesPerSec * n.Channels * wBitsPerSample/8
    pFormat.nBlockAlign = 2;                  // = n.Channels * wBitsPerSample/8
    pFormat.wBitsPerSample = 16;              //  16 for high quality, 8 for telephone-grade
    pFormat.cbSize = 0;

    HWAVEIN  hWaveIn;


    HWAVEOUT hWaveOut;
    MMRESULT result = waveOutOpen(&hWaveOut, WAVE_MAPPER, &pFormat, reinterpret_cast<DWORD_PTR>(&WaveOutProc), serverSocket, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to open wave output device!" << std::endl;
        return;
    }


    char buffer[BUFFER_SIZE];
    while (true) {

        int bytesReceived = recv(serverSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            std::cout << "[+]Received Packet: " << bytesReceived << std::endl;
            PlayAudioData(buffer, bytesReceived, hWaveOut);
            Sleep(100);
        }

    }


    waveOutClose(hWaveOut);
}

int main() {

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Socket binding failed!" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Socket listening failed!" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }


    std::cout << "[+]Server listening on port " << PORT << std::endl;

    // Accept incoming connections
    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Failed to accept connection!" << std::endl;
            continue;
        }


        char* ip = inet_ntoa(serverAddr.sin_addr);
        int port = ntohs(serverAddr.sin_port);
        std::cout << "[+]Connection from IP: " << ip << " PORT: " << port << std::endl;
        std::cout << "[+]Start Receiving Audio from IP: " << ip << " PORT: " << port << std::endl;
        std::thread audioThread(ReceiveAndPlayAudio, clientSocket);
        audioThread.detach(); // Detach the thread to let it run independently

        while (true) {
            // Other tasks or processing in the main thread
        }

        closesocket(clientSocket);
        closesocket(serverSocket);
        WSACleanup();

        return 0;
    }
}
