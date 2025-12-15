#include <iostream>
#include <vector>
#include <string>
using namespace std;
/// 两个npc npc有血量和名字
/// 处理npc被攻击
string npc1_name   = "npc1";
int    npc1_health = 100;

string npc2_name   = "npc2";
int    npc2_health = 100;

void AtackNpc1(int atk)
{
    npc1_health -= atk;
    cout << npc1_name << " was attacked!"
         << "-" << atk << endl;
}
void AtackNpc2(int atk)
{
    npc2_health -= atk;
    cout << npc2_name << " was attacked!"
         << "-" << atk << endl;
}

vector<string> npcs_name;
vector<int>    npcs_health;
void           AtackNpc(int index, int atk)
{
    npcs_health[index] -= atk;
    cout << npcs_name[index] << " was attacked!"
         << "-" << atk << endl;
}

class Npc
{
public:
    string name_;
    int    health_ = 100;
    void   Atack(int atk)
    {
        health_ -= atk;
        cout << name_ << " was attacked!"
             << "-" << atk << endl;
    }
};


int main()

{
    AtackNpc1(10);
    AtackNpc2(20);

    npcs_name.push_back("vector npc1");
    npcs_health.push_back(100);

    npcs_name.push_back("vector npc2");
    npcs_health.push_back(100);
    AtackNpc(0, 30);
    AtackNpc(1, 40);

    Npc npc1;
    npc1.name_ = "class npc1";

    Npc npc2;
    npc2.name_ = "class npc2";

    npc1.Atack(50);
    npc2.Atack(60);
}
