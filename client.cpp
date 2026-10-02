#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <sstream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

// ============================================================
// Hàm đọc một dòng từ socket cho đến khi gặp '\n'
// Trả về chuỗi đã bỏ '\r' và '\n'. Trả về "" nếu mất kết nối.
// ============================================================
string recvLine(SOCKET sock) {
    string line;
    char c;
    while (true) {
        int ret = recv(sock, &c, 1, 0);
        if (ret <= 0) return "";       // Lỗi hoặc server đóng kết nối
        if (c == '\n') break;          // Kết thúc dòng
        if (c != '\r') line += c;      // Bỏ qua '\r' (Windows)
    }
    return line;
}

// ============================================================
// Hàm gửi một dòng (tự động thêm '\n')
// ============================================================
bool sendLine(SOCKET sock, const string& line) {
    string msg = line + "\n";
    int sent = send(sock, msg.c_str(), (int)msg.length(), 0);
    return sent == (int)msg.length();
}

int main() {
    // -------- 1. Khởi tạo Winsock --------
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "[LOI] Khoi tao Winsock that bai!" << endl;
        return 1;
    }

    // -------- 2. Tạo socket --------
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cout << "[LOI] Tao socket that bai!" << endl;
        WSACleanup();
        return 1;
    }

    // -------- 3. Cấu hình địa chỉ Server --------
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // -------- 4. Kết nối tới Server --------
    cout << "Dang ket noi toi Server (127.0.0.1:8888)..." << endl;
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "[LOI] Khong the ket noi toi Server. Kiem tra lai Server da chay chua!" << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    cout << "[OK] Da ket noi toi Server!\n" << endl;

    // -------- 5. Vòng lặp xử lý task --------
    while (true) {
        // --- Bước 1: Nhận header TASK ---
        string header = recvLine(clientSocket);
        if (header.empty()) {
            cout << "[Thong bao] Server da dong ket noi." << endl;
            break;
        }

        cout << "Server: " << header << endl;

        // --- Kiểm tra header có đúng định dạng không ---
        stringstream hs(header);
        string tag, op;
        int N;
        hs >> tag >> op >> N;

        if (tag != "TASK" || hs.fail()) {
            cout << "[LOI] Header khong hop le. Dong ket noi." << endl;
            break;
        }
        if (op != "SUM") {
            cout << "[LOI] OP khong duoc ho tro: " << op << ". Dong ket noi." << endl;
            break;
        }

        // --- Bước 2: Nhận dòng dữ liệu gồm N số ---
        string dataLine = recvLine(clientSocket);
        if (dataLine.empty()) {
            cout << "[LOI] Khong nhan duoc du lieu. Dong ket noi." << endl;
            break;
        }
        cout << "Server: " << dataLine << endl;

        // --- Bước 3: Tính tổng bằng long long ---
        stringstream ds(dataLine);
        long long sum = 0;
        int count = 0;
        long long x;
        bool formatError = false;

        while (ds >> x) {
            sum += x;
            count++;
        }

        // Kiểm tra đủ N số không
        if (count != N) {
            cout << "[CANH BAO] Du lieu nhan duoc co " << count
                 << " so, nhung header bao " << N << " so." << endl;
            formatError = true;
        }

        // --- Bước 4: Gửi RESULT về Server ---
        string result;
        if (formatError) {
            // Vẫn gửi RESULT nhưng giá trị có thể sai
            result = "RESULT " + to_string(sum);
        } else {
            result = "RESULT " + to_string(sum);
        }

        cout << "Client gui: " << result << endl;
        if (!sendLine(clientSocket, result)) {
            cout << "[LOI] Gui RESULT that bai." << endl;
            break;
        }

        // --- Bước 5: Nhận phản hồi từ Server ---
        string response = recvLine(clientSocket);
        if (response.empty()) {
            cout << "[Thong bao] Server da dong ket noi." << endl;
            break;
        }
        cout << "Server: " << response << endl;

        // --- Bước 6: Xử lý phản hồi ---
        if (response == "OK") {
            cout << ">>> Ket qua DUNG!\n" << endl;
        } else if (response == "WRONG") {
            cout << ">>> Ket qua SAI!\n" << endl;
        } else if (response == "ERROR") {
            cout << ">>> Server bao loi dinh dang!\n" << endl;
        } else {
            cout << ">>> Phan hoi khong xac dinh: " << response << "\n" << endl;
        }

        // --- Bước 7: Nhận BYE để kết thúc phiên ---
        string bye = recvLine(clientSocket);
        if (!bye.empty()) {
            cout << "Server: " << bye << endl;
        }
        if (bye == "BYE" || bye.empty()) {
            cout << "[Thong bao] Phien lam viec ket thuc." << endl;
            break;
        }
    }

    // -------- 6. Dọn dẹp --------
    closesocket(clientSocket);
    WSACleanup();
    cout << "\nClient da thoat." << endl;
    return 0;
}