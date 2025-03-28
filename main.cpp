#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
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
        enum state_list state;
        int address;
        int operation;
        string dest;
        string src1;
        string src2;
        int tag;
    instruction (int address, int operation, string dest, string src1, string src2) {
        this->address = address;
        this->operation = operation;
        this->dest = dest;
        this->src1 = src1;
        this->src2 = src2;
        //this->tag = tag;
        this->state = IF;
    };
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

vector<instruction> dispatch;
vector<instruction> issue;
vector<instruction> execute;
ifstream file("val_trace_gcc.txt");

const int MAX_SIZE = 100000;

//Cycle that program is on
int cycle = 0;

int main(int argc, const char * argv[]) {
    /*do {
        FakeRetire();
        Execute();
        Issue();
        Dispatch();
        if (file.is_open() && !file.eof()) {
            Fetch();
        }
    } while(Advance_Cycle());
    
    return 0;*/

    
    //Some test code I used to test fetch.
    while (file.is_open() && !file.eof()) {
        Fetch();
    }
    cout << dispatch.back().address << endl; // Some test code to display if each instruction was read properly.
    
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
    // For reference
    // <PC> <operation type> <dest reg #> <src1 reg #> <src2 reg #>
    int address;
    int operation;
    string dest;
    string src1;
    string src2;
    int tag;
    string line;

    if (dispatch.size() < MAX_SIZE) {
        getline(file, line);
        if (line == "") {
            file.close();
            return;
        }
        if (file.eof()) {
            file.close();
        }
        stringstream ss(line);
        string temp;
        ss >> temp;
        address = stoi(temp, nullptr, 16);
        ss >> temp;
        operation = stoi(temp);
        ss >> dest;
        ss >> src1;
        ss >> src2;
        dispatch.push_back(instruction(address, operation, dest, src1, src2));
        return;
    }
    cerr << "\nDispatch queue at maximum size, will try again next cycle.\n";
    
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
