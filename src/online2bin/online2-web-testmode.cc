
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


int main(int argc, char * argv[]){
    
//    const char *usage =
//        "Compute WER by comparing different transcriptions\n"
//        "Takes two transcription files, in integer or text format,\n"
//        "and outputs overall WER statistics to standard output.\n"
//        "\n"
//        "Usage: compute-wer [options] <ref-rspecifier> <hyp-rspecifier>\n"
//        "E.g.: compute-wer --text --mode=present ark:data/train/text ark:hyp_text\n"
//        "See also: align-text,\n"
//        "Example scoring script: egs/wsj/s5/steps/score_kaldi.sh\n";
//
//    kaldi::ParseOptions po(usage);
//
//    std::string mode = "strict";
//    po.Register("mode", &mode,
//                "Scoring mode: \"present\"|\"all\"|\"strict\":\n"
//                "  \"present\" means score those we have transcriptions for\n"
//                "  \"all\" means treat absent transcriptions as empty\n"
//                "  \"strict\" means die if all in ref not also in hyp");
//    
//    bool dummy = false;
//    po.Register("text", &dummy, "Deprecated option! Keeping for compatibility reasons.");
//
//    po.Read(argc, argv);

    int numargc;
    kaldi_testmode_online_decoder::istestmode = true;
    numargc = kaldi_testmode_online_decoder::kaldi_init(argc, argv);
    std::cout<<"numargc : "<<numargc<<std::endl;
    if(numargc == -1)
        return -1;
    
    kaldi_testmode_online_decoder ktd;

    std::string wav_rspecifier = argv[argc -2];
    std::string decode_wspecifier = argv[argc - 1];
    std::cout<<"wav :"<<wav_rspecifier<<" decode : "<<decode_wspecifier<<std::endl;
    kaldi::SequentialTokenVectorReader ref_reader(wav_rspecifier);
    std::ofstream ofile (decode_wspecifier);
    std::string str;
    std::vector<short int> pcm16_buff;

    for (; !ref_reader.Done(); ref_reader.Next()){
        std::string key = ref_reader.Key();
        const std::vector<std::string> &ref_sent = ref_reader.Value();
        //std::string address = ref_reader.Value();
        std::string address = ref_sent[0];

        std::cout<<"address : "<<address<<std::endl;
        std::ifstream wav(address, std::ios::in|std::ios::binary);

        pcm16_buff.clear();
        str.clear();

        ktd.decoder_sess_create();
        ktd.decoder_init();
        ktd.sampling = 16000;

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
        
        ktd.decoder_decode(pcm16_buff);
        std::cout<<"wav size : "<<size<<std::endl;
        wav.clear();
        wav.close();
        std::string msg = ktd.get_str();
        //std::cout<<"msg : "<<msg<<std::endl;
        //ktd.decoded_list.push_back(msg);

        ktd.decoder_close();
        str = ktd.get_str();
        std::cout<<"decoded text : "<<str<<std::endl;
        if(ofile.is_open()){
            ofile<<key<<" ";
            ofile<<str<<std::endl;
        }
        ktd.decoder_sess_free();

    }

    ofile.close();

    return 1;
    //transcript와 decoded transcript를 비교하는 코드  or bash부분으로 넘길수도 있다.
}

