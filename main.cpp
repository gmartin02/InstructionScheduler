#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
using namespace std;

int fetch_count = 0; // tag counter.
const int MAX_SIZE = 1024; // Max size for queue sizes.
int cycle = 0; // Cycle that program is on
ifstream file("val_trace_gcc.txt");

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
    instruction (int address, int operation, string dest, string src1, string src2, int tag) {   // Added instruction constructor to make adding instructions to dispatch easier.
        this->address = address;
        this->operation = operation;
        this->dest = dest;
        this->src1 = src1;
        this->src2 = src2;
        this->tag = fetch_count;
        this->state = IF;
        fetch_count++; // increment tag at every instruction object creation.
    };
};

class Queue {   // Kept queue around just in case.
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
vector<instruction> dispatch_latency; // The temporary queue for dispatch. It doesn't have a max size defined to it.
vector<instruction> fakeROB;

int main(int argc, const char * argv[]) {
    
    do {
        //FakeRetire();
        //Execute();
        //Issue();
        Dispatch();
        if (file.is_open() && !file.eof()) {
            Fetch();
        }
        cout << "hit" << endl;
    } while(Advance_Cycle());
     
    return 0;
    
    /*
    //Some test code I used to test fetch.
    while (file.is_open() && !file.eof()) {
        Fetch();
    }
    cout << dispatch.front().address << endl; // Some test code to display if each instruction was read properly.
    */
}

void FakeRetire() {
    for(int i = 0; i < fakeROB.size(); ++i) { //Checks instrs in fakeROB. If WB state found, remove instr
        if(fakeROB[i].state == WB) {
            fakeROB.erase(fakeROB.begin() + i);
        }
    }
}

void Execute() {
    for (int i = 0; i < execute.size(); ++i) { // For loop that iterates through the instructions in the execute queue
        if (execute[i].state == EX) { //check if the insruction is in the execute state if it is then it is time to move it to writeback state
            execute[i].state = WB;  //These line of code helps with the transition of EX to WB by updating the state
            execute.erase(execute.begin() + i);  // Removing the instrucion from execute queue because it has now been move to the writeback queue
            --i;  //This line is to make sure the next instruction in the curent position is not skipped after removal of the previous instruction.
        }
    }
}

void Issue() {
    for (int i = 0; i < issue.size(); ++i) { // For loop that iterates through the instructions in the issue queue
        if (issue[i].state == IS) { //check if the insruction is in the issue state if it is then it is time to move it to execute state
            issue[i].state = EX;  // These line of code helps with the transition of IS to EX by updating the state
            execute.push_back(issue[i]);  // Adding the instruction to the execute queue
            issue.erase(issue.begin() + i);  // Removing the instrucion from issue queue because it has now been moved to the excute queue
            --i;  // This line is to make sure the next instruction in the curent position is not skipped after removal of the previous instruction.
        }
    }
}

void Dispatch () {

    // We check if there is space avaliable in the issue queue.
    if (issue.size() < MAX_SIZE) {
        for (int i = 0; i < MAX_SIZE - issue.size(); i++) {     // For the amount of avaliable spots in issue queue, we move that many from dispatch to temp queue.
            if (dispatch.front().state == IF) {break;}
            dispatch_latency.push_back(dispatch.front());   // We can assume the first instruction will be the first to go since we're dealing with queues here.
            dispatch.erase(dispatch.cbegin());
        }

        std::sort(dispatch_latency.begin(), dispatch_latency.end(), [](instruction a, instruction b) {  // Sort by tag, ascending order.
            return a.tag < b.tag;
        });

        for (int i = 0; i < MAX_SIZE - issue.size(); i++) {     // For amount of avaliable spaces in issue queue, move that many to issue queue.
            dispatch_latency.front().state = IS;
            issue.push_back(dispatch_latency.front());
            dispatch_latency.erase(dispatch_latency.cbegin());  // Erase moved instructions from temp queue.
        }
    }

    int i = 0;
    for (instruction ins : dispatch) {
        if (ins.state == IF) {
            ins.state = ID;
            i++;
        }
        if (i >= 30) {  // I'm not sure if we set all instructions to ID after one cycle delay or just a small number. I went with 30 at a time for now.
            break;
        }
    }

    

    // There needs to be something else in Dispatch() that renames the src1, src2, and dest operands. Its part of Tomasulo’s algorithm.
}

void Fetch() {
    // For reference
    // <PC> <operation type> <dest reg #> <src1 reg #> <src2 reg #>
    int address;
    int operation;
    int tag;
    int fetch_bandwidth = 0;
    string dest;
    string src1;
    string src2;
    string line;

    while (dispatch.size() < MAX_SIZE && fetch_bandwidth != 30) { // Check if the dispatch queue is full. If yes, we skip for the time-being.
        getline(file, line);
        if (line == "") { // The trace file has a weird empty line at the end which throws off the entire code, so I made sure to check for empty lines.
            file.close();
            return;
        }
        if (file.eof()) { // If the file has reached the end, we close it.
            file.close();
        }
        fetch_bandwidth++;
        stringstream ss(line);
        string temp;
        ss >> temp;
        address = stoi(temp, nullptr, 16); // Interpret string as hex and store it as integer, might have to change this to store as actual hex.
        ss >> temp;
        operation = stoi(temp);
        ss >> dest;
        ss >> src1;
        ss >> src2;
        dispatch.push_back(instruction(address, operation, dest, src1, src2, fetch_count)); // Add to dispatch queue.
        cout << tag << endl;
    }
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
    /*for(int i = 0; i <= MAX_SIZE; i++) {
        if (dispatch[i].state != WB && dispatch[i].state != IF) return false;
        if (issue[i].state != WB && issue[i].state != IF) return false;
        if (execute[i].state != WB && execute[i].state != IF) return false;
    }*/
    for(int i = 0; i <= MAX_SIZE; i++) {
        if (dispatch.size() != 0) return false;
        if (issue.size() != 0) return false;
        if (execute.size() != 0) return false;
    }
    return true;
}
