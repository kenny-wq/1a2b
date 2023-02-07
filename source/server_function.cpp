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
#include "../header/server_function.h"

using namespace std;

vector<string> split_string_deli(string s,string deli){
    vector<string> res;
    int start = 0;
    int end = s.find(deli);

    while(end!=string::npos){
        string tmp = s.substr(start,end-start);
        res.push_back(tmp);
        start = end+1;
        end = s.find(deli,start);
    }
    res.push_back(s.substr(start));
    return res;
}

string rand_gen(){
    srand(time(0));
    char str[100] = {0};
    int a = rand()%10000;
    sprintf(str,"%04d",a);
   
    string s = str;
    return s;
}

string game(string ans, string guess){
    int a[4] = {0};
    for(int i=0;i<4;i++){
        if(ans[i]==guess[i]){
            a[i] = 1;
        }
    }
    int b[4] = {0};
    for(int i=0;i<4;i++){  //guess
        if(a[i]==1){
            continue;
        }
        int n = guess[i];
        for(int j=0;j<4;j++){ //ans
            if(a[j]==1||b[j]==1){
                continue;
            }
            if(ans[j]==n){
                b[j] = 1;
                break;
            }
        }
    }
    int A=0,B=0;
    for(int i=0;i<4;i++){
        A+=a[i];
        B+=b[i];
    }
    string res;
    stringstream ss;
    ss<<A<<"A"<<B<<"B";
    ss>>res;
    return res;
}

void start_game(int room_id){
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
        id.pop_back();
        if(stoi(id)!=room_id){
            fout<<line<<"\n";
        }
        else if(status=="Waiting_for_player,"){
            fout<<type<<" "
            <<id<<", "
            <<"Started_Game, "
            <<invitation_code<<" "
            <<manager<<"\n";
        }
    }

    fin.close();
    fout.close();

    remove("room.csv");
    rename("temp.csv","room.csv");
}
void end_game(int room_id){
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
        id.pop_back();
        if(stoi(id)!=room_id){
            fout<<line<<"\n";
        }
        else if(status=="Started_Game,"){
            fout<<type<<" "
            <<id<<", "
            <<"Waiting_for_player, "
            <<invitation_code<<" "
            <<manager<<"\n";
        }
    }

    fin.close();
    fout.close();

    remove("room.csv");
    rename("temp.csv","room.csv");
}


bool is_four_digit_with_leading_zero(string number){
    bool is_four_digit = true;

    for(int i=0;i<4;i++){
        if(isdigit(number[i])==false){
            is_four_digit = false;
            break;
        }
    }
    if(number.size()!=4||is_four_digit==false){
        return false;
    }
    else{
        return true;
    }
}