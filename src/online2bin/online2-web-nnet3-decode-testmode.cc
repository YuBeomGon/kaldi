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
#include <sstream>
#include <vector>
#include <istream>

struct tag_kaldi_static_tdata {
    /** this is the static data for kaldi ASR */
    kaldi::LatticeFasterDecoderConfig *decoder_opts;
    kaldi::OnlineNnet2FeaturePipelineInfo *feature_info;
    kaldi::TransitionModel *trans_model;
    kaldi::nnet3::DecodableNnetSimpleLoopedInfo * decodable_info;
    kaldi::OnlineEndpointConfig *endpoint_opts;
    fst::Fst<fst::StdArc> *decode_fst;
    fst::SymbolTable *word_syms;
    kaldi::nnet3::NnetSimpleLoopedComputationOptions *decodable_opts;
    kaldi::OnlineNnet2FeaturePipelineConfig *feature_opts;
    BaseFloat frame_shift;// = feature_info.FrameShiftInSeconds();
    int32 frame_subsampling;// = decodable_opts.frame_subsampling_factor;

    BaseFloat chunk_length_secs;// = 0.18;
    bool do_endpointing;// = false;
    bool online;// = true;   
};
typedef tag_kaldi_static_tdata kaldi_static_tdata;

class kaldi_testmode : public kaldi_online_decoder{
    public:
        static kaldi_static_tdata tinit;
        //static kaldi::RandomAccessTableReader<kaldi::WaveHolder> wav_reader;       
        //static RandomAccessTableReader<WaveHolder> wav_reader;
        kaldi_testmode():kaldi_online_decoder() {};
        ~kaldi_testmode(){};
        static int kaldi_tinit(int argc, char* argv[]);

        int read_filenum(const char*);//check the number of files in specified folder.
        void decoder_tdecode(std::vector<short int> &pcm);
        void decoder_tclose( );
        void decoder_tinit( );
        bool istestmode;
        const char* rfile_addr;
        const char* wfile_addr;
        per_session_data * sess;
    private:
        //per_session_data * sess;
    protected:
};

kaldi_static_tdata kaldi_testmode::tinit = {NULL,};
//kaldi::RandomAccessTableReader<kaldi::WaveHolder> kaldi_testmode::wav_reader = {NULL,};

int kaldi_testmode::kaldi_tinit(int argc, char* argv[]){
    {
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
        static std::string word_syms_rxfilename;

        // feature_opts includes configuration for the iVector adaptation,
        // as well as the basic features.
        tinit.feature_opts = new OnlineNnet2FeaturePipelineConfig();
        tinit.decodable_opts = new nnet3::NnetSimpleLoopedComputationOptions();
        tinit.decoder_opts = new LatticeFasterDecoderConfig();
        tinit.endpoint_opts = new OnlineEndpointConfig(); 

        tinit.chunk_length_secs = 0.18;
        tinit.do_endpointing = false;
        tinit.online = true;

        po.Register("chunk-length", &tinit.chunk_length_secs,
                "Length of chunk size in seconds, that we process.  Set to <= 0 "
                "to use all input in one chunk.");
        po.Register("word-symbol-table", &word_syms_rxfilename,
                "Symbol table for words [for debug output]");
        po.Register("do-endpointing", &tinit.do_endpointing,
                "If true, apply endpoint detection");
        po.Register("online", &tinit.online,
                "You can set this to false to disable online iVector estimation "
                "and have all the data for each utterance used, even at "
                "utterance start.  This is useful where you just want the best "
                "results and don't care about online operation.  Setting this to "
                "false has the same effect as setting "
                "--use-most-recent-ivector=true and --greedy-ivector-extractor=true "
                "in the file given to --ivector-extraction-config, and "
                "--chunk-length=-1.");
        po.Register("num-threads-startup", &g_num_threads,
                "Number of threads used when initializing iVector extractor.");

        tinit.feature_opts->Register(&po);
        tinit.decodable_opts->Register(&po);
        tinit.decoder_opts->Register(&po);
        tinit.endpoint_opts->Register(&po);

        po.Read(argc, argv);
        if (po.NumArgs() != 5) {
            po.PrintUsage();
            return -1;
        }


        static std::string nnet3_rxfilename = po.GetArg(1);
        static std::string fst_rxfilename = po.GetArg(2);
        static std::string word_syms_filename = po.GetArg(3);
        static std::string wav_rspecifier = po.GetArg(4);

        tinit.feature_info = new OnlineNnet2FeaturePipelineInfo(*tinit.feature_opts); 
        tinit.frame_shift = tinit.feature_info->FrameShiftInSeconds();
        tinit.frame_subsampling = tinit.decodable_opts->frame_subsampling_factor;
        std::cout<<"feature_info addr : "<<tinit.feature_info<<std::endl;
        std::cout<<"frame_shift :"<<tinit.frame_shift<<" frame_subsampling :"
            <<tinit.frame_subsampling<<"\n";

        KALDI_VLOG(1) << "Loading AM...";
        tinit.trans_model = new TransitionModel();
        static nnet3::AmNnetSimple am_nnet;

        {
            static bool binary;
            static Input ki(nnet3_rxfilename, &binary);
            tinit.trans_model->Read(ki.Stream(), binary);
            am_nnet.Read(ki.Stream(), binary);
            SetBatchnormTestMode(true, &(am_nnet.GetNnet()));
            SetDropoutTestMode(true, &(am_nnet.GetNnet()));
            nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(am_nnet.GetNnet()));
        }

        // this object contains precomputed stuff that is used by all decodable
        // objects.  It takes a pointer to am_nnet because if it has iVectors it has
        // to modify the nnet to accept iVectors at intervals.
        tinit.decodable_info = new nnet3::DecodableNnetSimpleLoopedInfo(*tinit.decodable_opts, &am_nnet);
        //            nnet3::DecodableNnetSimpleLoopedInfo decodable_info(decodable_opts,
        //                    &am_nnet);

        KALDI_VLOG(1) << "Loading FST...";

        tinit.decode_fst = ReadFstKaldiGeneric(fst_rxfilename);

        tinit.word_syms = NULL;
        if (!word_syms_filename.empty())
            if (!(tinit.word_syms = fst::SymbolTable::ReadText(word_syms_filename)))
                KALDI_ERR << "Could not read symbol table from file "
                    << word_syms_filename;

        //wav_reader(wav_rspecifier);

        return 1;
    }
}

