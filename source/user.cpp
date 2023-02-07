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
#include "../header/user.h"
#include "../header/room.h"

using namespace std;


void save_user_to_csv(User u){
    fstream fout;
    fout.open("user.csv",ios::out | ios::app);

    fout<<u.name<<", "
        <<u.email<<", "
        <<u.password<<", "
        <<u.status<<", "
        <<u.game_room_id<<", "
        <<u.enter_room_order<<", "
        <<u.client_sd<<"\n";

    fout.close();
}

vector<User> read_user_from_csv(){
    fstream fin;
    fin.open("user.csv",ios::in);

    vector<User> res;

    string line;
    while(getline(fin,line,'\n')){
        stringstream ss(line);
        string name,email,password,status;
        string room_id,enter_room_order,client_sd;

        ss>>name>>email>>password>>status>>room_id>>enter_room_order>>client_sd;

        name.pop_back();
        email.pop_back();
        password.pop_back();
        status.pop_back();
        room_id.pop_back();
        enter_room_order.pop_back();

        res.push_back(User(name,email,password,status,stoi(room_id),stoi(enter_room_order),stoi(client_sd)));
    }
    fin.close();
    return res;
}

bool user_order_compare(User a, User b){
    return a.enter_room_order<b.enter_room_order;
}

bool user_alpha_order(User a, User b){
    return a.name<b.name;
}

string list_user(vector<User> users){
    std::sort(users.begin(),users.end(),user_alpha_order);
    stringstream res;
    res << "List Users\n";

    if(users.size()==0){
        res<<"No Users\n";
    }
    else{
        for(int i=1;i<=users.size();i++){
            string name = users[i-1].name;
            string email = users[i-1].email;
            string status = users[i-1].status;

            res<<to_string(i)<<". "<<name<<"<"<<email<<"> "<<status<<"\n";
        }
    }
    
    return res.str();
}

void login_user(string username,int client_sd){
    ifstream fin;
    fin.open("user.csv");

    ofstream fout;
    fout.open("temp.csv");

    username+=",";

    string line;
    while(getline(fin,line,'\n')){
        string name,email,password,status,game_room_id,enter_room_order,_client_sd;
        stringstream ss(line);
        ss>>name>>email>>password>>status>>game_room_id>>enter_room_order>>_client_sd;
        
        if(name==username&&status=="Offline,"){
            fout<<name<<" "
            <<email<<" "
            <<password<<" "
            <<"Online, "
            <<game_room_id<<" "
            <<enter_room_order<<" "
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

void logout_user(string username){
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
        if(name==username&&status=="Online,"){
            fout<<name<<" "
            <<email<<" "
            <<password<<" "
            <<"Offline, "
            <<game_room_id<<" "
            <<enter_room_order<<" "
            <<-1<<"\n";
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

User* get_user(string username){
    vector<User> user_list = read_user_from_csv();
    for(int i=0;i<user_list.size();i++){
        if(user_list[i].name==username){
            User c_user = user_list[i];
            User* u = new User(c_user.name,c_user.email,c_user.password,c_user.status,c_user.game_room_id,c_user.enter_room_order,c_user.client_sd);
            return u;
        }
    }
    return nullptr;
}

User* get_user_by_email(string email){
    vector<User> user_list = read_user_from_csv();
    for(int i=0;i<user_list.size();i++){
        if(user_list[i].email==email){
            User c_user = user_list[i];
            User* u = new User(c_user.name,c_user.email,c_user.password,c_user.status,c_user.game_room_id,c_user.enter_room_order,c_user.client_sd);
            return u;
        }
    }
    return nullptr;
}

vector<User> get_user_in_room(int room_id){
    vector<User> users = read_user_from_csv();
    vector<User> res;

    for(int i=0;i<users.size();i++){
        if(users[i].game_room_id==room_id){
            res.push_back(users[i]);
        }
    }
    return res;
}