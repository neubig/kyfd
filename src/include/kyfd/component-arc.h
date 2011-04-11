///   Copyright 2009, Kyfd Project Team
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
// component-arc.cc
//
//  An arc that is able to hold multiple components. This allows weights of
//   various models to be held separately for later weight tuning.

#ifndef FST_COMPONENT_ARC_H__
#define FST_COMPONENT_ARC_H__

#include <fst/arc.h>
#include <kyfd/component-weight.h>

namespace fst {

// Arc with integer labels and state Ids and weights over the
// product of the tropical semiring with itself.
struct ComponentArc {
  typedef int Label;
  typedef ComponentWeight Weight;
  typedef int StateId;

  ComponentArc(Label i, Label o, Weight w, StateId s)
      : ilabel(i), olabel(o), weight(w), nextstate(s) {}

  ComponentArc() {}

  static const string &Type() {  // Arc type name
    static const string type = "component";
    return type;
  }

  Label ilabel;       // Transition input label
  Label olabel;       // Transition output label
  Weight weight;      // Transition weight
  StateId nextstate;  // Transition destination state
};

}  // namespace fst;

#endif  // FST_COMPONENT_ARC_H__