void kaldi_testmode::decoder_tdecode(std::vector<short int> &pcm){

}

void kaldi_testmode::decoder_tclose( void ){

}

void kaldi_testmode::decoder_tinit( void ){

    sess->frame_offset = 0;
    //std::cout<<tinit.feature_info<<"\n";

    sess->feature_pipeline = new kaldi::OnlineNnet2FeaturePipeline(*(tinit.feature_info));

    sess->decoder = new kaldi::SingleUtteranceNnet3Decoder(*(tinit.decoder_opts), *(tinit.trans_model),
            *(tinit.decodable_info),
            *(tinit.decode_fst),
            sess->feature_pipeline);

    sess->decoder->InitDecoding(sess->frame_offset);

    sess->silence_weighting = new kaldi::OnlineSilenceWeighting(*tinit.trans_model,
            tinit.feature_info->silence_weighting_config,
            tinit.decodable_opts->frame_subsampling_factor);

    //sess->delta_weights.clear();
    sess->delta_weights = new std::vector<std::pair<int32, BaseFloat>>();
    pcm_buff_size = (long long)(sampling*time_duration*kaldi_frame_size);

}

#if 0
int main(int argc, char * argv[]){

    int numargc;
    // init the static member.
    numargc = kaldi_testmode::kaldi_tinit(argc, argv);
    if(numargc == -1)
        return -1;

    kaldi_testmode test_decoder;
    test_decoder.istestmode = true;
    test_decoder.decoder_tinit();

    int sentence_num = 0;
    int num = 0;
    std::vector<std::string> file_list;
    //check the audio(sentence) file numbers //file num would be from bash
    sentence_num = read_files(file_list);
    std::cout<<"Num Files : " <<sentence_num<<std::endl;

    //    const char *usage =
    //        "Reads in wav file(s) and simulates online decoding with neural nets\n"
    //        "(nnet3 setup), with optional iVector-based speaker adaptation and\n"
    //        "optional endpointing.  Note: some configuration values and inputs are\n"
    //        "set via config files whose filenames are passed as options\n"
    //        "\n"
    //        "Usage: online2-wav-nnet3-latgen-faster [options] <nnet3-in> <fst-in> "
    //        "<spk2utt-rspecifier> <wav-rspecifier> <lattice-wspecifier>\n"
    //        "The spk2utt-rspecifier can just be <utterance-id> <utterance-id> if\n"
    //        "you want to decode utterance by utterance.\n";
    //
    //    BaseFloat chunk_length_secs = 0.18; 
    //    static kaldi::ParseOptions po(usage);
    //    po.Read(argc, argv);
    //    static std::string wav_rspecifier = po.GetArg(4);
    //    kaldi::RandomAccessTableReader<kaldi::WaveHolder> wav_reader(wav_rspecifier);

    int num_err = 0;
    BaseFloat chunk_length_secs = 0.18;
    //long long buff_size = read_ 
    while(num<sentence_num){
        //use string or file pointer to save the decoded audio text data, decoder_decode overiding is needed.
        //after file open, save it to buffer, array size should be set by daynamically, need to change.
        //currently size is set to 12s duration
        std::string utt = file_list[num];
        if(!test_decoder.wav_reader.HasKey(utt)){
            KALDI_WARN << "Did not find audio for utterance " << utt;
            num_err++;
            continue;
        }        

        const kaldi::WaveData &wave_data = test_decoder.wav_reader.Value(utt);    
        kaldi::SubVector<BaseFloat> data(wave_data.Data(), 0);

        BaseFloat samp_freq = wave_data.SampFreq();
        int32 chunk_length;
        if (chunk_length_secs > 0) {
            chunk_length = int32(samp_freq * chunk_length_secs);
            if (chunk_length == 0) chunk_length = 1;
        } else {
            chunk_length = std::numeric_limits<int32>::max();
        }        
        int32 samp_offset = 0;

        while (samp_offset < data.Dim()) {
            int32 samp_remaining = data.Dim() - samp_offset;
            int32 num_samp = chunk_length < samp_remaining ? chunk_length
                : samp_remaining;

            kaldi::SubVector<BaseFloat> wave_part(data, samp_offset, num_samp);
            test_decoder.sess->feature_pipeline->AcceptWaveform(samp_freq, wave_part);

            samp_offset += num_samp;
            //test_decoder.sess->decoding_timer.WaitUntil(samp_offset / samp_freq);
            if (samp_offset == data.Dim()) {
                // no more input. flush out last frames
                test_decoder.sess->feature_pipeline->InputFinished();
            }

            test_decoder.sess->decoder->AdvanceDecoding();
            bool do_endpointing = false;
            if (do_endpointing && test_decoder.sess->decoder->EndpointDetected(test_decoder.tinit.endpoint_opts)) {
                break;
            }
        }        


        //test_decoder.decoder_tdecode(pcm_bytes_vec);
        num++;

    }
    test_decoder.decoder_tclose();
    return 1;
    //transcript와 decoded transcript를 비교하는 코드  or bash부분으로 넘길수도 있다.

}
#endif
namespace kaldi {

