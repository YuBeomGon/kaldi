
all:

include ../kaldi.mk

LDFLAGS += $(CUDA_LDFLAGS)
LDLIBS += $(CUDA_LDLIBS)
LDLIBS += -lwebsockets
LDLIBS += -lopus
LDLIBS += -lvorbis
LDLIBS += -lvpx
LDLIBS += -lm

BINFILES = online2-wav-gmm-latgen-faster apply-cmvn-online \
     extend-wav-with-silence compress-uncompress-speex \
     online2-wav-nnet2-latgen-faster ivector-extract-online2 \
     online2-wav-dump-features ivector-randomize \
     online2-wav-nnet2-am-compute  online2-wav-nnet2-latgen-threaded \
     online2-wav-nnet3-latgen-faster online2-wav-nnet3-latgen-grammar \
     online2-tcp-nnet3-decode-faster online2-web-nnet3-decode-faster \
	 online2-web-testmode online2-wav-nnet3-latgen-incremental

OBJFILES =

TESTFILES =

ADDLIBS = ../online2/kaldi-online2.a ../ivector/kaldi-ivector.a \
          ../nnet3/kaldi-nnet3.a ../chain/kaldi-chain.a ../nnet2/kaldi-nnet2.a \
          ../cudamatrix/kaldi-cudamatrix.a ../decoder/kaldi-decoder.a \
          ../lat/kaldi-lat.a ../fstext/kaldi-fstext.a ../hmm/kaldi-hmm.a \
          ../feat/kaldi-feat.a ../transform/kaldi-transform.a \
          ../gmm/kaldi-gmm.a ../tree/kaldi-tree.a ../util/kaldi-util.a \
          ../matrix/kaldi-matrix.a ../base/kaldi-base.a \
		  /usr/lib/x86_64-linux-gnu/libsimplewebm.a #/usr/lib/x86_64-linux-gnu/libwebsockets.a /usr/lib/x86_64-linux-gnu/libwebm.a  
include ../makefiles/default_rules.mk
