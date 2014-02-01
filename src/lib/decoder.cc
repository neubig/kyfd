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
// decoder.cc
//
//  The main body of the decoder code

#include <fst/rmepsilon.h>
#include <fst/vector-fst.h>
#include <fst/shortest-path.h>
#include <fst/project.h>
#include <fst/compose.h>
#include <kyfd/decoder.h>
#include <kyfd/beam-trim.h>
#include <kyfd/sampgen.h>

using namespace std;
using namespace fst;
using namespace kyfd;


Decoder::Decoder(const DecoderConfig & config) : 
    sentenceId_(0), compModels_(), stdModels_(), compFallbacks_(), stdFallbacks_(), config_(config) {

    // initialize the time values
    int NUM_TIMES = 9;
    currTime_.push_back(0);
    for(int i = 0; i < NUM_TIMES; i++) {
        currTime_.push_back(0);
        timeSpent_.push_back(0);
    }
    
    // get whether or not to reverse the sign
    multiplier_ = ( config_.isNegativeProbabilities() ? -1 : 1 );

    buildModels();

}

void Decoder::buildModels() {
    // load the model
    if(config_.getOutputFormat() == COMPONENT_OUTPUT) {
        for(unsigned i = 0; i < compModels_.size(); i++)
            delete compModels_[i];
        compFallbacks_.clear();
        compModels_.clear();
        for(unsigned i = 0; i < config_.getNumModels(); i++) {
            const FstNode<ComponentArc> * myNode = config_.getComponentNode(i);
            compModels_.push_back(myNode->buildFst());
            compFallbacks_.push_back(myNode->getFallbackMap());
        }
    }
    else {
        for(unsigned i = 0; i < stdModels_.size(); i++)
            delete stdModels_[i];
        stdFallbacks_.clear();
        stdModels_.clear();
        for(unsigned i = 0; i < config_.getNumModels(); i++) {
            const FstNode<StdArc> * myNode = config_.getStdNode(i);
            stdModels_.push_back(myNode->buildFst());
            stdFallbacks_.push_back(myNode->getFallbackMap());
        }
    }
}

bool Decoder::decode(istream& in, ostream& out) {
    timeStep_ = 0;
    currTime_[timeStep_++] = clock();

    if(config_.getOutputFormat() == COMPONENT_OUTPUT)
        return process<ComponentArc, ComponentWeight, CompLabelMap>(compModels_, compFallbacks_, in, out);
    else
        return process<StdArc, TropicalWeight, StdLabelMap>(stdModels_, stdFallbacks_, in, out);
}

template <class A, class W, class LM>
bool Decoder::process(const vector< Fst<A>* > & models,
                        const std::vector< const LM* > & fallbacks,
                        istream & in, ostream & out) {
    currTime_[timeStep_++] = clock();
    Fst<A> * input = makeFst<A>(in);
    if(input == NULL)
        return false;
    currTime_[timeStep_++] = clock();
    Fst<A> * best = findBestPaths<A>(input, models, fallbacks);
    currTime_[timeStep_++] = clock();
    // if nothing could be found, print the input
    bool hasAnswer = best->Start() != kNoStateId;
    if(!hasAnswer) {
        // printPaths<A,W>(*input,"WARNING, no path found for: ",cerr);
        cerr  << "WARNING, no path found" << endl;
        Fst<A> * swapPtr = input; input = best; best = swapPtr;
    }
    ostringstream buff;
    if(config_.getN() > 1)
        buff << sentenceId_ << "|||"; 
    string str = buff.str();
    W weight = W::One();
    printPaths<A,W>(*best, str, out, !hasAnswer);
    currTime_[timeStep_++] = clock();
    if(input != best)
        delete input;
    delete best;
    currTime_[timeStep_++] = clock();
    for(unsigned i = 0; i < currTime_.size()-1; i++)
        timeSpent_[i] += (currTime_[i+1]-currTime_[i]);
    sentenceId_++;
    return true;
}

const string & Decoder::getWeightString(const ComponentWeight & weight) {
    ostringstream buff;
    for(unsigned short i = 1; i < weight.getWidth(); i++)
        buff << (weight.getComponent(i) * multiplier_) << " ";
    buff << "||| " << (weight.Value() * multiplier_);
    str_ = buff.str();
    return str_;
}

const string & Decoder::getWeightString(const TropicalWeight & weight) {
    ostringstream buff;
    buff << (weight.Value() * multiplier_);
    str_ = buff.str();
    return str_;
}

