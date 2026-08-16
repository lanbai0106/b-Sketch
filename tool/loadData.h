#ifndef LOADDATA1_H
#define LOADDATA1_H

#include <fstream>
#include <queue>
#include <sstream>
#include "BOBHash32.h"
#include <filesystem>
// namespace fs = std::filesystem;
const int MAX_INSERT_PACKAGE = 1.e7;
int packetnum;

struct Object {
    vector<vector<uint32_t>> pac_list;
    vector<uint64_t> timestamp;
};


class DataLoader {
    private:
        Object obj;
        string filePath;
    public:
        DataLoader(string filePath): filePath(filePath){
            if(filePath == "ca19") {
                filePath = "../data/CAIDA19.txt";
            }else if(filePath == "ca16") {
                filePath = "../data/CAIDA16.txt";
            }else if(filePath == "so") {
                filePath = "../data/stackoverflow.txt";
            }else if(filePath == "wd") {
                filePath = "../data/webdocs.txt";
            }
            clear();
            load_data(filePath);
            cout << "Loading dataset" << endl;
        }
        void clear() {
            obj.pac_list.clear();
            obj.timestamp.clear();
        }

        void load_data(string filePath) {
            std::ifstream pf(filePath);
            if (!pf) {
                cerr << filePath << " not found." << endl;
                exit(-1);
            }

            int ret = 0;
            string line;

            while (getline(pf, line)) {
                std::istringstream stream(line);
                std::string str1, str2;
                stream >> str1 >> str2;
                bool f1 = std::any_of(str1.begin(), str1.end(), [](char ch) {
                    return std::isalpha(static_cast<unsigned char>(ch));
                }),f2 = std::any_of(str2.begin(), str2.end(), [](char ch) {
                    return std::isalpha(static_cast<unsigned char>(ch));
                });
                if(f2) {
                    continue;
                }
                char *s2 = &str2[0];
                uint32_t x = convertIPv4ToUint32(s2);
                // uint32_t x = stoul(s1), y = stoul(s2);
                if(x != 0){
                    vector<uint32_t> tmp;
                    for (int i = 1; i <= 32; i++) {
                        tmp.push_back(getPrefix(x,i));
                    }
                    obj.pac_list.push_back(tmp);
                    obj.timestamp.push_back(ret);
                    ret++;
                    if (ret == MAX_INSERT_PACKAGE)
                        break;

                }
                packetnum = ret;
            }
            pf.close();

        }
    Object get_object() {return obj;}
};
#endif //LOADDATA_H
