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
// fst-node.h
//
//  A node holding information about an FSTs place in the heirarchical tree
//   structure.

#ifndef _FST_NODE_H__
#define _FST_NODE_H__

#include <list>
#include <iterator>
#include <stdexcept>
#include <fst/vector-fst.h>
#include <fst/compose.h>
#include <fst/encode.h>
#include <fst/intersect.h>
#include <fst/determinize.h>
#include <fst/minimize.h>
#include <fst/project.h>
#include <fst/arcsort.h>
#include <kyfd/fallback-matcher.h>
#include <kyfd/component-arc.h>
#include <kyfd/component-map.h>

namespace kyfd {

//////////////
// A node to hold the configuration for various fsts
//////////////
template <class A>
class FstNode {

public:

    // typedef NodeList::iterator NodeIterator;
    enum Operation { PLAIN, COMPOSE, INTERSECT, MINIMIZE, DETERMINIZE, PROJECT, ARCSORT };
    enum Method { STATIC, DYNAMIC };
    
    const static int kDirectionOutput = 1;
    typedef typename fst::FallbackMatcher<fst::Matcher<fst::Fst<A> > >::LabelMap LabelMap;

private:

    // data
    string name_;
    string file_;
    Operation operation_;
    Method method_;
    int properties_;
    int id_;
    float weight_;
    LabelMap * fbMap_;

    FstNode<A>* leftChild_;
    FstNode<A>* rightChild_;

public:

    // ctor
    FstNode() : id_(-1), properties_(0), operation_(PLAIN), method_(STATIC), weight_(1.0), leftChild_(0), rightChild_(0), fbMap_(0) { };
   
    // dtor 
    ~FstNode() {
        if(leftChild_) delete leftChild_;
        if(rightChild_) delete rightChild_;
        leftChild_ = 0;
        rightChild_ = 0;    
    }

    // accessors
    Operation getOperation() const { return operation_; }
    void setOperation(Operation operation) { operation_ = operation; }
    Method getMethod() const { return method_; }
    void setMethod(Method method) { method_ = method; }
    int getProperties() const { return properties_; }
    void setProperties(int properties) { properties_ = properties; }
    const string & getFile() const { return file_; }
    void setFile(const string &file) { file_ = file; }
    const string & getName() const { return name_; }
    void setName(const string &name) { name_ = name; }
    int getId() const { return id_; }
    void setId(int id) { id_ = id; }
    const LabelMap * getFallbackMap() const { return fbMap_; }
    void setFallbackMap(const LabelMap & fbMap) { 
        if(fbMap_) delete fbMap_;
        fbMap_ = new LabelMap(fbMap);
    }
    FstNode<A>* getRight() { return rightChild_; }
    FstNode<A>* getLeft() { return leftChild_; }

    /**
     * A function to add a child node
     */
    void addChild(FstNode<A>* child) {
        if(!leftChild_) leftChild_ = child;
        else if (!rightChild_) rightChild_ = child;
        else throw std::runtime_error( "More than two children added to a single FST node" );
    }

    /**
     * load an FST from a file and convert it to the proper format
     */
    fst::Fst<A> * loadFst() const;

