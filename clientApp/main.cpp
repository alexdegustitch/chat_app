#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h> 

const std::string SERVER_IP = "127.0.0.1"; // Localhost
const int PORT = 8080;

// Function to get terminal width
int getTerminalWidth() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

void clearLastInput(const std::string& input) {
    // Calculate the number of terminal lines the input spans
    const int terminalWidth = getTerminalWidth(); // Adjust for actual terminal width
    int lines = (input.length() / terminalWidth) + 1;

    // Move the cursor up and clear lines
    for (int i = 0; i < lines; ++i) {
        std::cout << "\033[F" // Move cursor up one line
                  << "\033[2K"; // Clear the current line
    }
    std::flush(std::cout); // Ensure changes take effect
}

void receiveMessages(int socket) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(socket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cout << "Server disconnected." << std::endl;
            break;
        }

        std::cout << std::string(buffer, bytesReceived) << std::endl;
    }
}

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error creating socket." << std::endl;
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error connecting to server." << std::endl;
        return -1;
    }

    std::cout << "Connected to server!" << std::endl;

    std::thread receiveThread(receiveMessages, clientSocket);
    receiveThread.detach();

    while (true) {
        std::string message;
        std::getline(std::cin, message);
        if (message == "/exit") {
            break;
        }
        if(message.size() > 0 && message[0] != '/')
            clearLastInput(message);
        send(clientSocket, message.c_str(), message.size(), 0);
    }

    close(clientSocket);
    return 0;
}
