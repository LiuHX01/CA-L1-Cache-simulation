#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world.h"
#include <iostream>
#include <utility>
#include <algorithm>


// 构造函数
Cache::Cache(int bsize, int size, int assoc, int repolicy, int wpolicy) {
	VALID = 0;
	DIRTY = 1;
	COUNT_BLOCK = 2;
	COUNT_SET = 3;

	LRU = 0;
	LFU = 1;
	WBWA = 0;
	WTNA = 1;

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


	// 分配Cache空间 状态位空间 初始化
	// L1_set = new unsigned int *[L1_GROUP_COUNT];
	// L1_state = new unsigned short **[L1_GROUP_COUNT];
	// for (int i = 0; i < L1_GROUP_COUNT; i++) {
	// 	L1_set[i] = new unsigned int[L1_ASSOC];
	// 	L1_state[i] = new unsigned short *[L1_ASSOC];
	// 	for (int j = 0; j < 4; j++) {
	// 		L1_state[i][j] = new unsigned short[4];
	// 	}
	// }
	memset(L1_set, 0, sizeof(L1_set));
	memset(L1_state, 0, sizeof(L1_state));


	// 总结
	std::cout << "tag_bit: " << L1_TAG_BITCOUNT << std::endl;
	std::cout << "group_bit: " << L1_GROUP_BITCOUNT << std::endl;
	std::cout << "block_bit: " << L1_BLOCKSIZE_BITCOUNT << std::endl;
	std::cout << "block_count: " << L1_BLOCK_COUNT << std::endl;
	std::cout << "group_count: " << L1_GROUP_COUNT << std::endl;
}


// TODO:修改
void Cache::read(long int addr) {
	// 得到信息
	unsigned int tag = getTag(addr);
	unsigned int index = getIndex(addr);
	

	printSingleSet(index);

	// 判断命中情况
	std::pair<bool, unsigned int>miss_hit = missOrHit(index, tag);
	
	// 命中
	if (miss_hit.first == true) {
		cout << "L1 HIT" << endl;
		// std::cout << "READ--tag: " << tag << "  index:" << index << "  block:" << miss_hit.second << "  replace:" << miss_hit.first << std::endl;

		// 更新内容
		L1_set[index][miss_hit.second] = tag;
		// 设置计数器 自己清零 比他小的+1
		for (int i = 0; i < L1_ASSOC; i++) {
			unsigned short temp = L1_state[index][miss_hit.second][2];
			if (i == miss_hit.second) {
				L1_state[index][miss_hit.second][2] = 0;
			} else {
				if (L1_state[index][miss_hit.second][2] < temp) {
					L1_state[index][miss_hit.second][2]++;
				}
			}
		}
	} else { // 未命中 
		cout << "L1 MISS" << endl;
		if (miss_hit.second == L1_ASSOC) { // 无空位
			miss_hit.second = findMaxCount(index);
		}

		// std::cout << "READ--tag: " << tag << "  index:" << index << "  block:" << miss_hit.second << "  replace:" << miss_hit.first << std::endl;

		// 更新内容 设置valid
		L1_set[index][miss_hit.second] = tag;
		L1_state[index][miss_hit.second][0] = 1;
		// 设置计数器 自己清零 其他都+1
		for (int i = 0; i < L1_ASSOC; i++) {
			if (i == miss_hit.second) {
				L1_state[index][miss_hit.second][2] = 0;
			} else {
				L1_state[index][miss_hit.second][2]++;
			}
		}
		L1_state[index][miss_hit.second][1] = 0;
	}

	printSingleSet(index);

	return;
}





