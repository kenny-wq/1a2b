#ifndef USER_H
#define USER_H

using namespace std;
class User
{
public:
    string name;
    string email;
    string password;
    string status;
    int game_room_id;
    int enter_room_order;
    int client_sd;
    User(){}

    User(string u_name,string u_email, string u_password, string u_status, int u_room_id, int u_order,int u_client_sd){
        name = u_name;
        email = u_email;
        password = u_password;
        status = u_status;
        game_room_id = u_room_id;
        enter_room_order = u_order;
        client_sd = u_client_sd;
    }
};

void save_user_to_csv(User u);

vector<User> read_user_from_csv();

bool user_order_compare(User a, User b);

bool user_alpha_order(User a, User b);

string list_user(vector<User> users);

void login_user(string username,int client_sd);

void logout_user(string username);

User* get_user(string username);

User* get_user_by_email(string email);

vector<User> get_user_in_room(int room_id);

#endif //USER_H