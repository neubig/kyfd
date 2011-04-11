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
// buildfstmodel.cc
//
//  A program that builds an FST model according to a configuration file

#include <iostream>
#include <kyfd/decoder.h>
#include <kyfd/decoder-config.h>

using namespace std;
using namespace kyfd;
using namespace fst;

int main(int argc, char** argv) {

    if(argc == 0) {
        cerr << "Usage: " << argv[0] << " config.xml output1.fst output2.fst ..." << endl;
        return 1;
    }

    cerr << "------------------------------------------------" << endl
         << "-- Started Kyfd Model Building " << argv[argc-1] << endl
         << "------------------------------------------------" << endl << endl;
    
    // load the configuration
    DecoderConfig config;
    config.parseCommandLine(2, argv);

    if(config.getNumModels() != argc-2) {
        cerr << "Incompatible number of output files ("<<(argc-2)<<") and models ("<<config.getNumModels()<<")"<<endl;
        return 1;
    }

    cerr << "Loaded configuration, initializing decoder..." << endl;

    if(config.getOutputFormat() == COMPONENT_OUTPUT) {
        for(unsigned i = 0; i < config.getNumModels(); i++) {
            Fst<ComponentArc> * comp = config.getComponentNode(i)->buildFst();
            VectorFst<ComponentArc> out(*comp);
            out.Write(argv[i+2]);
            delete comp;
        }
    } else {
        for(unsigned i = 0; i < config.getNumModels(); i++) {
            Fst<StdArc> * std = config.getStdNode(i)->buildFst();
            VectorFst<StdArc> out(*std);
            out.Write(argv[i+2]);
            delete std;
        }
    } 

}
