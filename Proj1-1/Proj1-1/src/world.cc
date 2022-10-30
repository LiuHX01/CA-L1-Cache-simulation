#include "world.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>


// 构造函数
Cache::Cache(int bsize, int size, int assoc, int repolicy, int wpolicy) {
    // 传入参数
    L1_BLOCKSIZE = bsize;
    L1_SIZE = size;
    L1_ASSOC = assoc;
    L1_REPLACEMENT_POLICY = repolicy;
    L1_WRITE_POLICY = wpolicy;


    // 计算位数 tag 组号 块内地址
    L1_BLOCKSIZE_BITCOUNT = 0;
    int temp = L1_BLOCKSIZE;
    while (temp != 1) {
        L1_BLOCKSIZE_BITCOUNT++;
        temp >>= 1;
    }
    L1_BLOCK_COUNT = L1_SIZE / L1_BLOCKSIZE;
    L1_GROUP_COUNT = L1_BLOCK_COUNT / L1_ASSOC;
    L1_GROUP_BITCOUNT = 0;
    temp = L1_GROUP_COUNT;
    while (temp != 1) {
        L1_GROUP_BITCOUNT++;
        temp >>= 1;
    }
    L1_TAG_BITCOUNT = 32 - L1_BLOCKSIZE_BITCOUNT - L1_GROUP_BITCOUNT;


    // 初始化
    memset(L1_set, 0, sizeof(L1_set));
    memset(L1_state, 0, sizeof(L1_state));


    // 统计量
    r_count = 0.0;
    r_miss_count = 0.0;
    w_count = 0.0;
    w_miss_count = 0.0;
    wb_count = 0.0;


    // debug用
    DEBUG = false;
}


void Cache::read(unsigned int addr) {
    r_count++;
    unsigned int tag = getTag(addr);  // 得到信息
    unsigned int index = getIndex(addr);
    if (DEBUG) printSingleSet(index);

    std::pair<bool, unsigned int> miss_hit = missOrHit(index, tag);  // 判断命中情况

    if (miss_hit.first == true) {  // 命中
        if (DEBUG) cout << "L1 HIT" << endl;

        L1_set[index][miss_hit.second] = tag;  // 更新内容

        if (L1_REPLACEMENT_POLICY == LRU) {
            for (unsigned int i = 0; i < L1_ASSOC; i++) {  // 设置计数器 自己清零 其他被占用且比他小的+1
                if (L1_state[index][i][VALID] == 1 && L1_state[index][i][COUNT_BLOCK] < L1_state[index][miss_hit.second][COUNT_BLOCK]) {
                    L1_state[index][i][COUNT_BLOCK]++;
                }
            }
            L1_state[index][miss_hit.second][COUNT_BLOCK] = 0;
        }
        else {
            // 当一个块被引用时 其计数器自增1
            L1_state[index][miss_hit.second][COUNT_BLOCK]++;
        }
    }
    else {  // 未命中
        r_miss_count++;
        if (DEBUG) cout << "L1 MISS" << endl;
        bool have_replace = false;
        if (miss_hit.second == L1_ASSOC) {  // 无空位
            have_replace = true;
            miss_hit.second = selectReplaced(index);
        }

        // 更新内容 设置valid
        L1_set[index][miss_hit.second] = tag;
        L1_state[index][miss_hit.second][VALID] = 1;
        if (L1_state[index][miss_hit.second][DIRTY] == 1) {
            wb_count++;
        }
        L1_state[index][miss_hit.second][DIRTY] = 0;


        if (L1_REPLACEMENT_POLICY == LRU) {
            for (unsigned int i = 0; i < L1_ASSOC; i++) {  // 设置计数器 自己清零 其他被占用的都+1
                if (i != miss_hit.second && L1_state[index][i][VALID] == 1) {
                    L1_state[index][i][COUNT_BLOCK]++;
                }
            }
            L1_state[index][miss_hit.second][COUNT_BLOCK] = 0;
        }
        else {
            unsigned int tmp = L1_state[index][miss_hit.second][COUNT_BLOCK];
            // 一个块调入时 其引用次数被初始化为COUNT_SET+1
            // L1_state[index][miss_hit.second][COUNT_BLOCK] = L1_state[index][miss_hit.second][COUNT_SET] + 1;
            // 如果有替换 该组COUNT_SET被设置为被替换块的COUNT_BLOCK
            if (have_replace) {
                for (unsigned int i = 0; i < L1_ASSOC; i++) {
                    L1_state[index][i][COUNT_SET] = tmp;
                }
            }
            L1_state[index][miss_hit.second][COUNT_BLOCK] = L1_state[index][miss_hit.second][COUNT_SET] + 1;
        }
    }

    return;
}


