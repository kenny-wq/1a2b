#ifndef SERVER_FUNCTION_H
#define SERVER_FUNCTION_H

using namespace std;

vector<string> split_string_deli(string s,string deli);

string rand_gen();

string game(string ans, string guess);

void start_game(int room_id);
void end_game(int room_id);


bool is_four_digit_with_leading_zero(string number);

#endif //SERVER_FUNCTION_H