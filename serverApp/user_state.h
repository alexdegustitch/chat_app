#ifndef USER_STATE_H
#define USER_STATE_H
#include "state_machine.h"

class UserState{
private:
    State state;
    std::string name;
    int id_room;
    int userType;
    int id_user;
    std::string username;
public:
    UserState(State s){
        state = s;
    };

    UserState(){
        state = State::INCORRECT_COMMAND;
    }

    std::string getUsername(){
        return username;
    }

    void setUsername(std::string username){
        this->username = username;
    }

    void setUserType(int userType){
        this->userType = userType;
    }

    int getUserType(){
        return this->userType;
    }
    
    int getIdUser(){
        return id_user;
    }

    void setIdUser(int id_user){
        this->id_user = id_user;
    }

    std::string getName(){
        return name;
    }

    void setName(std::string name){
        this->name = name;
    }

    int getRoom(){
        return id_room;
    }

    void setRoom(int id_room){
        this->id_room = id_room;
    }

    State getState(){
        return state;
    }

    void setState(State state){
        this->state = state;
    }
};

#endif