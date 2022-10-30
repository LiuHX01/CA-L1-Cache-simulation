#include "world.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>


// 构造函数
Cache::Cache(int bsize, int size, int assoc, int repolicy, int wpolicy) :
    L1_BLOCKSIZE(bsize), L1_SIZE(size), L1_ASSOC(assoc), L1_REPLACEMENT_POLICY(repolicy), L1_WRITE_POLICY(wpolicy) {
    // 计算块内地址位数
    L1_BLOCKSIZE_BIT_COUNT = calcBitCount(L1_BLOCKSIZE);
    // 计算块数组数
    L1_BLOCK_COUNT = L1_SIZE / L1_BLOCKSIZE;
    L1_GROUP_COUNT = L1_BLOCK_COUNT / L1_ASSOC;
    // 计算组号位数
    L1_GROUP_BIT_COUNT = calcBitCount(L1_GROUP_COUNT);
    // 计算tag位数
    L1_TAG_BIT_COUNT = 32 - L1_BLOCKSIZE_BIT_COUNT - L1_GROUP_BIT_COUNT;

    // cache及其状态初始化
    // memset(L1_set, 0, sizeof(L1_set));
    // memset(L1_state, 0, sizeof(L1_state));
    L1_set = new unsigned int*[L1_GROUP_COUNT + 1];
    for (unsigned int i = 0; i < L1_GROUP_COUNT + 1; i++) {
        L1_set[i] = new unsigned int[L1_ASSOC + 1];
        for (unsigned int j = 0; j < L1_ASSOC + 1; j++)
            L1_set[i][j] = 0;
    }
    L1_state = new unsigned int**[L1_GROUP_COUNT + 1];
    for (unsigned int i = 0; i < L1_GROUP_COUNT + 1; i++) {
        L1_state[i] = new unsigned int*[L1_ASSOC + 1];
        for (unsigned int j = 0; j < L1_ASSOC + 1; j++) {
            L1_state[i][j] = new unsigned int[4];
            for (unsigned int k = 0; k < 4; k++)
                L1_state[i][j][k] = 0;
        }
    }


    // 统计量
    r_count = 0.0;
    r_miss_count = 0.0;
    w_count = 0.0;
    w_miss_count = 0.0;
    wb_count = 0.0;
}


// 析构函数 主要是释放动态数组
Cache::~Cache() {
    for (unsigned int i = 0; i < L1_GROUP_COUNT + 1; i++)
        delete[] L1_set[i];
    delete[] L1_set;

    for (unsigned int i = 0; i < L1_GROUP_COUNT + 1; i++) {
        for (unsigned int j = 0; j < L1_ASSOC + 1; j++)
            delete[] L1_state[i][j];
        delete[] L1_state[i];
    }
    delete[] L1_state;
}


// 读
void Cache::read(unsigned int addr) {
    r_count++;
    unsigned int tag = getTag(addr);
    unsigned int index = getIndex(addr);

    std::pair<bool, unsigned int> miss_hit = missOrHit(index, tag);
    bool have_replace = false;

    // hit
    if (miss_hit.first == true) {
        L1_set[index][miss_hit.second] = tag;
        updateCounter(miss_hit.first, index, miss_hit.second, have_replace, 0);
    }
    // miss
    else {
        r_miss_count++;

        // 没有空位 寻找替换
        if (miss_hit.second == L1_ASSOC) {
            have_replace = true;
            miss_hit.second = selectReplaced(index);
        }

        // 更新内容 设置valid dirty
        L1_set[index][miss_hit.second] = tag;
        L1_state[index][miss_hit.second][VALID_INDEX] = VALID;
        if (L1_state[index][miss_hit.second][DIRTY_INDEX] == DIRTY)
            wb_count++;
        L1_state[index][miss_hit.second][DIRTY_INDEX] = NO_DIRTY;

        updateCounter(miss_hit.first, index, miss_hit.second, have_replace, 0);
    }
    return;
}


// 写
void Cache::write(unsigned int addr) {
    w_count++;
    unsigned int tag = getTag(addr);
    unsigned int index = getIndex(addr);

    std::pair<bool, unsigned int> miss_hit = missOrHit(index, tag);
    bool have_replace = false;

    // hit
    if (miss_hit.first == true) {
        L1_set[index][miss_hit.second] = tag;
        updateCounter(miss_hit.first, index, miss_hit.second, have_replace, 1);

        if (L1_WRITE_POLICY == WBWA)
            L1_state[index][miss_hit.second][DIRTY_INDEX] = DIRTY;
    }
    // miss 若是WTNA Cache不分配
    else {
        w_miss_count++;
        if (L1_WRITE_POLICY == WBWA) {
            // 没有空位 寻找替换
            if (miss_hit.second == L1_ASSOC) {
                have_replace = true;
                miss_hit.second = selectReplaced(index);
            }

            // 更新内容 设置valid dirty
            L1_set[index][miss_hit.second] = tag;
            L1_state[index][miss_hit.second][VALID_INDEX] = VALID;
            if (L1_state[index][miss_hit.second][DIRTY_INDEX] == DIRTY)
                wb_count++;
            L1_state[index][miss_hit.second][DIRTY_INDEX] = DIRTY;

            updateCounter(miss_hit.first, index, miss_hit.second, have_replace, 1);
        }
    }
    return;
}


