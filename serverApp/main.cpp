#include <iostream>
#include <csignal>
#include <thread>
#include <vector>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <pqxx/pqxx>
#include <sys/ioctl.h>
#include <random>
#include <chrono>
#include "state_machine.h"
#include "connection_pool.h"
#include "user_state.h"

const int PORT = 8080;

std::map<int, UserState> clients; // Store client sockets

void handleResize(int signal) {
    if (signal == SIGWINCH) {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); // Get new terminal size
        std::cout << "Terminal resized: " << w.ws_row << " rows, " << w.ws_col << " cols" << std::endl;
    }
}

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
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S %d-%b-%y", &tm);
    return std::string(buffer);
}

// Function to get terminal width
int getTerminalWidth() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

std::string getCurrentTime(){
        // Get the current time from C++
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm* local_time = std::localtime(&now_time);

        // Format the time as YYYY-MM-DD HH:MM:SS
        std::ostringstream timeStream;
        timeStream << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");
        std::string currentTime = timeStream.str();
        return currentTime;
}

// Function to format message with right-aligned timestamp
std::string formatMessage(const std::string &username, const std::string &message_content, const std::string &postgresTimestamp) {
    std::string message = "[" + username + "] " + message_content; 
    
    std::ostringstream formattedMessage;
    
    std::string timestamp = formatTimestamp(postgresTimestamp);
    int terminalWidth = getTerminalWidth();
    int messageLength = message.length();
    int timestampLength = timestamp.length();
    int availableSpace = terminalWidth - timestampLength - 1;

    bool isFirstLine = true;
    int nextPos = 0;

    while(nextPos < message.size()){
        std::string currMessLine;
        if(isFirstLine){
            int padding = terminalWidth - messageLength - timestampLength;
            if(padding < 1) padding = 1;
            currMessLine = (messageLength - nextPos <= availableSpace) ? message.substr(nextPos) : message.substr(nextPos, nextPos + availableSpace);
            nextPos += currMessLine.size();
            
            isFirstLine = false;
            availableSpace = terminalWidth;

            formattedMessage << currMessLine << std::setw(padding + timestampLength) << timestamp;
        }else{
            currMessLine = (messageLength - nextPos <= availableSpace) ? message.substr(nextPos) : message.substr(nextPos, nextPos + availableSpace);
            nextPos += currMessLine.size();
             
            formattedMessage << currMessLine << "\n";
        }
    }
    return formattedMessage.str();
}

void handleClient(ConnectionPool &pool, int clientSocket) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cout << "Client disconnected." << std::endl;
            close(clientSocket);
            break;
        }

        std::string input = std::string(buffer, bytesReceived);
        std::cout << "Received: " << input << std::endl;

        size_t pos = input.find(' ');
        std::string command = (pos != std::string::npos) ? input.substr(0, pos) : input;

        std::string message;
        // reads a command
        if(command.length() > 0 && command[0] == '/'){
            command = command.substr(1);
            std::string argument = (pos == std::string::npos) ? "" : input.substr(pos + 1);

            if(StateMachine::transition(clients[clientSocket].getState(), command) == State::INCORRECT_COMMAND){
                continue;
            }
            bool ok = true;
            if(command == "guest"){
                if(argument != ""){
                    message = "Too many arguments";
                    ok = false;
                }
                if(ok){
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> distrib(1, 1000);
                    std::string name = "guest" + std::to_string(distrib(gen));
                    clients[clientSocket].setName(name);
                    message = "Hello " + name + "!";
                }
            }else if(command == "rooms"){
                if(argument != ""){
                    message = "Too many arguments!";
                    ok = false;
                }
                if(ok){
                    auto conn = pool.getConnection();
                    pqxx::work txn(*conn);
                    pqxx::result res = txn.exec("SELECT * FROM room");
                    txn.commit();

                    for(const auto &row: res){
                        std::string formattedMessage = std::string(row["name"].c_str()) + "\n";
                        send(clientSocket, formattedMessage.c_str(), formattedMessage.size(), 0);
                    }

                    pool.releaseConnection(std::move(conn));
                }   
            }else if(command == "room"){     
                if(argument == ""){
                    message = "Room name required!";
                    ok = false;
                }
                pos = argument.find(' ');
                if(pos != std::string::npos){
                    message = "Too many arguments!";
                    ok = false;
                }
                std::string roomName = argument;
                std::transform(roomName.begin(), roomName.end(), roomName.begin(),
                   [](unsigned char c) { return std::tolower(c); });
                int idRoom;
                auto conn = pool.getConnection();
                pqxx::work txn(*conn);
                pqxx::result res = txn.exec("SELECT id_room FROM room where LOWER(name) = " + txn.quote(roomName));
                pool.releaseConnection(std::move(conn));
                if(res.empty()){
                    message = "No rome with that name";
                    ok = false;
                }else{
                    idRoom = std::stoi(res[0]["id_room"].c_str());
                }
                if(ok && idRoom != clients[clientSocket].getRoom()){ 
                    conn = pool.getConnection();
                    res = txn.exec("SELECT * FROM(SELECT * FROM message WHERE id_room = "  + txn.quote(idRoom) + " order by timestamp_message desc limit 5) temp order by timestamp_message asc");
                    for(const auto &row: res){
                        std::string formattedMessage = formatMessage(row["username"].c_str(), row["text_message"].c_str(), row["timestamp_message"].c_str());
                        send(clientSocket, formattedMessage.c_str(), formattedMessage.size(), 0);
                    }
                    clients[clientSocket].setRoom(idRoom);
                    pool.releaseConnection(std::move(conn));        
                }
            }
            if(ok){
                clients[clientSocket].setState(StateMachine::transition(clients[clientSocket].getState(), command));
            }
            send(clientSocket, message.data(), message.size(), 0);
        }else{
            // Save a message
            std::string user = clients[clientSocket].getName();
            std::string message = input;
            std::string currTime = getCurrentTime();
            int idRoom = clients[clientSocket].getRoom();

            auto conn = pool.getConnection();
            pqxx::work txn(*conn);
            txn.exec("INSERT INTO message(text_message, timestamp_message, id_room, username) VALUES(" + txn.quote(message) + ", " + txn.quote(currTime) + ", " + txn.quote(idRoom) + ", " + txn.quote(user) + ")");
            txn.commit();
            pool.releaseConnection(std::move(conn));

            message = formatMessage(user, message, currTime);
            // Broadcast the message to all clients
            for (auto it = clients.begin(); it != clients.end(); ++it) {
                if(it->second.getRoom() == idRoom)
                    send(it->first, message.data(), message.size(), 0);
            }
        }
    }
}


int main() {
    //std::signal(SIGWINCH, handleResize);
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

    ConnectionPool pool(connectionString, 10);
    
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
        if (clientSocket == -1) {
            std::cerr << "Error accepting client connection." << std::endl;
            continue;
        }

        UserState s(State::START);
        clients[clientSocket] = s;
        std::cout << "New client connected." << std::endl;

        std::thread clientThread(handleClient, std::ref(pool), clientSocket);
        clientThread.detach();
    }

    return 0;
}
