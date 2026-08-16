#include <iostream>
#include <cmath>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <string>
#include <cstring>
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <time.h>
#include <random>
#include "tool/inc.h"
#include "tool/loadData.h"
#include "tool/geData.h"
#include "sketch/CM.h"
#include "sketch/CM_sample.h"
#include "sketch/separate.h"
#include "report/report_memory.h"
#include "sketch/separate_sample.h"
#include "sketch/separate_sample_super.h"
#include "sketch/separate_super.h"
using namespace std;

int main()
{
    DataLoader data_loader = DataLoader("ca19"); Object obj = data_loader.get_object();
    vector<vector<uint32_t>> pac_list  = obj.pac_list; vector<uint64_t> timestamp = obj.timestamp;

    DataGenerator dg = DataGenerator("ca19", pac_list); vector<double> b_list = dg.getb_list();
    for (int i = 0; i < packetnum; i++) {
        const vector<uint32_t>& pac = pac_list[i];

        if (pac.size() < 32) {
            throw invalid_argument(
                "Each packet must contain 32 prefix values"
            );
        }

        for (int j = 0; j < 32; j++) {
            mp[j][pac[j]]++;
        }
    }

    cout << endl;

    for (double b : b_list) {
        std::cout << "Ture: b = " << b << ", CC = " << getCC1(b, mp,10000)  << std::endl;
    }
    cout << "------------------------------------------------------------\n";
    CM<32> cm(3, 512); // 64 KB

    for (int i = 0; i < packetnum; i++) {
        cm.insert(pac_list[i], 32);
    }

    std::vector<double> b = cm.query(32);

    double bF2 = b[0];
    double bVariance = b[1];
    double bEntropy = b[2];


    cout << endl;
    for (double b : b) {
        std::cout << "CM: b_ = " << b << ", CC = " << getCC1(b, mp,10000) << std::endl;
    }
    cout << "------------------------------------------------------------\n";

    CM_sample<32> cm_s(3, 256,0.25); // 64 KB

    for (int i = 0; i < packetnum; i++) {
        cm_s.insert(pac_list[i], 32);
    }

    b = cm_s.query(32);

    bF2 = b[0];
    bVariance = b[1];
    bEntropy = b[2];


    cout << endl;
    for (double b : b) {
        std::cout << "CM_sample: b_ = " << b << ", CC = " << getCC1(b, mp,10000) << std::endl;
    }
    cout << "------------------------------------------------------------\n";
    double memory = 3*512*32*32;
    int d = 2;
    int m = 512;
    int m_small = 64;
    int v_max = 4;
    int l = 4;
    int c = ((memory - d*m*32*11)/21 - ( d*m_small*v_max))/(l*55);

    cout << "c = " << c << "total_memory: "<< (d*m*32*11+d*m_small*v_max*21+c*l*55*21)/(1024*8) <<  endl;


    Separate<32> sep(d, m, m_small, c, l);

    for (int i = 0; i < packetnum; i++) {
        sep.insert(pac_list[i], 32);
    }

    b = sep.query(32);

    bF2 = b[0];
    bVariance = b[1];
    bEntropy = b[2];

    cout << endl;

    for (double b_ : b) {
        std::cout
            << "separe: b_ = " << b_
            << ", CC = " << getCC1(b_, mp, 10000)
            << std::endl;
    }
    cout << "------------------------------------------------------------\n";
    Separate_sample<32> sep_sa(d, m, m_small, c, l,0.25);

    for (int i = 0; i < packetnum; i++) {
        sep_sa.insert(pac_list[i], 32);
    }

    b = sep_sa.query(32);

    bF2 = b[0];
    bVariance = b[1];
    bEntropy = b[2];

    cout << endl;

    for (double b_ : b) {
        std::cout
            << "separe_sample: b_ = " << b_
            << ", CC = " << getCC1(b_, mp, 10000)
            << std::endl;
    }

    cout << "------------------------------------------------------------\n";
    c = ((memory - d*m*32*11)/21)/(l*55);
    cout << "c = " << c << "total_memory: "<< (d*m*32*11+d*m_small*v_max*21+c*l*55*21)/(1024*8) <<  endl;
    Separate_super<32> sep_su(d, m, c, l);

    for (int i = 0; i < packetnum; i++) {
        sep_su.insert(pac_list[i], 32);
    }

    b = sep_su.query(32);

    bF2 = b[0];
    bVariance = b[1];
    bEntropy = b[2];

    cout << endl;

    for (double b_ : b) {
        std::cout
            << "separe_su: b_ = " << b_
            << ", CC = " << getCC1(b_, mp, 10000)
            << std::endl;
    }
    cout << "------------------------------------------------------------\n";
    Separate_sample_super<32> sep_sa_su(d, m, c, l,0.25);

    for (int i = 0; i < packetnum; i++) {
        sep_sa_su.insert(pac_list[i], 32);
    }

    b = sep_sa_su.query(32);

    bF2 = b[0];
    bVariance = b[1];
    bEntropy = b[2];

    cout << endl;

    for (double b_ : b) {
        std::cout
            << "separe_sample_super: b_ = " << b_
            << ", CC = " << getCC1(b_, mp, 10000)
            << std::endl;
    }
    return 0;
}
