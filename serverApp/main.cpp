#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <pqxx/pqxx>
#include <sys/ioctl.h>
#include <random>
#include "state_machine.cpp"
#include "connection_pool.cpp"

const int PORT = 8080;

std::map<int, std::pair<std::string, State>> clients; // Store client sockets

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
            std::string argument = input.substr(pos + 1);
            if(StateMachine::transition(clients[clientSocket].second, command) == State::INCORRECT_COMMAND){
                //std::cout << command << " " << clients[clientSocket].second << std::endl;
                //std::cout << "Greska" << std::endl;
                continue;
            }
            if(command == "guest"){
                std::cout << "Greska guest" << std::endl;
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> distrib(1, 1000);
                std::string name = "guest" + std::to_string(distrib(gen));
                clients[clientSocket].first = name;
                message = "Hello " + name + "!";
            }else if(command == "rooms"){
                std::cout << "Greska rooms" << std::endl;
                //send(clientSocket, command.c_str(), command.size(), 0);
                bool ok = true;
                auto conn = pool.getConnection();
                // connecting to the database
                pqxx::work txn(*conn);
                pqxx::result res = txn.exec("SELECT * FROM room");
                txn.commit();

                for(const auto &row: res){
                    std::string formattedMessage = std::string(row["name"].c_str()) + "\n";
                    send(clientSocket, formattedMessage.c_str(), formattedMessage.size(), 0);
                }

                pool.releaseConnection(std::move(conn));
                
            }else if(command == "room"){
                bool ok = true;
                if(argument.size() == 0){
                    message = "Room name required!";
                    ok = false;
                }
                pos = argument.find(' ');
                if(pos != std::string::npos){
                    message = "Too many arguments!";
                    ok = false;
                }
                std::string roomName = argument;
                auto conn = pool.getConnection();
                pqxx::work txn(*conn);
                pqxx::result res = txn.exec("select * from(select * from message m join room r on m.id_room = r.id_room where r.name = "  + txn.quote(roomName) + " order by m.timestamp_message desc limit 5) temp order by timestamp_message asc");
            
                for(const auto &row: res){
                    std::string formattedMessage = formatMessage(row["text_message"].c_str(), row["timestamp_message"].c_str());
                    send(clientSocket, formattedMessage.c_str(), formattedMessage.size(), 0);
                }

                pool.releaseConnection(std::move(conn));
            }
            clients[clientSocket].second = StateMachine::transition(clients[clientSocket].second, command);
            send(clientSocket, message.data(), message.size(), 0);
        }else{
            // Broadcast the message to all clients
            input = "[" + clients[clientSocket].first + "] " + input;
            for (auto it = clients.begin(); it != clients.end(); ++it) {
                send(it->first, input.data(), input.size(), 0);
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

    ConnectionPool pool(connectionString, 10);
    
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
        if (clientSocket == -1) {
            std::cerr << "Error accepting client connection." << std::endl;
            continue;
        }

        clients[clientSocket] = {"", State::START};
        /*std::string message = "Hello! Please press 1 to continue to login and 0 to continue as a guest.";
        send(clientSocket, message.data(), message.size(), 0);
        pqxx::nontransaction txn(conn);
        pqxx::result res = txn.exec("select * from room");
    
        for(const auto &row: res){
            std::string formattedMessage = formatMessage(row["name"].c_str(), row["date_created"].c_str());
            send(clientSocket, formattedMessage.c_str(), formattedMessage.size(), 0);
        }*/
        std::cout << "New client connected." << std::endl;

        std::thread clientThread(handleClient, std::ref(pool), clientSocket);
        clientThread.detach();
    }

    return 0;
}
