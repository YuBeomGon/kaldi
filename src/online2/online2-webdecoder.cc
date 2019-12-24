// online2/online2-webdecoder.cc
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
//
//
#include <netinet/in.h>
#include <sys/types.h>
#include <poll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include "online2/online2-websockets.h"

kaldi_static_data kaldi_online_decoder::init = {NULL,};
bool kaldi_online_decoder::istestmode = false;

namespace kaldi {

    std::string LatticeToString(const Lattice &lat, const fst::SymbolTable &word_syms) {
        LatticeWeight weight;
        std::vector<int32> alignment;
        std::vector<int32> words;
        GetLinearSymbolSequence(lat, &alignment, &words, &weight);

        std::ostringstream msg;
        for (size_t i = 0; i < words.size(); i++) {
            std::string s = word_syms.Find(words[i]);
            if (s.empty()) {
                KALDI_WARN << "Word-id " << words[i] << " not in symbol table.";
                msg << "<#" << std::to_string(i) << "> ";
            } else
                msg << s << " ";
        }
        return msg.str();
    }

    std::string GetTimeString(int32 t_beg, int32 t_end, BaseFloat time_unit) {
        char buffer[100];
        double t_beg2 = t_beg * time_unit;
        double t_end2 = t_end * time_unit;
        snprintf(buffer, 100, "%.2f %.2f", t_beg2, t_end2);
        return std::string(buffer);
    }
    int32 GetLatticeTimeSpan(const Lattice& lat) {
        std::vector<int32> times;
        LatticeStateTimes(lat, &times);
        return times.back();
    }

    std::string LatticeToString(const CompactLattice &clat, const fst::SymbolTable &word_syms) {
        if (clat.NumStates() == 0) {
            KALDI_WARN << "Empty lattice.";
            return "";
        }
        CompactLattice best_path_clat;
        CompactLatticeShortestPath(clat, &best_path_clat);

        Lattice best_path_lat;
        ConvertLattice(best_path_clat, &best_path_lat);
        return LatticeToString(best_path_lat, word_syms);
    }
}

void kaldi_online_decoder::decoder_sess_create(void){
    sess = new per_session_data();
};
per_session_data* kaldi_online_decoder::get_sess(void){
    return sess;
};
void kaldi_online_decoder::decoder_sess_free(void){
    delete sess;
};

std::string kaldi_online_decoder::get_str(void){
    return str;
}

