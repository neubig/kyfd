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
// decoder-config.cc
//
//  Contains routines to handle the decoder configuration file and command
//  line input

#include <fst/vector-fst.h>

#include <kyfd/string-manager.h>
#include <kyfd/decoder-config.h>

#include <algorithm>
#include "config.h"

using namespace std;
using namespace xercesc;
using namespace fst;
using namespace kyfd;

//////////////////
// DecoderConfigImpl //
//////////////////

// ctor/dtor
DecoderConfigImpl::DecoderConfigImpl() : 
    compRoots_(), stdRoots_(), iSymbols_(0), oSymbols_(0), n_(1),
    iUnkId_(-1), iBrId_(-1), oUnkId_(-1), oBrId_(-1), count_(1),
    beamWidth_(0), trimWidth_(0), printDuplicates_(false), printInput_(false), 
    printAll_(false), sample_(false), negProb_(false), staticSearch_(), reload_(0), 
    inFormat_(TEXT_INPUT), outFormat_(TEXT_OUTPUT) {
    
    // set up xerces infrastructure
    XMLPlatformUtils::Initialize();

    // load the parser
    xercesParser_ = new XercesDOMParser;

}

// dtor
DecoderConfigImpl::~DecoderConfigImpl() {
    
    // dkelete the data
    if(iSymbols_) delete iSymbols_;
    if(oSymbols_) delete oSymbols_;

    iSymbols_ = 0;
    oSymbols_ = 0;

    for(int i = 0; i < stdRoots_.size(); i++)
        delete stdRoots_[i];
    for(int i = 0; i < compRoots_.size(); i++)
        delete compRoots_[i];

    // de-allocate all the strings
    tags_.drain(); 
    
    delete xercesParser_;

    // terminate Xerces
    try { XMLPlatformUtils::Terminate(); }
    catch( XMLException& e ) {
        cerr << "XML toolkit teardown error: " << tags_.convert(e.getMessage()) << endl;
    }
}

// populate a fallback map
template <class LM>
void PopulateMap(const char* fileName, LM & map) {
    
    ifstream instr(fileName);
    if(!instr) {
        ostringstream buff;
        buff << "Fallback map file file '" << fileName << "' could not be found.";
        throw runtime_error(buff.str());
    }

    // TODO: make this robust
    int size = 1024;
    char buff[size];
    typename LM::key_type label1, label2;
    while(instr.getline(buff, size)) {
        istringstream iss(buff);
        iss >> label1 >> label2;
        map[label1] = label2;
    }

    instr.close();

}

