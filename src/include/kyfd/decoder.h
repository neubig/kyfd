//   Copyright 2009, Kyfd Project Team
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

//
// decoder.h
//
//  A header interface to the decoder itself

#ifndef KYFD_DECODER_H__
#define KYFD_DECODER_H__

#include <vector>
#include <string>
#include <iostream>
#include <ctime>
#include <fst/arc.h>
#include <fst/vector-fst.h>
#include <kyfd/component-arc.h>
#include <kyfd/decoder-config.h>

namespace kyfd {

class Decoder {

public:
    
    typedef std::vector<std::string> Strings;

    Decoder(const DecoderConfig & config);

    ~Decoder() {
        for(int i = 0; i < stdModels_.size(); i++)
            delete stdModels_[i];
        for(int i = 0; i < compModels_.size(); i++)
            delete compModels_[i];
    }

    void buildModels();

    bool decode(std::istream& in, std::ostream& out);

    void printTimes() {
        double dub = CLOCKS_PER_SEC;
        double sum = 0;
        for(unsigned i = 0; i < timeSpent_.size(); i++)
            sum += timeSpent_[i];
        for(unsigned i = 0; i < timeSpent_.size(); i++)
            cerr << "Stage " << (i+1) << ": " << (timeSpent_[i]/dub) << " sec (" << (timeSpent_[i]/sum*100) << "%)"<<endl;
    }

private:

    // process a single sentence
    template <class A, class W, class LM>
    bool process(const std::vector< fst::Fst<A> * > & models, 
                    const std::vector< const LM* > & fallbacks,
                    std::istream & in, std::ostream & out);

    // print out the paths
    template <class A, class W>
    void printPaths(
        const fst::Fst<A> &bestFst, 
        const std::string &header,
        std::ostream & resultStream,
        bool bothInput);

    // compose and get the best paths
    template <class A, class LM>
    fst::Fst<A> * findBestPaths(const fst::Fst<A> * input, 
                                const std::vector< fst::Fst<A> * > & models,
                                const std::vector< const LM* > & fallbacks
                                 );

    // make the input fst with a template for arcs
    template <class A> 
    fst::VectorFst<A> * makeFst(std::istream & arr);
    template <class A> 
    fst::VectorFst<A> * makeFst(const Strings & arr);
    template <class W>
    W parseWeight(const std::string & str);

    // split a string into tokens
    Strings splitTokens(const std::string & input);

    const std::string & getWeightString(const fst::TropicalWeight & weight);
    const std::string & getWeightString(const fst::ComponentWeight & weight);

    // members
    DecoderConfig config_;
    Strings unknowns_;
    int sentenceId_;

    // two possible models based
    std::vector< fst::Fst<fst::ComponentArc>* > compModels_;
    std::vector< fst::Fst<fst::StdArc>* > stdModels_;

    // fallback maps for each of the models
    typedef fst::FallbackMatcher<fst::Matcher<fst::Fst<fst::ComponentArc> > >::LabelMap CompLabelMap;
    std::vector< const CompLabelMap* >    compFallbacks_;
    typedef fst::FallbackMatcher<fst::Matcher<fst::Fst<fst::StdArc> > >::LabelMap StdLabelMap;
    std::vector< const StdLabelMap* >     stdFallbacks_;

    // a string buffer
    std::string str_;
    int multiplier_;

    // debugging values to keep track of how much time is spent doing what
    unsigned timeStep_;
    std::vector<clock_t> timeSpent_;
    std::vector<clock_t> currTime_;

};

// TODO make these work with component weights as well
template<> inline
fst::ComponentWeight Decoder::parseWeight(const string & str) {
    float weight = atof(str.c_str());
    float comp[2] = { weight * config_.getWeights()[0], weight };
    return fst::ComponentWeight(2, comp); 
}

template<> inline
fst::TropicalWeight Decoder::parseWeight(const string & str) {
    return fst::TropicalWeight(atof(str.c_str()));
}

}

#endif // KYFD_DECODER_H__
