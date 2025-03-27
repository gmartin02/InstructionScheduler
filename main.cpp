using namespace std;
#include <string>;
instruction dispatch[1024];
instruction issue[1024];
instruction execute[1024];

enum state_list {
    IF,
    ID,
    IS,
    EX,
    WB
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