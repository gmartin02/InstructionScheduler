#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>
using namespace std;

const int N = 8; // Max size for queue sizes.
const int S = 2; // Scheduling queue size.

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
    int IF_duration = 0; 
    int ID_duration = 0; 
    int IS_duration = 0; 
    int EX_duration = 0; 
    int WB_duration = 0; 
    int IF_cycle = 0;
    int ID_cycle = 0; 
    int IS_cycle = 0; 
    int EX_cycle = 0; 
    int WB_cycle = 0; 
    int exec_latency = 0;

    int special_flag = 0;

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
        this->IF_cycle = cycle;

        this->IF_duration = 1;
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

// Pointers are useful for a project like this since it allows us to access the data easily anywhere, anytime, between any data structure.
vector<instruction*> dispatch;
vector<instruction*> issue;
vector<instruction*> execute;
vector<instruction*> fakeROB;

int main(int argc, const char * argv[]) {
    init_RF();
    
    do {
        FakeRetire();
        Execute();
        Issue();
        Dispatch();
        if (file.is_open()) {
            Fetch();
        }
    } while(Advance_Cycle());
    cout << "number of instructions = " << tag_counter << endl;
    cout << "number of cycles       = " << cycle << endl;
    cout << std::fixed << std::setprecision(5) << "IPC                    = " << float(tag_counter) / float(cycle) << endl;
     
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
        if ((*it)->state != WB) return;
        (*it)->WB_duration = cycle - ((*it)->WB_cycle);

        cout << (*it)->tag << " fu{" << (*it)->operation << "} src{" << (*it)->src1_o << ","  << (*it)->src2_o << "} dest{" << (*it)->dest_o 
        << "} IF{"  << (*it)->IF_cycle << "," << (*it)->IF_duration 
        << "} ID{"  << (*it)->ID_cycle << "," << (*it)->ID_duration 
        << "} IS{" << (*it)->IS_cycle  << "," << (*it)->IS_duration 
        << "} EX{" << (*it)->EX_cycle << "," << (*it)->EX_duration 
        << "} WB{" << (*it)->WB_cycle << "," << (*it)->WB_duration << "}" << endl;

        delete (*it);
        it = fakeROB.erase(it);
        
    }
}

// Each instruction has an operation value (0, 1, 2) which describe its execution latency, or how many cycles we delay it in the simulator.
// Once the instruction passes through its required cycles, we remove it from the execute queue.
void Execute() {
    if (execute.empty()) return;
    for (auto it = execute.begin(); it != execute.end();) {
        if ((*it)->operation == 0 && (*it)->exec_latency < 1) {
            (*it)->exec_latency++; 
            it++;
            continue;
        }

        if ((*it)->operation == 1 && (*it)->exec_latency < 2) {
            (*it)->exec_latency++; 
            it++;
            continue;
        }

        if ((*it)->operation == 2 && (*it)->exec_latency < 5) {
            (*it)->exec_latency++; 
            it++; 
            continue;
        }

        // We access the destination register (dest_o), convert it to integer with stoi(), plug that number into the RF array, and finally get
        // the tag by using the second row of the array [<some number>][1];
        if ((*it)->dest_o != "-1") {
            RF[stoi((*it)->dest_o)][1] = 0; 
        }

        (*it)->state = WB;
        (*it)->WB_cycle = cycle;

        (*it)->EX_duration = cycle - (*it)->EX_cycle;
        it = execute.erase(it);
    }
}

/*
// Send to functional units (AKA the execute queue) if required registers are avaliable. Also check to see if the required sources registers are being used.
// Issue sends out N+1 instructions to execute.
void Issue() {
    int count = 0;
    
    std::sort(issue.begin(), issue.end(), [](instruction *a, instruction *b) {  // Sort by tag, ascending order.
        return a->tag < b->tag;
    });
    if (!issue.empty() && execute.size() <= N) {
        for (auto it = issue.begin(); it != issue.end();) {
            if (count >= N || execute.size() > N) break;
            
            // Instructions can have two source registers. If one of them is marked with -1, it's not using
            // another source register. Otherwise we check to see if the source reg is being used in the RF.
            if ((*it)->src1_o != "-1" && RF[stoi((*it)->src1_o)][1] == 1) { 
                it++;  
                continue;
            }
            
            if ((*it)->src2_o != "-1" && RF[stoi((*it)->src2_o)][1] == 1) {
                it++;
                continue;
            }
            
            if ((*it)->dest_o != "-1") {
                RF[stoi((*it)->dest_o)][1] = 1; // 1 means not ready; register is being used.
            }
            
            (*it)->state = EX;
            (*it)->EX_cycle = cycle;
            (*it)->IS_duration = cycle - (*it)->IS_cycle;

            execute.push_back(*it);
            it = issue.erase(it);
            count++;
        }
    }
}
*/