void Cache::write(long int addr) {
	// 得到信息
	unsigned int tag = getTag(addr);
	unsigned int index = getIndex(addr);
	
	printSingleSet(index);

	// 判断命中情况
	std::pair<bool, unsigned int>miss_hit = missOrHit(index, tag);
	
	// 命中
	if (miss_hit.first == true) {
		cout << "L1 HIT" << endl;
		// std::cout << "WRITE--tag: " << tag << "  index:" << index << "  block:" << miss_hit.second << "  replace:" << miss_hit.first << std::endl;

		// 更新内容
		L1_set[index][miss_hit.second] = tag;
		// 设置计数器 自己清零 比他小的+1
		for (int i = 0; i < L1_ASSOC; i++) {
			unsigned short temp = L1_state[index][miss_hit.second][COUNT_BLOCK];
			if (i == miss_hit.second) {
				L1_state[index][miss_hit.second][COUNT_BLOCK] = 0;
			} else {
				if (L1_state[index][miss_hit.second][COUNT_BLOCK] < temp) {
					L1_state[index][miss_hit.second][COUNT_BLOCK]++;
				}
			}
		}
		// 设置脏位
		if (L1_WRITE_POLICY == 0) {
			L1_state[index][miss_hit.second][DIRTY] = 1;
		}
	} else if (miss_hit.first == false && L1_WRITE_POLICY == 0) { // 未命中 且不是WTNA
		cout << "L1 MISS" << endl;
		if (miss_hit.second == L1_ASSOC) { // 无空位
			miss_hit.second = findMaxCount(index);
		}

		// std::cout << "WRITE--tag: " << tag << "  index:" << index << "  block:" << miss_hit.second << "  replace:" << miss_hit.first << std::endl;

		// 更新内容 设置valid
		L1_set[index][miss_hit.second] = tag;
		L1_state[index][miss_hit.second][VALID] = 1;
		// 设置计数器 自己清零 其他都+1
		for (int i = 0; i < L1_ASSOC; i++) {
			if (i == miss_hit.second) {
				L1_state[index][miss_hit.second][COUNT_BLOCK] = 0;
			} else {
				L1_state[index][miss_hit.second][COUNT_BLOCK]++;
			}
		}
		// 设置脏位
		L1_state[index][miss_hit.second][DIRTY] = 1;
	}

	printSingleSet(index);

	return;
}


// 得到组号
unsigned int Cache::getIndex(long int addr) {
	unsigned int ret = 0;
	for (int i = 0; i < L1_BLOCKSIZE_BITCOUNT; i++) {
		addr >>= 1;
	}
	int w = 1;
	for (int i = 0; i < L1_GROUP_BITCOUNT; i++) {
		ret += w * (addr & 1);
		w *= 2;
		addr >>= 1;
	}
	cout << "index " << ret << ")" << endl;
	return ret;
}


// 得到tag(内容)
unsigned int Cache::getTag(long int addr) {
	unsigned int ret = 0;
	for (int i = 0; i < L1_BLOCKSIZE_BITCOUNT + L1_GROUP_BITCOUNT; i++) {
		addr >>= 1;
	}
	int w = 1;
	for (int i = 0; i < L1_TAG_BITCOUNT; i++) {
		ret += w * (addr & 1);
		w *= 2;
		addr >>= 1;
	}
	cout << "(tag " << dec2Hex(ret) << ", ";
	return ret;
}


// true:hit+位置  false:miss+优先空位，无空位设置为L1_ASSOC
std::pair<bool, unsigned int> Cache::missOrHit(unsigned int index, unsigned int tag) {
	std::pair<bool, unsigned int>ret(false, L1_ASSOC);
	// 对比该组每个块
	for (int i = 0; i < L1_ASSOC; i++) {
		if (L1_state[index][i][0] == 0) { // 首先valid 记录第一个空位
			if (i < ret.second) {
				ret.second = i;
			}
		} else if (L1_set[index][i] == tag) { // 不空且命中，直接返回位置
			ret.first = true;
			ret.second = i;
			return ret;
		} else { // 不空且未命中，跳过

		}
	}
	return ret;
}


// 执行此函数说明该组已经满了 找到最大Count
unsigned int Cache::findMaxCount(unsigned int index) {
	unsigned int ret = 0;
	unsigned short count = 0;
	for (int i = 0; i < L1_ASSOC; i++) {
		if (L1_state[index][i][2] > count) {
			count = L1_state[index][i][2];
			ret = i;
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
		} else {
			s += remainder - 10 + 'A';
		}
	}
	reverse(s.begin(), s.end());
	return s;
}


void Cache::printCache() {
	for (int setNum = 0; setNum < L1_GROUP_COUNT; setNum++) {
		printSingleSet(setNum);
	}
}


void Cache::printSingleSet(unsigned int index) {
	cout << "Current set " << index << ":\t";
	for (int i = 0; i < L1_ASSOC; i++) {
		if (L1_state[index][i][VALID] == 0) {
			cout << "-\t";
		} else {
			cout << dec2Hex(L1_set[index][i]);
			if (L1_state[index][i][DIRTY] == 1) {
				cout << "D";
			}
			cout << "\t";
		}
	}
	cout << endl;
}