// parse an FstNode
template <class A>
FstNode<A> * ParseNode(const DOMElement* elem, XercesStringManager &tags_) {

    typedef typename FstNode<A>::LabelMap LabelMap;

    // initialize
    FstNode<A> * ret = new FstNode<A>(); 

    // get the type of the FST
    char* str = tags_.convert(elem->getAttribute(tags_.convert("type")));
    if(strlen(str) == 0 || 0 == strcmp(str, "plain")) ret->setOperation(FstNode<A>::PLAIN);
    else if(0 == strcmp(str, "compose")) ret->setOperation(FstNode<A>::COMPOSE);
    else if(0 == strcmp(str, "intersect")) ret->setOperation(FstNode<A>::INTERSECT);
    else if(0 == strcmp(str, "minimize")) ret->setOperation(FstNode<A>::MINIMIZE);
    else if(0 == strcmp(str, "determinize")) ret->setOperation(FstNode<A>::DETERMINIZE);
    else if(0 == strcmp(str, "project")) ret->setOperation(FstNode<A>::PROJECT);
    else if(0 == strcmp(str, "arcsort")) ret->setOperation(FstNode<A>::ARCSORT);
    else throw runtime_error( "Unknown type in FST tree" );

    // get the phi value
    DOMAttr* fallbackNode = elem->getAttributeNode(tags_.convert("fallback"));
    if(fallbackNode) {
        LabelMap map;
        cerr << "Loading fallback file: " << tags_.convert(fallbackNode->getValue()) << endl;
        PopulateMap<LabelMap>(tags_.convert(fallbackNode->getValue()), map);
        ret->setFallbackMap( map );
    }
    
    // if this is a plain FST get the file and read in
    if(ret->getOperation() == FstNode<A>::PLAIN) {
        // set the file to import from
        ret->setFile( tags_.convert(elem->getAttribute(tags_.convert("file"))) );
        // if the weight element exists, set the initial weight to adjust, otherwise don't
        DOMAttr* idNode = elem->getAttributeNode(tags_.convert("id"));
        if(idNode)
            ret->setId( atoi(tags_.convert(idNode->getValue())) );
        // get the name, or use the file name if it doesn't exist
        DOMAttr* nameNode = elem->getAttributeNode(tags_.convert("name"));
        ret->setName( nameNode ? tags_.convert(nameNode->getValue()) : ret->getFile().c_str() );
    }
    // otherwise, it's an operation
    else {
        // get the method
        char* meth = tags_.convert(elem->getAttribute(tags_.convert("method")));
        if(strlen(meth) == 0 || 0 == strcmp(meth, "static")) ret->setMethod(FstNode<A>::STATIC);
        else if(0 == strcmp(meth, "dynamic")) ret->setMethod(FstNode<A>::DYNAMIC);
        else throw runtime_error( "Unknown method type in FST tree" );
       
        // get the type of projection 
        char* project = tags_.convert(elem->getAttribute(tags_.convert("direction")));
        if(strlen(project) == 0 || 0 == strcmp(project, "output")) 
            ret->setProperties(ret->getProperties() | FstNode<A>::kDirectionOutput);
        else if(0 != strcmp(project, "input"))
            throw runtime_error( "Unknown projection type in FST tree" );

        DOMNodeList* nodeList = elem->getChildNodes(); 
        XMLSize_t size = nodeList->getLength();
        // get the remainder of the nodes
        for(XMLSize_t i = 0; i < size; i++) {
            DOMNode* currNode = nodeList->item(i);
            if( currNode->getNodeType() && currNode->getNodeType() == DOMNode::ELEMENT_NODE )
                ret->addChild( ParseNode<A>(dynamic_cast< DOMElement* >( currNode ), tags_) );
        }
        // check for validity
        if(ret->getOperation() == FstNode<A>::COMPOSE || ret->getOperation() == FstNode<A>::INTERSECT) {
            if (!ret->getRight())
                throw runtime_error("Exactly 2 FSTs must be present for intersection or composition");
            if (ret->getLeft()->getFallbackMap() != 0)
                throw runtime_error("Only the right side of a composed FST may be assigned a phi value");
        }
        else if (ret->getRight() || !ret->getLeft()) {
            ostringstream buff;
            buff << "Not exactly one child in " << str << " operation";
            throw runtime_error(buff.str());
        }
        DOMAttr* nameNode = elem->getAttributeNode(tags_.convert("name"));
        ret->setName( nameNode ? tags_.convert(nameNode->getValue()) : str );
    }
    
    return ret;
}

