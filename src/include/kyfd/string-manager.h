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
// string-manager.h
//
//  A header for the string manager for xerces strings.

#ifndef STRING_MANAGER_H__
#define STRING_MANAGER_H__

#include <xercesc/util/XMLString.hpp>
#include <string>
#include <list>

namespace kyfd {

class XercesStringManager {

public:
    
    // constructor
    XercesStringManager() : xmlStrings_(), cStrings_() { }
    
    // destructor
    ~XercesStringManager() { drain(); }
    
    // delete all the data
    void drain() {
        for(std::list< XMLCh* >::iterator xit = xmlStrings_.begin(); xit != xmlStrings_.end(); xit++)
            delete *xit;
        for(std::list< char* >::iterator cit = cStrings_.begin(); cit != cStrings_.end(); cit++)
            delete *cit;
        xmlStrings_.clear();
        cStrings_.clear();
    }
    
    // convert from C string to Xerces string
    XMLCh* convert( const char* str ){
    	XMLCh* result = xercesc::XMLString::transcode( str ) ;
    	xmlStrings_.push_back( result ) ;
    	return result;
    }
    
    // convert from C++ string to Xerces string
    XMLCh* convert( const string& str ){
    	XMLCh* result = xercesc::XMLString::transcode( str.c_str() );
    	xmlStrings_.push_back( result );
    	return result;
    }
    
    // convert from Xerces string to C string
    char* convert( const XMLCh* str ){
    	char* result = xercesc::XMLString::transcode( str );
    	cStrings_.push_back( result );
    	return( result );
    }
    
    // whether the string is equal or not
    bool streq( const XMLCh* xs, const char* cs ) {
        char* result = xercesc::XMLString::transcode(xs);
        int ret = strcmp(result, cs);
        xercesc::XMLString::release(&result);
        return ret == 0;
    }
    bool streq( const char* cs, const XMLCh* xs ) {
        return streq(xs,cs);
    }
   
    // functions to release strings
	static void releaseXMLString( XMLCh* str ) {
		xercesc::XMLString::release( &str );
	}
	static void releaseCString( char* str ) {
		xercesc::XMLString::release( &str );
	}

private:

    std::list< XMLCh* > xmlStrings_;
    std::list< char* > cStrings_;

};

} // end namespace

#endif // STRING_MANAGER_H__
