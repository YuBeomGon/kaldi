
all:

include ../kaldi.mk

TESTFILES =

OBJFILES = online-gmm-decodable.o online-feature-pipeline.o online-ivector-feature.o \
           online-nnet2-feature-pipeline.o online-gmm-decoding.o online-timing.o \
           online-endpoint.o onlinebin-util.o online-speex-wrapper.o \
           online-nnet2-decoding.o online-nnet2-decoding-threaded.o \
           online-nnet3-decoding.o online2-websockets.o online2-webdecoder.o \
           online-nnet3-incremental-decoding.o

LIBNAME = kaldi-online2

ADDLIBS = ../ivector/kaldi-ivector.a ../nnet3/kaldi-nnet3.a \
          ../chain/kaldi-chain.a ../nnet2/kaldi-nnet2.a \
          ../cudamatrix/kaldi-cudamatrix.a ../decoder/kaldi-decoder.a \
          ../lat/kaldi-lat.a ../hmm/kaldi-hmm.a ../feat/kaldi-feat.a \
          ../transform/kaldi-transform.a ../gmm/kaldi-gmm.a \
          ../tree/kaldi-tree.a ../util/kaldi-util.a ../matrix/kaldi-matrix.a \
          ../base/kaldi-base.a ../fstext/kaldi-fstext.a  \
          /usr/lib/x86_64-linux-gnu/libwebm.a \
          /usr/lib/x86_64-linux-gnu/libsimplewebm.a #/usr/lib/x86_64-linux-gnu/libwebsockets.a



include ../makefiles/default_rules.mk

