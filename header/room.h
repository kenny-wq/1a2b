#ifndef ROOM_H
#define ROOM_H
using namespace std;

class Room
{
public:
    int id;
    unsigned invitation_code;
    string type;
    string status;
    string manager;
    Room(){}
    Room(string room_type,int room_id,string room_status, unsigned invi_code,string room_manager){
        type = room_type;
        id = room_id;
        status = room_status;
        invitation_code = invi_code;
        manager = room_manager;
    }
    bool operator <(Room a){
        return this->id < a.id;
    }
};

void save_room_to_csv(Room room);
vector<Room> read_room_from_csv();

void remove_room(int room_id);

string list_room(vector<Room> rooms);
void enter_room(string username, int room_id, int order);

void leave_room(string username, int room_id);

Room* get_room(unsigned room_id);

#endif //ROOM_H