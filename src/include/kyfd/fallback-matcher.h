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
// fallback-matcher.cc
//
//  Similar to PhiMatcher, except with a heirarchical fall-back strategy.
//  Failure checking will be performed up until a state is found that falls
//  back to itself.

#ifndef KYFD_FALLBACK_MATCHER_H__
#define KYFD_FALLBACK_MATCHER_H__

#include <algorithm>

#include <map>
#include <fst/fst.h>
#include <fst/matcher.h>

namespace fst {

// A heirarchical failure transition model that allows multiple levels of
//  fallback for phi-transitions
template <class M>
class FallbackMatcher {
 public:
    typedef typename M::FST FST;
    typedef typename M::Arc Arc;
    typedef typename Arc::StateId StateId;
    typedef typename Arc::Label Label;
    typedef typename Arc::Weight Weight;
    typedef typename std::map<Label,Label> LabelMap;

    FallbackMatcher(const FST &fst,
                         MatchType match_type,
                         const LabelMap * fallbacks = 0,
                         bool phi_loop = true,
                         bool rewrite_both = false,
                         M *matcher = 0)
            : matcher_(matcher ? matcher : new M(fst, match_type)),
                match_type_(match_type),
                fallbacks_(fallbacks),
                state_(kNoStateId),
                rewrite_both_(rewrite_both ? true : fst.Properties(kAcceptor, true)),
                phi_loop_(phi_loop) {
        if (match_type == MATCH_BOTH)
            LOG(FATAL) << "FallbackMatcher: bad match type";
        // TODO: check compatibility with the symbol set
     }

    FallbackMatcher(const FallbackMatcher<M> &matcher)
            : matcher_(new M(*matcher.matcher_)),
                match_type_(matcher.match_type_),
                fallbacks_(matcher.fallbacks_),
                rewrite_both_(matcher.rewrite_both_),
                state_(kNoStateId),
                phi_loop_(matcher.phi_loop_) {}

    FallbackMatcher *Copy(bool safe = false) const {
        return new FallbackMatcher(*this);
    }


    ~FallbackMatcher() {
        delete matcher_;
        // this does not own its fallback map
        // if(fallbacks_)
        //     delete fallbacks_;
    }

    const FST &GetFst() const {
        return matcher_->GetFst();    
    }

    MatchType Type(bool test) const { return matcher_->Type(test); }

    void SetState(StateId s) {
        matcher_->SetState(s);
        state_ = s;
        // has_phi_ = phi_label_ != kNoLabel;
    }


    bool Find(Label match_label) {
        // if (match_label == phi_label_ && phi_label_ != kNoLabel) {
        //     LOG(FATAL) << "FallbackMatcher::Find: bad label (phi)";
        // }
        matcher_->SetState(state_);
        phi_match_in_ = kNoLabel;
        phi_match_out_ = kNoLabel;
        phi_weight_ = Weight::One();
        if (!fallbacks_ || match_label == 0 || match_label == kNoLabel)
            return matcher_->Find(match_label);
        StateId state = state_;
        while (!matcher_->Find(match_label)) {
            Label curr_label = match_label;
            while(1) {
                typename LabelMap::const_iterator labelit = fallbacks_->find(curr_label);
                if(labelit == fallbacks_->end() 
                    || curr_label == (*labelit).second 
                    || kNoLabel == (*labelit).second)
                    return false;
                curr_label = (*labelit).second;
                if(matcher_->Find(curr_label))
                    break;
            }
            phi_arc_ = matcher_->Value();
            if (phi_loop_ && phi_arc_.nextstate == state) {
                // save the things to be deleted
                if (rewrite_both_) {
                    if (phi_arc_.ilabel == curr_label)
                        phi_match_in_ = match_label;
                    if (phi_arc_.olabel == curr_label)
                        phi_match_out_ = match_label;
                } else if (match_type_ == MATCH_INPUT) {
                    phi_match_in_ = match_label;
                } else {
                    phi_match_out_ = match_label;
                }
                return true;
            }
            phi_weight_ = Times(phi_weight_, matcher_->Value().weight);
            state = matcher_->Value().nextstate;
            matcher_->SetState(state);
        }
        return true;
    }

    bool Done() const { return matcher_->Done(); }

    const Arc& Value() const {
        if ((fallbacks_ == 0) && (phi_weight_ == Weight::One())) {
            return matcher_->Value();
        } else {
            phi_arc_ = matcher_->Value();
            phi_arc_.weight = Times(phi_weight_, phi_arc_.weight);
            if (phi_match_in_ != kNoLabel)
                phi_arc_.ilabel = phi_match_in_;
            if (phi_match_out_ != kNoLabel)
                phi_arc_.ilabel = phi_match_out_;
            return phi_arc_;
        }
    }

    void Next() { matcher_->Next(); }


    uint64 Properties(uint64 props) const {
        if (match_type_ == MATCH_NONE)
            return props;
        else
            return props & ~kString;
    }
    virtual uint32 Flags() const { return 0; }

private:
    M *matcher_;
    MatchType match_type_;          // Type of match requested
    const LabelMap *fallbacks_;     // A map holding information about which label to fallback to on failure
    bool rewrite_both_;             // Rewrite both sides when both are 'phi_label_'
    Label phi_label_;               // Label that represents the phi transition
    Label phi_match_in_;            // A label that should replace a phi-matched arc's input
    Label phi_match_out_;           // A label that should replace a phi-matched arc's output
    mutable Arc phi_arc_;           // Arc to return
    StateId state_;                 // State where looking for matches
    Weight phi_weight_;             // Product of the weights of phi transitions taken
    bool phi_loop_;                 // When true, phi self-loop are allowed and treated
                                                    // as rho (required for Aho-Corasick)

    void operator=(const FallbackMatcher<M> &);    // disallow
};

}    // namespace fst



#endif    // FST_LIB_MATCHER_H__
