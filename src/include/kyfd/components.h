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
// components.h
//
//  A data structure to hold values for the component semiring


#ifndef _COMPONENTS_H_
#define _COMPONENTS_H_

#include <iostream>
#include <map>
#include <utility>
#include <set>
#include <cmath>
#include <stdexcept>
#include <climits>
#include <kyfd/util.h>

namespace fst {

class Components {

public:

    // typedef __gnu_cxx::hash_map<int,double> Container;
    // todo: use unsigned, not int
    typedef unsigned Index;
    typedef float Value;
    typedef std::vector< Value > Container;
    typedef Container::const_iterator const_iterator;
    typedef Container::iterator iterator;
    typedef Container::size_type size_type;

    // maximum value
    const static Index BAD_INDEX = UINT_MAX;

    Components() : count_(1) {}

    Components(const Components& other) : count_(1) { components_ = other.get(); }

    virtual ~Components() {}

    /**
     * Inserts a set of components
     * @param s The string that describes the components, e.g. [0=0.69123,12=-3.4524]
     */
    void insert(const std::string& s) {
        char delims[] = ",";
        char* token = 0;
        token = strtok( (char*)s.c_str(), delims );
        while( token != 0 ) { // e.g. 1=0.70711
            std::string entryStr = token;
            std::string::size_type equalSignPos = entryStr.find("=");
            if(equalSignPos != std::string::npos){
                std::string indexStr = entryStr.substr(0,equalSignPos);
                Index index = kyfd::stringToUnsignedLong(indexStr);
                std::string vStr = entryStr.substr(equalSignPos+1);
                Value v = kyfd::stringToFloat(vStr);
                insert(index, v);
            }
            else{
                throw std::runtime_error("Format error in components");
            }
            token = strtok(0, delims);
        }
    } 

    void insert(Index i, Value d) {
        if(components_.size() <= i)
            components_.resize(i+1);
        components_[i] = d;
    }
    
    inline Value get(Index i) const { return components_[i]; }

    inline iterator begin() { return components_.begin(); }

    inline iterator end() { return components_.end(); }

    inline void clear() { return components_.clear(); } 

    inline const_iterator begin() const { return components_.begin(); }

    inline const_iterator end() const { return components_.end(); }

    inline size_type size() const { return components_.size(); }

    std::ostream& print(std::ostream& out) const {
        if(components_.size()){
            out << "[";
            bool start = true;
            for(Index i = 0; i < components_.size(); i++) {
                if(!start)
                    out << ",";
                out << i << "=" << components_[i]; 
                start = false;
            }
            out << "]";
        }
        return out;
    }

    static void computeUnion(const Components& e1, const Components& e2, Components& ret) {

        const unsigned s1 = e1.size();
        const unsigned s2 = e2.size();
        const Container & arr1 = e1.get();
        const Container & arr2 = e2.get();
        if(s1 >= s2) {
            ret.set(arr1);
            for(unsigned i = 0; i < s2; i++)
                ret.components_[i] += arr2[i];
        }
        else {
            ret.set(arr2);
            for(unsigned i = 0; i < s1; i++)
                ret.components_[i] += arr1[i];
        }

    }

    unsigned getCount() const { return count_; }
    unsigned incCount(){ return ++count_; }
    unsigned decCount(){ return --count_; }

protected:

    Components(const Container& components) : count_(1), components_(components) { }

    const Container& get() const { return components_; }
    void set(const Container & components) { components_ = components; }

private:

    Container components_;
    unsigned count_;

}; // end class Components

inline std::ostream &operator<<(std::ostream &out, const Components &w) {
    w.print(out);
    return out;
}

}

#endif // _COMPONENTS_H_