// Issue, but flagging doesn't take place here. But we do check for special flags.
void Issue() {
    int count = 0;
    std::sort(issue.begin(), issue.end(), [](instruction *a, instruction *b) {  // Sort by tag, ascending order.
        return a->tag < b->tag;
    });
    if (!issue.empty() && execute.size() <= N) {
        for (auto it = issue.begin(); it != issue.end();) {
            if (count >= N || execute.size() > N) break;
            //if ((*it)->dest_o != "-1" && RF[stoi((*it)->dest_o)][1] == 1) {
                //it++;
                //continue;
            //}
            
            if ((*it)->src1_o != "-1" && RF[stoi((*it)->src1_o)][1] == 1 && (*it)->special_flag != 1) { 
                it++;  
                continue;
            }
            
            if ((*it)->src2_o != "-1" && RF[stoi((*it)->src2_o)][1] == 1 && (*it)->special_flag != 1) {
                it++;
                continue;
            }
            
            (*it)->state = EX;
            (*it)->EX_cycle = cycle + 1;
            (*it)->IS_duration = cycle - (*it)->IS_cycle + 1;

            execute.push_back(*it);
            it = issue.erase(it);
            count++;
        }
    }
}

/*
// We rename the instructions here, and push to the queue. Note, issue can only contain S instructions.
void Dispatch() {
    int count = 0;
    if (!dispatch.empty() && issue.size() < S && dispatch.front()->state != IF) {
        for (auto it = dispatch.begin(); it != dispatch.end();) {     
            if (issue.size() >= S || count >= N || (*it)->state == IF) {break;}

            RenameOps((*it));
            (*it)->state = IS;
            (*it)->IS_cycle = cycle;
            issue.push_back(*it);

            (*it)->ID_duration = cycle - (*it)->ID_cycle;
            it = dispatch.erase(it);  
            count++;
        }
    }
    int i = 0;
    for (instruction *ins : dispatch) {
        if (i >= N) break;
        if (ins->state != ID) {
            ins->state = ID;
            i++;
        }
    }
    // We only set N instructions at a time (dispatch has a size of 2N) to the ID state. This simulates the 1 cycle latency between IF -> ID.
}
*/

// Dispatch, but flagging takes place here in this version.
void Dispatch() {
    int count = 0;
    if (!dispatch.empty() && issue.size() < S && dispatch.front()->state != IF) {
        for (auto it = dispatch.begin(); it != dispatch.end();) {     
            //Since the issue queue essentially contains only N instructions, we push only that amount.
            if (issue.size() >= S || count >= N || (*it)->state == IF) {break;}
            
            if ((*it)->dest_o != "-1") {
                if (RF[stoi((*it)->dest_o)][1] == 0) {
                    if ((*it)->dest_o == (*it)->src1_o) {
                        
                        (*it)->special_flag = 1;
                    }
                    if ((*it)->dest_o == (*it)->src2_o) {
                        
                        (*it)->special_flag = 1;
                    }
                }
                RF[stoi((*it)->dest_o)][1] = 1; // 1 means not ready; register is being used. // went up ^ there where special flag line is.
            }

            RenameOps((*it));
            (*it)->state = IS;
            (*it)->IS_cycle = cycle + 1;
            issue.push_back(*it);

            (*it)->ID_duration = cycle - (*it)->ID_cycle + 1;
            it = dispatch.erase(it);  
            count++;
        }
    }
    int i = 0;
    for (instruction *ins : dispatch) {
        if (i >= N) break;
        if (ins->state != ID) {
            ins->state = ID;
            i++;
        }
    }
}


// Where the actual renaming takes place.
// If any one of the instruction's register is marked as -1, it doesn't have that register, and so there's nothing to rename.
void RenameOps(instruction *dispatched) {
    
    if (dispatched->dest_o != "-1") {
        RF[stoi(dispatched->dest)][0] = dispatched->tag;    // Here we assign a tag to the register in the RF.
        dispatched->dest = to_string(RF[stoi(dispatched->dest_o)][0]);
    }
    if (dispatched->src1_o != "-1") {
        dispatched->src1 = to_string(RF[stoi(dispatched->src1_o)][0]);
    }
    if (dispatched->src2_o != "-1") {
        dispatched->src2 = to_string(RF[stoi(dispatched->src2_o)][0]);
    }
}

// Get instructions. We fetch N instructions at a time.
void Fetch() {
    // For reference:
    // <PC> <operation type> <dest reg #> <src1 reg #> <src2 reg #>
    int address;
    int operation;
    int tag;
    string dest;
    string src1;
    string src2;
    string line;
    int count = 0;

    //while (dispatch.size() < 2*N) { // Check if the dispatch queue is full. If yes, we skip fetching for the time-being.
    while (count < N && dispatch.size() < 2*N) {
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
        dispatch.push_back(ins);    // Add to dispatch queue.
        fakeROB.push_back(ins);     // Add to fakeROB
        ins->ID_cycle = cycle + 1;
        tag_counter++;
        count++;
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

//Checks if fakeROB empty. This means there are no longer any instructons left in the pipeline!
bool isEmpty() {
    if (fakeROB.size() != 0) return false;
    
    return true;
}

/* Some handy debug code
cout << "<name of function this code is in>:" << endl;
cout << (*it)->address << " " << (*it)->dest_o << " " << (*it)->src1_o <<  " " << (*it)-> src2_o << " " << (*it)->state << endl;
cout << RF[stoi((*it)->dest_o)][1] << " " << RF[stoi((*it)->src1_o)][1] << " " << RF[stoi((*it)->src2_o)][1] <<  " " << endl;
cout << endl;
*/