// 计算某十进制位数
unsigned int Cache::calcBitCount(unsigned int x) {
    unsigned int ret = 0;
    while (x != 1) {
        ret++;
        x >>= 1;
    }
    return ret;
}


// 得到组号
unsigned int Cache::getIndex(unsigned int addr) {
    unsigned int ret = 0;
    for (unsigned int i = 0; i < L1_BLOCKSIZE_BIT_COUNT; i++) {
        addr >>= 1;
    }
    unsigned int w = 1;
    for (unsigned int i = 0; i < L1_GROUP_BIT_COUNT; i++) {
        ret += w * (addr & 1);
        w *= 2;
        addr >>= 1;
    }
    return ret;
}


// 得到tag
unsigned int Cache::getTag(unsigned int addr) {
    unsigned int ret = 0;
    for (unsigned int i = 0; i < L1_BLOCKSIZE_BIT_COUNT + L1_GROUP_BIT_COUNT; i++) {
        addr >>= 1;
    }
    unsigned int w = 1;
    for (unsigned int i = 0; i < L1_TAG_BIT_COUNT; i++) {
        ret += w * (addr & 1);
        w *= 2;
        addr >>= 1;
    }
    return ret;
}


// true:hit+位置  false:miss+优先空位，无空位设置为L1_ASSOC
std::pair<bool, unsigned int> Cache::missOrHit(unsigned int index, unsigned int tag) {
    std::pair<bool, unsigned int> ret(false, L1_ASSOC);
    // 对比该组每个块
    for (unsigned int i = 0; i < L1_ASSOC; i++) {
        // 若空 记录第一个空位
        if (L1_state[index][i][VALID_INDEX] == INVALID) {
            if (i < ret.second) {
                ret.second = i;
            }
        }
        // 不空且命中，直接返回位置
        else if (L1_state[index][i][VALID_INDEX] == VALID && L1_set[index][i] == tag) {
            ret.first = true;
            ret.second = i;
            return ret;
        }
    }
    // 此时miss
    return ret;
}


// 执行此函数说明该组已经满了 找到最大/小Count
unsigned int Cache::selectReplaced(unsigned int index) {
    unsigned int ret = 0;
    if (L1_REPLACEMENT_POLICY == LRU) {
        unsigned int count = 0;
        for (unsigned int i = 0; i < L1_ASSOC; i++) {
            if (L1_state[index][i][COUNT_BLOCK_INDEX] > count) {
                count = L1_state[index][i][COUNT_BLOCK_INDEX];
                ret = i;
            }
        }
    }
    else {
        unsigned int count = 0xffffffffu;
        for (unsigned int i = 0; i < L1_ASSOC; i++) {
            if (L1_state[index][i][COUNT_BLOCK_INDEX] < count) {
                count = L1_state[index][i][COUNT_BLOCK_INDEX];
                ret = i;
            }
        }
    }
    return ret;
}


// 更新计数器
void Cache::updateCounter(bool is_hit, unsigned int index, unsigned int nth_block, bool have_replace, int wr_type) {
    if (is_hit) {
        // 设置计数器 自己清零 比他小的+1
        if (L1_REPLACEMENT_POLICY == LRU) {
            for (unsigned int i = 0; i < L1_ASSOC; i++) {
                if (L1_state[index][i][VALID_INDEX] == VALID && L1_state[index][i][COUNT_BLOCK_INDEX] < L1_state[index][nth_block][COUNT_BLOCK_INDEX]) {
                    L1_state[index][i][COUNT_BLOCK_INDEX]++;
                }
            }
            L1_state[index][nth_block][COUNT_BLOCK_INDEX] = 0;
        }
        else {
            // 当一个块被引用时 其计数器自增1
            L1_state[index][nth_block][COUNT_BLOCK_INDEX]++;
        }
    }
    else if (wr_type == 0 || (wr_type == 1 && L1_WRITE_POLICY == WBWA)) {
        if (L1_REPLACEMENT_POLICY == LRU) {
            for (unsigned int i = 0; i < L1_ASSOC; i++) {
                if (i != nth_block && L1_state[index][i][VALID_INDEX] == VALID) {
                    L1_state[index][i][COUNT_BLOCK_INDEX]++;
                }
            }
            L1_state[index][nth_block][COUNT_BLOCK_INDEX] = 0;
        }
        else {
            unsigned int tmp = L1_state[index][nth_block][COUNT_BLOCK_INDEX];
            // 一个块调入时 其引用次数被初始化为COUNT_SET+1
            // 如果有替换 该组COUNT_SET被设置为被替换块的COUNT_BLOCK
            if (have_replace) {
                for (unsigned int i = 0; i < L1_ASSOC; i++) {
                    L1_state[index][i][COUNT_SET_INDEX] = tmp;
                }
            }
            L1_state[index][nth_block][COUNT_BLOCK_INDEX] = L1_state[index][nth_block][COUNT_SET_INDEX] + 1;
        }
    }
    return;
}


