#include "world.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
using namespace std;

char trace_file[15];
char path[30] = "../traces/";
char o_file_name[30];

int main(int argc, char* argv[]) {
    Cache MyCache(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
    strcpy(trace_file, argv[6]);

    strcpy(o_file_name, argv[6]);
    strcat(o_file_name, ".txt");

    strcat(path, trace_file);
    strcat(path, ".txt");

    ifstream in(path);
    string s;
    while (getline(in, s)) {
        unsigned int addr;
        string addrs = s.substr(2, 8);
        addr = strtol(addrs.c_str(), NULL, 16);

        if (s[0] == 'w' || s[0] == 'W') {
            MyCache.write(addr);
        }
        else if (s[0] == 'r' || s[0] == 'R') {
            MyCache.read(addr);
        }
        else {
            cout << "Operate ERROR!" << endl;
            break;
        }
    }

    MyCache.printHead();
    cout << "  trace_file:";
    cout << setiosflags(ios::right) << setw(24) << o_file_name << endl;
    cout << "  ===================================" << endl;
    cout << endl;
    cout << "===== L1 contents =====" << endl;
    MyCache.printCache();
    cout << endl;
    MyCache.printResult();
    cout << endl;
    MyCache.printPResult();
}
