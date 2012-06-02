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
// component-weight.h
//
//  A weight on the component semi-ring, a version of the tropical semiring
//   that is able to hold several components separately.

#ifndef FST_LIB_COMPONENT_WEIGHT_H__
#define FST_LIB_COMPONENT_WEIGHT_H__

#include <fst/float-weight.h>
#include <stdexcept>
// #include <kyfd/components.h>
#include <kyfd/util.h>

// #define DEBUG_COMPONENT

#define SAFE_COMPONENT

using namespace std;

namespace fst {

class ComponentWeight;
inline ostream& operator<<(ostream& strm, const ComponentWeight &w);

// Tropical semiring with tracking capability for all the individual weights
class ComponentWeight {

private:
    
    // a pointer to an array including count, width, data
    unsigned short* ptr_;
    // unsigned short count_;
    // unsigned short width_;
    // float* components_;

public:

    typedef ComponentWeight ReverseWeight;
    const static unsigned short BAD_INDEX = UCHAR_MAX;
    
    ComponentWeight() : ptr_(0) {}

    ComponentWeight(float component) {
        ptr_ = new unsigned short[sizeof(float)/2+2];
        ptr_[0] = 1;
        ptr_[1] = 1;
        setComponent(0, component);
    }

    ComponentWeight(unsigned short width, const float* components) : ptr_(0) {
        if(width > 0) {
            ptr_ = new unsigned short[width*sizeof(float)/2+2];
            ptr_[0] = 1;
            ptr_[1] = width;
            memcpy(ptr_+2, components, width*sizeof(float));
            // std::cerr << "Created(w,c) [" << (unsigned long)ptr_ << "] = " << (int)ptr_[0] << ", " <<(int)ptr_[1] << " = " << *this << std::endl;
        }
    }

    ComponentWeight(const ComponentWeight &w) {
        ptr_ = w.ptr_;
        if(ptr_ != 0) {
            ptr_[0]++;
            if(ptr_[0] == 65535) {
                unsigned short * newPtr = new unsigned short[ptr_[1]*sizeof(float)/2+2];
                memcpy(newPtr, ptr_, ptr_[1]*sizeof(float)+4);
                ptr_[0]--;
                newPtr[0] = 1;
                ptr_ = newPtr;
            }
            // std::cerr << "Created(w) [" << (unsigned long)ptr_ << "] = " << (int)ptr_[0] << ", " <<(int)ptr_[1] << " = " << *this << std::endl;
        }
    }

    ComponentWeight &operator=(const ComponentWeight &w) {
        if(ptr_ && (0 == --ptr_[0]))
            delete [] ptr_;
        ptr_ = w.ptr_;
        if(ptr_ != 0) {
            ++ptr_[0];
            if(ptr_[0] == 65535) {
                unsigned short * newPtr = new unsigned short[ptr_[1]*sizeof(float)/2+2];
                memcpy(newPtr, ptr_, ptr_[1]*sizeof(float)+4);
                ptr_[0]--;
                newPtr[0] = 1;
                ptr_ = newPtr;
            }
            // std::cerr << "=(w) [" << (unsigned long)ptr_ << "] = " << (int)ptr_[0] << ", " <<(int)ptr_[1] << " = " << *this << std::endl;
        }
        return *this;
    }

    ~ComponentWeight() {
        if(ptr_ != 0) {
            // std::cerr << "Deleting [" << (unsigned long)ptr_ << "] = " << (int)ptr_[0] << ", " <<(int)ptr_[1] << " = " << *this << std::endl;
            if(--ptr_[0] == 0) {
                delete [] ptr_;
                ptr_ = 0;
            }
        }
    }

    // accessors
    inline unsigned short getWidth() const { return (ptr_?ptr_[1]:0); }
    inline float getComponent(unsigned short i) const {
        if(!ptr_ || i >= ptr_[1])
            throw runtime_error("Bad read of component");
        else
            return ((float*)(ptr_+2))[i];
    }

    // set a particular component
    inline float setComponent(unsigned short i, float f) {
        if(ptr_ || i >= ptr_[1]) {
            if(ptr_[0] != 1) {
                unsigned short * newPtr = new unsigned short[ptr_[1]*sizeof(float)/2+2];
                memcpy(newPtr, ptr_, ptr_[1]*sizeof(float)+4);
                ptr_[0]--;
                newPtr[0] = 1;
                ptr_ = newPtr;
            }
            ((float*)(ptr_+2))[i] = f;
        }
        else
            throw runtime_error("Attempt to set component for uninitialized value");
    }

    // set the value of the component
    inline float Value() const { return (getWidth()>0?getComponent(0):0.0F); }

    // Check that it works
    bool Member() const {
      return Value() == Value() && Value() != FloatLimits<float>::NegInfinity();
    }

    static const ComponentWeight NoWeight() {
      return ComponentWeight(FloatLimits<float>::NumberBad()); }
      
    istream &Read(istream &strm) {
        if(ptr_ && (--ptr_[0] != 0)) {
            delete [] ptr_;
            ptr_ = 0;
        }
        unsigned short width;
        ReadType(strm, &width);
        if(width > 0) {
            ptr_ = new unsigned short[width*sizeof(float)/2+2];
            ptr_[0] = 1;
            ptr_[1] = width;
            float * fptr_ = (float*)(ptr_+2);
            for(unsigned short i = 0; i < width; i++)
                ReadType(strm, fptr_ + i);
        }
        return strm;
    }
    
