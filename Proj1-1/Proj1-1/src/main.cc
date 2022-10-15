#include "world.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

char trace_file[15];
char path[30] = "../traces/";

int main(int argc, char* argv[]) {
    Cache MyCache(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
    strcpy(trace_file, argv[6]);

    strcat(path, trace_file);
    strcat(path, ".txt");

    int a = 1;

    ifstream in(path);
    string s;
    while (getline(in, s)) {
        unsigned int addr;
        string addrs = s.substr(2, 8);
        addr = strtol(addrs.c_str(), NULL, 16);

        // cout << "# " << a << " : ";

        if (s[0] == 'w' || s[0] == 'W') {
            // cout << "Write " << MyCache.dec2Hex(addr) << endl;
            // cout << "L1 Write: " << MyCache.dec2Hex(addr);
            MyCache.write(addr);
        }
        else if (s[0] == 'r' || s[0] == 'R') {
            // cout << "Read " << MyCache.dec2Hex(addr) << endl;
            // cout << "L1 Read: " << MyCache.dec2Hex(addr);
            MyCache.read(addr);
        }
        else {
            cout << "Operate ERROR!" << endl;
            break;
        }
        a++;
        // cout << a << endl;
        // if (a == 20) break;
    }

    MyCache.printCache();
}
