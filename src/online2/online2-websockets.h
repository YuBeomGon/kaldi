#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <libwebsockets.h>
#include <vector>
//kaldi
#include "feat/wave-reader.h"
#include "online2/online-nnet3-decoding.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "online2/onlinebin-util.h"
#include "online2/online-timing.h"
#include "online2/online-endpoint.h"
#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"
#include "util/kaldi-thread.h"
#include "nnet3/nnet-utils.h"

/*The frame size is hardcoded for this sample code but it doesn't have to be*/
#define FRAME_SIZE 960 //60ms
//#define SAMPLE_RATE 48000
#define SAMPLE_RATE 16000 //kaldi has input of  16bit resolution, 16khz sampling
//#define CHANNELS 2
#define CHANNELS 1 //mono channel
#define APPLICATION OPUS_APPLICATION_AUDIO
#define BITRATE 64000
#define MAX_FRAME_SIZE 1*960 // 3*960 for 48Khz -> 1*960 for 16Khz , mono, for stereo, multiply by 2, 60ms 0.06*16000 = 960
#define MAX_BUFFER_SIZE (MAX_FRAME_SIZE * CHANNELS * 200) //12 seconds
#define MAX_PACKET_SIZE (3*1276)
#define MAX_TIME_DURATION   12

namespace kaldi {
    std::string LatticeToString(const Lattice &lat, const fst::SymbolTable &word_syms);
    std::string GetTimeString(int32 t_beg, int32 t_end, BaseFloat time_unit);
    int32 GetLatticeTimeSpan(const Lattice& lat);
    std::string LatticeToString(const CompactLattice &clat, const fst::SymbolTable &word_syms);
}

struct tag_kaldi_static_data {
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
    BaseFloat output_period;// = 1;
    BaseFloat samp_freq;// = 16000.0;
    int read_timeout;// = 3;
    bool produce_time;// = false;
};
typedef tag_kaldi_static_data kaldi_static_data;

struct tag_per_session_data {
    /** this is the static data for kaldi ASR */
    //tag_kaldi_static_data sdata;
    /** this is the session data for user */
    kaldi::OnlineNnet2FeaturePipeline *feature_pipeline;
    kaldi::SingleUtteranceNnet3Decoder *decoder;
    kaldi::OnlineSilenceWeighting *silence_weighting;
    std::vector<std::pair<int32, BaseFloat>> *delta_weights;
    //user datatranscriptions/online2_decode/
    int32 samp_count;// this is used for output refresh rate
    size_t chunk_len;
    int32 check_period;
    int32 check_count;
    int32 frame_offset;

    int receive_count; // = 0;
    int max_count;// = 0;
    bool size_check_flag;// = true;
    int json_data_size;// = 0;//json data
    int data_size;//only data size
    int blob_size;
    int timeout_cnt;
    string* myjson;
};
typedef tag_per_session_data per_session_data;
/** this is the data for each users */
//const int max_time_duration = 12
//const int kaldi_frame_size = 0.06 //60ms
//extern per_session_data psdata;

class kaldi_online_decoder{
    friend int ws_service_callback(
        struct lws *wsi,
        enum lws_callback_reasons reason, void *user,
        void *in, size_t len);
       
    public:
        static kaldi_static_data init;
        static bool istestmode;
        long long pcm_buff_size;
        const int time_duration;// 12sec
        const BaseFloat kaldi_frame_size; //60ms
        int sampling;
        struct lws* wsi;
        short int* pcm_bytes;
        char* base64_dbuff;

        kaldi_online_decoder( ):sampling(48000),pcm_buff_size(0),/*wsi(NULL),sess(NULL),str(NULL),*/
                                time_duration(12),kaldi_frame_size(0.06)

        {
            //pcm_buff_size(sampling*kaldi_frame_size*time_duration);
        };

        ~kaldi_online_decoder()
        {
        }; 
        static int kaldi_init(int argc, char* argv[]);
        void decoder_init();
        void decoder_decode(std::vector<short int>& pcm_int16_vec);
        void decoder_close();
        void decoder_sess_create(void);
        per_session_data* get_sess(void);
        std::string get_str(void);
        void decoder_sess_free(void);
        //char* decoder_getstr() const;

    private:
        per_session_data *sess;
        std::string str;

    protected:


};
//typedef kaldi::kaldi_websockets 

int websocket_write_back(struct lws *wsi_in, char *str, int str_size_in);

// static int kaldi_init(int argc, char* argv[]);
//extern void decoder_init();
//extern void decoder_decode(short int* pcm_bytes);
//extern void decoder_close();

extern int webm_parser2buf(long long buff_size, const char* src, short* dst, long long &pcm_buff_size );
extern int webm_parser2vec(long long buff_size, const char* src, std::vector<short> &dst );
//void libwebsocket_init(struct lws_context_creation_info *info );
void libwebsocket_init( );
int ws_service_callback(
        struct lws *wsi,
        enum lws_callback_reasons reason, void *user,
        void *in, size_t len);

//extern const char* kaldi_decode(void* user, short* pcm_bytes, long long pcm_buff_size);


