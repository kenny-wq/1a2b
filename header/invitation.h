#ifndef INVITATION_H
#define INVITATION_H

using namespace std;

class Invitation
{
public:
    string inviter;
    string invitee;
    unsigned game_room_id;
    unsigned code;
    Invitation(){}
    Invitation(string Inviter, string Invitee,unsigned room_id, unsigned the_code){
        inviter = Inviter;
        invitee = Invitee;
        game_room_id = room_id;
        code = the_code;
    }
    bool operator < ( const Invitation &b) const{
        return this->game_room_id < b.game_room_id;
    }
};

void save_invitation_to_csv(Invitation inv);

vector<Invitation> read_invitation_from_csv();


string list_inv(vector<Invitation> inv_vec);

#endif //INVITATION_H