#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <deque>
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
    WB,     //Write Back State
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

    int src1_flag = 1;
    int src2_flag = 1;

    instruction *depend_1 = NULL;
    instruction *depend_2 = NULL;

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

        if (operation == 0) {
            exec_latency = 1;
            EX_duration = 1;
        }
        else if (operation == 1) {
            exec_latency = 2;
            EX_duration = 2;
        }
        else {
            exec_latency = 5;
            EX_duration = 5;
        }
    };
};
void FetchtoDispatch();
void ClearROB();
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
//vector<instruction*> fetch;
vector<instruction*> fetch(0);
vector<instruction*> dispatch(0);
vector<instruction*> issue(0);
vector<instruction*> execute(0);
vector<instruction*> fakeROB(0);
vector<instruction*> disposal(0);

int main(int argc, const char * argv[]) {
    init_RF();
    
    do {
        //cout << "FR" << endl;
        FakeRetire();
        //cout << "EX" << endl;
        Execute();
        //cout << "IS" << endl;
        Issue();
        //cout << "ID" << endl;
        Dispatch();
        //cout << "IF->ID" << endl;
        //FetchtoDispatch();
        if (file.is_open()) {
            //cout << "IF" << endl;
            Fetch();
        }
        //cout << "adv cyc" << endl;
    } while(Advance_Cycle());
    //cout << "dispatch size: " << dispatch.size() << " issue size: " << issue.size() << " execute size " << execute.size() << endl;
    cout << "number of instructions = " << tag_counter << endl;
    cout << "number of cycles       = " << cycle << endl;
    cout << std::fixed << std::setprecision(5) << "IPC                    = " << float(tag_counter) / float(cycle) << endl;

    ClearROB();
     
    return 0;
}

void init_RF() {
    for (int i = 0; i < 128; i++) {
        RF[i][0] = i;
        RF[i][1] = 0; // 0 means resgister is unused or ready.
    }
}

void ClearROB() {
    for (instruction *ins : disposal) {
        delete ins;
    }
}

// Check fakeROB for any instructions in the WB state and remove them.
void FakeRetire() {
    if (fakeROB.empty()) return;
    for (auto it = fakeROB.begin(); it != fakeROB.end();) {
        if ((*it)->state != WB) return;

        cout << (*it)->tag << " fu{" << (*it)->operation << "} src{" << (*it)->src1_o << ","  << (*it)->src2_o << "} dst{" << (*it)->dest_o 
        << "} IF{"  << (*it)->IF_cycle << "," << (*it)->IF_duration 
        << "} ID{"  << (*it)->ID_cycle << "," << (*it)->IS_cycle - (*it)->ID_cycle
        << "} IS{" << (*it)->IS_cycle  << "," << (*it)->EX_cycle - (*it)->IS_cycle
        << "} EX{" << (*it)->EX_cycle << "," << (*it)->WB_cycle - (*it)->EX_cycle
        << "} WB{" << (*it)->WB_cycle << "," << 1 << "}" << endl;
        
        disposal.push_back((*it));
        it = fakeROB.erase(it);
        
    }
}


// Each instruction has an operation value (0, 1, 2) which describe its execution latency, or how many cycles we delay it in the simulator.
// Once the instruction passes through its required cycles, we remove it from the execute queue.
void Execute() {
    if (execute.empty()) return;

    for (auto it = execute.begin(); it != execute.end();) {
        if ((*it)->exec_latency <= 0) {
            (*it)->state = WB;
            (*it)->WB_cycle = cycle;
            
            it = execute.erase(it);
        }
        else {
            (*it)->exec_latency--; 
            it++;
        }
    }
}

