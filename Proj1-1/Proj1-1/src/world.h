#include <string>
#include <utility>

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

    unsigned int L1_set[1024][8];
    unsigned int L1_state[1024][8][4];

    int VALID;  // 0可用 1占用
    int DIRTY;  // 0不脏 1脏
    int COUNT_BLOCK;
    int COUNT_SET;

    int LRU;
    int LFU;
    int WBWA;
    int WTNA;

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