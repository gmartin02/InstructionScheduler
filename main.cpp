#include <string>
#include <iostream>
using namespace std;

enum state_list {
    IF,     //Fetch State
    ID,     //Dispatch State
    IS,     //Issue State
    EX,     //Execute State
    WB      //Write Back State
};

class instruction {
    public:
        unsigned long long int address;
        enum state_list state;
        string dest;
        string src1;
        string src2;
        int tag;
};

class Queue {
    private:
        instruction front;
        instruction rear;
        instruction arr[1024];
    public:

};

void FakeRetire();
void Execute();
void Issue();
void Dispatch();
void Fetch();
bool Advance_Cycle();
bool isEmpty();

instruction dispatch[1024];
instruction issue[1024];
instruction execute[1024];

//Cycle that program is on
int cycle = 0;

int main(int argc, const char * argv[]) {
    
    do {
        FakeRetire();
        Execute();
        Issue();
        Dispatch();
        Fetch();
    } while(Advance_Cycle());
    
    return 0;
}

void FakeRetire() {
    
}

void Execute() {
    
}

void Issue() {
    
}

void Dispatch () {
    
}

void Fetch() {
    
}

bool Advance_Cycle() {
    if(isEmpty()) //If there are no more instructions left, end calculations
        return false;
    else { //If there are more instructions to calculate, increment to next cycle and continue
        cycle++;
        return true;
    }
}

//Checks if empty. Very bad optimization, needs to be adjusted
bool isEmpty() {
    for(int i = 0; i <= 1024; i++) {
        if (dispatch[i].state != WB && dispatch[i].state != IF) return false;
        if (issue[i].state != WB && issue[i].state != IF) return false;
        if (execute[i].state != WB && execute[i].state != IF) return false;
    }
    return true;
}
