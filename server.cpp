#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <random>
#include <cstring>
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using SocketType = SOCKET;
#define CLOSESOCKET(s) closesocket(s)
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
using SocketType = int;
#define CLOSESOCKET(s) close(s)
#endif

constexpr int PORT = 8888;

bool readLine(SocketType sock, std::string &line)
{
    line.clear();
    char ch;
    while (true)
    {
        int bytesRead = recv(sock, &ch, 1, 0);
        if (bytesRead <= 0)
        {
            return false;
        }
        if (ch == '\r')
        {
            continue;
        }
        if (ch == '\n')
        {
            break;
        }
        line.push_back(ch);
    }
    return true;
}

bool sendAll(SocketType sock, const std::string &msg)
{
    size_t totalSent = 0;
    while (totalSent < msg.size())
    {
        int sent = send(sock, msg.data() + totalSent, static_cast<int>(msg.size() - totalSent), 0);
        if (sent <= 0)
        {
            return false;
        }
        totalSent += static_cast<size_t>(sent);
    }
    return true;
}

void handleClient(SocketType clientSock, sockaddr_in clientAddr)
{
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
    std::cout << "[+] Client ket noi tu: " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;

    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::uniform_int_distribution<int> distN(5, 15);
    std::uniform_int_distribution<long long> distVal(-2147483648LL, 2147483647LL);

    int N = distN(rng);
    std::vector<long long> numbers(N);
    long long expectedSum = 0;

    for (int i = 0; i < N; ++i)
    {
        numbers[i] = distVal(rng);
        expectedSum += numbers[i];
    }

    std::string headerMsg = "TASK SUM " + std::to_string(N) + "\n";
    if (!sendAll(clientSock, headerMsg))
    {
        CLOSESOCKET(clientSock);
        return;
    }

    std::ostringstream dataStream;
    for (int i = 0; i < N; ++i)
    {
        dataStream << numbers[i] << (i + 1 == N ? "" : " ");
    }
    dataStream << "\n";
    if (!sendAll(clientSock, dataStream.str()))
    {
        CLOSESOCKET(clientSock);
        return;
    }

    std::cout << "[*] Da gui " << N << " so cho client. Tong dung la: " << expectedSum << std::endl;

    std::string clientResponse;
    if (readLine(clientSock, clientResponse))
    {
        std::cout << "[<] Nhan tu client: " << clientResponse << std::endl;

        std::istringstream iss(clientResponse);
        std::string tag;
        long long clientSum;

        if ((iss >> tag >> clientSum) && tag == "RESULT")
        {
            if (clientSum == expectedSum)
            {
                sendAll(clientSock, "OK\n");
                std::cout << "[>] Ket qua: DUNG (Da gui OK)" << std::endl;
            }
            else
            {
                sendAll(clientSock, "WRONG\n");
                std::cout << "[>] Ket qua: SAI (Da gui WRONG)" << std::endl;
            }
        }
        else
        {
            sendAll(clientSock, "ERROR\n");
            std::cout << "[!] Sai cu phap result tu client (Da gui ERROR)" << std::endl;
        }
    }

    sendAll(clientSock, "BYE\n");
    CLOSESOCKET(clientSock);
    std::cout << "[-] Dong ket noi voi: " << clientIP << std::endl;
}

int main()
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "[-] Khong the khoi tao Winsock!" << std::endl;
        return 1;
    }
#endif

    SocketType serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSock == -1)
    {
        std::cerr << "[-] Khong the khoi tao socket!" << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&opt), sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSock, reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0)
    {
        std::cerr << "[-] Loi khi bind cong " << PORT << "!" << std::endl;
        CLOSESOCKET(serverSock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(serverSock, 10) < 0)
    {
        std::cerr << "[-] Loi khi listen!" << std::endl;
        CLOSESOCKET(serverSock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << " SERVER DANG LANG NGHE TAI CONG: " << PORT << std::endl;
    std::cout << "========================================" << std::endl;

    while (true)
    {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        SocketType clientSock = accept(serverSock, reinterpret_cast<struct sockaddr *>(&clientAddr), &clientLen);
        if (clientSock < 0)
        {
            std::cerr << "[-] Loi accept client!" << std::endl;
            continue;
        }

        std::thread clientThread(handleClient, clientSock, clientAddr);
        clientThread.detach();
    }

    CLOSESOCKET(serverSock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
