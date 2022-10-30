#include <string>
#include <utility>

using namespace std;

class Cache {
  private:
    unsigned int L1_BLOCKSIZE;
    unsigned int L1_SIZE;
    unsigned int L1_ASSOC;
    unsigned int L1_REPLACEMENT_POLICY;
    unsigned int L1_WRITE_POLICY;

    unsigned int L1_BLOCK_COUNT;
    unsigned int L1_GROUP_COUNT;

    unsigned int L1_TAG_BITCOUNT;        // 高
    unsigned int L1_GROUP_BITCOUNT;      // 中
    unsigned int L1_BLOCKSIZE_BITCOUNT;  // 低

    unsigned int L1_set[1024][8];
    unsigned int L1_state[1024][8][4];

    enum {
        VALID_INDEX = 0,  // 0为空 1被占用
        DIRTY_INDEX = 1,  // 0不脏 1脏
        COUNT_BLOCK_INDEX = 2,
        COUNT_SET_INDEX = 3,

        VALID = 1,
        INVALID = 0,
        NODIRTY = 0,
        DIRTY = 1,

        LRU = 0,
        LFU = 1,

        WBWA = 0,
        WTNA = 1
    };

    double r_count;
    double r_miss_count;
    double w_count;
    double w_miss_count;
    double wb_count;

    bool DEBUG;

  public:
    Cache(int bsize, int size, int assoc, int repolicy, int wpolicy);  // 构造函数
    void read(unsigned int addr);
    void write(unsigned int addr);
    unsigned int getIndex(unsigned int addr);
    unsigned int getTag(unsigned int addr);
    pair<bool, unsigned int> missOrHit(unsigned int index, unsigned int tag);
    unsigned int selectReplaced(unsigned int index);
    string dec2Hex(unsigned int x);
    void printCache();
    void printSingleSet(unsigned int index);
    void printHead();
    void printResult();
    void printPResult();
};