
#include "online2/online2-websockets.h"
#include "rapidjson/document.h"

#define KGRN "\033[0;32;32m"
#define KCYN "\033[0;36m"
#define KRED "\033[0;32;31m"
#define KYEL "\033[1;33m"
#define KMAG "\033[0;35m"
#define KBLU "\033[0;32;34m"
#define KCYN_L "\033[1;36m"
#define RESET "\033[0m"

const int websocket_buffer_size = 4096;

static int destroy_flag = 0;

static void INT_HANDLER(int signo) {
    destroy_flag = 1;
}

void libwebsocket_init( ){
    // server url will usd port 5000
    int port = 5000;
    const char *interface = NULL;
    struct lws_context_creation_info info;
    struct lws_protocols protocol;
    struct lws_context *context;
    // Not using ssl
    const char *cert_path = NULL;
    const char *key_path = NULL;

    // no special options
    int opts = 0;

    //* register the signal SIGINT handler */
    struct sigaction act;
    act.sa_handler = INT_HANDLER;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction( SIGINT, &act, 0);

    //* setup websocket protocol */
    protocol.name = "my-echo-protocol";
    protocol.callback = ws_service_callback;
    //protocol.per_session_data_size=sizeof(per_session_data);
    protocol.per_session_data_size=sizeof(kaldi_online_decoder);
    protocol.rx_buffer_size = 0;

    //* setup websocket context info*/
    memset(&info, 0, sizeof info);
    info.port = port;
    info.iface = interface;
    info.protocols = &protocol;
    info.extensions = lws_get_internal_extensions();
    info.ssl_cert_filepath = cert_path;
    info.ssl_private_key_filepath = key_path;
    info.gid = -1;
    info.uid = -1;
    info.options = opts;

    //* create libwebsocket context. */
    context = lws_create_context(&info);
    if (context == NULL) {
        printf(KRED"[Main] Websocket context create error.\n"RESET);
        return ;
    }


    printf(KGRN"[Main] Websocket context create success.\n"RESET);

    //* websocket service */
    while ( !destroy_flag ) {
        lws_service(context, 50);
    }
    usleep(10);
    lws_context_destroy(context);
}

/* *
 * websocket_write_back: write the string data to the destination wsi.
 */
