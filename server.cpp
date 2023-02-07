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
#include<pthread.h>
#include<cstdio>

#include "./header/server_function.h"
#include "./header/room.h"
#include "./header/user.h"
#include "./header/invitation.h"

using namespace std;

string my_login_file = "/efs/b_login";
int login_user_number = 0;

string game_ans = "no game";

vector<User> user_order;
int game_limit_round_number=0;

string curr_player_name;
int curr_round;

pthread_mutex_t user_mutex;
pthread_mutex_t room_mutex;
pthread_mutex_t invitation_mutex;

struct sockaddr_in* create_addr(int port_number,char server_ip[]){
    struct sockaddr_in* server_address = new struct sockaddr_in;
    int addr_len = sizeof(server_address);

    server_address->sin_family = AF_INET;
    server_address->sin_port = htons(port_number);
    inet_aton(server_ip,&server_address->sin_addr);
    return server_address;
}

int tcp_connect(struct sockaddr_in server_address){
    int tcp_client_socket[15] = {0};    
    int master_tcp_socket = socket(AF_INET, SOCK_STREAM, 0); //tcp socket

    int opt = 1;

    if (setsockopt(master_tcp_socket, SOL_SOCKET,
                   SO_REUSEADDR, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        std::exit(EXIT_FAILURE);
    }

    bind(master_tcp_socket, (struct sockaddr*) &server_address,sizeof(server_address));
    listen(master_tcp_socket,50);
    // cout<<"tcp server is running"<<endl;
    return master_tcp_socket;
}

int udp_connect(struct sockaddr_in server_address){
    int opt = 1;
    int udp_fd = socket(AF_INET, SOCK_DGRAM,0);  //udp socket
    if (setsockopt(udp_fd, SOL_SOCKET,
                   SO_REUSEADDR, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        std::exit(EXIT_FAILURE);
    }
    bind(udp_fd,(struct sockaddr*) &server_address, sizeof(server_address));
    // cout<<"udp server is running"<<endl;
    return udp_fd;
}

void *tcp_worker(void* arg){
    int* data = (int*) arg;
    int clientsd = data[0];
    int client_id = data[1];
    bool someone_login = false;
    string curr_user_name = "";
    int valread;
    while(1){
        char recive_buffer[110] = {0};
        char send_buffer[1000] = {0};
        
        if((valread=read(clientsd,recive_buffer,110))==0){
            if(someone_login){

                pthread_mutex_lock(&user_mutex);
                User* curr_user = get_user(curr_user_name);
                pthread_mutex_unlock(&user_mutex);

                if(curr_user->game_room_id!=-1){

                    pthread_mutex_lock(&room_mutex);
                    Room* the_room = get_room(curr_user->game_room_id);
                    pthread_mutex_unlock(&room_mutex);

                    if(curr_user->name==the_room->manager){

                        pthread_mutex_lock(&room_mutex);
                        remove_room(the_room->id);
                        pthread_mutex_unlock(&room_mutex);

                        pthread_mutex_lock(&user_mutex);
                        leave_room(curr_user->name,curr_user->game_room_id);
                        pthread_mutex_unlock(&user_mutex);
                    }
                    else{
                        pthread_mutex_lock(&user_mutex);
                        leave_room(curr_user->name,curr_user->game_room_id);
                        pthread_mutex_unlock(&user_mutex);
                    }
                }
                pthread_mutex_lock(&user_mutex);
                logout_user(curr_user_name);
                login_user_number--;
                ofstream login_file;
                login_file.open(my_login_file);
                login_file<<login_user_number;
                login_file.close();
                pthread_mutex_unlock(&user_mutex);
            }
            close(clientsd);

            // cout<<"client "<<client_id<<" exit"<<endl;
            break;
        }
        else{
            recive_buffer[valread-1] = '\0';
            // printf("%s\n",recive_buffer);
            // fflush(stdout);
            string str_buf = recive_buffer;
            vector<string> info = split_string_deli(str_buf," ");

            if(info[0]=="login"){
                string user_name = info[1];
                string user_password = info[2];
                // cout<<user_name<<endl;
                // cout<<user_password<<endl;

                pthread_mutex_lock(&user_mutex);
                User* the_user = get_user(user_name);
                pthread_mutex_unlock(&user_mutex);
                if(the_user==nullptr){
                    strcpy(send_buffer,"Username does not exist\n");
                }
                else if(the_user->status=="Online"){
                    string message = "Someone already logged in as " + user_name +"\n";
                    strcpy(send_buffer,message.c_str());
                }
                else if(someone_login){
                    string message = "You already logged in as " + curr_user_name +"\n";
                    strcpy(send_buffer,message.c_str());
                }
                else if(user_password!=the_user->password){
                    strcpy(send_buffer,"Wrong password\n");
                }
                else{
                    char message[100] = {0};
                    sprintf(message,"Welcome, %s\n",user_name.c_str());
                    strcpy(send_buffer,message);
                    // cout<<user_name<<" login success"<<endl;
                    someone_login = true;
                    curr_user_name = user_name;
                    pthread_mutex_lock(&user_mutex);
                    login_user(user_name,clientsd);
                    login_user_number++;
                    ofstream login_file;
                    login_file.open(my_login_file);
                    login_file<<login_user_number;
                    login_file.close();
                    pthread_mutex_unlock(&user_mutex);
                }
                write(clientsd,(char*) send_buffer,strlen(send_buffer));
            }
            else if(info[0]=="logout"){
                if(someone_login){
                    pthread_mutex_lock(&user_mutex);
                    User* this_user = get_user(curr_user_name);
                    pthread_mutex_unlock(&user_mutex);

                    if(this_user->game_room_id!=-1){
                        char message[100] = {0};
                        sprintf(message,"You are already in game room %d, please leave game room\n",this_user->game_room_id);
                        strcpy(send_buffer,message);
                    }
                    else{
                        char message[100] = {0};
                        sprintf(message,"Goodbye, %s\n",curr_user_name.c_str());
                        strcpy(send_buffer,message);
                        // cout<<curr_user_name[client_id]<<" logout"<<endl;
                        someone_login = false;
                        pthread_mutex_lock(&user_mutex);
                        logout_user(curr_user_name);
                        login_user_number--;
                        ofstream login_file;
                        login_file.open(my_login_file);
                        login_file<<login_user_number;
                        login_file.close();
                        pthread_mutex_unlock(&user_mutex);
                        curr_user_name = "";
                    }
                }
                else{
                    strcpy(send_buffer,"You are not logged in\n");
                }
                write(clientsd,(char*) send_buffer,strlen(send_buffer));
            }
            else if(info[0]=="create"&&info[1]=="public"){
                string room_id = info[3];
                if(!someone_login){
                    string message = "You are not logged in\n";
                    strcpy(send_buffer,message.c_str());
                }
                else{
                    pthread_mutex_lock(&user_mutex);
                    User* curr_user = get_user(curr_user_name);
                    pthread_mutex_unlock(&user_mutex);

                    pthread_mutex_lock(&room_mutex);
                    Room* the_room = get_room(stoi(info[3]));
                    pthread_mutex_unlock(&room_mutex);
                    if(curr_user->game_room_id!=-1){
                        char message[100] = {0};
                        sprintf(message,"You are already in game room %d, please leave game room\n",curr_user->game_room_id);
                        strcpy(send_buffer,message);
                    }
                    else if(the_room!=nullptr){
                        string message = "Game room ID is used, choose another one\n";
                        strcpy(send_buffer,message.c_str());
                    }
                    else{
                        Room room = Room("Public",stoi(room_id),"Waiting_for_player",0,curr_user_name);

                        pthread_mutex_lock(&room_mutex);
                        save_room_to_csv(room);
                        pthread_mutex_unlock(&room_mutex);

                        pthread_mutex_lock(&user_mutex);
                        enter_room(curr_user_name,stoi(room_id),1);
                        pthread_mutex_unlock(&user_mutex);

                        string message = "You create public game room "+room_id+"\n";
                        strcpy(send_buffer,message.c_str());
                    }
                }
                write(clientsd,(char*) send_buffer,strlen(send_buffer));
            }
            else if(info[0]=="create"&&info[1]=="private"){
                string room_id = info[3];
                string invitation_code = info[4];
                if(!someone_login){
                    string message = "You are not logged in\n";
                    strcpy(send_buffer,message.c_str());
                }
                else{
                    pthread_mutex_lock(&user_mutex);
                    User* curr_user = get_user(curr_user_name);
                    pthread_mutex_unlock(&user_mutex);

                    pthread_mutex_lock(&room_mutex);
                    Room* the_room = get_room(stoi(info[3]));
                    pthread_mutex_unlock(&room_mutex);
                    if(curr_user->game_room_id!=-1){
                        char message[100] = {0};
                        sprintf(message,"You are already in game room %d, please leave game room\n",curr_user->game_room_id);
                        strcpy(send_buffer,message);
                    }
                    else if(the_room!=nullptr){
                        string message = "Game room ID is used, choose another one\n";
                        strcpy(send_buffer,message.c_str());
                    }
                    else{
                        Room room = Room("Private",stoi(room_id),"Waiting_for_player",stoi(invitation_code),curr_user_name);
                        
                        pthread_mutex_lock(&room_mutex);
                        save_room_to_csv(room);
                        pthread_mutex_unlock(&room_mutex);

                        pthread_mutex_lock(&user_mutex);
                        enter_room(curr_user_name,stoi(room_id),1);
                        pthread_mutex_unlock(&user_mutex);

                        string message = "You create private game room "+room_id+"\n";
                        strcpy(send_buffer,message.c_str());
                    }
                }
                
                write(clientsd,(char*) send_buffer,strlen(send_buffer));
            }
            else if(info[0]=="join"){
                if(!someone_login){
                    strcpy(send_buffer,"You are not logged in\n");
                    write(clientsd,(char*) send_buffer,strlen(send_buffer));
                }
                else{
                    pthread_mutex_lock(&user_mutex);
                    User* curr_user = get_user(curr_user_name);
                    pthread_mutex_unlock(&user_mutex);

                    if(curr_user->game_room_id!=-1){
                        string message = "You are already in game room "+to_string(curr_user->game_room_id)+", please leave game room\n";
                        strcpy(send_buffer,message.c_str());
                        write(clientsd,(char*) send_buffer,strlen(send_buffer));
                    }
                    else{
                        pthread_mutex_lock(&room_mutex);
                        Room* curr_room = get_room(stoi(info[2]));
                        pthread_mutex_unlock(&room_mutex);

                        if(curr_room==nullptr){
                            string message = "Game room "+to_string(stoi(info[2]))+" is not exist\n";
                            strcpy(send_buffer,message.c_str());
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                        }
                        else if(curr_room->type=="Private"){
                            string message = "Game room is private, please join game by invitation code\n";
                            strcpy(send_buffer,message.c_str());
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                        }
                        else if(curr_room->status=="Started_Game"){
                            strcpy(send_buffer,"Game has started, you can't join now\n");
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                        }
                        else{
                            string room_id = info[2];
                            pthread_mutex_lock(&user_mutex);
                            vector<User> user_in_the_room = get_user_in_room(stoi(room_id));
                            pthread_mutex_unlock(&user_mutex);
                            
                            if(user_in_the_room.size()==0){
                                pthread_mutex_lock(&user_mutex);
                                enter_room(curr_user_name,stoi(room_id),2);
                                pthread_mutex_unlock(&user_mutex);
                            }
                            else{
                                pthread_mutex_lock(&user_mutex);
                                enter_room(curr_user_name,stoi(room_id),user_in_the_room.size()+1);
                                pthread_mutex_unlock(&user_mutex);
                            }
                            string message = "You join game room "+room_id+"\n";
                            strcpy(send_buffer,message.c_str());
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                            for(auto user:user_in_the_room){  // send message to all users
                                if(user.name!=curr_user_name){
                                    string message_to_people_in_the_room = "Welcome, "+curr_user_name+" to game!\n";
                                    strcpy(send_buffer,message_to_people_in_the_room.c_str());
                                    int ret = write(user.client_sd,(char*) send_buffer,strlen(send_buffer));
                                }
                            }
                        }
                    }
                }
            }
            else if(info[0]=="invite"){
                string invitee_email = info[1];
                pthread_mutex_lock(&user_mutex);
                User* invitee = get_user_by_email(invitee_email);
                pthread_mutex_unlock(&user_mutex);
                if(!someone_login){
                    strcpy(send_buffer,"You are not logged in\n");
                    write(clientsd,(char*) send_buffer,strlen(send_buffer));
                }
                else{
                    pthread_mutex_lock(&user_mutex);
                    User* curr_user = get_user(curr_user_name);
                    pthread_mutex_unlock(&user_mutex);
                    if(curr_user->game_room_id==-1){
                        strcpy(send_buffer,"You did not join any game room\n");
                        write(clientsd,(char*) send_buffer,strlen(send_buffer));
                    }
                    else{
                        pthread_mutex_lock(&room_mutex);
                        Room* the_room = get_room(curr_user->game_room_id);
                        pthread_mutex_unlock(&room_mutex);
                        if(the_room->manager!=curr_user_name){
                            strcpy(send_buffer,"You are not private game room manager\n");
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                        }
                        else if(invitee->status=="Offline"){
                            strcpy(send_buffer,"Invitee not logged in\n");
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                        }
                        else{
                            pthread_mutex_lock(&invitation_mutex);
                            save_invitation_to_csv(Invitation(curr_user_name,invitee->name,curr_user->game_room_id,the_room->invitation_code));
                            pthread_mutex_unlock(&invitation_mutex);
                            char message[1000] = {0};
                            sprintf(message,"You send invitation to %s<%s>\n",invitee->name.c_str(),invitee->email.c_str());
                            strcpy(send_buffer,message);
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                            
                            sprintf(message,"You receive invitation from %s<%s>\n",curr_user_name.c_str(),curr_user->email.c_str());
                            strcpy(send_buffer,message);
                            write(invitee->client_sd,(char*) send_buffer,strlen(send_buffer));
                        }
                    }
                }
            }
            else if(info[0]=="list"&&info[1]=="invitations"){
                pthread_mutex_lock(&invitation_mutex);
                vector<Invitation> inv_vec = read_invitation_from_csv();
                pthread_mutex_unlock(&invitation_mutex);
                vector<Invitation> inv_to_invitee;
                for(int i=0;i<inv_vec.size();i++){
                    if(inv_vec[i].invitee==curr_user_name){
                        inv_to_invitee.push_back(inv_vec[i]);
                    }
                }

                string inv_list=list_inv(inv_to_invitee);

                strcpy(send_buffer,inv_list.c_str());
                write(clientsd,(char*) send_buffer,strlen(send_buffer));
            }
            else if(info[0]=="accept"){
                if(someone_login==false){
                    strcpy(send_buffer,"You are not logged in\n");
                    write(clientsd,(char*) send_buffer,strlen(send_buffer));
                }
                else{
                    pthread_mutex_lock(&user_mutex);
                    User* curr_user = get_user(curr_user_name);
                    pthread_mutex_unlock(&user_mutex);
                    if(curr_user->game_room_id!=-1){
                        char message[1000] = {0};
                        sprintf(message,"You are already in game room %d, please leave game room\n",curr_user->game_room_id);
                        strcpy(send_buffer,message);
                        write(clientsd,(char*) send_buffer,strlen(send_buffer));
                    }
                    else{
                        bool invitation_found = false;
                        bool code_correct = false;
                        pthread_mutex_lock(&user_mutex);
                        User* inviter = get_user_by_email(info[1]);
                        pthread_mutex_unlock(&user_mutex);

                        pthread_mutex_lock(&invitation_mutex);
                        vector<Invitation> inv_vec = read_invitation_from_csv();
                        pthread_mutex_unlock(&invitation_mutex);
                        vector<Invitation> inv_to_invitee;
                        for(int i=0;i<inv_vec.size();i++){             // 從當前的invitation裡面找出給這個user的
                            if(inv_vec[i].invitee==curr_user_name){
                                inv_to_invitee.push_back(inv_vec[i]);
                            }
                        }
                        Room* the_true_room = nullptr;
                        for(int i=0;i<inv_to_invitee.size();i++){  //給這個user的invitation裡面,找出user accept的那一個,看code有沒有對
                            pthread_mutex_lock(&user_mutex);
                            User* inviter_user = get_user(inv_to_invitee[i].inviter);
                            pthread_mutex_unlock(&user_mutex);
                            pthread_mutex_lock(&room_mutex);
                            Room* inviter_room = get_room(inviter_user->game_room_id);
                            pthread_mutex_unlock(&room_mutex);
                            if(inviter_user->email==info[1]){
                                if(inviter_room==nullptr){  // 這個invitation的sender已經離開房間了
                                    continue;
                                }
                                else{
                                    invitation_found = true;
                                    if(inviter_room->invitation_code==stoi(info[2])){
                                        code_correct = true;
                                        the_true_room = inviter_room;
                                        break;
                                    }
                                }
                            }
                        }

                        if(invitation_found&&code_correct){
                            if(the_true_room->status=="Started_Game"){
                                strcpy(send_buffer,"Game has started, you can't join now\n");
                                write(clientsd,(char*) send_buffer,strlen(send_buffer));
                            }
                            else{
                                char message_to_invitee[1000];
                                sprintf(message_to_invitee,"You join game room %d\n",the_true_room->id);
                                strcpy(send_buffer,message_to_invitee);
                                write(clientsd,(char*) send_buffer,strlen(send_buffer));

                                pthread_mutex_lock(&user_mutex);
                                vector<User> user_in_the_room = get_user_in_room(the_true_room->id);
                                pthread_mutex_unlock(&user_mutex);

                                pthread_mutex_lock(&user_mutex);
                                enter_room(curr_user->name,the_true_room->id,user_in_the_room.size()+1);
                                pthread_mutex_unlock(&user_mutex);

                                for(auto user:user_in_the_room){
                                    if(user.name!=curr_user_name){
                                        string message_to_people_in_the_room = "Welcome, "+curr_user_name+" to game!\n";
                                        strcpy(send_buffer,message_to_people_in_the_room.c_str());
                                        write(user.client_sd,(char*) send_buffer,strlen(send_buffer));
                                    }
                                }
                            }
                        }
                        else if(invitation_found&&!code_correct){
                            strcpy(send_buffer,"Your invitation code is incorrect\n");
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                        }
                        else if(!invitation_found){
                            strcpy(send_buffer,"Invitation not exist\n");
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                        }
                    }
                }
            }
            else if(info[0]=="leave"&&info[1]=="room"){
                if(someone_login==false){
                    strcpy(send_buffer,"You are not logged in\n");
                    write(clientsd,(char*) send_buffer,strlen(send_buffer));
                }
                else{
                    pthread_mutex_lock(&user_mutex);
                    User* curr_user = get_user(curr_user_name);
                    pthread_mutex_unlock(&user_mutex);
                    if(curr_user->game_room_id==-1){
                        strcpy(send_buffer,"You did not join any game room\n");
                        write(clientsd,(char*) send_buffer,strlen(send_buffer));
                    }
                    else{
                        pthread_mutex_lock(&room_mutex);
                        Room* curr_room = get_room(curr_user->game_room_id);
                        pthread_mutex_unlock(&room_mutex);

                        pthread_mutex_lock(&user_mutex);
                        vector<User> user_in_this_room = get_user_in_room(curr_room->id);
                        pthread_mutex_unlock(&user_mutex);

                        if(curr_room->manager==curr_user->name){
                            for(auto user:user_in_this_room){
                                pthread_mutex_lock(&user_mutex);
                                leave_room(user.name,user.game_room_id);
                                pthread_mutex_unlock(&user_mutex);
                            }
                            pthread_mutex_lock(&user_mutex);
                            leave_room(curr_user->name,curr_room->id);
                            pthread_mutex_unlock(&user_mutex);

                            pthread_mutex_lock(&room_mutex);
                            remove_room(curr_room->id);
                            pthread_mutex_unlock(&room_mutex);

                            char message[1000] = {0};
                            sprintf(message,"You leave game room %d\n",curr_room->id);
                            strcpy(send_buffer,message);
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));

                            for(auto user:user_in_this_room){
                                if(user.name!=curr_room->manager){
                                    char message_to_others[1000] = {0};
                                    sprintf(message_to_others,"Game room manager leave game room %d, you are forced to leave too\n",curr_room->id);
                                    strcpy(send_buffer,message_to_others);
                                    write(user.client_sd,(char*) send_buffer,strlen(send_buffer));
                                }
                            }
                        }
                        else if(curr_room->status=="Started_Game"){
                            pthread_mutex_lock(&user_mutex);
                            leave_room(curr_user->name,curr_room->id);
                            pthread_mutex_unlock(&user_mutex);

                            char message[1000] = {0};
                            sprintf(message,"You leave game room %d, game ends\n",curr_room->id);
                            strcpy(send_buffer,message);
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));

                            for(auto user:user_in_this_room){
                                if(user.name!=curr_user_name){
                                    char message_to_others[1000] = {0};
                                    sprintf(message_to_others,"%s leave game room %d, game ends\n",curr_user->name.c_str(),curr_room->id);
                                    strcpy(send_buffer,message_to_others);
                                    write(user.client_sd,(char*) send_buffer,strlen(send_buffer));
                                }
                            }
                        }
                        else if(curr_room->status=="Waiting_for_player"){
                            pthread_mutex_lock(&user_mutex);
                            leave_room(curr_user->name,curr_room->id);
                            pthread_mutex_unlock(&user_mutex);

                            char message[1000] = {0};
                            sprintf(message,"You leave game room %d\n",curr_room->id);
                            strcpy(send_buffer,message);
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));

                            for(auto user:user_in_this_room){  // send message to all users in the rooom
                                if(user.name!=curr_user_name){
                                    char message_to_others[1000] = {0};
                                    sprintf(message_to_others,"%s leave game room %d\n",curr_user->name.c_str(),curr_room->id);
                                    strcpy(send_buffer,message_to_others);
                                    write(user.client_sd,(char*) send_buffer,strlen(send_buffer));
                                }
                            }
                        }
                    }
                }
                usleep(1000);
            }
            else if(info[0]=="start"&&info.size()>=3){
                if(!someone_login){
                    strcpy(send_buffer,"You are not logged in\n");
                    write(clientsd,(char*) send_buffer,strlen(send_buffer));
                }
                else{
                    pthread_mutex_lock(&user_mutex);
                    User* curr_user = get_user(curr_user_name);
                    pthread_mutex_unlock(&user_mutex);

                    if(curr_user->game_room_id==-1){
                        strcpy(send_buffer,"You did not join any game room\n");
                        write(clientsd,(char*) send_buffer,strlen(send_buffer));
                    }
                    else{
                        pthread_mutex_lock(&room_mutex);
                        Room* curr_room = get_room(curr_user->game_room_id);
                        pthread_mutex_unlock(&room_mutex);

                        if(curr_user->name!=curr_room->manager){
                            strcpy(send_buffer,"You are not game room manager, you can't start game\n");
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                        }
                        else if(curr_room->status=="Started_Game"){
                            strcpy(send_buffer,"Game has started, you can't start again\n");
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                        }
                        else{
                            if(info.size()==3){
                                game_ans = rand_gen();
                                game_limit_round_number = stoi(info[2]);

                                pthread_mutex_lock(&room_mutex);
                                start_game(curr_room->id);
                                pthread_mutex_unlock(&room_mutex);

                                curr_round = 1;

                                pthread_mutex_lock(&user_mutex);
                                user_order = get_user_in_room(curr_room->id);
                                pthread_mutex_unlock(&user_mutex);

                                sort(user_order.begin(),user_order.end(),user_order_compare);
                                curr_player_name = user_order[0].name;

                                char message_to_all[100] = {0};
                                for(auto user:user_order){  // send message to all users
                                    sprintf(message_to_all,"Game start! Current player is %s\n",user_order[0].name.c_str());
                                    strcpy(send_buffer,message_to_all);
                                    write(user.client_sd,(char*) send_buffer,strlen(send_buffer));
                                }
                            }
                            else if(info.size()==4){
                                if(is_four_digit_with_leading_zero(info[3])){
                                    game_ans = info[3];
                                    game_limit_round_number = stoi(info[2]);

                                    pthread_mutex_lock(&room_mutex);
                                    start_game(curr_room->id);
                                    pthread_mutex_unlock(&room_mutex);

                                    curr_round = 1;

                                    pthread_mutex_lock(&user_mutex);
                                    user_order = get_user_in_room(curr_room->id);
                                    pthread_mutex_unlock(&user_mutex);

                                    sort(user_order.begin(),user_order.end(),user_order_compare);
                                    curr_player_name = user_order[0].name;

                                    char message_to_all[100] = {0};
                                    for(auto user:user_order){  // send message to all users
                                        sprintf(message_to_all,"Game start! Current player is %s\n",user_order[0].name.c_str());
                                        strcpy(send_buffer,message_to_all);
                                        write(user.client_sd,(char*) send_buffer,strlen(send_buffer));
                                    }
                                }
                                else{
                                    strcpy(send_buffer,"Please enter 4 digit number with leading zero\n");
                                    write(clientsd,(char*)send_buffer,strlen(send_buffer));
                                }
                            }
                        }
                    }
                }
            }
            else if(info[0]=="guess"){
                if(someone_login==false){
                    strcpy(send_buffer,"You are not logged in\n");
                    write(clientsd,(char*) send_buffer,strlen(send_buffer));
                }
                else{
                    pthread_mutex_lock(&user_mutex);
                    User* curr_user = get_user(curr_user_name);
                    pthread_mutex_unlock(&user_mutex);
                    if(curr_user->game_room_id==-1){
                        strcpy(send_buffer,"You did not join any game room\n");
                        write(clientsd,(char*) send_buffer,strlen(send_buffer));
                    }
                    else{
                        pthread_mutex_lock(&room_mutex);
                        Room* curr_room = get_room(curr_user->game_room_id);
                        pthread_mutex_unlock(&room_mutex);

                        if(curr_room->status=="Waiting_for_player"){
                            if(curr_user->name==curr_room->manager){
                                strcpy(send_buffer,"You are game room manager, please start game first\n");
                            }
                            else{
                                strcpy(send_buffer,"Game has not started yet\n");
                            }
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                        }
                        else if(curr_player_name!=curr_user->name){
                            char message[100] = {0};
                            sprintf(message,"Please wait..., current player is %s\n",curr_player_name.c_str());
                            strcpy(send_buffer,message);
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                        }
                        else if(is_four_digit_with_leading_zero(info[1])==false){
                            strcpy(send_buffer,"Please enter 4 digit number with leading zero\n");
                            write(clientsd,(char*) send_buffer,strlen(send_buffer));
                        }
                        else{
                            string game_result = game(game_ans,info[1]);
                            
                            //猜完了，換下一個人
                            for(int i=0;i<user_order.size();i++){
                                if (user_order[i].name==curr_player_name){
                                    curr_player_name = user_order[(i+1)%user_order.size()].name;
                                    break;
                                }
                            }
                            if(curr_user->name==user_order[0].name){
                                curr_round++;
                            }

                            char message[100] = {0};
                            if(game_result=="4A0B"){
                                sprintf(message,"%s guess '%s' and got Bingo!!! %s wins the game, game ends\n",curr_user->name.c_str(),info[1].c_str(),curr_user->name.c_str());
                                pthread_mutex_lock(&room_mutex);
                                end_game(curr_room->id);
                                pthread_mutex_unlock(&room_mutex);
                            }
                            else{
                                if(curr_round>game_limit_round_number&&curr_player_name==user_order[0].name){
                                    sprintf(message,"%s guess '%s' and got '%s'\nGame ends, no one wins\n",curr_user->name.c_str(),info[1].c_str(),game_result.c_str());
                                    pthread_mutex_lock(&room_mutex);
                                    end_game(curr_room->id);
                                    pthread_mutex_unlock(&room_mutex);
                                }
                                else{
                                    sprintf(message,"%s guess '%s' and got '%s'\n",curr_user->name.c_str(),info[1].c_str(),game_result.c_str());
                                }
                            }
                            for(auto user:user_order){  // send message to all users
                                strcpy(send_buffer,message);
                                write(user.client_sd,(char*) send_buffer,strlen(send_buffer));
                            }
                        }
                    }
                }
            }
            else if(info[0]=="status"){
                string b_login_file,c_login_file,d_login_file;
                b_login_file = "/efs/b_login";
                c_login_file = "/efs/c_login";
                d_login_file = "/efs/d_login";
                int b_login_user_number,c_login_user_number, d_login_user_number;
                fstream login_file;

                login_file.open(b_login_file);
                login_file>>b_login_user_number;
                login_file.close();

                login_file.open(c_login_file);
                login_file>>c_login_user_number;
                login_file.close();

                login_file.open(d_login_file);
                login_file>>d_login_user_number;
                login_file.close();

                char message[100] = {0};

                sprintf(message,"Server1: %d\nServer2: %d\nServer3: %d",b_login_user_number, c_login_user_number, d_login_user_number);

                strcpy(send_buffer,message);
                write(clientsd,send_buffer,strlen(send_buffer));
            }
            else if(info[0]=="exit"){
                if(someone_login){
                    pthread_mutex_lock(&user_mutex);
                    User* curr_user = get_user(curr_user_name);
                    pthread_mutex_unlock(&user_mutex);

                    if(curr_user->game_room_id!=-1){
                        pthread_mutex_lock(&room_mutex);
                        Room* the_room = get_room(curr_user->game_room_id);
                        pthread_mutex_unlock(&room_mutex);

                        if(curr_user->name==the_room->manager){
                            pthread_mutex_lock(&room_mutex);
                            remove_room(the_room->id);
                            pthread_mutex_unlock(&room_mutex);

                            pthread_mutex_lock(&user_mutex);
                            leave_room(curr_user->name,curr_user->game_room_id);
                            pthread_mutex_unlock(&user_mutex);
                        }
                        else{
                            pthread_mutex_lock(&user_mutex);
                            leave_room(curr_user->name,curr_user->game_room_id);
                            pthread_mutex_unlock(&user_mutex);
                        }
                    }
                    pthread_mutex_lock(&user_mutex);
                    logout_user(curr_user_name);
                    login_user_number--;
                    ofstream login_file;
                    login_file.open(my_login_file);
                    login_file<<login_user_number;
                    login_file.close();
                    pthread_mutex_unlock(&user_mutex);
                }
                close(clientsd);
                // cout<<"client "<<client_id<<" exit"<<endl;
                break;
            }
            else{
                strcpy(send_buffer,"this command is invalid");
                write(clientsd,send_buffer,strlen(send_buffer));
            }
        }
    }
    pthread_exit(NULL);
}