template <class A, class W>
void Decoder::printPaths(
    const Fst<A> &bestFst,
    const string &header,
    ostream & resultStream,
    bool bothInput) {
    
    // loop through all the paths
    for(ArcIterator< Fst<A> > aiter(bestFst, bestFst.Start()); !aiter.Done(); aiter.Next()) {

        unsigned oUnkId = 0, iUnkId = 0;
        W weight = W::One();
        A arc = aiter.Value();
        bool ok = true;
        bool printed = header.length() > 0;
        if(printed)
            resultStream << header;
        ostringstream input;

        while(ok) {
            weight = Times(weight, arc.weight);
            if(config_.isPrintAll()) {
                if(arc.ilabel != 0 || arc.olabel != 0) {
                    if(printed)
                        resultStream << " ";
	            	if(arc.ilabel==config_.getInputUnknownId())
                        resultStream << unknowns_[iUnkId++];
	            	else 
                        resultStream << config_.getInputSymbol(arc.ilabel);
                    resultStream << "|";
	            	if(arc.olabel==config_.getOutputUnknownId())
                        resultStream << unknowns_[oUnkId++];
	            	else 
                        resultStream << config_.getOutputSymbol(arc.olabel);
                    printed = true;
                }
            } else {
                // print the input
	            if(config_.isPrintInput() && arc.ilabel != 0 && arc.ilabel != config_.getInputTerminalId()) {
	            	if(arc.ilabel==config_.getInputUnknownId())
                        input << " " << unknowns_[iUnkId++];
	            	else 
                        input << " " << config_.getInputSymbol(arc.ilabel);
	            }
                // print the output
	            if(arc.olabel != 0 && arc.olabel != (bothInput?config_.getInputTerminalId():config_.getOutputTerminalId())) {
                    if(printed)
                        resultStream << " ";
	            	if(arc.olabel==(bothInput?config_.getInputUnknownId():config_.getOutputUnknownId())) {
                        if(oUnkId < 0 || oUnkId >= unknowns_.size())
                            throw runtime_error("Unmatched number of unknown symbols in output");
                        resultStream << unknowns_[oUnkId++];
                    }
	            	else 
                        resultStream << (bothInput?config_.getInputSymbol(arc.olabel):config_.getOutputSymbol(arc.olabel));
                    printed = true;
	            }
            }
            // get the next value
            ArcIterator< Fst<A> > aiter2(bestFst, arc.nextstate);
            ok = !aiter2.Done();
            if(ok)
                arc = aiter2.Value();
        }

        // print the weight if necessary
        if(config_.isPrintInput())
            resultStream << " |||" << input.str();
        if(config_.getOutputFormat() != TEXT_OUTPUT)
            resultStream << " ||| " << getWeightString(weight);
        resultStream << endl;

    }

}

// compose and get the best paths
template <class A, class LM>
fst::Fst<A> * Decoder::findBestPaths(const fst::Fst<A> * input, 
                                     const std::vector< fst::Fst<A> * > & models,
                                     const std::vector< const LM* > & fallbacks) {
    
    typedef fst::FallbackMatcher< fst::Matcher<fst::Fst<A> > > FB;

    currTime_[timeStep_++] = clock();
    
    // compose the models in order
    Fst<A> * searchFst = new VectorFst<A>(*input);
    for(unsigned i = 0; i < models.size(); i++) {
        fst::ComposeFstOptions<A, FB> copts(CacheOptions(),
                      new FB(*searchFst, ( fallbacks[i] == 0 ? fst::MATCH_OUTPUT : fst::MATCH_NONE ) ),
                      new FB(*models[i], fst::MATCH_INPUT, fallbacks[i]));
        Fst<A> * nextFst = new ComposeFst<A>(*searchFst, *models[i], copts);
        delete searchFst;
        if(nextFst->Start() == kNoStateId)
            return nextFst;
        if(config_.isStaticSearch(i)) {
            VectorFst<A> * vecFst = new VectorFst<A>(*nextFst);
            delete nextFst;
            Connect(vecFst);
            nextFst = vecFst;
        }
        searchFst = nextFst;
    }

    currTime_[timeStep_++] = clock();
    // trim down the FST if necessary
    if(config_.getBeamWidth() + config_.getTrimWidth() > 0) {
        VectorFst<A> * trimFst = new VectorFst<A>;
        if(config_.getBeamWidth())
            BeamTrim(*searchFst, trimFst, config_.getBeamWidth());
        else
            Prune(*searchFst, trimFst, config_.getTrimWidth());
        delete searchFst;
        searchFst = trimFst;
    }
    
    currTime_[timeStep_++] = clock();
    // remove duplicate paths if called for
    bool removeDup = !config_.isPrintDuplicates() && !(config_.isPrintAll() || config_.isPrintInput());
    if(config_.getN() > 1 && removeDup) {
        VectorFst<A> * vecFst = new VectorFst<A>(*searchFst);
        Project(vecFst, PROJECT_OUTPUT);
        RmEpsilon(vecFst);
        delete searchFst;
        searchFst = vecFst;
    }
    
    currTime_[timeStep_++] = clock();
    // find the shortest path, or sample as necessary
    VectorFst<A> * bestFst = new VectorFst<A>;
    if(config_.isSample()) {
        SampGen(*searchFst, *bestFst, config_.getN());
    } else {
        ShortestPath(*searchFst, bestFst, config_.getN(), removeDup);
    }
    delete searchFst;
    
    return bestFst;

}

