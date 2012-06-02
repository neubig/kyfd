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
// component-map.h
//
//  Mappings between component and standard semirings

#ifndef _COMPONENT_MAP_H__
#define _COMPONENT_MAP_H__

#include <fst/fst.h>
#include <fst/map.h>
#include <fst/arc.h>

#include <kyfd/component-arc.h>
#include <kyfd/component-weight.h>

namespace fst {

// a map to go from float arcs to component arcs
struct ComponentMapper {
    
    unsigned char idx_;
    bool idxOk_;

    ComponentMapper(unsigned char idx) : idx_(idx) {
        idxOk_ = (idx_ != ComponentWeight::BAD_INDEX);
    }

    ComponentArc operator()(const StdArc &arc) const {
        unsigned char width = (idxOk_?idx_+2:1);
        float comp[width];
        comp[0] = arc.weight.Value();
        if(idxOk_) {
            for(unsigned short i = 1; i <= idx_; i++)
                comp[i] = 0.0F;
            comp[idx_+1] = comp[0];
        }
        return ComponentArc(arc.ilabel, arc.olabel, ComponentWeight(width, comp), arc.nextstate);
    }

    MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }

    MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }

    MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS;}

    uint64 Properties(uint64 props) const { return props; }
};

// a map to go from float arcs to component arcs, putting a log-linear
//  weight on the main part of the function
struct WeightedComponentMapper {

    unsigned char idx_;
    bool idxOk_;
    float weight_;
    
    WeightedComponentMapper(unsigned idx, float weight) : idx_(idx), weight_(weight) { 
        idxOk_ = (idx_ != ComponentWeight::BAD_INDEX);
    }

    ComponentArc operator()(const StdArc &arc) const {
        unsigned char width = 1;
        if(idxOk_)
            width = idx_+2;
        float comp[width];

        comp[0] =  arc.weight.Value() * ( arc.weight.Value() == FloatLimits<float>::PosInfinity() ? 1 : weight_ );
        if(idxOk_) {
            for(unsigned char i = 1; i <= idx_; i++)
                comp[i] = 0.0F;
            comp[idx_+1] = arc.weight.Value();
        }
        return ComponentArc(arc.ilabel, arc.olabel, ComponentWeight(width, comp), arc.nextstate);
    }

    MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }

    MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }

    MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS;}

    uint64 Properties(uint64 props) const { return props; }
};

// a map putting a log-linear weight on the arc value
struct WeightedMapper {

    float weight_;
    
    WeightedMapper(float weight) : weight_(weight) { }

    StdArc operator()(const StdArc &arc) const {
        TropicalWeight ret( arc.weight.Value() *
            ( arc.weight.Value() == FloatLimits<float>::PosInfinity() ? 1 : weight_ ) );
        return StdArc(arc.ilabel, arc.olabel, ret, arc.nextstate);
    }

    MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }

    MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }

    MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS;}

    uint64 Properties(uint64 props) const { return props; }

};

}

#endif
