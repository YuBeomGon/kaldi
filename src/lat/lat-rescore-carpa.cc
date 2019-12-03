// lat/lat-lmrescore-carpa.cc
// Copyright 2014  Guoguo Chen
// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
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


//#include "base/kaldi-common.h"
//#include "fstext/fstext-lib.h"
//#include "lat/kaldi-lattice.h"
//#include "lat/lattice-functions.h"
//#include "lm/const-arpa-lm.h"
//#include "util/common-utils.h"
#include "online2/online2-websockets.h"

kaldi::CompactLattice lat_rescore(kaldi::CompactLattice &clat ) {
    using namespace kaldi;
    typedef kaldi::int32 int32;
    typedef kaldi::int64 int64;
    BaseFloat lm_scale = 1.0;

    // Reads the language model in ConstArpaLm format.
    //ConstArpaLm &const_arpa = *new_carpa;
    CompactLattice determinized_clat = clat;

    if (lm_scale != 0.0) {
        // Before composing with the LM FST, we scale the lattice weights
        // by the inverse of "lm_scale".  We'll later scale by "lm_scale".
        // We do it this way so we can determinize and it will give the
        // right effect (taking the "best path" through the LM) regardless
        // of the sign of lm_scale.
        fst::ScaleLattice(fst::GraphLatticeScale(1.0/lm_scale), &clat);
        ArcSort(&clat, fst::OLabelCompare<CompactLatticeArc>());

        // Wraps the ConstArpaLm format language model into FST. We re-create it
        // for each lattice to prevent memory usage increasing with time.
        ConstArpaLmDeterministicFst const_arpa_fst(const_arpa);

        // Composes lattice with language model.
        CompactLattice composed_clat;
        ComposeCompactLatticeDeterministic(clat,
                &const_arpa_fst, &composed_clat);

        // Determinizes the composed lattice.
        Lattice composed_lat;
        ConvertLattice(composed_clat, &composed_lat);
        Invert(&composed_lat);
        //CompactLattice determinized_clat;

        DeterminizeLattice(composed_lat, &determinized_clat);
        fst::ScaleLattice(fst::GraphLatticeScale(lm_scale), &determinized_clat);
        if (determinized_clat.Start() == fst::kNoStateId ){
            KALDI_WARN << "Empty lattice for utterance " 
                << " (incompatible LM?)";
        } 
    }

    //lattice scale
    BaseFloat acoustic_scale = 1.0;
    {
        BaseFloat inv_acoustic_scale = 11.0;
        BaseFloat lm_scale = 1.0;
        BaseFloat acoustic2lm_scale = 0.0;
        BaseFloat lm2acoustic_scale = 0.0;        

        if (inv_acoustic_scale != 1.0)
            acoustic_scale = 1.0 / inv_acoustic_scale;

        std::vector<std::vector<double> > scale(2);
        scale[0].resize(2);
        scale[1].resize(2);
        scale[0][0] = lm_scale;
        scale[0][1] = acoustic2lm_scale;
        scale[1][0] = lm2acoustic_scale;
        scale[1][1] = acoustic_scale;

        ScaleLattice(scale, &determinized_clat);
    }

    //lattice add penalty
    BaseFloat word_ins_penalty = 0.0;
    {
        AddWordInsPenToCompactLattice(word_ins_penalty, &determinized_clat);
    }

    //lattice 1best
    CompactLattice best_path;
    {
        CompactLatticeShortestPath(determinized_clat, &best_path);
        int n_err = 0;
        int n_done = 0;

        if (best_path.Start() == fst::kNoStateId) {
            KALDI_WARN << "Possibly empty lattice for utterance-id " << "(no output)";
            n_err++;
        } else {
            if (word_ins_penalty > 0.0) {
                AddWordInsPenToCompactLattice(-word_ins_penalty, &best_path);
            }
            fst::ScaleLattice(fst::LatticeScale(1.0 / lm_scale, 1.0/acoustic_scale),
                    &best_path);
            n_done++;
        }      
    }

    return best_path ;
}
