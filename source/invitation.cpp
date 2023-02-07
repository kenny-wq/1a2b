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
#include <set>
#include "../header/invitation.h"
#include "../header/user.h"
#include "../header/room.h"
using namespace std;

void save_invitation_to_csv(Invitation inv){
    ofstream fout;
    fout.open("invitation.csv", ios::app);

    fout<<inv.inviter<<", "
        <<inv.invitee<<", "
        <<inv.game_room_id<<", "
        <<inv.code<<"\n";

    fout.close();
}

vector<Invitation> read_invitation_from_csv(){
    ifstream fin;
    fin.open("invitation.csv");

    vector<Invitation> res;
    
    string line;
    while(getline(fin,line,'\n')){
        stringstream ss(line);
        string inviter,invitee,code;
        string room_id;

        ss>>inviter>>invitee>>room_id>>code;

        inviter.pop_back();
        invitee.pop_back();
        room_id.pop_back();

        res.push_back(Invitation(inviter,invitee,stoi(room_id),stoi(code)));
    }
    fin.close();
    return res;
}


string list_inv(vector<Invitation> inv_vec){
    stringstream res;
    res<< "List invitations\n";

    if(inv_vec.size()==0){
        res<<"No Invitations\n";
    }
    else{
        set<Invitation> st;
        for(int i=0;i<inv_vec.size();i++){
            st.insert(inv_vec[i]);
        }
        int counter = 1;
        for(auto it: st){
            string inviter_name = it.inviter;
            string invitee_name = it.invitee;
            unsigned room_id = it.game_room_id;
            unsigned code = it.code;

            User* inviter_user = get_user(inviter_name);

            if(inviter_user->game_room_id==room_id){
                res<<to_string(counter)<<". "<<inviter_user->name<<"<"<<inviter_user->email<<">"
                <<" invite you to join game room "<<room_id<<", invitation code is "<<code<<"\n";
                counter++;
            }
        }
    }
    return res.str();
}