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
// string-manager.cc
//
//  A file to handle Xerces strings

#include <kyfd/string-manager.h>

#include <algorithm>

using namespace xercesc;
using namespace kyfd;
using namespace std;

// constructor
XercesStringManager::XercesStringManager() : xmlStrings_(), cStrings_() { }

// destructor
XercesStringManager::~XercesStringManager() { drain(); }

// delete all the data
void XercesStringManager::drain() {
    for_each(xmlStrings_.begin(), xmlStrings_.end(), releaseXMLString);
	for_each(cStrings_.begin(), cStrings_.end(), releaseCString);
    xmlStrings_.clear();
    cStrings_.clear();
}

// convert from C string to Xerces string
XMLCh* XercesStringManager::convert( const char* str ){
	XMLCh* result = XMLString::transcode( str ) ;
	xmlStrings_.push_back( result ) ;
	return result;
}

// convert from C++ string to Xerces string
XMLCh* XercesStringManager::convert( const string& str ){
	XMLCh* result = XMLString::transcode( str.c_str() );
	xmlStrings_.push_back( result );
	return result;
}

// convert from Xerces string to C string
char* XercesStringManager::convert( const XMLCh* str ){
	char* result = XMLString::transcode( str );
	cStrings_.push_back( result );
	return( result );
}

// whether the string is equal or not
bool XercesStringManager::streq( const XMLCh* xs, const char* cs ) {
    char* result = XMLString::transcode(xs);
    int ret = strcmp(result, cs);
    XMLString::release(&result);
    return ret == 0;
}
bool XercesStringManager::streq( const char* cs, const XMLCh* xs ) {
    return streq(xs,cs);
}