// handle a single configuration argument
void DecoderConfig::handleArgument(const char* name, const char* val) {

    // if this argument has already been handled already, skip it
    if(impl_->hasArgument_.count(name))
        return;
    impl_->hasArgument_.insert(name);

    if(!strcmp(name, "isymbols")) {
        impl_->iSymbols_ = SymbolTable::ReadText(val);
        if(!impl_->iSymbols_) throw runtime_error( "Error reading input symbol table" );
    }
    else if(!strcmp(name, "osymbols")) {
        impl_->oSymbols_ = SymbolTable::ReadText(val);
        if(!impl_->oSymbols_) throw runtime_error( "Error reading output symbol table" );
    }
    // TODO: make these more robust
    else if(!strcmp(name, "nbest"))
        impl_->n_ = atoi(val);
    else if(!strcmp(name, "weights")) {
        impl_->weights_.clear();
        // TODO: this is a dirty hack, do it better
        int len = strlen(val);
        char * newVal = new char[len+1];
        strcpy(newVal, val);
        for(int i = 0; i < len; i++)
            if(newVal[i] == ',')
                newVal[i] = ' ';
        istringstream iss(newVal);
        string buff;
        while(iss >> buff)
            impl_->weights_.push_back(atof(buff.c_str()));
        delete[] newVal;
    }
    else if(!strcmp(name, "staticsearch")) {
        impl_->staticSearch_.clear();
        // TODO: this is a dirty hack, do it better
        int len = strlen(val);
        char * newVal = new char[len+1];
        strcpy(newVal, val);
        for(int i = 0; i < len; i++)
            if(newVal[i] == ',')
                newVal[i] = ' ';
        istringstream iss(newVal);
        string buff;
        while(iss >> buff)
            impl_->staticSearch_.push_back(!strcmp(buff.c_str(), "true"));
        delete[] newVal;
    }
    else if(!strcmp(name, "output")) {
        if(!strcmp(val, "text")) impl_->outFormat_ = TEXT_OUTPUT;
        else if(!strcmp(val, "score")) impl_->outFormat_ = SCORE_OUTPUT;
        else if(!strcmp(val, "component")) impl_->outFormat_ = COMPONENT_OUTPUT;
        else throw runtime_error( "Bad output format specified" );
    }
    else if(!strcmp(name, "input")) {
        if(!strcmp(val, "text")) impl_->inFormat_ = TEXT_INPUT;
        else if(!strcmp(val, "std")) impl_->inFormat_ = STD_INPUT;
        else if(!strcmp(val, "component")) impl_->inFormat_ = COMPONENT_INPUT;
        else throw runtime_error( "Bad input format specified" );
    }
    else if(!strcmp(name, "unknown"))
        setUnknownSymbol(val);
    else if(!strcmp(name, "terminal"))
        setTerminalSymbol(val);
    else if(!strcmp(name, "duplicates"))
        setPrintDuplicates(!strcmp(val, "true"));
    else if(!strcmp(name, "printin"))
        setPrintInput(!strcmp(val, "true"));
    else if(!strcmp(name, "printall"))
        setPrintAll(!strcmp(val, "true"));
    else if(!strcmp(name, "sample"))
        setSample(!strcmp(val, "true"));
    else if(!strcmp(name, "negprob"))
        setNegativeProbabilities(!strcmp(val, "true"));
    else if(!strcmp(name, "beam")) {
        if(getTrimWidth() != 0.0)
            throw runtime_error( "Cannot set both a beam width and trimming width" );
        setBeamWidth(atoi(val));
    }
    else if(!strcmp(name, "trim")) {
        if(getBeamWidth() != 0)
            throw runtime_error( "Cannot set both a beam width and trimming width" );
        setTrimWidth(atof(val));
    }
    else if(!strcmp(name, "reload"))
        setReload(atoi(val));
    else {
        ostringstream buff;
        buff << "Bad argument " << name;
        throw runtime_error( buff.str() );
    }
}

