// online2bin/online2-web-nnet3-decode-faster.cc
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

//#include <netinet/in.h>
//#include <sys/types.h>
//#include <poll.h>
//#include <arpa/inet.h>
//#include <unistd.h>
//#include <string>
#include "online2/online2-websockets.h"

int main(int argc, char *argv[]) {
    try {
        
        int numargc;

        numargc = kaldi_online_decoder::kaldi_init(argc, argv);
        if(numargc == -1)
            return -1;

        libwebsocket_init();

    } catch (const std::exception &e) {
        std::cerr << e.what();

        return -1;
    }
}
