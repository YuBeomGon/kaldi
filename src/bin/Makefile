
all:
	-rm -f arpa2fst
EXTRA_CXXFLAGS = -Wno-sign-compare
include ../kaldi.mk

BINFILES = align-equal align-equal-compiled acc-tree-stats \
        show-alignments compile-questions cluster-phones \
        compute-wer compute-wer-test compute-wer-bootci make-h-transducer \
        add-self-loops convert-ali \
        compile-train-graphs compile-train-graphs-fsts \
        make-pdf-to-tid-transducer make-ilabel-transducer show-transitions \
        ali-to-phones ali-to-post weight-silence-post acc-lda est-lda \
        ali-to-pdf est-mllt build-tree build-tree-two-level decode-faster \
        decode-faster-mapped vector-scale copy-transition-model \
        phones-to-prons prons-to-wordali copy-gselect copy-tree scale-post \
        post-to-weights sum-tree-stats weight-post post-to-tacc copy-matrix \
        copy-vector copy-int-vector sum-post sum-matrices draw-tree \
        align-mapped align-compiled-mapped latgen-faster-mapped latgen-faster-mapped-parallel \
        hmm-info analyze-counts post-to-phone-post \
        post-to-pdf-post logprob-to-post prob-to-post copy-post \
        matrix-sum build-pfile-from-ali get-post-on-ali tree-info am-info \
        vector-sum matrix-sum-rows est-pca sum-lda-accs sum-mllt-accs \
        transform-vec align-text matrix-dim post-to-smat compile-graph \
        compare-int-vector latgen-incremental-mapped compute-gop


OBJFILES =

ADDLIBS = ../decoder/kaldi-decoder.a ../lat/kaldi-lat.a ../lm/kaldi-lm.a \
          ../fstext/kaldi-fstext.a ../hmm/kaldi-hmm.a \
          ../transform/kaldi-transform.a ../gmm/kaldi-gmm.a \
          ../tree/kaldi-tree.a ../util/kaldi-util.a ../matrix/kaldi-matrix.a \
          ../base/kaldi-base.a


TESTFILES =

include ../makefiles/default_rules.mk
