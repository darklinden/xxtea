//
//  main.cpp
//  xxtea
//
//  Created by whoami on 12/6/17.
//  Copyright © 2017年 darklinden. All rights reserved.
//

#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <unistd.h>
#include "xxtea.h"
#include "CCData.h"

// trim from start
std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

std::string key_padding(const std::string &inkey)
{
    std::string key = inkey;
    auto n = key.length();
    if (n > 16) {
        key = key.substr(0, 16);
    }
    else if (n < 16) {
        std::stringstream stream;
        stream << std::hex << (16 - n);
        std::string pad(stream.str());
        
        for (int i = 0; i < (16 - n); i++) {
            key += pad;
        }
    }
    return key;
}

Data encrypt(const Data &data, const std::string &inkey)
{
    size_t outLen = 0;
    auto key = key_padding(inkey);
    auto outData = xxtea_encrypt((const void *)data.getBytes(), (size_t)data.getSize(), (const void *)key.c_str(), (size_t *)&outLen);
    Data ret;
    ret.copy((const unsigned char*)outData, outLen);
    return ret;
}

Data decrypt(const Data &data, const std::string &inkey)
{
    size_t outLen = 0;
    auto key = key_padding(inkey);
    auto outData = xxtea_decrypt((const void *)data.getBytes(), (size_t)data.getSize(), (const void *)key.c_str(), (size_t *)&outLen);
    Data ret;
    ret.copy((const unsigned char*)outData, outLen);
    return ret;
}

std::string absolut_path(const std::string &inpath)
{
    std::string path = inpath;
    path = trim(path);
    
    if (inpath.substr(0, 1) == "/") {
        // pass
    }
    else {
        char temp[1024];
        auto cwd = getcwd(temp, 1024) ? std::string( temp ) : std::string("");
        cwd = trim(cwd);
        if (cwd.substr(cwd.length() - 1, 1) == "/") {
            path = cwd + path;
        }
        else {
            path = cwd + "/" + path;
        }
    }
    return path;
}

Data data_from_file(const std::string &inpath)
{
    auto path = absolut_path(inpath);
    
    FILE *pFile = fopen(path.c_str(), "rb");
    
    if (!pFile) {
        printf("str_from_file %s failed.\n", path.c_str());
        return Data::Null;
    }
    
    // obtain file size:
    fseek (pFile , 0 , SEEK_END);
    auto lSize = ftell (pFile);
    rewind (pFile);
    
    // allocate memory to contain the whole file:
    auto buffer = (char*) malloc (sizeof(char)*lSize);
    
    // copy the file into the buffer:
    fread (buffer, 1, lSize, pFile);
    
    if (lSize <= 0) {
        printf("str_from_file %s failed, length = 0.\n", path.c_str());
    }
    
    Data ret;
    ret.copy((const unsigned char*)buffer, lSize);
    
    // terminate
    fclose (pFile);
    free (buffer);

    return ret;
}

void data_to_file(const std::string &inpath, const Data &content)
{
    auto path = absolut_path(inpath);
    
    FILE *pFile = fopen(path.c_str(), "wb");
    
    if (!pFile) {
        return;
    }
    
    auto result = fwrite(content.getBytes(), sizeof(char), content.getSize(), pFile);
    if (result <= 0) {
        printf("error: fwrite to %s failed.\n", path.c_str());
    }
    
    fclose (pFile);
}

int main(int argc, const char * argv[]) {
    
// args
    auto arg_len = argc;
    
    std::map<std::string, std::string> operations;
    
    int idx = 1;
    while (idx < arg_len){
        std::string cmd_s = argv[idx];
        if (cmd_s.c_str()[0] == '-') {
            std::string value_s = "";
            if (idx + 1 < arg_len) {
                value_s = argv[idx + 1];
            }
            cmd_s = cmd_s.substr(1);
            if (value_s.c_str()[0] == '-') {
                operations[cmd_s] = "1";
                idx++;
            }
            else {
                operations[cmd_s] = value_s;
                idx += 2;
            }
        }
        else {
            idx++;
        }
    }
    
    bool has_opt = false;
    if (operations.count("e")) {
        // encrypt
        if (operations.count("p")) {
            if (operations.count("s")) {
                // str
                has_opt = true;
                
                std::string content = operations["s"];
                std::string key = operations["p"];
                Data inData;
                inData.copy((const unsigned char*)content.c_str(), content.length());
                auto data = encrypt(inData, key);
                printf("[%s]\n", data.getBytes());
            }
            else if (operations.count("f")) {
                // file
                has_opt = true;
                
                std::string path = operations["f"];
                Data inData = data_from_file(path);
                std::string key = operations["p"];
                Data ret = encrypt(inData, key);
                data_to_file(path, ret);
            }
        }
    }
    else if (operations.count("d")) {
        // decrypt
        if (operations.count("p")) {
            if (operations.count("s")) {
                // str
                has_opt = true;
                
                std::string content = operations["s"];
                std::string key = operations["p"];
                Data inData;
                inData.copy((const unsigned char*)content.c_str(), content.length());
                auto data = decrypt(inData, key);
                printf("[%s]\n", data.getBytes());
            }
            else if (operations.count("f")) {
                // file
                has_opt = true;
                
                std::string path = operations["f"];
                Data inData = data_from_file(path);
                std::string key = operations["p"];
                Data ret = decrypt(inData, key);
                data_to_file(path, ret);
            }
        }
    }
    
    if (!has_opt) {
        printf("using [xxtea] [-e encrypt / -d decrypt] [-p password] [-s string] [-f filepath] to do encryption or decryptions.\n");
    }
    else {
        printf("Done\n");
    }
    
    return 0;
}