    /**
     * A function to build an FST
     */
    fst::Fst<A> * buildFst() const {
    
        fst::Fst<A> * ret;
        fst::VectorFst<A> * vecRet;
        typedef fst::FallbackMatcher< fst::Matcher<fst::Fst<A> > > FB;
        typedef fst::CacheOptions CacheOptions;
    
        // for plain FSTs
        if(operation_ == PLAIN) {
            cerr << "Loading fst " << name_ << "... " << endl;
            return loadFst();
        }
        // for multiple operations
        if (operation_ == COMPOSE || operation_ == INTERSECT) {
            assert(leftChild_ && rightChild_);
            fst::Fst<A> * leftFst = leftChild_->buildFst();
            fst::Fst<A> * rightFst = rightChild_->buildFst();
            cerr << ( operation_ == COMPOSE ? "Composing" : "Intersecting" ) 
                 << " fsts " << leftChild_->getName() << " and " << rightChild_->getName() << " "
                 << (method_ == STATIC ? "statically" : "dynamically" ) << "... ";
            if(method_ == STATIC) {
                fst::VectorFst<A> * vecRet = NULL;
                if(operation_ == COMPOSE) {
                    fst::ComposeFstOptions<A, FB> copts(CacheOptions(),
                                  new FB(*leftFst,  ( rightChild_->getFallbackMap() == 0 ? fst::MATCH_OUTPUT : fst::MATCH_NONE ) ),
                                  new FB(*rightFst, fst::MATCH_INPUT, rightChild_->getFallbackMap()));
                    fst::ComposeFst<A> compFst(*leftFst, *rightFst, copts);
                    vecRet = new fst::VectorFst<A>(compFst);
                    Connect(vecRet);
                }
                else {
                    vecRet = new fst::VectorFst<A>;
                    fst::EncodeMapper<A> encoder(fst::kEncodeLabels, fst::ENCODE);
                    fst::VectorFst<A> leftVec(*leftFst);
                    fst::VectorFst<A> rightVec(*rightFst);
                    fst::Encode(&leftVec, &encoder);
                    fst::Encode(&rightVec, &encoder);
                    fst::ArcSort(&leftVec, fst::OLabelCompare<A>());
                    fst::Intersect(leftVec, rightVec, vecRet);
                    fst::Decode(vecRet, encoder);
                }
                ret = vecRet;
            }
            else if (operation_ == COMPOSE) {
                fst::ComposeFstOptions<A, FB> copts(CacheOptions(),
                              new FB(*leftFst, ( rightChild_->getFallbackMap() == 0 ? fst::MATCH_OUTPUT : fst::MATCH_NONE ) ),
                              new FB(*rightFst, fst::MATCH_INPUT, rightChild_->getFallbackMap()));
                ret = new fst::ComposeFst<A>(*leftFst, *rightFst, copts);
            }
            else {
                fst::EncodeMapper<A> encoder(fst::kEncodeLabels, fst::ENCODE);
                fst::EncodeFst<A> leftEnc(*leftFst, &encoder);
                fst::EncodeFst<A> rightEnc(*rightFst, &encoder);
                fst::ArcSortFst<A, fst::OLabelCompare<A> > leftSort(leftEnc, fst::OLabelCompare<A>());
                fst::IntersectFst<A> interFst(leftSort, rightEnc);
                ret = new fst::DecodeFst<A>(interFst, encoder);
            }
            delete leftFst;
            delete rightFst;
        }
        // for single operations
        else if(operation_ == MINIMIZE || operation_ == DETERMINIZE || operation_ == PROJECT || operation_ == ARCSORT) {
            assert(leftChild_);
            fst::Fst<A> * childFst = leftChild_->buildFst();
            if(operation_ == MINIMIZE) cerr << "Minimizing ";
            else if(operation_ == DETERMINIZE) cerr << "Determinizing ";
            else if(operation_ == PROJECT) cerr << "Projecting ";
            else cerr << "Arc sorting ";
            cerr << " fst " << leftChild_->getName() << "... ";
            if(method_ == STATIC) {
                fst::VectorFst<A> * vecRet = new fst::VectorFst<A>(*childFst);

                if (operation_ == DETERMINIZE){
                    vecRet = new fst::VectorFst<A>;
                    fst::Determinize(*childFst, vecRet);
                }
                else {
                    vecRet = new fst::VectorFst<A>(*childFst);
                    if(operation_ == MINIMIZE)
                        fst::Minimize(vecRet);
                    else if(operation_ == PROJECT) 
                        fst::Project(vecRet, (properties_ & kDirectionOutput?fst::PROJECT_OUTPUT:fst::PROJECT_INPUT));
                    else { // _operation == ARCSORT
                        if(properties_ & kDirectionOutput)
                            fst::ArcSort(vecRet, fst::ILabelCompare<A>() );
                        else
                            fst::ArcSort(vecRet, fst::OLabelCompare<A>() );
                    }

                }
                ret = vecRet;
            }
            else {
                if(operation_ == MINIMIZE)
                    throw std::runtime_error("Dynamic minimization is not possible");
                else if(operation_ == DETERMINIZE)
                    ret = new fst::DeterminizeFst<A>(*childFst);
                else if(operation_ == PROJECT)
                    ret = new fst::ProjectFst<A>(*childFst, (properties_ & kDirectionOutput?fst::PROJECT_OUTPUT:fst::PROJECT_INPUT));
                else { // operation_ == ARCSORT
                    if(properties_ & kDirectionOutput)
                        ret = new fst::ArcSortFst< A, fst::OLabelCompare<A> >(*childFst, fst::OLabelCompare<A>());
                    else
                        ret = new fst::ArcSortFst< A, fst::ILabelCompare<A> >(*childFst, fst::ILabelCompare<A>());
                }
            }
            delete childFst;
        }
        else
            throw std::runtime_error( "Unknown operation for fst::FstNode" );
    
        cerr << "done" << endl;

        return ret;
    
    }
    
    /**
     * A function to build an FST
     */
    void adjustWeights(const std::vector<float> & weights) {

        if(id_ >= 0) {
            if(id_ >= weights.size()) {
                std::ostringstream buff;
                buff << "An FST id (" << id_ << ") larger than the weight vector (" << weights.size() << ") exists";
                throw std::runtime_error( buff.str() );
            }
            weight_ = weights[id_];
        }
        if(leftChild_) leftChild_->adjustWeights(weights);
        if(rightChild_) rightChild_->adjustWeights(weights);

    }

};

template<> inline 
fst::Fst<fst::ComponentArc> * FstNode<fst::ComponentArc>::loadFst() const {
    fst::StdFst * temp = fst::StdFst::Read(file_.c_str());
    fst::VectorFst<fst::ComponentArc> * ret = new fst::VectorFst<fst::ComponentArc>();
    fst::Map(*temp, ret, fst::WeightedComponentMapper(id_, weight_));
    delete temp;
    return ret;
}

template<> inline
fst::StdFst * FstNode<fst::StdArc>::loadFst() const {
    fst::StdFst * temp = fst::StdFst::Read(file_.c_str());
    fst::VectorFst<fst::StdArc> * ret = new fst::VectorFst<fst::StdArc>();
    fst::Map(*temp, ret, fst::WeightedMapper(weight_));
    delete temp;
    return ret;
}

}

#endif