int websocket_write_back(struct lws *wsi_in, char *str, int str_size_in)
{
    if (str == NULL || wsi_in == NULL)
        return -1;

    int n;
    int len;
    unsigned char *out = NULL;

    if (str_size_in < 1)
        len = strlen(str);
    else
        len = str_size_in;

    out = (unsigned char *)malloc(sizeof(unsigned char)*(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
    //* setup the buffer*/
    memcpy (out + LWS_SEND_BUFFER_PRE_PADDING, str, len );
    //* write out*/
    n = lws_write(wsi_in, out + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);

    printf(KBLU"[websocket_write_back] %s\n"RESET, str);

    //* free the buffer*/
    free(out);

    return n;
}
//kaldi_online_decoder kd;
int ws_service_callback(
        struct lws *wsi,
        enum lws_callback_reasons reason, void *user,
        void *in, size_t len)
{
    kaldi_online_decoder* kd;
    //userinfo = new per_session_data;
    if(user != NULL)
    {
        kd = ( kaldi_online_decoder *)user;
        kd->wsi = wsi;
        //printf( "user address : [%x] userinfo->asr_static_data : [%x]\n", user, userinfo->asr_static_data );
    }

//    printf(KCYN_L"[Main Service] Server recvived: reason %d\n"RESET, reason);
    //static string myjson = "";
    switch (reason) {

        case LWS_CALLBACK_ESTABLISHED:
            {
                //printf(KYEL"[Main Service] Connection established\n"RESET);
                //printf(KCYN_L"[Main Service] connection established %s\n"RESET,(char *)in);
                
                //kd.wsi = wsi;
//                std::cout<<"user addr : "<<user<<std::endl;
//                std::cout<<"kd addr : "<<kd<<std::endl;              
//                std::cout<<"kd sess  addr : "<<kd->sess<<std::endl;               

//                kd->sess = new per_session_data();
                kd->decoder_sess_create();
                kd->decoder_init();
                
//                kd->sess->size_check_flag = true;
//                kd->sess->json_data_size = 0;
//                kd->sess->data_size = 0;
//                kd->sess->blob_size = 0;
//                kd->sess->myjson = new string("");
//                kd->sess->myjson->clear();
                
                break;
            }

            //* If receive a data from client*/
        case LWS_CALLBACK_RECEIVE:
            {
                //printf(KCYN_L"[Main Service] Server recvived:%s\n"RESET,(char *)in);
                short pcm_bytes[MAX_BUFFER_SIZE*3];// for test 12 seconds, for 48000hz, multiply by 3
                std::vector<short> pcm_int16_vec;
                long long pcm_buff_size = 0;
                int sampling = 0;

                if(kd->sess->size_check_flag == true){
                    printf(KCYN_L"[Main Service] Server recvived: size %d msg %s \n"RESET, strlen((char *)in), (char*) in);
                    rapidjson::Document document;
                    document.Parse((char*)in);
                    assert(document.IsObject());
                    assert(document.HasMember("dataSize"));
                    assert(document.HasMember("jsonDataSize"));
                    assert(document["jsonDataSize"].IsString());
                    string s1 = document["dataSize"].GetString();
                    string s2 = document["jsonDataSize"].GetString();

                    string s3 = document["blobSize"].GetString();
                    //kd->sess->json_data_size = document["JsonDataSize"].GetInt();
                    kd->sess->json_data_size = std::stoi(s2);
                    kd->sess->data_size = std::stoi(s1);
                    kd->sess->blob_size = std::stoi(s3);
                    kd->sess->max_count = kd->sess->json_data_size/websocket_buffer_size;
                    std::cout<<"data size: "<<s1<< " json data size:  "<<kd->sess->json_data_size<<
                        " Max Count : "<<kd->sess->max_count<<" blob size : "<<kd->sess->blob_size<<std::endl;
                    kd->sess->myjson->clear();

                    kd->sess->size_check_flag = false;
                    kd->sess->receive_count = 0;
                    break;
                }
                else{

                    //printf(KCYN_L"[Main Service] Server recvived: size %d \t"RESET,strlen((char *)in));
                    kd->sess->myjson->append((char*) in);
                    if(kd->sess->myjson->size() < kd->sess->json_data_size)
                        break;
                    if(kd->sess->myjson->size() > kd->sess->json_data_size){
                        const char * cstr = "received data is more than setting data size \n";
                        std::cout<<"recieved json size : "<<kd->sess->myjson->size();
                        std::cout<<" server sent json size : "<<kd->sess->json_data_size<<std::endl;
                        websocket_write_back(wsi ,(char *)cstr, -1);
                        break;
                    }

                    char *temp = new char[kd->sess->myjson->size()+1];
                    rapidjson::Document document;

                    std::copy(kd->sess->myjson->begin(), kd->sess->myjson->end(), temp);

                    temp[kd->sess->myjson->size()]='\0';
                    document.Parse(temp);
                    {
                        assert(document.IsObject());
                        assert(document.HasMember("action"));
                        assert(document.HasMember("message"));
                        assert(document.HasMember("sampling"));
                    }
                    //string s1 = document["action"].GetString();
                    string s2 = document["sampling"].GetString();
                    string s3 = document["message"].GetString();
                    kd->sampling = std::stoi(s2);
                    std::cout<<"kd->sampling  : "<<s2<<std::endl;
                    if (kd->sampling!= SAMPLE_RATE && kd->sampling !=SAMPLE_RATE*3)
                    {
                        std::cout<<"wrong sampling rate \n" ;
                        break;
                    }

                    const char * c = s3.c_str();
                    kd->base64_dbuff = new char[sizeof(char)*(kd->sess->data_size)];
                    int b64_decode_length =0;
                    b64_decode_length = lws_b64_decode_string(c, kd->base64_dbuff, kd->sess->data_size);
                    printf("out size : %d b64 decoded lengh %d blob size : %d \n ",
                            sizeof(kd->base64_dbuff), b64_decode_length, kd->sess->blob_size);
                    delete[] temp;
                    kd->sess->size_check_flag = true;

                }
                {
                    /*this is test code for webm file checking between server and front end*/
                    FILE* file;
                    file = fopen("/home/beomgon/test.webm","wb");
                    fwrite(kd->base64_dbuff, sizeof(char), kd->sess->data_size, file);
                    fclose(file);
                    
                }

                {
                    pcm_int16_vec.clear();
                    webm_parser2vec(kd->sess->blob_size, kd->base64_dbuff, pcm_int16_vec);
                    fprintf(stderr, "webm parsing is done kd->pcm_buff_size : %d bytes \n", pcm_int16_vec.size()); //#define OPUS_INVALID_PACKET 4
                    kd->pcm_buff_size =  pcm_int16_vec.size();
                    delete [] kd->base64_dbuff;
                    
                    kd->decoder_decode(pcm_int16_vec);
                }
                {
                    /*this is test code for raw pcm file checking between server and front end*/
                    FILE* file;
                    file = fopen("/home/beomgon/test.pcm","wb");
                    fwrite(&pcm_int16_vec[0], 2*sizeof(char), pcm_int16_vec.size(), file);
                    fclose(file);
                    
                }

                break;
            }
        case LWS_CALLBACK_CLOSED:
            {
                printf(KYEL"[Main Service] Client close.\n"RESET);
                kd->decoder_close();
                kd->decoder_sess_free();
//                delete kd->sess->myjson;;
//                delete kd->sess;
                //const char* str = kd->decoder_getstr();
                //websocket_write_back(wsi,(char*) str, -1);

                break;
            }

        default:
            break;
    }
    return 0;
}

