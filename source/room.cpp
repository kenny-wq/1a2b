#include<iostream>
#include<cstdlib>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<cstring>
#include<unistd.h>
#include<vector>
#include<sstream>
#include<sys/wait.h>
#include<fstream>
#include<algorithm>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "../header/room.h"

using namespace std;


void save_room_to_csv(Room room){
    ofstream fout;
    fout.open("room.csv", ios::app);

    fout<<room.type<<", "
        <<room.id<<", "
        <<room.status<<", "
        <<room.invitation_code<<", "
        <<room.manager<<"\n";

    fout.close();
}
vector<Room> read_room_from_csv(){
    ifstream fin;
    fin.open("room.csv");

    vector<Room> res;
    
    string line;
    while(getline(fin,line,'\n')){
        stringstream ss(line);
        string type,status;
        string id;
        string invitation_code;
        string manager;

        ss>>type>>id>>status>>invitation_code>>manager;

        type.pop_back();
        id.pop_back();
        status.pop_back();
        invitation_code.pop_back();

        res.push_back(Room(type,stoi(id),status,stoi(invitation_code),manager));
    }
    fin.close();
    return res;
}

void remove_room(int room_id){
    ifstream fin;
    fin.open("room.csv");

    ofstream fout;
    fout.open("temp.csv");

    string line;
    while(getline(fin,line,'\n')){
        stringstream ss(line);
        string type,status;
        string id;
        string invitation_code;
        string manager;
        ss>>type>>id>>status>>invitation_code>>manager;
        if(id!=(to_string(room_id)+",")){
            fout<<line<<"\n";
        }
    }

    fin.close();
    fout.close();

    remove("room.csv");
    rename("temp.csv","room.csv");
}

string list_room(vector<Room> rooms){
    sort(rooms.begin(),rooms.end());
    stringstream res;
    res << "List Game Rooms\n";

    if(rooms.size()==0){
        res<<"No Rooms\n";
    }
    else{
        for(int i=1;i<=rooms.size();i++){
            string type = rooms[i-1].type;
            int id = rooms[i-1].id;
            string status = rooms[i-1].status;

            res<<to_string(i)<<". ("<<type<<")"<<" Game Room "<<to_string(id)<<" ";

            if(status=="Started_Game"){
                res<<"has started playing\n";
            }
            else if(status=="Waiting_for_player"){
                res<<"is open for players\n";
            }
        }
    }
    return res.str();
}

void enter_room(string username, int room_id, int order){
    ifstream fin;
    fin.open("user.csv");

    ofstream fout;
    fout.open("temp.csv");

    username+=",";

    string line;
    while(getline(fin,line,'\n')){
        string name,email,password,status,game_room_id,enter_room_order,client_sd;
        stringstream ss(line);
        ss>>name>>email>>password>>status>>game_room_id>>enter_room_order>>client_sd;
        if(name==username&&game_room_id=="-1,"){
            fout<<name<<" "
            <<email<<" "
            <<password<<" "
            <<status<<" "
            <<room_id<<", "
            <<order<<", "
            <<client_sd<<"\n";
        }
        else{
            fout<<line<<"\n";
        }
    }

    fin.close();
    fout.close();

    remove("user.csv");
    rename("temp.csv","user.csv");
}

void leave_room(string username, int room_id){
    ifstream fin;
    fin.open("user.csv");

    ofstream fout;
    fout.open("temp.csv");

    username+=",";

    string line;
    while(getline(fin,line,'\n')){
        string name,email,password,status,game_room_id,enter_room_order,client_sd;
        stringstream ss(line);
        ss>>name>>email>>password>>status>>game_room_id>>enter_room_order>>client_sd;
        if(name==username&&game_room_id==(to_string(room_id)+",")){
            fout<<name<<" "
            <<email<<" "
            <<password<<" "
            <<status<<" "
            <<-1<<", "
            <<-1<<", "
            <<client_sd<<"\n";
        }
        else{
            fout<<line<<"\n";
        }
    }

    fin.close();
    fout.close();

    remove("user.csv");
    rename("temp.csv","user.csv");
}

Room* get_room(unsigned room_id){
    vector<Room> rooms = read_room_from_csv();
    for(int i=0;i<rooms.size();i++){
        if(rooms[i].id==room_id){
            Room* res = new Room(rooms[i].type,rooms[i].id,rooms[i].status,rooms[i].invitation_code,rooms[i].manager);
            return res;
        }
    }
    return nullptr;
}
