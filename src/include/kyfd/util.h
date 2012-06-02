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
// util.h
//
//  Simple utility functions

#ifndef _UTIL_H__
#define _UTIL_H__

#include <string>
#include <fst/float-weight.h>

namespace kyfd {

inline float stringToFloat(const string &str) {
    return atof(str.c_str());
}

inline double stringToDouble(const string &str) {
    return atof(str.c_str());
}

inline unsigned long stringToUnsignedLong(const string &str) {
    return (unsigned long)atoi(str.c_str());
}

inline float logExp(float x) { return log(1.0F + exp(-x)); }

inline float logPlus(const float& f1, const float& f2) {
  if (f1 == fst::FloatLimits<float>::PosInfinity())
    return f2;
  else if (f2 == fst::FloatLimits<float>::PosInfinity())
    return f1;
  else if (f1 > f2)
    return (f2 - logExp(f1 - f2));
  else
    return (f1 - logExp(f2 - f1));
}

}

#endif
