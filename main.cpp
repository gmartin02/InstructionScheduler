#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
using namespace std;

const int N = 2; // Max size for queue sizes.

int cycle = 0; // Cycle that program is on
ifstream file("val_trace_gcc.txt");
int tag_counter = 0; // Tag counter.
int RF[128][2];


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
    int tag;
    int address;
    int operation;
    string dest;
    string src1;
    string src2;
    string dest_o; // The difference bewteen <reg>_o and <reg> is that <reg>_o is the original unmodified register and the other one gets overwritten with a tag later on.
    string src1_o;
    string src2_o;
    int cycle;
    int IF_duration; // unused atm.
    int ID_duration; // unused atm.
    int IS_duration; // unused atm.
    int EX_duration; // unused atm.
    int WB_duration; // unused atm.
    int exec_latency = 0;
    instruction (int address, int operation, string dest, string src1, string src2) {   // Added instruction constructor to make adding instructions to dispatch easier.
        this->address = address;
        this->operation = operation;

        this->dest = dest;
        this->src1 = src1;
        this->src2 = src2;

        this->dest_o = dest;
        this->src1_o = src1;
        this->src2_o = src2;

        this->state = IF;
        this->tag = tag_counter;
    };
};
void RenameOps(instruction *);
void init_RF();
void FakeRetire();
void Execute();
void Issue();
void Dispatch();
void Fetch();
bool Advance_Cycle();
bool isEmpty();

vector<instruction*> dispatch;
vector<instruction*> issue;
vector<instruction*> execute;
vector<instruction*> fakeROB;

int main(int argc, const char * argv[]) {

    init_RF();
    
    do {
        cout << "----------- Iteration: " << cycle << "-----------" << endl;
        cout << "dispatch" << dispatch.size() << endl;
        cout << "issue" << issue.size() << endl;
        cout << "execute" << execute.size() << endl;
        cout << "fakeROB" << fakeROB.size() << endl;
        cout << endl;
        
        //cout << "hit before fakeretire" << endl;
        FakeRetire();
        //cout << "hit before execute" << endl;
        Execute();
        //cout << "hit before issue" << endl;
        Issue();
        //cout << "hit before dispatch" << endl;
        Dispatch();
        //cout << "hit before fetch" << endl;
        if (file.is_open()) {
            Fetch();
        }
        //cout << "hit before adv cycle" << endl;
    } while(Advance_Cycle());

    cout << "Program End" << endl;
     
    return 0;
}

// Initialize Register File (RF). It basically keeps track of each registers usage with a flag, 1 = being used, 0 = ready to go.
void init_RF() {
    for (int i = 0; i < 128; i++) {
        RF[i][0] = i;
        RF[i][1] = 0; // 0 means resgister is unused or ready.
    }
}

// Check fakeROB for any instructions in the WB state and remove them.
void FakeRetire() {
    if (fakeROB.empty()) return;
    for (auto it = fakeROB.begin(); it != fakeROB.end();) {
        if((*it)->state == WB) {
            instruction *temp = *it;
            it = fakeROB.erase(it);
            delete (temp);
        }
        else {
            it++;
        }
    }
}

// Each instruction has an operation value (0, 1, 2) which describe its execution latency, or how many cycles we delay it in the simulator
// Once the instruction passes through its required cycles, we remove it from the execute queue.
void Execute() {
    if (execute.empty()) return;
    for (auto it = execute.begin(); it != execute.end();) {
        if ((*it)->operation == 0 && (*it)->exec_latency != 1) {
            (*it)->exec_latency++; 
            it++;
            continue;
        }

        if ((*it)->operation == 1 && (*it)->exec_latency != 2) {
            (*it)->exec_latency++; 
            it++;
            continue;
        }

        if ((*it)->operation == 2 && (*it)->exec_latency != 5) {
            (*it)->exec_latency++; 
            it++; 
            continue;
        }

        if ((*it)->dest_o != "-1") {
            RF[stoi((*it)->dest_o)][1] = 0; // We access the destination register (dest_o), convert it to integer, plug that number into the RF array, and finally get
                                            // the tag from the 2nd row. 
        }

        (*it)->state = WB;
        it = execute.erase(it);
    }
}