void kaldi_online_decoder::decoder_init(){
    per_session_data* sess = get_sess();
    //sess->samp_count = 0;// this is used for output refresh rate
    sess->chunk_len = static_cast<size_t>(init.chunk_length_secs * init.samp_freq);
    sess->check_period = static_cast<int32>(init.samp_freq * init.output_period);
    sess->check_count = sess->check_period;

    sess->frame_offset = 0;
    //std::cout<<init.feature_info<<"\n";

    sess->feature_pipeline = new kaldi::OnlineNnet2FeaturePipeline(*(init.feature_info));

    sess->decoder = new kaldi::SingleUtteranceNnet3Decoder(*(init.decoder_opts), *(init.trans_model),
            *(init.decodable_info),
            *(init.decode_fst),
            sess->feature_pipeline);

    sess->decoder->InitDecoding(sess->frame_offset);

    sess->silence_weighting = new kaldi::OnlineSilenceWeighting(*init.trans_model,
            init.feature_info->silence_weighting_config,
            init.decodable_opts->frame_subsampling_factor);

    //sess->delta_weights.clear();
    sess->delta_weights = new std::vector<std::pair<int32, BaseFloat>>();
    pcm_buff_size = (long long)(sampling*time_duration*kaldi_frame_size);

    sess->size_check_flag = true;
    sess->json_data_size = 0;
    sess->data_size = 0;
    sess->blob_size = 0;
    sess->myjson = new string("");
    sess->myjson->clear();
}
void kaldi_online_decoder::decoder_decode(std::vector<short int> &pcm){
    per_session_data* sess = get_sess();
    //const int max_frame_size = (int)pcm_buff_size/(2*sampling/SAMPLE_RATE); //buff/2 means 8bit -> 16 bit
//    int denom = (sampling/SAMPLE_RATE);
//    std::cout<<"denom : "<<denom<<std::endl;
    const int max_frame_size = (int)pcm_buff_size/(sampling/SAMPLE_RATE); //buff/2 means 8bit -> 16 bit
    BaseFloat out[max_frame_size] = {0,};

    //downsampling by 3 if sampling rate is 48khz
    for (int i =0; i<max_frame_size ; i++){
        out[i] = (BaseFloat)pcm[i*(sampling/SAMPLE_RATE)];
    }

    sess->samp_count = 0;
    while(sess->samp_count < max_frame_size ){
        if(max_frame_size - sess->samp_count < sess->chunk_len)
            sess->chunk_len = max_frame_size - sess->samp_count;

        kaldi::SubVector<BaseFloat> sv(&out[sess->samp_count], (sess->chunk_len));
        kaldi::Vector<BaseFloat> wave_data(sv);        
        sess->feature_pipeline->AcceptWaveform(init.samp_freq, wave_data);
        sess->samp_count += sess->chunk_len;


        //////////////////////////////////////////
        if (sess->silence_weighting->Active() &&
                sess->feature_pipeline->IvectorFeature() != NULL) {
            sess->silence_weighting->ComputeCurrentTraceback(sess->decoder->Decoder());

            sess->silence_weighting->GetDeltaWeights(sess->feature_pipeline->NumFramesReady(),
                    sess->frame_offset * init.decodable_opts->frame_subsampling_factor,
                    (sess->delta_weights));
            sess->feature_pipeline->UpdateFrameWeights(*(sess->delta_weights));
        }

        sess->decoder->AdvanceDecoding();       
        if (sess->decoder->EndpointDetected(*init.endpoint_opts)&&(istestmode == false)) {
            sess->decoder->FinalizeDecoding();
            sess->frame_offset += sess->decoder->NumFramesDecoded();
            kaldi::CompactLattice lat;
            sess->decoder->GetLattice(true, &lat);
            std::string msg = kaldi::LatticeToString(lat, *init.word_syms);

            // get time-span between endpoints,
            if (init.produce_time) {
                int32 t_beg = sess->frame_offset - sess->decoder->NumFramesDecoded();
                int32 t_end = sess->frame_offset;
                msg = kaldi::GetTimeString(t_beg, t_end, init.frame_shift * init.frame_subsampling) + " " + msg;
            }

            KALDI_VLOG(1) << "Endpoint, sending message: " << msg;
            //server.WriteLn(msg);
            if(istestmode == false){
                char *str = (char*)msg.c_str();
                websocket_write_back(wsi,(char*) str, -1);
            }
            //            else{
            //                str = msg;
            //            }

            ////////////////////////////////////////////////
            sess->decoder->InitDecoding(sess->frame_offset);
            //sess->silence_weighting.clear();
            delete sess->silence_weighting;
            sess->silence_weighting = new kaldi::OnlineSilenceWeighting(*init.trans_model,
                    init.feature_info->silence_weighting_config,
                    init.decodable_opts->frame_subsampling_factor);           
        }
    }
}
void kaldi_online_decoder::decoder_close(){
    per_session_data* sess = get_sess();
    sess->feature_pipeline->InputFinished();
    sess->decoder->AdvanceDecoding();
    sess->decoder->FinalizeDecoding();
    sess->frame_offset += sess->decoder->NumFramesDecoded();
    if (sess->decoder->NumFramesDecoded() > 0) {
        kaldi::CompactLattice lat;
        sess->decoder->GetLattice(true, &lat);
        std::string msg = kaldi::LatticeToString(lat, *init.word_syms);

        // get time-span from previous endpoint to end of audio,
        if (init.produce_time) {
            int32 t_beg = sess->frame_offset - sess->decoder->NumFramesDecoded();
            int32 t_end = sess->frame_offset;
            msg = kaldi::GetTimeString(t_beg, t_end, init.frame_shift * init.frame_subsampling) + " " + msg;
        }

        KALDI_VLOG(1) << "EndOfAudio, sending message: " << msg;
        if(istestmode == false){
            char* str = (char*)msg.c_str();
            websocket_write_back(wsi,(char*) "\n", -1);
        }
        else
            str = msg;

        // server.WriteLn(msg);
    } else
    {
        if(istestmode == false)
            websocket_write_back(wsi,(char*) "\n", -1);
        else
            str = "\n";
    }
    delete sess->feature_pipeline;
    delete sess->decoder;
    delete sess->silence_weighting;
    delete sess->delta_weights;
    delete sess->myjson;
    //delete sess;   
    //sess->delta_weights.clear();


    // server.Write("\n");
    //server.Disconnect();
}
//char* kaldi_online_decoder::decoder_getstr(){
//    return str;
//}

