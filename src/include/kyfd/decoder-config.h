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
// decoder-config.h
//
//  Configuration for the decoder

#ifndef DECODER_CONFIG_H__
#define DECODER_CONFIG_H__

// stl classes
#include <vector>
#include <set>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <fst/arc.h>
#include <fst/symbol-table.h>
#include <kyfd/string-manager.h>
#include <kyfd/fst-node.h>
#include <kyfd/component-arc.h>
#include <kyfd/fallback-matcher.h>

//////////////////////////////////////////////////
// A class that parses KYFD configuration files //
//////////////////////////////////////////////////

namespace kyfd {

typedef enum {TEXT_INPUT, STD_INPUT, COMPONENT_INPUT} InputFormat;
typedef enum {TEXT_OUTPUT, SCORE_OUTPUT, COMPONENT_OUTPUT} OutputFormat;

class DecoderConfig;

class DecoderConfigImpl
{
public:

    friend class DecoderConfig;
    
    typedef std::vector<float> Weights;

    // ctor/dtor
    DecoderConfigImpl();
    ~DecoderConfigImpl();

private:

    // number of pointers to this impl
    int count_;

    // string buffer
    string str_;

    // members
    fst::SymbolTable* iSymbols_;
    fst::SymbolTable* oSymbols_;
    unsigned n_;
    unsigned beamWidth_;
    float trimWidth_;
    unsigned reload_;
    Weights weights_;
    InputFormat inFormat_;
    OutputFormat outFormat_;
    std::vector< FstNode<fst::ComponentArc>* > compRoots_;
    std::vector< FstNode<fst::StdArc>* > stdRoots_;
    bool printInput_;
    bool printAll_;
    bool printDuplicates_;
    bool sample_;
    bool negProb_;
    std::vector< bool > staticSearch_;
    
    // info about terminal and unknown symbols
    std::string unkSym_;
    std::string brSym_;
    int iUnkId_;
    int iBrId_;
    int oUnkId_;
    int oBrId_;

    // XML parsing objects
    xercesc::XercesDOMParser* xercesParser_;
    XercesStringManager tags_;

    std::set<std::string> hasArgument_;

};

class DecoderConfig
{
public:

    typedef DecoderConfigImpl::Weights Weights;

    // ctor/dtor
    DecoderConfig() {
        impl_ = new DecoderConfigImpl();
    }
    DecoderConfig(const DecoderConfig& conf) {
        impl_ = conf.impl_;
        impl_->count_++;
    }
    ~DecoderConfig() {
        if(--impl_->count_ == 0)
            delete impl_;
    }
    DecoderConfig & operator=(const DecoderConfig &conf) {
        impl_ = conf.impl_;
        impl_->count_++;
        
    }

    // commands to parse input
    void parseCommandLine(int argc, char** argv);
    void parseConfigFile(const char*);
    void printTree(xercesc::DOMElement* elem, int lev);
    
    // symbol functions
    fst::SymbolTable* getISymbols() const;
    fst::SymbolTable* getOSymbols() const;
    void loadISymbols(const char* fileName);
    void loadOSymbols(const char* fileName);
    int getInputId(const string & str) const {
        return ( impl_->iSymbols_ ? impl_->iSymbols_->Find(str.c_str()) : -1 );
    }
    int getOutputId(const string & str) const {
        return ( impl_->oSymbols_ ? impl_->oSymbols_->Find(str.c_str()) : -1 );
    }
    const char * getInputSymbol(int id) const {
        impl_->str_ = ( impl_->iSymbols_ ? impl_->iSymbols_->Find(id).c_str() : "" );
        return impl_->str_.c_str();
    }
    const char* getOutputSymbol(int id) const {
        impl_->str_ = ( impl_->oSymbols_ ? impl_->oSymbols_->Find(id).c_str() : "" );
        return impl_->str_.c_str();
    }

