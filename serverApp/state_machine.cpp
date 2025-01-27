#include <iostream>
#include <string>

enum class State{
    START,
    GUEST_GENERAL,
    GUEST_ROOM,
    USERNAME_LOGIN,
    PASS_LOGIN,
    USER_GENERAL,
    USER_ROOM,
    EXIT,
    INCORRECT_COMMAND
};

class StateMachine{
public:
    static State transition(const State &currentState, const std::string& command){
        switch (currentState)
        {
        case State::START:
            if(command == "guest"){
                return State::GUEST_GENERAL;
            }else if(command == "login"){
                return State::USERNAME_LOGIN;
            }else if(command == "exit"){
                return State::EXIT;
            }else{
                return State::INCORRECT_COMMAND;
            }
            break;
        case State::USERNAME_LOGIN:
            return State::PASS_LOGIN;
            break;
        case State::PASS_LOGIN:
            return State::USER_GENERAL;
            break;
        case State::GUEST_GENERAL:
            if(command == "room"){
                return State::GUEST_ROOM;
            }else if(command == "rooms"){
                return currentState;
            }else if(command == "logout"){
                return State::START;
            }else if(command == "exit"){
                return State::EXIT;
            }else{
                return State::INCORRECT_COMMAND;
            }
            break;
        case State::USER_GENERAL:
            if(command == "room"){
                return State::USER_ROOM;
            }else if(command == "rooms"){
                return currentState;
            }else if(command == "logout"){
                return State::START;
            }else if(command == "exit"){
                return State::EXIT;
            }else{
                return State::INCORRECT_COMMAND;
            }
            break;
        case State::GUEST_ROOM:
            if(command == "exit_room"){
                return State::GUEST_GENERAL;
            }else if(command == "rooms"){
                return currentState;
            }else if(command == "logout"){
                return State::START;
            }else if(command == "exit"){
                return State::EXIT;
            }else if(command == "room"){
                return currentState;
            }else{
                return State::INCORRECT_COMMAND;
            }
            break;
        case State::USER_ROOM:
            if(command == "exit_room"){
                return State::USER_GENERAL;
            }else if(command == "rooms"){
                return currentState;
            }else if(command == "logout"){
                return State::START;
            }else if(command == "exit"){
                return State::EXIT;
            }else if(command == "room"){
                return currentState;
            }else{
                return State::INCORRECT_COMMAND;
            }
            break;
        default:
            return State::INCORRECT_COMMAND;
            break;
        }
    }
};