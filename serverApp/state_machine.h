#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <iostream>
#include <string>

enum class State{
    START,
    GUEST_GENERAL,
    GUEST_ROOM,
    USERNAME_LOGIN,
    PASS_LOGIN,
    SIGN_UP_USERNAME,
    SIGN_UP_PASS1,
    SIGN_UP_PASS2,
    USER_GENERAL,
    USER_ROOM,
    EXIT,
    INCORRECT_COMMAND
};

class StateMachine{
public:
    static State transition(const State &currentState, const std::string& command = "nocommand"){
        switch (currentState)
        {
        case State::START:
            if(command == "guest"){
                return State::GUEST_GENERAL;
            }else if(command == "login"){
                return State::USERNAME_LOGIN;
            }else if(command == "exit"){
                return State::EXIT;
            }else if(command == "signup"){
                return State::SIGN_UP_USERNAME;
            }else{
                return State::INCORRECT_COMMAND;
            }
            break;
        case State::SIGN_UP_USERNAME:
            if(command == "back"){
                return State::START;
            }
            return State::SIGN_UP_PASS1;
            break;
        case State::SIGN_UP_PASS1:
            return State::SIGN_UP_PASS2;
            break;
        case State::SIGN_UP_PASS2:
            return State::USER_GENERAL;
            break;
        case State::USERNAME_LOGIN:
            if(command == "back"){
                return State::START;
            }
            return State::PASS_LOGIN;
            break;
        case State::PASS_LOGIN:
            if(command == "back"){
                return State::USERNAME_LOGIN;
            }
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
            }else if(command == "back"){
                return State::START;
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
            }else if(command == "back"){
                return State::USER_GENERAL;
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
            }else if(command == "back"){
                return State::GUEST_GENERAL;
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
            }else if(command == "back"){
            return State::USER_GENERAL;
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

#endif