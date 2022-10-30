#include <string>
#include <utility>

using namespace std;

class Cache {
  private:
    const unsigned int L1_BLOCKSIZE;
    const unsigned int L1_SIZE;
    const unsigned int L1_ASSOC;
    const unsigned int L1_REPLACEMENT_POLICY;
    const unsigned int L1_WRITE_POLICY;

    unsigned int L1_BLOCK_COUNT;
    unsigned int L1_GROUP_COUNT;

    unsigned int L1_TAG_BIT_COUNT;        // 高
    unsigned int L1_GROUP_BIT_COUNT;      // 中
    unsigned int L1_BLOCKSIZE_BIT_COUNT;  // 低

    // unsigned int L1_set[1024][8];
    // unsigned int L1_state[1024][8][4];
    unsigned int** L1_set;
    unsigned int*** L1_state;

    enum {
        VALID_INDEX = 0,
        DIRTY_INDEX = 1,
        COUNT_BLOCK_INDEX = 2,
        COUNT_SET_INDEX = 3,

        VALID = 1,
        INVALID = 0,
        NO_DIRTY = 0,
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

    unsigned int calcBitCount(unsigned int x);
    unsigned int getIndex(unsigned int addr);
    unsigned int getTag(unsigned int addr);
    pair<bool, unsigned int> missOrHit(unsigned int index, unsigned int tag);
    unsigned int selectReplaced(unsigned int index);
    string dec2Hex(unsigned int x);
    void updateCounter(bool is_hit, unsigned int index, unsigned int nth_block, bool have_replace, int wr_type);


  public:
    Cache(int bsize, int size, int assoc, int repolicy, int wpolicy);  // 构造函数
    ~Cache();

    void read(unsigned int addr);
    void write(unsigned int addr);

    void printCache();
    void printSingleSet(unsigned int index);
    void printHead();
    void printResult();
    void printPResult();
};