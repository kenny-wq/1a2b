#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<cstring>
#include<cstdlib>
#include<cstdio>
#include<vector>
#include<sstream>
#include<regex>

#define MAX 1000

using namespace std;

vector<string> split_string(string str){
    vector<string> result;
    stringstream ss(str);

    string temp;
    while(ss>>temp){
        result.push_back(temp);
    }
    return result;
}

int main(int argc, char* argv[]){

    char server_ip[] = "127.0.0.1";
    int port_number = 8888;

    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_number);
    inet_aton(server_ip,&server_address.sin_addr);

    socklen_t addr_len = sizeof(server_address);

    int tcp_fd = socket(AF_INET, SOCK_STREAM,0);  //connect to tcp

    if(tcp_fd<0){
        cerr<<"tcp socket build fail, please try again";
        exit(EXIT_FAILURE);
    }

    if(connect(tcp_fd,(struct sockaddr*) &server_address,addr_len)<0){
        cerr<<"socket connect fail, please try again";
        exit(EXIT_FAILURE);
    }

    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);  // connect to udp
            
    if (udp_fd< 0) {
        cerr<<"udp socket creation failed"<<endl;
        exit(EXIT_FAILURE);
    }

    cout<<"*****Welcome to Game 1A2B*****"<<endl;
    // cout<<tcp_fd<<endl;

    fd_set read_fds;
    while(1){
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO,&read_fds);
        FD_SET(tcp_fd,&read_fds);

        int max_fd = tcp_fd;

        int activity = select(max_fd+1,&read_fds,nullptr,nullptr,nullptr);

        if(FD_ISSET(tcp_fd,&read_fds)){
            char recive_buffer[110] = {0};
            int str_len = read(tcp_fd,recive_buffer, 110);
            
            recive_buffer[str_len] = '\0';

            printf("%s\n",recive_buffer);
        }
        if(FD_ISSET(STDIN_FILENO,&read_fds)){
            string cmd;
            getline(cin,cmd,'\n');

            if(cmd.size()==0){
                continue;
            }

            cmd += "\n";
            vector<string> splitted_cmd = split_string(cmd);

            if(splitted_cmd[0]=="register"||(splitted_cmd[0]=="list"&&splitted_cmd[1]!="invitations")){
                char send_buffer[1000]={0};

                strcpy(send_buffer,cmd.c_str());

                sendto(udp_fd, (char*)send_buffer, strlen(send_buffer),0, 
                (struct sockaddr*)&server_address,sizeof(server_address));

                socklen_t len = sizeof(server_address);

                char recive_buffer[110] = {0};
                int str_len = recvfrom(udp_fd,(char *)recive_buffer, sizeof(recive_buffer),0, (struct sockaddr *) & server_address, &len);
                
                recive_buffer[str_len] = '\0';
                printf("%s\n",recive_buffer);
            }
            else if(splitted_cmd[0]=="exit"){
                char send_buffer[1000]={0};

                strcpy(send_buffer,cmd.c_str());

                write(tcp_fd, (char*)send_buffer, strlen(send_buffer));

                break;
            }
            else {
                char send_buffer[1000]={0};

                strcpy(send_buffer,cmd.c_str());

                write(tcp_fd, (char*)send_buffer, strlen(send_buffer));

                char recive_buffer[110] = {0};
                int str_len = read(tcp_fd,recive_buffer, 110);
                
                recive_buffer[str_len] = '\0';

                printf("%s\n",recive_buffer);
            }
        }
        
    }
    return 0;
}