// 最后输出用10-16进制转换
string Cache::dec2Hex(unsigned int x) {
    string s;
    while (x) {
        int remainder = x % 16;
        x /= 16;
        if (remainder < 10) {
            s += remainder + '0';
        }
        else {
            s += remainder - 10 + 'a';
        }
    }
    reverse(s.begin(), s.end());
    return s;
}


// 以下全是输出
void Cache::printCache() {
    for (unsigned int i_set = 0; i_set < L1_GROUP_COUNT; i_set++) {
        printSingleSet(i_set);
    }
}


void Cache::printSingleSet(unsigned int index) {
    cout << "set";
    cout << setiosflags(ios::right) << setw(4) << index << ":";
    for (unsigned int i = 0; i < L1_ASSOC; i++) {
        if (L1_state[index][i][VALID_INDEX] == INVALID) {
            cout << "-\t";
        }
        else {
            cout << setiosflags(ios::right) << setw(8) << dec2Hex(L1_set[index][i]);
            if (L1_state[index][i][DIRTY_INDEX] == DIRTY) {
                cout << " D";
            }
            else {
                if (i != L1_ASSOC - 1) {
                    cout << "  ";
                }
            }
        }
    }
    cout << endl;
}


void Cache::printHead() {
    cout << "  ===== Simulator configuration =====" << endl;
    cout << "  L1_BLOCKSIZE:";
    cout << setiosflags(ios::right) << setw(22) << L1_BLOCKSIZE << endl;
    cout << "  L1_SIZE:";
    cout << setiosflags(ios::right) << setw(27) << L1_SIZE << endl;
    cout << "  L1_ASSOC:";
    cout << setiosflags(ios::right) << setw(26) << L1_ASSOC << endl;
    cout << "  L1_REPLACEMENT_POLICY:";
    cout << setiosflags(ios::right) << setw(13) << L1_REPLACEMENT_POLICY << endl;
    cout << "  L1_WRITE_POLICY:";
    cout << setiosflags(ios::right) << setw(19) << L1_WRITE_POLICY << endl;
}


void Cache::printResult() {
    double miss_rate = (r_miss_count + w_miss_count) / (r_count + w_count);
    unsigned int traffic;
    if (L1_WRITE_POLICY == WBWA) {
        traffic = r_miss_count + w_miss_count + wb_count;
    }
    else {
        traffic = r_miss_count + w_count;
    }

    cout << "  ====== Simulation results (raw) ======" << endl;
    cout << "  a. number of L1 reads:";
    cout << setiosflags(ios::right) << setw(16) << int(r_count) << endl;
    cout << "  b. number of L1 read misses:";
    cout << setiosflags(ios::right) << setw(10) << int(r_miss_count) << endl;
    cout << "  c. number of L1 writes:";
    cout << setiosflags(ios::right) << setw(15) << int(w_count) << endl;
    cout << "  d. number of L1 write misses:";
    cout << setiosflags(ios::right) << setw(9) << int(w_miss_count) << endl;
    cout << "  e. L1 miss rate:";
    cout << setiosflags(ios::right) << setw(22) << fixed << setprecision(4) << miss_rate << endl;
    cout << "  f. number of writebacks from L1:";
    cout << setiosflags(ios::right) << setw(6) << int(wb_count) << endl;
    cout << "  g. total memory traffic:";
    cout << setiosflags(ios::right) << setw(14) << traffic << endl;
}


void Cache::printPResult() {
    double HT1 = 0.25 + 2.5 * (L1_SIZE / 512.0 / 1024.0) + 0.025 * (L1_BLOCKSIZE / 16.0) + 0.025 * L1_ASSOC;
    double MP1 = 20.0 + 0.5 * (L1_BLOCKSIZE / 16.0);
    double miss_rate = (r_miss_count + w_miss_count) / (r_count + w_count);
    double AAT = HT1 + (MP1 * miss_rate);

    cout << "  ==== Simulation results (performance) ====" << endl;
    cout << "  1. average access time:";
    cout << setiosflags(ios::right) << setw(15) << fixed << setprecision(4) << AAT;
    cout << " ns";
}