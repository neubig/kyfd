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
// beamtrim.cc
//
//  A program that can be used to trim an FST using beam search

#include <iostream>

#include <fst/fst.h>
#include <fst/vector-fst.h>

#include <kyfd/beam-trim.h>

using namespace std;
using namespace fst;

int main(int argc, const char* argv[]) {

    if(argc != 4) {
        cerr << "Usage: " << argv[0] << " model.fst out.fst beam_width" << endl;
        return 1;
    }

    // load and map the first model
    StdFst * readModel = StdFst::Read(argv[1]);
    if(readModel == 0) {
        cerr << "Error reading in model file from " << argv[1] << endl;
        return 1;
    }

    unsigned beamWidth = atoi(argv[3]);

    StdVectorFst * trimFst = new StdVectorFst;
    BeamTrim<StdArc>(*readModel, trimFst, beamWidth);

    trimFst->Write(argv[2]);

    delete trimFst;
    delete readModel;

}