// Send to functional units (AKA the execute queue) if required registers are avaliable. Also check to see if the required sources registers are being used.
void Issue() {
    int count = 0;
    int debug = 0;
    if (issue.empty()) return;

    std::sort(issue.begin(), issue.end(), [](instruction *a, instruction *b) {  // Sort by tag, ascending order.
        return a->tag < b->tag;
    });
    
    for (auto it = issue.begin(); it != issue.end();) {

        /*
        cout << "Current Instruction:" << endl;
        cout << (*it)->address << " " << (*it)->dest_o << " " << (*it)->src1_o <<  " " << (*it)-> src2_o << " " << endl;
        cout << RF[stoi((*it)->dest_o)][1] << " " << RF[stoi((*it)->src1_o)][1] << " " << RF[stoi((*it)->src2_o)][1] <<  " " << endl;
        cout << endl;
        */

        if ((*it)->src1_o != "-1" && RF[stoi((*it)->src1_o)][1] == 1) { // Instructions can have two source registers. If one of them is marked with -1, it's not using
                                                                        // another source register. Otherwise we check to see if the source reg is being used in the RF.
            it++;  
            continue;
        }

        if ((*it)->src2_o != "-1" && RF[stoi((*it)->src2_o)][1] == 1) {
            it++;
            continue;
        }

        (*it)->state = EX;
        execute.push_back(*it);
        it = issue.erase(it);
        count++;
    }
}

// We rename the instructions here.
void Dispatch() {
    int count = 0;
    if (!dispatch.empty() && issue.size() < N) {

        for (auto it = dispatch.begin(); it != dispatch.end();) {     
            //cout << "count: " << count << endl;
            if (issue.size() >= N) {break;}

            if ((*it)->dest_o != "-1") {
                RF[stoi((*it)->dest_o)][1] = 1; // 1 means not ready; register is being used.
            }

            RenameOps((*it));
            (*it)->state = IS;

            issue.push_back(*it);
            it = dispatch.erase(it);  
            count++;
        }
    }

    int i = 0;
    for (instruction *ins : dispatch) {
        if (i >= N) {  
            break;
        }
        (ins)->state = ID;
        i++;
    }
    //cout << "hit" << endl;
}

// Again, if any one of the instruction's register is marked as -1, it doesn't have that register. 
void RenameOps(instruction *dispatched) {

    if (dispatched->dest != "-1") {
        dispatched->dest = to_string(RF[stoi(dispatched->dest_o)][0]);
    }
    if (dispatched->src1 != "-1") {
        dispatched->src1 = to_string(RF[stoi(dispatched->src1_o)][0]);
    }
    if (dispatched->src2 != "-1") {
        dispatched->src2 = to_string(RF[stoi(dispatched->src2_o)][0]);
    }
}


// Get instructions.
void Fetch() {
    // For reference
    // <PC> <operation type> <dest reg #> <src1 reg #> <src2 reg #>
    int address;
    int operation;
    int tag;
    string dest;
    string src1;
    string src2;
    string line;

    while (dispatch.size() < 2*N) { // Check if the dispatch queue is full. If yes, we skip for the time-being.
        getline(file, line);
        if (line == "") { // The trace file has a weird empty line at the end which throws off the entire code, so I made sure to check for empty lines.
            file.close();
            return;
        }
        if (file.eof()) { // If the file has reached the end, we close it.
            file.close();
        }
        stringstream ss(line);
        string temp;
        ss >> temp;
        address = stoi(temp, nullptr, 16); // Interpret string as hex and store it as integer, might have to change this to store as actual hex.
        ss >> temp;
        operation = stoi(temp);
        ss >> dest;
        ss >> src1;
        ss >> src2;
        instruction *ins = new instruction(address, operation, dest, src1, src2);
        dispatch.push_back(ins); // Add to dispatch queue.
        fakeROB.push_back(ins);     // Add to fakeROB
        if (dest != "-1") {
            RF[stoi(dest)][0] = tag_counter;    // Here we assign a tag to the register in the RF.
        }
        tag_counter++;
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

//Checks if empty. It's fine.
bool isEmpty() {
    /*if (cycle < 3) {
        return false;
    }*/
        
    if (dispatch.size() != 0) return false;
    if (issue.size() != 0) return false;
    if (execute.size() != 0) return false;
    if (fakeROB.size() != 0) return false;
    
    return true;
}
