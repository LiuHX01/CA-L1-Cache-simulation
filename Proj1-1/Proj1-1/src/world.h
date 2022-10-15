#include <utility>
#include <string>

using namespace std;

class Cache {
private:
    int L1_BLOCKSIZE;
    int L1_SIZE;
    int L1_ASSOC;
    int L1_REPLACEMENT_POLICY;
    int L1_WRITE_POLICY;
    
    int L1_BLOCK_COUNT;
    int L1_GROUP_COUNT;
    
    int L1_TAG_BITCOUNT;        // 高
    int L1_GROUP_BITCOUNT;      // 中
    int L1_BLOCKSIZE_BITCOUNT;  // 低

    // unsigned int **L1_set;
    unsigned int L1_set[1024][8];
    // unsigned short ***L1_state;
    unsigned short L1_state[1024][8][4];

    int VALID;
    int DIRTY;
    int COUNT_BLOCK;
    int COUNT_SET;
    // 0: valid, 1: dirty, 2: count_block, 3: count_set
    // 0可用       0不脏

    int LRU;
    int LFU;
    int WBWA;
    int WTNA;

public:
    int r;
    int r_miss;
    int w;
    int w_miss;

    Cache(int bsize, int size, int assoc, int repolicy, int wpolicy); // 构造函数

    void read(long int addr);
    void write(long int addr);
    unsigned int getIndex(long int addr);
    unsigned int getTag(long int addr);
    pair<bool, unsigned int> missOrHit(unsigned int index, unsigned int tag);
    unsigned int findMaxCount(unsigned int index);
    string dec2Hex(unsigned int x);
    void printCache();
    void printSingleSet(unsigned int index);
};