// parse the configuration file
void DecoderConfig::parseConfigFile(const char* fileName) {

    if(!ifstream(fileName)) {
        ostringstream buff;
        buff << "Configuration file '" << fileName << "' could not be found.";
        throw runtime_error(buff.str());
    }

    // Configure DOM parser.
    impl_->xercesParser_->setValidationScheme( XercesDOMParser::Val_Never );
    impl_->xercesParser_->setDoNamespaces( false );
    impl_->xercesParser_->setDoSchema( false );
    impl_->xercesParser_->setLoadExternalDTD( false );
    impl_->xercesParser_->parse( fileName );

    // no need to free this pointer - owned by the parent parser object
    DOMDocument* xmlDoc = impl_->xercesParser_->getDocument();

    // Get the top-level element: Name is "config". No attributes for "config"
    DOMElement* elementRoot = xmlDoc->getDocumentElement();
    if( !elementRoot ) throw(std::runtime_error( "empty XML document" ));
    if( !impl_->tags_.streq(elementRoot->getTagName(), "kyfd") )
        throw(std::runtime_error( "The input file must be of type kyfd" ));
    
    // print the document tree recursively
    // printTree(elementRoot, 0);
    
    // go through and handle the nodes
    DOMNodeList* domList = elementRoot->getChildNodes();
    for(XMLSize_t i = 0; i < domList->getLength(); i++) {
        DOMNode* currNode = domList->item(i);
        if( !currNode->getNodeType() || currNode->getNodeType() != DOMNode::ELEMENT_NODE )
            continue;
        DOMElement* node = dynamic_cast< DOMElement* >( currNode );
        char* name = impl_->tags_.convert(node->getTagName());
        // handle arguments
        if(!strcmp(name, "arg")) {
            char* nodeName = impl_->tags_.convert(node->getAttribute(impl_->tags_.convert("name")));
            char* nodeValue = impl_->tags_.convert(node->getAttribute(impl_->tags_.convert("value")));
            handleArgument(nodeName, nodeValue);
        }
        // handle the fst function
        else if(!strcmp(name, "fst")) {
            // load in component format if we need to track individual values, std format if not
            if(impl_->outFormat_ == COMPONENT_OUTPUT)
                impl_->compRoots_.push_back( ParseNode<ComponentArc>(node, impl_->tags_) );
            else
                impl_->stdRoots_.push_back( ParseNode<StdArc>(node, impl_->tags_) );
        }
        else {
            ostringstream buff;
            buff << "illegal tag '" << name << "' in configuration file";
            throw runtime_error(buff.str());
        }
    }
 
}


// print the XML tree
void DecoderConfig::printTree(DOMElement* elem, int lev) {

    // print this element's level
    for(int i = 0; i < lev; i++)
        cout << ' ';
    cout << impl_->tags_.convert(elem->getTagName()) << ": ";

    // print out the attributes
    DOMNamedNodeMap* attrs = elem->getAttributes();
    for(XMLSize_t i = 0; i < attrs->getLength(); i++) {
        DOMNode* attr = attrs->item(i);
        cout << impl_->tags_.convert(attr->getNodeName()) << '=' << impl_->tags_.convert(attr->getNodeValue()) << ' ';
    }
    cout << endl;

    // print the children
    DOMNodeList* children = elem->getChildNodes();
    for(XMLSize_t i = 0; i < children->getLength(); i++) {
        DOMNode* currNode = children->item(i);
        if( currNode->getNodeType() && currNode->getNodeType() == DOMNode::ELEMENT_NODE )
            printTree(dynamic_cast< DOMElement* >( currNode ), lev+1);
    }

}

// TODO: Make this more robust
// TODO: Make command line help
void DecoderConfig::parseCommandLine(int argc, char** argv) {

    for(int i = 1; i < argc; i += 2) {
        if(!strcmp(argv[i], "-help") || !strcmp(argv[i], "--help")) {
            cerr << "Kyfd help is under construction" << endl;
            exit(0);
        }
        else if(!strcmp(argv[i], "-version") || !strcmp(argv[i], "--version")) {
            cerr << "Kyfd version " << PACKAGE_VERSION << endl;
            exit(0);
        }
        else if(i != argc-1) {
            if(*argv[i] != '-')
                throw runtime_error( "Invalid command line input" );
            handleArgument(argv[i]+1, argv[i+1]);
        }
    }

    parseConfigFile(argv[argc-1]);

}

// build the models
const FstNode<ComponentArc> * DecoderConfig::getComponentNode(unsigned id) {
    if(id >= impl_->compRoots_.size())
        throw runtime_error( "Attempt to get a component node larger than exists" );
    if(impl_->weights_.size() > 0)
        impl_->compRoots_[id]->adjustWeights(impl_->weights_);
    return impl_->compRoots_[id];
}
const FstNode<StdArc> * DecoderConfig::getStdNode(unsigned id) {
    if(id >= impl_->stdRoots_.size())
        throw runtime_error( "Attempt to get a stdonent node larger than exists" );
    if(impl_->weights_.size() > 0)
        impl_->stdRoots_[id]->adjustWeights(impl_->weights_);
    return impl_->stdRoots_[id];
}