void *udp_worker(void* arg){
    int* data = (int*)arg;
    int udp_fd = data[0];
    while(1){
        char recive_buffer[110] = {0};
        char send_buffer[1000] = {0};

        struct sockaddr_in client_address;

        socklen_t len = sizeof(client_address);

        int str_len = recvfrom(udp_fd, recive_buffer, sizeof(recive_buffer),0, (struct sockaddr*) &client_address, &len);

        recive_buffer[str_len-1] = '\0';

        string cmd = recive_buffer;

        vector<string> info = split_string_deli(cmd," ");

        if(info[0]=="register"){ //register
            pthread_mutex_lock(&user_mutex);
            vector<User> user_list = read_user_from_csv();
            pthread_mutex_unlock(&user_mutex);
            bool name_ok = true;
            for(int i=0;i<user_list.size();i++){
                if(user_list[i].name==info[1]){
                    name_ok = false;
                    break;
                }
            }

            bool email_ok = true;
            for(int i=0;i<user_list.size();i++){
                if(user_list[i].email==info[2]){
                    email_ok = false;
                    break;
                }
            }

            if(name_ok&&email_ok){
                User new_user = User(info[1],info[2],info[3],"Offline",-1,-1,-1);
                pthread_mutex_lock(&user_mutex);
                save_user_to_csv(new_user);
                pthread_mutex_unlock(&user_mutex);
                strcpy(send_buffer,"Register Successfully\n");
                // cout<<"user register"<<endl;
                // cout<<"name: "<<new_user.name<<endl;
                // cout<<"email: "<<new_user.email<<endl;
                // cout<<"password: "<<new_user.password<<endl;
                // cout<<endl;
            }
            else {
                strcpy(send_buffer,"Username or Email is already used\n");
            }
        }
        else if(info[0]=="list"){
            if(info[1]=="rooms"){
                pthread_mutex_lock(&room_mutex);
                vector<Room> rooms = read_room_from_csv();
                pthread_mutex_unlock(&room_mutex);
                string res = list_room(rooms);
                
                strcpy(send_buffer,res.c_str());
            }
            else if(info[1]=="users"){
                pthread_mutex_lock(&user_mutex);
                vector<User> users = read_user_from_csv();
                pthread_mutex_unlock(&user_mutex);
                string res = list_user(users);
                strcpy(send_buffer,res.c_str());
            }
            else{
                strcpy(send_buffer,"this command is invalid\n");
            }
        }
        else{
            strcpy(send_buffer,"this command is invalid\n");
        }
        sendto(udp_fd,(char*) send_buffer, strlen(send_buffer), 0, (struct sockaddr *) &client_address, len);
    }
    pthread_exit(NULL);
}
int main(int argc, char* argv[]){
    usleep(15000);

    pthread_mutex_init(&user_mutex,NULL);
    pthread_mutex_init(&room_mutex,NULL);
    pthread_mutex_init(&invitation_mutex,NULL);
    //check
    

    fstream login_file;

    login_file.open(my_login_file);
    login_file<<0;
    login_file.close();
                
    ofstream output;
    // pthread_mutex_lock(&user_mutex);
    output.open("user.csv");
    output<<"";
    output.close();
    // pthread_mutex_unlock(&user_mutex);

    // pthread_mutex_init(&room_mutex,NULL);
    output.open("room.csv");
    output<<"";
    output.close();
    // pthread_mutex_unlock(&room_mutex);

    // pthread_mutex_lock(&invitation_mutex);
    output.open("invitation.csv");
    output<<"";
    output.close();
    // pthread_mutex_unlock(&invitation_mutex);

    usleep(10000);

    int port_number = 8888;
    char server_ip[] = "127.0.0.1";
    struct sockaddr_in* server_address = create_addr(port_number,server_ip);

    int master_tcp_socket = tcp_connect(*server_address);
    int udp_fd = udp_connect(*server_address);

    pthread_t udpthread;
    int udp_data[1] = {udp_fd};
    pthread_create(&udpthread,NULL,udp_worker,(void*)udp_data);

    // int pid_list[15] = {-1};
    pthread_t pthread_t_list[15] = {0};
    int client_sd_list[15] = {0};
    int client_id = 0;
    while(1){
        struct sockaddr_in client_address;
        socklen_t len = sizeof(client_address);
        int clientsd = accept(master_tcp_socket,(struct sockaddr*)&client_address,&len);
        cout<<"New connection "<<client_id<<endl;
        client_sd_list[client_id] = clientsd;
        int tcp_data[2] = {clientsd,client_id};
        pthread_create(&pthread_t_list[client_id],NULL,tcp_worker,(void*)tcp_data);
        usleep(100);
        client_id++;
    }

    return 0;
}