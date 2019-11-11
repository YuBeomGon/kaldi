
// online2bin/online2-web-nnet3-decode-testmode.cc
// this is for testing kaldi decoder for lots of voice
// Copyright 2014  Johns Hopkins University (author: Daniel Povey)
//           2016  Api.ai (Author: Ilya Platonov)
//           2018  Polish-Japanese Academy of Information Technology (Author: Danijel Korzinek)

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
//test code for kaldi decoder
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#include <netinet/in.h>
#include <sys/types.h>
#include <poll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include "online2/online2-websockets.h"
#include <iostream>
#include <fstream>
//#include <sstream>
#include <vector>
//#include <istream>

class kaldi_testmode_online_decoder : public kaldi_online_decoder{
    public:
        kaldi_testmode_online_decoder():kaldi_online_decoder() {
            istestmode = true;
        };
        ~kaldi_testmode_online_decoder(){};

        int file_num;
        std::vector<std::string> user_list;
        std::vector<std::string> addr_list;
        //std::vector<std::string> trans_list;
        std::vector<std::string> decoded_list;

        int read_files();      

    private:
        //per_session_data * sess;
    protected:
};

//typedef kaldi_testmode_online_decoder kaldi;
void read_file(std::ifstream &file, std::vector<std::string> &list)
{

    const int file_buff_size = 300;//transcription text sentence size
    if(!file.is_open()){
        std::cout<<"file stream open error\n";
        return ;
    }
    while(!file.eof()){
        char buf[file_buff_size];
        file.getline(buf, file_buff_size);
//        std::cout<<buf<<std::endl;
        list.push_back(buf);
    } 
}

//int read_files(std::vector<std::string> &user_list, std::vector<std::string> &addr_list){
int kaldi_testmode_online_decoder::read_files(){
    //std::vector<std::string> file;
    std::ifstream user("/home/beomgon/data/test/user.list");
    read_file(user, user_list);

    std::ifstream addr("/home/beomgon/data/test/addr.list");
    read_file(addr, addr_list);

    return user_list.size() -1;
}

int main(int argc, char * argv[]){

    int numargc;
    numargc = kaldi_testmode_online_decoder::kaldi_init(argc, argv);
    std::cout<<"numargc : "<<numargc<<std::endl;

    if(numargc == -1)
        return -1;
    
    kaldi_testmode_online_decoder ktd;

    ktd.istestmode = true;

    int sentence_num = 0;
    int num = 0;
    
    //check the audio(sentence) file numbers //file num would be from bash
    sentence_num = ktd.read_files( );
    std::cout<<"Num Users : " <<ktd.user_list.size()<<std::endl;
    std::cout<<"Num file addrs : " <<ktd.addr_list.size()<<std::endl;
    ktd.file_num = ktd.addr_list.size();
    std::ofstream ofile ("/home/beomgon/data/test/decoded.txt");

    //long long buff_size = read_ 
    while(num<ktd.addr_list.size()){
        std::string user = ktd.user_list[num];
        std::cout<<"user : "<<user<<std::endl;
        std::string addr = ktd.addr_list[num];
        std::cout<<"addr : "<<addr<<std::endl;
        std::ifstream file(addr, std::ios::in|std::ios::binary);
        std::vector<short int> pcm16_buff;
        pcm16_buff.clear();
        std::string str;
        str.clear();
       
        ktd.decoder_sess_create();
        ktd.decoder_init();
        ktd.sampling = 16000;

        if(!file.is_open()){
            std::cout<<"file stream open error\n";
            return 0;
        }
            
        file.seekg(0, std::ios::end);
        int size = file.tellg();

        pcm16_buff.resize(size);
        ktd.pcm_buff_size = size;
        file.seekg(0, std::ios::beg);
        file.read((char*)&pcm16_buff[0], size*sizeof(short ));
        
        ktd.decoder_decode(pcm16_buff);
        std::cout<<"file size : "<<size<<std::endl;
        file.clear();
        std::string msg = ktd.get_str();
        //std::cout<<"msg : "<<msg<<std::endl;
        //ktd.decoded_list.push_back(msg);

        ktd.decoder_close();
        str = ktd.get_str();
        std::cout<<"decoded text : "<<str<<std::endl;
        if(ofile.is_open()){
            ofile<<user<<" ";
            ofile<<str<<std::endl;
        }
        ktd.decoder_sess_free();

        num++;
//        if(num>20)
//            break;
    }
    ofile.close();

    return 1;
    //transcript와 decoded transcript를 비교하는 코드  or bash부분으로 넘길수도 있다.
}

