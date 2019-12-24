
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
#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "tree/context-dep.h"
#include "util/edit-distance.h"
#include "util/parse-options.h"
//#include <istream>

class kaldi_testmode_online_decoder : public kaldi_online_decoder{
    public:
        kaldi_testmode_online_decoder():kaldi_online_decoder() {
            istestmode = true;
        };
        ~kaldi_testmode_online_decoder(){};

    private:
        //per_session_data * sess;
    protected:
};

//typedef kaldi_testmode_online_decoder kaldi;
std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

int main(int argc, char * argv[]){
    
    int numargc;
    kaldi_testmode_online_decoder::istestmode = true;
    numargc = kaldi_testmode_online_decoder::kaldi_init(argc, argv);
    std::cout<<"numargc : "<<numargc<<std::endl;
    if(numargc == -1)
        return -1;
    
    std::string wav_rspecifier = argv[argc -2];
    std::string decode_wspecifier = argv[argc - 1];
    std::cout<<"wav :"<<wav_rspecifier<<" decode : "<<decode_wspecifier<<std::endl;
    kaldi::SequentialTokenVectorReader ref_reader(wav_rspecifier);

    std::ofstream ofile (decode_wspecifier);
    kaldi_testmode_online_decoder ktd;
    ktd.sampling = 16000;

    std::string str;
    std::vector<short int> pcm16_buff;

    for (; !ref_reader.Done(); ref_reader.Next()){
        std::string key = ref_reader.Key();
        const std::vector<std::string> &ref_sent = ref_reader.Value();
        //std::string address = ref_reader.Value();
        std::string address = ref_sent[0];

        ktd.decoder_sess_create();
        ktd.decoder_init();
        pcm16_buff.clear();
        str.clear();

        std::cout<<"address : "<<address<<std::endl;
            
        {
            std::ifstream wav(address, std::ios::in|std::ios::binary);

            if(!wav.is_open()){
                std::cout<<"wav stream open error\n";
                return 0;
            }
            wav.seekg(0, std::ios::end);
            int size = wav.tellg();

            pcm16_buff.resize(size);
            ktd.pcm_buff_size = size;
            wav.seekg(0, std::ios::beg);
            wav.read((char*)&pcm16_buff[0], size*sizeof(short ));
            std::cout<<"wav size : "<<size<<std::endl;

            ktd.decoder_decode(pcm16_buff);
            //ktd.decoder_init();

            wav.clear();
            wav.close();
        }

        ktd.decoder_close();
        ktd.decoder_sess_free();
        str = ktd.get_str();
        std::cout<<"decoded text : "<<str<<std::endl;

//        str = replaceAll(str, std::string("번"), std::string(""));
        str = replaceAll(str, std::string("지아이"), std::string("GI"));
        str = replaceAll(str, std::string("에스비에스"), std::string("SBS"));
        str = replaceAll(str, std::string("에이피에프"), std::string("APF"));
        str = replaceAll(str, std::string("씨티"), std::string("CT"));
        str = replaceAll(str, std::string("이피티"), std::string("EPT"));
        str = replaceAll(str, std::string("씨알"), std::string("CR"));
        str = replaceAll(str, std::string("에프유"), std::string("FU"));
        std::cout<<"decoded text : " << str <<std::endl;
//        str.replace(str.find("번"), , " ");
//        std::replace(str.begin(), str.end(), '번', ' ');
        if(ofile.is_open()){
            ofile<<key<<" ";
            ofile<<str<<std::endl;
        }

    }

    ofile.close();

    return 1;
}