// load an FST from the input stream
template <class A>
VectorFst<A> * Decoder::makeFst(istream &in) {
    typedef typename A::Weight Weight;
    string line;
    // flat input
    if(config_.getInputFormat() == TEXT_INPUT) {
        if(!getline(in, line))
            return NULL;
        Strings tokens = splitTokens(line);
        return makeFst<A>(tokens);
    } 
    // fst input
    else {
        VectorFst<A> * ret = new VectorFst<A>;
        int start = -1;
        while(getline(in, line) && line.length() > 0) {
            Strings tokens = splitTokens(line);
            if(tokens.size() != 1 && tokens.size() != 4 && tokens.size() != 5)
                throw runtime_error("Bad number of columns in FST input");
            // get the states
            int inState = atoi(tokens[0].c_str());
            int outState = (tokens.size()==1 ? 0 : atoi(tokens[1].c_str()));
            // cerr << "inState="<<inState<<", outState="<<outState;
            while(ret->NumStates() <= max(inState, outState))
                ret->AddState();
            if(start == -1) {
                start = inState;
                ret->SetStart(inState);
            }
            // add a final state  
            Weight weight;
            if(tokens.size() == 1) {
                // cerr << " Setting final weight to: " << weight;
                ret->SetFinal(inState, Weight::One());
            }
            // add an arc
            else {
		        int inSym = config_.getInputId(tokens[2]);
		        int outSym = config_.getInputId(tokens[3]);
                // cerr << ", inSym="<<inSym<<", outSym="<<outSym;
                if(inSym == -1 || outSym == -1) {
                    ostringstream ss;
                    ss << "Unknown symbol '" << tokens[(inSym==-1?2:3)] << "' found in FST input" << endl;
                    throw runtime_error(ss.str());
                }
                if(tokens.size() == 5)
                    weight = parseWeight<Weight>(tokens[4]);
                else
                    weight = Weight::One();
                // cerr << ", weight="<<weight;
		        ret->AddArc(inState, A(inSym, outSym, weight, outState));
            }
            // cerr << endl;

        }
        if(start == -1) {
            delete ret;
            ret = NULL;
        } 
        return ret;
    }
}

// make the input fst with a template for arcs
template <class A> 
VectorFst<A> * Decoder::makeFst(const Strings & arr) {
 
	// create the input FST
	VectorFst<A> * inputFst = new VectorFst<A>();
	typename A::StateId nextState;
    typename A::StateId currState = inputFst->AddState();
	inputFst->SetStart(currState);

    // get the ids
    int unkId = config_.getInputUnknownId();
    int brId = config_.getInputTerminalId();

    // clear the vector of unknown words
    unknowns_.clear();

	// get the IDs for all the tokens
	for(Strings::const_iterator it = arr.begin(); it != arr.end(); it++) {
		// find the token, unknown if necessary
		int id = config_.getInputId(*it);
		if(id == -1) {
            if(unkId == -1) 
                throw runtime_error("Unknown symbol exists in input, but no unknown ID set");
            unknowns_.push_back(*it);
			id = unkId;
        }
		// create the next state and a link to it
		nextState = inputFst->AddState();
		inputFst->AddArc(currState, A(id, id, A::Weight::One(), nextState));
		currState = nextState;
	}

    // add a link with the final symbol
    if(brId != -1) {
		nextState = inputFst->AddState();
		inputFst->AddArc(currState, A(brId, brId, A::Weight::One(), nextState));
		currState = nextState;
    }

    // set the final state
    inputFst->SetFinal(currState, A::Weight::One());

    return inputFst;

}

// split a string into tokens
Decoder::Strings Decoder::splitTokens(const string & input) {
    istringstream iss(input);
    Strings ret;
    string buff;
    while(iss >> buff)
        ret.push_back(buff);
    return ret; 
}