void Issue() {
    std::sort(issue.begin(), issue.end(), [](instruction *a, instruction *b) {  // Sort by tag, ascending order.
        return a->tag < b->tag;
    });
    for (auto it = issue.begin(); it != issue.end();) {
        if (issue.size() > N) break;
        if ((*it)->depend_1 != NULL && (*it)->src1_flag == 0) {
            //if ((*it)->depend_1->state == WB ) {
            if ((*it)->depend_1->state == WB) {
                (*it)->src1_flag = 1;
            }
        }   
        
        if ((*it)->depend_2 != NULL && (*it)->src2_flag == 0) {
            //if ((*it)->depend_2->state == WB) {
                if ((*it)->depend_2->state == WB) {
                (*it)->src2_flag = 1;
            }
        }

        if ((*it)->src1_flag && (*it)->src2_flag) {
            (*it)->state = EX;
            (*it)->EX_cycle = cycle;
            (*it)->exec_latency--;

            execute.push_back(*it);
            it = issue.erase(it);
        }
        else {
            it++;
        }
    }
}

// Dispatch, but flagging takes place here in this version.
void Dispatch() {

    if (issue.size() < S && !dispatch.empty()) {
        for (auto it = dispatch.begin(); it != dispatch.end();) {     
            //Since the issue queue essentially contains only N instructions, we push only that amount.
            
            if (issue.size() >= S || (*it)->state == IF) break;
            RenameOps(*it);
            
            (*it)->state = IS;
            (*it)->IS_cycle = cycle;
            
            issue.push_back(*it);
            it = dispatch.erase(it);  
            
        }
    }
    int i = 0;
    for (instruction *ins : dispatch) {
        if (i >= N) break;
        if (ins->state == IF) {
            ins->state = ID;
            ins->ID_cycle = cycle;
            i++;
        }
    }
}

// Where the actual renaming takes place.
// If any one of the instruction's register is marked as -1, it doesn't have that register, and so there's nothing to rename.
void RenameOps(instruction *dispatched) {
    if (dispatched->dest_o != "-1") {
        RF[stoi(dispatched->dest_o)][0] = dispatched->tag;    // Here we assign a tag to the register in the RF.
        dispatched->dest = to_string(RF[stoi(dispatched->dest_o)][0]);
    }
    if (dispatched->src1_o != "-1") {
        dispatched->src1 = to_string(RF[stoi(dispatched->src1_o)][0]);
    }
    if (dispatched->src2_o != "-1") {
        dispatched->src2 = to_string(RF[stoi(dispatched->src2_o)][0]);
    }
}

void FetchtoDispatch() {
    int i = 0;
    if (fetch.empty()) return;
    for (auto it = fetch.begin(); it != fetch.end();) {
        if (dispatch.size() >= N) break;
        
        (*it)->state = ID;
        (*it)->ID_cycle = cycle;
        dispatch.push_back(*it); 
        it = fetch.erase(it); 
        i++;
        
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
    while (dispatch.size() < 2*N && count < N && fakeROB.size() < 1024) {
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

        
        for (auto it = fakeROB.rbegin(); it != fakeROB.rend(); it++) {
            if ((*it)->dest_o == src1 && (*it)->state != WB && (*it)->dest_o != "-1" && (*it)->tag < tag_counter) {
                ins->depend_1 = (*it);
                ins->src1_flag = 0;
                break;
            }
        }
        for (auto it = fakeROB.rbegin(); it != fakeROB.rend(); it++) {
            if ((*it)->dest_o == src2 && (*it)->state != WB && (*it)->dest_o != "-1" && (*it)->tag < tag_counter) {
                ins->depend_2 = (*it);
                ins->src2_flag = 0;
                break;
            }
        }
        
            
        ins->IF_cycle = cycle;
        dispatch.push_back(ins);    // Add to dispatch queue.
        fakeROB.push_back(ins);     // Add to fakeROB
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
    //if (cycle == 41) return true;
    if (fakeROB.size() != 0) return false;
    if (execute.size() != 0) return false;
    if (issue.size() != 0) return false;
    if (dispatch.size() != 0) return false;
    if (fetch.size() != 0) return false;
    if (file.is_open()) return false;
    
    return true;
}

/* Some handy debug code
cout << "<name of function this code is in>:" << endl;
cout << (*it)->address << " " << (*it)->dest_o << " " << (*it)->src1_o <<  " " << (*it)-> src2_o << " " << (*it)->state << endl;
cout << RF[stoi((*it)->dest_o)][1] << " " << RF[stoi((*it)->src1_o)][1] << " " << RF[stoi((*it)->src2_o)][1] <<  " " << endl;
cout << endl;
*/