    // accessors
    unsigned getN() const { return impl_->n_; }
    void setN(unsigned n) { impl_->n_ = n; }
    unsigned getReload() const { return impl_->reload_; }
    void setReload(unsigned n) { impl_->reload_ = n; }
    bool isPrintDuplicates() const { return impl_->printDuplicates_; }
    void setPrintDuplicates(bool printDuplicates) { impl_->printDuplicates_ = printDuplicates; }
    bool isPrintInput() const { return impl_->printInput_; }
    void setPrintInput(bool printInput) { impl_->printInput_ = printInput; }
    bool isPrintAll() const { return impl_->printAll_; }
    void setPrintAll(bool printAll) { impl_->printAll_ = printAll; }
    bool isSample() const { return impl_->sample_; }
    void setSample(bool sample) { impl_->sample_ = sample; }
    bool isNegativeProbabilities() const { return impl_->negProb_; }
    void setNegativeProbabilities(bool negProb) { impl_->negProb_ = negProb; }
    bool isStaticSearch(unsigned id) const { return (id < impl_->staticSearch_.size() && impl_->staticSearch_[id]); }
    void setStaticSearch(unsigned id, bool staticSearch) { impl_->staticSearch_[id] = staticSearch; }
    unsigned getBeamWidth() const { return impl_->beamWidth_; }
    void setBeamWidth(unsigned beamWidth) { impl_->beamWidth_ = beamWidth; }
    float getTrimWidth() const { return impl_->trimWidth_; }
    void setTrimWidth(float trimWidth) { impl_->trimWidth_ = trimWidth; }
    
    // symbol functions
    const string & getUnknownSymbol() const { return impl_->unkSym_; }
    void setUnknownSymbol(const string & unkSym) { 
        impl_->unkSym_ = unkSym;
        if(impl_->iSymbols_) impl_->iUnkId_ = impl_->iSymbols_->Find(unkSym.c_str());
        else impl_->iUnkId_ = -1;
        if(impl_->oSymbols_) impl_->oUnkId_ = impl_->oSymbols_->Find(unkSym.c_str());
        else impl_->oUnkId_ = -1;
    }
    const string & getTerminalSymbol() const { return impl_->brSym_; }
    void setTerminalSymbol(const string & brSym) { 
        impl_->brSym_ = brSym; 
        if(impl_->iSymbols_) impl_->iBrId_ = impl_->iSymbols_->Find(brSym.c_str());
        else impl_->iBrId_ = -1;
        if(impl_->oSymbols_) impl_->oBrId_ = impl_->oSymbols_->Find(brSym.c_str());
        else impl_->oBrId_ = -1;
    }
    int getInputTerminalId() const { return impl_->iBrId_; }
    int getInputUnknownId() const { return impl_->iUnkId_; }
    int getOutputTerminalId() const { return impl_->oBrId_; }
    int getOutputUnknownId() const { return impl_->oUnkId_; }

    // weight functions
    Weights & getWeights() { return impl_->weights_; };
    void setWeights(const Weights & weights);
    void loadWeights(const char* fileName);

    // file format options
    OutputFormat getOutputFormat() { return impl_->outFormat_; }
    void setOutputFormat(OutputFormat outFormat) { impl_->outFormat_ = outFormat; }
    InputFormat getInputFormat() { return impl_->inFormat_; }
    void setInputFormat(InputFormat inFormat) { impl_->inFormat_ = inFormat; }

    // model functions
    int getNumModels() { 
        return (impl_->compRoots_.size() > impl_->stdRoots_.size() ? impl_->compRoots_.size() : impl_->stdRoots_.size());
    }
    const FstNode<fst::ComponentArc> * getComponentNode(unsigned id);
    const FstNode<fst::StdArc> * getStdNode(unsigned id);

private:
    
    // handle a single argument
    void handleArgument(const char* name, const char* val);
    
    // the implementation
    DecoderConfigImpl* impl_;

};

} // end namespace

#endif // DECODER_CONFIG_H__