    void GetDiagnosticsAndPrintOutput(const std::string &utt,
            const fst::SymbolTable *word_syms,
            const CompactLattice &clat,
            int64 *tot_num_frames,
            double *tot_like) {
        if (clat.NumStates() == 0) {
            KALDI_WARN << "Empty lattice.";
            return;
        }
        CompactLattice best_path_clat;
        CompactLatticeShortestPath(clat, &best_path_clat);

        Lattice best_path_lat;
        ConvertLattice(best_path_clat, &best_path_lat);

        double likelihood;
        LatticeWeight weight;
        int32 num_frames;
        std::vector<int32> alignment;
        std::vector<int32> words;
        GetLinearSymbolSequence(best_path_lat, &alignment, &words, &weight);
        num_frames = alignment.size();
        likelihood = -(weight.Value1() + weight.Value2());
        *tot_num_frames += num_frames;
        *tot_like += likelihood;
        KALDI_VLOG(2) << "Likelihood per frame for utterance " << utt << " is "
            << (likelihood / num_frames) << " over " << num_frames
            << " frames.";

        if (word_syms != NULL) {
            std::cerr << utt << ' ';
            for (size_t i = 0; i < words.size(); i++) {
                std::string s = word_syms->Find(words[i]);
                if (s == "")
                    KALDI_ERR << "Word-id " << words[i] << " not in symbol table.";
                std::cerr << s << ' ';
            }
            std::cerr << std::endl;
        }
    }

}
int read_files(std::vector<std::string> &file_list){
    //std::vector<std::string> file;
    std::ifstream infile("/home/haruband/data/test/KsponSpeech_0001/pcm.list");
    //std::ifstream infile("/home/haruband/data/test/wav.scp");

    while(!infile.eof()){
        char buf[200];
        infile.getline(buf, 200);
        std::cout<<buf<<std::endl;
        file_list.push_back(buf);
    }
    std::cout<<"file size : "<<file_list.size()<<std::endl;
    return file_list.size() -1;
}
int main(int argc, char* argv[]){

    try {
        using namespace kaldi;
        using namespace fst;

        typedef kaldi::int32 int32;
        typedef kaldi::int64 int64;

        const char *usage =
            "Reads in wav file(s) and simulates online decoding with neural nets\n"
            "(nnet3 setup), with optional iVector-based speaker adaptation and\n"
            "optional endpointing.  Note: some configuration values and inputs are\n"
            "set via config files whose filenames are passed as options\n"
            "\n"
            "Usage: online2-wav-nnet3-latgen-faster [options] <nnet3-in> <fst-in> "
            "<spk2utt-rspecifier> <wav-rspecifier> <lattice-wspecifier>\n"
            "The spk2utt-rspecifier can just be <utterance-id> <utterance-id> if\n"
            "you want to decode utterance by utterance.\n";

        ParseOptions po(usage);

        std::string word_syms_rxfilename;

        // feature_opts includes configuration for the iVector adaptation,
        // as well as the basic features.
        OnlineNnet2FeaturePipelineConfig feature_opts;
        nnet3::NnetSimpleLoopedComputationOptions decodable_opts;
        LatticeFasterDecoderConfig decoder_opts;
        OnlineEndpointConfig endpoint_opts;

        BaseFloat chunk_length_secs = 0.18;
        bool do_endpointing = false;
        bool online = true;

        po.Register("chunk-length", &chunk_length_secs,
                "Length of chunk size in seconds, that we process.  Set to <= 0 "
                "to use all input in one chunk.");
        po.Register("word-symbol-table", &word_syms_rxfilename,
                "Symbol table for words [for debug output]");
        po.Register("do-endpointing", &do_endpointing,
                "If true, apply endpoint detection");
        po.Register("online", &online,
                "You can set this to false to disable online iVector estimation "
                "and have all the data for each utterance used, even at "
                "utterance start.  This is useful where you just want the best "
                "results and don't care about online operation.  Setting this to "
                "false has the same effect as setting "
                "--use-most-recent-ivector=true and --greedy-ivector-extractor=true "
                "in the file given to --ivector-extraction-config, and "
                "--chunk-length=-1.");
        po.Register("num-threads-startup", &g_num_threads,
                "Number of threads used when initializing iVector extractor.");

        feature_opts.Register(&po);
        decodable_opts.Register(&po);
        decoder_opts.Register(&po);
        endpoint_opts.Register(&po);


        po.Read(argc, argv);

        if (po.NumArgs() != 5) {
            po.PrintUsage();
            return 1;
        }
        std::cout<<"1\n";
        std::string nnet3_rxfilename = po.GetArg(1),
            fst_rxfilename = po.GetArg(2),
            spk2utt_rspecifier = po.GetArg(3),
            wav_rspecifier = po.GetArg(4),
            clat_wspecifier = po.GetArg(5);

        std::cout<<"2\n";
        OnlineNnet2FeaturePipelineInfo feature_info(feature_opts);

        if (!online) {
            feature_info.ivector_extractor_info.use_most_recent_ivector = true;
            feature_info.ivector_extractor_info.greedy_ivector_extractor = true;
            chunk_length_secs = -1.0;
        }

        std::cout<<"3\n";
        TransitionModel trans_model;
        nnet3::AmNnetSimple am_nnet;
        {
            bool binary;
            Input ki(nnet3_rxfilename, &binary);
            trans_model.Read(ki.Stream(), binary);
            am_nnet.Read(ki.Stream(), binary);
            SetBatchnormTestMode(true, &(am_nnet.GetNnet()));
            SetDropoutTestMode(true, &(am_nnet.GetNnet()));
            nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(am_nnet.GetNnet()));
        }

        std::cout<<"4\n";
        // this object contains precomputed stuff that is used by all decodable
        // objects.  It takes a pointer to am_nnet because if it has iVectors it has
        // to modify the nnet to accept iVectors at intervals.
        nnet3::DecodableNnetSimpleLoopedInfo decodable_info(decodable_opts,
                &am_nnet);


        fst::Fst<fst::StdArc> *decode_fst = ReadFstKaldiGeneric(fst_rxfilename);

        fst::SymbolTable *word_syms = NULL;
        if (word_syms_rxfilename != "")
            if (!(word_syms = fst::SymbolTable::ReadText(word_syms_rxfilename)))
                KALDI_ERR << "Could not read symbol table from file "
                    << word_syms_rxfilename;

        std::cout<<"5\n";
        int32 num_done = 0, num_err = 0;
        double tot_like = 0.0;
        int64 num_frames = 0;

        SequentialTokenVectorReader spk2utt_reader(spk2utt_rspecifier);
        RandomAccessTableReader<WaveHolder> wav_reader(wav_rspecifier);
        CompactLatticeWriter clat_writer(clat_wspecifier);

        OnlineTimingStats timing_stats;

        int sentence_num = 0;
        int num = 0;
        std::vector<std::string> uttlist;
        //check the audio(sentence) file numbers //file num would be from bash
        sentence_num = read_files(uttlist);
        std::cout<<"Num Files : " <<sentence_num<<std::endl;

        OnlineIvectorExtractorAdaptationState adaptation_state(
                feature_info.ivector_extractor_info);
        for (size_t i = 0; i < uttlist.size(); i++) {
            std::string utt = uttlist[i];
            std::cout<<utt<<std::endl;
            if (!wav_reader.HasKey(utt)) {
                KALDI_WARN << "Did not find audio for utterance " << utt;
                num_err++;
                continue;
            }
            const WaveData &wave_data = wav_reader.Value(utt);
            // get the data for channel zero (if the signal is not mono, we only
            // take the first channel).
            SubVector<BaseFloat> data(wave_data.Data(), 0);

            OnlineNnet2FeaturePipeline feature_pipeline(feature_info);
            feature_pipeline.SetAdaptationState(adaptation_state);

            OnlineSilenceWeighting silence_weighting(
                    trans_model,
                    feature_info.silence_weighting_config,
                    decodable_opts.frame_subsampling_factor);

            SingleUtteranceNnet3Decoder decoder(decoder_opts, trans_model,
                    decodable_info,
                    *decode_fst, &feature_pipeline);
            OnlineTimer decoding_timer(utt);

            BaseFloat samp_freq = wave_data.SampFreq();
            int32 chunk_length;
            if (chunk_length_secs > 0) {
                chunk_length = int32(samp_freq * chunk_length_secs);
                if (chunk_length == 0) chunk_length = 1;
            } else {
                chunk_length = std::numeric_limits<int32>::max();
            }

            int32 samp_offset = 0;
            std::vector<std::pair<int32, BaseFloat> > delta_weights;

            while (samp_offset < data.Dim()) {
                int32 samp_remaining = data.Dim() - samp_offset;
                int32 num_samp = chunk_length < samp_remaining ? chunk_length
                    : samp_remaining;

                SubVector<BaseFloat> wave_part(data, samp_offset, num_samp);
                feature_pipeline.AcceptWaveform(samp_freq, wave_part);

                samp_offset += num_samp;
                decoding_timer.WaitUntil(samp_offset / samp_freq);
                if (samp_offset == data.Dim()) {
                    // no more input. flush out last frames
                    feature_pipeline.InputFinished();
                }

                if (silence_weighting.Active() &&
                        feature_pipeline.IvectorFeature() != NULL) {
                    silence_weighting.ComputeCurrentTraceback(decoder.Decoder());
                    silence_weighting.GetDeltaWeights(feature_pipeline.NumFramesReady(),0,
                            &delta_weights);
                    feature_pipeline.IvectorFeature()->UpdateFrameWeights(delta_weights);
                }

                decoder.AdvanceDecoding();

                if (do_endpointing && decoder.EndpointDetected(endpoint_opts)) {
                    break;
                }
            }
            decoder.FinalizeDecoding();

            CompactLattice clat;
            bool end_of_utterance = true;
            decoder.GetLattice(end_of_utterance, &clat);

            GetDiagnosticsAndPrintOutput(utt, word_syms, clat,
                    &num_frames, &tot_like);

            decoding_timer.OutputStats(&timing_stats);

            // In an application you might avoid updating the adaptation state if
            // you felt the utterance had low confidence.  See lat/confidence.h
            feature_pipeline.GetAdaptationState(&adaptation_state);

            // we want to output the lattice with un-scaled acoustics.
            BaseFloat inv_acoustic_scale =
                1.0 / decodable_opts.acoustic_scale;
            ScaleLattice(AcousticLatticeScale(inv_acoustic_scale), &clat);

            clat_writer.Write(utt, clat);
            KALDI_LOG << "Decoded utterance " << utt;
            num_done++;
        }
        timing_stats.Print(online);

        KALDI_LOG << "Decoded " << num_done << " utterances, "
            << num_err << " with errors.";
        KALDI_LOG << "Overall likelihood per frame was " << (tot_like / num_frames)
            << " per frame over " << num_frames << " frames.";
        delete decode_fst;
        delete word_syms; // will delete if non-NULL.
        return (num_done != 0 ? 0 : 1);
    } catch(const std::exception& e) {
        std::cerr << e.what();
        return -1;
    }
}