int kaldi_online_decoder::kaldi_init(int argc, char *argv[]){
    using namespace kaldi;
    using namespace fst;

    typedef kaldi::int32 int32;
    typedef kaldi::int64 int64;

    //static per_session_data psdata;

    static const char *usage =
        "Reads in audio from a network socket and performs online\n"
        "decoding with neural nets (nnet3 setup), with iVector-based\n"
        "speaker adaptation and endpointing.\n"
        "Note: some configuration values and inputs are set via config\n"
        "files whose filenames are passed as options\n"
        "\n"
        "Usage: online2-tcp-nnet3-decode-faster [options] <nnet3-in> "
        "<fst-in> <word-symbol-table>\n";

    static ParseOptions po(usage);

    // feature_opts includes configuration for the iVector adaptation,
    // as well as the basic features.
    init.feature_opts = new OnlineNnet2FeaturePipelineConfig();
    init.decodable_opts = new nnet3::NnetSimpleLoopedComputationOptions();
    init.decoder_opts = new LatticeFasterDecoderConfig();
    init.endpoint_opts = new OnlineEndpointConfig(); 

    init.chunk_length_secs = 0.18;
    init.output_period = 1;
    init.samp_freq = 16000.0;
    init.read_timeout = 3;

    init.produce_time = false;

    po.Register("samp-freq", &init.samp_freq,
            "Sampling frequency of the input signal (coded as 16-bit slinear).");
    po.Register("chunk-length", &init.chunk_length_secs,
            "Length of chunk size in seconds, that we process.");
    po.Register("output-period", &init.output_period,
            "How often in seconds, do we check for changes in output.");
    po.Register("num-threads-startup", &g_num_threads,
            "Number of threads used when initializing iVector extractor.");
    po.Register("read-timeout", &init.read_timeout,
            "Number of seconds of timout for TCP audio data to appear on the stream. Use -1 for blocking.");
    po.Register("produce-time", &init.produce_time,
            "Prepend begin/end times between endpoints (e.g. '5.46 6.81 <text_output>', in seconds)");

    init.feature_opts->Register(&po);
    init.decodable_opts->Register(&po);
    init.decoder_opts->Register(&po);
    init.endpoint_opts->Register(&po);

    po.Read(argc, argv);
    if (po.NumArgs() !=4 && istestmode == false ) {
        po.PrintUsage();
        return -1;
    }

    static std::string nnet3_rxfilename = po.GetArg(1);
    static std::string fst_rxfilename = po.GetArg(2);
    static std::string word_syms_filename = po.GetArg(3);
    std:cout<<"nnet3 rxfilename : "<<nnet3_rxfilename<<std::endl;

    init.feature_info = new OnlineNnet2FeaturePipelineInfo(*init.feature_opts); 
    init.frame_shift = init.feature_info->FrameShiftInSeconds();
    init.frame_subsampling = init.decodable_opts->frame_subsampling_factor;
    std::cout<<"feature_info addr : "<<init.feature_info<<std::endl;
    std::cout<<"frame_shift :"<<init.frame_shift<<" frame_subsampling :"
        <<init.frame_subsampling<<"\n";

    KALDI_VLOG(1) << "Loading AM...";
    init.trans_model = new TransitionModel();
    static nnet3::AmNnetSimple am_nnet;
    {
        static bool binary;
        static Input ki(nnet3_rxfilename, &binary);
        init.trans_model->Read(ki.Stream(), binary);
        am_nnet.Read(ki.Stream(), binary);
        SetBatchnormTestMode(true, &(am_nnet.GetNnet()));
        SetDropoutTestMode(true, &(am_nnet.GetNnet()));
        nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(am_nnet.GetNnet()));
    }

    // this object contains precomputed stuff that is used by all decodable
    // objects.  It takes a pointer to am_nnet because if it has iVectors it has
    // to modify the nnet to accept iVectors at intervals.
    init.decodable_info = new nnet3::DecodableNnetSimpleLoopedInfo(*init.decodable_opts, &am_nnet);
    //            nnet3::DecodableNnetSimpleLoopedInfo decodable_info(decodable_opts,
    //                    &am_nnet);

    KALDI_VLOG(1) << "Loading FST...";

    init.decode_fst = ReadFstKaldiGeneric(fst_rxfilename);

    init.word_syms = NULL;
    if (!word_syms_filename.empty())
        if (!(init.word_syms = fst::SymbolTable::ReadText(word_syms_filename)))
            KALDI_ERR << "Could not read symbol table from file "
                << word_syms_filename;


    return 1;
    //            signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE to avoid crashing when socket forcefully disconnected
}


