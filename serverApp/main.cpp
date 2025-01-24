#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <pqxx/pqxx>
#include <sys/ioctl.h>

const int PORT = 8080;

std::vector<int> clients; // Store client sockets

std::string formatTimestamp(const std::string &postgresTimestamp) {
    // Parse the PostgreSQL timestamp string into std::tm
    std::tm tm = {};
    std::istringstream ss(postgresTimestamp);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S"); // Adjust format as needed
    if (ss.fail()) {
        throw std::runtime_error("Failed to parse timestamp: " + postgresTimestamp);
    }

    // Format std::tm into the desired string format
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buffer);
}

// Function to get terminal width
int getTerminalWidth() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

// Function to format message with right-aligned timestamp
std::string formatMessage(const std::string &message, const std::string &postgresTimestamp) {
    std::string timestamp = formatTimestamp(postgresTimestamp);
    int terminalWidth = getTerminalWidth();
    int messageLength = message.length();
    int timestampLength = timestamp.length();
    int padding = terminalWidth - messageLength - timestampLength;

    if (padding < 1) padding = 1; // Ensure at least 1 space

    std::ostringstream formattedMessage;
    formattedMessage << message << std::setw(padding + timestampLength) << timestamp;
    return formattedMessage.str();
}

void handleClient(int clientSocket) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cout << "Client disconnected." << std::endl;
            close(clientSocket);
            break;
        }

        std::string message = std::string(buffer, bytesReceived);
        std::cout << "Received: " << message << std::endl;

        // Broadcast the message to all clients
        for (int client : clients) {
            if (client != clientSocket) {
                send(client, message.data(), message.size(), 0);
            }
        }
    }
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating server socket." << std::endl;
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error binding to port." << std::endl;
        return -1;
    }

    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Error listening on port." << std::endl;
        return -1;
    }

    std::string connectionString = "host=localhost port=5432 dbname=chat_app user=postgres password=stic";

    // connecting to the database
    pqxx::connection conn(connectionString);

    std::cout << "Server listening on port 5432" << std::endl;

    if (conn.is_open()) {
        std::cout << "Connected to database: " << conn.dbname() << std::endl;
    
    } else {
        std::cout << "Failed to connect to the database." << std::endl;
        return 0;
    }
    
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
        if (clientSocket == -1) {
            std::cerr << "Error accepting client connection." << std::endl;
            continue;
        }

        clients.push_back(clientSocket);
        std::string message = "Hello! Please press 1 to continue to login and 0 to continue as a guest.";
        send(clientSocket, message.data(), message.size(), 0);
        pqxx::nontransaction txn(conn);
        pqxx::result res = txn.exec("select * from room");
    
        for(const auto &row: res){
            std::string formattedMessage = formatMessage(row["name"].c_str(), row["date_created"].c_str());
            send(clientSocket, formattedMessage.c_str(), formattedMessage.size(), 0);
        }
        std::cout << "New client connected." << std::endl;

        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    return 0;
}