    ostream &Write(ostream &strm) const {
        unsigned short width = getWidth();
        WriteType(strm, width);
        float * fptr_ = (float*)(ptr_+2);
        for(unsigned short i = 0; i < width; i++)
            WriteType(strm, fptr_[i]);
        return strm;
    }

    static const ComponentWeight Zero() {
        float comp[] = { FloatLimits<float>::PosInfinity() };
        return ComponentWeight(1, comp);
    }
  
    static const ComponentWeight One() {
        return ComponentWeight(); }


    ComponentWeight Quantize(float delta = kDelta) const {
        ComponentWeight result(*this);
        unsigned short width = getWidth();
        for(unsigned short i = 0; i < width; i++)
            result.setComponent(i, floor( getComponent(i) / delta+0.5F) * delta);
        return result;
    }

    ComponentWeight Reverse() const { return *this; }

    static uint64 Properties() {
      return kLeftSemiring | kRightSemiring | kCommutative |
          kPath | kIdempotent;
    }

    static const string &Type() {
        static const string type = "component";
        return type;
    }

    size_t Hash() const {
        union {
            float f;
            size_t s;
        } u = { Value() };
        return u.s;
    }

};

// print to a stream
inline ostream& operator<<(ostream& strm, const ComponentWeight &w) {
    // print the components
    if(w.getWidth()){
        strm << "[";
        for(unsigned short i = 0; i < w.getWidth(); i++) {
            if(i) strm << ",";
            const float comp = w.getComponent(i);
            if(comp == FloatLimits<float>::PosInfinity())
                strm << "Infinity";
            else if(comp == FloatLimits<float>::NegInfinity())
                strm << "-Infinity";
            else if(comp != comp)
                strm << "BadFloat";
            else
                strm << comp;
        }
        strm << "]";
    }
    return strm;
}

inline istream &operator>>(istream &strm, ComponentWeight &w) {
    std::string s;
    strm >> s;
    if(s[0] != '[' || s[s.length()-1] != ']')
        throw runtime_error("Format error in components");
    string compStr = s.substr(1, s.length()-2);
    unsigned width = 1;
    for(unsigned i = 0; i < compStr.size(); i++)
        if(compStr[i] == ',') width++;
    char delims[] = ",";
    unsigned curr = 0;
    for(char* tok = strtok( (char*)compStr.c_str(), delims ); tok != 0; tok = strtok(0, delims) )
        w.setComponent(curr++, kyfd::stringToFloat(tok) );
    return strm;
}

inline bool operator==(const ComponentWeight &w1,
                       const ComponentWeight &w2) {
    if(w1.getWidth() != w2.getWidth())
        return false;
    for(unsigned short i = 0; i < w1.getWidth(); i++) {
        // // std::cerr << "getComponent("<<i<<"): "<<w1.getComponent(i)<<", "<<w2.getComponent(i)<<std::endl;
        if(w1.getComponent(i) != w2.getComponent(i))
            return false;
    }
    return true;
}

inline bool operator!=(const ComponentWeight &w1,
                       const ComponentWeight &w2) {
  return !(w1 == w2);
}

inline ComponentWeight Times(const ComponentWeight &w1, const ComponentWeight &w2) {
    // // std::cerr << "Times( " << w1 << ", " << w2 << " )" << std::endl;
    unsigned short s1 = w1.getWidth(), s2 = w2.getWidth();
    if(s1 == 0)
        return w2;
    else if(s2 == 0 || w1.Value() == FloatLimits<float>::PosInfinity())
        return w1;
    else if(w2.Value() == FloatLimits<float>::PosInfinity())
        return w2;
    else {
        unsigned short s3 = max( s1, s2 );
        float c3[s3];
        for(unsigned short i = 0; i < s3; i++) {
            c3[i] = (i < s1 ? w1.getComponent(i) : 0.0F ) + (i < s2 ? w2.getComponent(i) : 0.0F );
            // // std::cerr << "c3["<<i<<"]="<<w1.getComponent(i)<<"+"<<w2.getComponent(i)<<"="<<c3[i] << std::endl;
        }
        ComponentWeight w3(s3, c3);
        return w3;
    }
}

inline ComponentWeight Plus(const ComponentWeight &w1, const ComponentWeight &w2) {
    return w1.Value() < w2.Value() ? w1 : w2;
}

inline ComponentWeight OneOver(const ComponentWeight &w1){
    ComponentWeight result(w1);
    for(unsigned short i = 0; i < result.getWidth(); i++)
        result.setComponent(i, 0.0F - result.getComponent(i));
    return result;
}

inline ComponentWeight Divide(const ComponentWeight &w1,
                              const ComponentWeight &w2,
                              DivideType typ = DIVIDE_ANY) {
    return Times(w1, OneOver(w2));
}

inline bool ApproxEqual(const ComponentWeight &w1,
                        const ComponentWeight &w2,
                        float delta = kDelta) {
  return w1.Value() <= w2.Value() + delta && w2.Value() <= w1.Value() + delta;
}

}    // namespace fst;

#endif    // FST_LIB_PRODUCT_WEIGHT_H__