void Cache::write(unsigned int addr) {
    w_count++;
    unsigned int tag = getTag(addr);  // 得到信息
    unsigned int index = getIndex(addr);
    if (DEBUG) printSingleSet(index);

    std::pair<bool, unsigned int> miss_hit = missOrHit(index, tag);  // 判断命中情况

    if (miss_hit.first == true) {  // 命中
        if (DEBUG) cout << "L1 HIT" << endl;

        L1_set[index][miss_hit.second] = tag;  // 更新内容

        if (L1_REPLACEMENT_POLICY == LRU) {  // 设置计数器 自己清零 比他小的+1
            for (unsigned int i = 0; i < L1_ASSOC; i++) {
                if (L1_state[index][i][VALID] == 1 && L1_state[index][i][COUNT_BLOCK] < L1_state[index][miss_hit.second][COUNT_BLOCK]) {
                    L1_state[index][i][COUNT_BLOCK]++;
                }
            }
            L1_state[index][miss_hit.second][COUNT_BLOCK] = 0;
        }
        else {
            // 当一个块被引用时 其计数器自增1
            L1_state[index][miss_hit.second][COUNT_BLOCK]++;
        }
        if (L1_WRITE_POLICY == WBWA) {  // 设置脏位
            L1_state[index][miss_hit.second][DIRTY] = 1;
        }
    }
    else if (miss_hit.first == false) {  // 未命中 且不是WTNA
        w_miss_count++;
        if (L1_WRITE_POLICY == WBWA) {
            if (DEBUG) cout << "L1 MISS" << endl;

            bool have_replace = false;
            if (miss_hit.second == L1_ASSOC) {  // 无空位
                have_replace = true;
                miss_hit.second = selectReplaced(index);
            }

            // 更新内容 设置valid
            L1_set[index][miss_hit.second] = tag;
            L1_state[index][miss_hit.second][VALID] = 1;

            if (L1_state[index][miss_hit.second][DIRTY] == 1) {
                wb_count++;
            }

            // 设置计数器 自己清零 其他都+1
            if (L1_REPLACEMENT_POLICY == LRU) {
                for (unsigned int i = 0; i < L1_ASSOC; i++) {
                    if (i != miss_hit.second && L1_state[index][i][VALID] == 1) {
                        L1_state[index][i][COUNT_BLOCK]++;
                    }
                    L1_state[index][miss_hit.second][COUNT_BLOCK] = 0;
                }
            }
            else {
                unsigned int tmp = L1_state[index][miss_hit.second][COUNT_BLOCK];
                // 一个块调入时 其引用次数被初始化为COUNT_SET+1
                // L1_state[index][miss_hit.second][COUNT_BLOCK] = L1_state[index][miss_hit.second][COUNT_SET] + 1;
                // 如果有替换 该组COUNT_SET被设置为被替换块的COUNT_BLOCK
                if (have_replace) {
                    for (unsigned int i = 0; i < L1_ASSOC; i++) {
                        L1_state[index][i][COUNT_SET] = tmp;
                    }
                }
                L1_state[index][miss_hit.second][COUNT_BLOCK] = L1_state[index][miss_hit.second][COUNT_SET] + 1;
            }
            // 设置脏位
            L1_state[index][miss_hit.second][DIRTY] = 1;
        }
    }

    if (DEBUG) printSingleSet(index);
    return;
}


// 得到组号
unsigned int Cache::getIndex(unsigned int addr) {
    unsigned int ret = 0;
    for (unsigned int i = 0; i < L1_BLOCKSIZE_BITCOUNT; i++) {
        addr >>= 1;
    }
    int w = 1;
    for (unsigned int i = 0; i < L1_GROUP_BITCOUNT; i++) {
        ret += w * (addr & 1);
        w *= 2;
        addr >>= 1;
    }
    if (DEBUG) cout << "index " << ret << ")" << endl;
    return ret;
}


// 得到tag(内容)
unsigned int Cache::getTag(unsigned int addr) {
    unsigned int ret = 0;
    for (unsigned int i = 0; i < L1_BLOCKSIZE_BITCOUNT + L1_GROUP_BITCOUNT; i++) {
        addr >>= 1;
    }
    int w = 1;
    for (unsigned int i = 0; i < L1_TAG_BITCOUNT; i++) {
        ret += w * (addr & 1);
        w *= 2;
        addr >>= 1;
    }
    if (DEBUG) cout << "(tag " << dec2Hex(ret) << ", ";
    return ret;
}


// true:hit+位置  false:miss+优先空位，无空位设置为L1_ASSOC
std::pair<bool, unsigned int> Cache::missOrHit(unsigned int index, unsigned int tag) {
    std::pair<bool, unsigned int> ret(false, L1_ASSOC);
    // 对比该组每个块
    for (unsigned int i = 0; i < L1_ASSOC; i++) {
        if (L1_state[index][i][0] == 0) {  // 首先valid 记录第一个空位
            if (i < ret.second) {
                ret.second = i;
            }
        }
        else if (L1_set[index][i] == tag) {  // 不空且命中，直接返回位置
            ret.first = true;
            ret.second = i;
            return ret;
        }
        else {  // 不空且未命中，跳过
        }
    }
    return ret;
}


// 执行此函数说明该组已经满了 找到最大/小Count
unsigned int Cache::selectReplaced(unsigned int index) {
    unsigned int ret = 0;
    if (L1_REPLACEMENT_POLICY == LRU) {
        unsigned int count = 0;
        for (unsigned int i = 0; i < L1_ASSOC; i++) {
            if (L1_state[index][i][COUNT_BLOCK] > count) {
                count = L1_state[index][i][COUNT_BLOCK];
                ret = i;
            }
        }
    }
    else {
        unsigned int count = 0xffffffffu;
        for (unsigned int i = 0; i < L1_ASSOC; i++) {
            if (L1_state[index][i][COUNT_BLOCK] < count) {
                count = L1_state[index][i][COUNT_BLOCK];
                ret = i;
            }
        }
    }
    return ret;
}


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


void Cache::printCache() {
    for (unsigned int setNum = 0; setNum < L1_GROUP_COUNT; setNum++) {
        printSingleSet(setNum);
    }
}


void Cache::printSingleSet(unsigned int index) {
    cout << "set";
    cout << setiosflags(ios::right) << setw(4) << index << ":";
    for (unsigned int i = 0; i < L1_ASSOC; i++) {
        if (L1_state[index][i][VALID] == 0) {
            cout << "-\t";
        }
        else {
            cout << setiosflags(ios::right) << setw(8) << dec2Hex(L1_set[index][i]);
            if (L1_state[index][i][DIRTY] == 1) {
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