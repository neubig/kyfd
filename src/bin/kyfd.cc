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
// kyfd.cc
//
//  The main decoder program in the Kyfd toolkit. It takes a configuration
//   file and command line properties, and input is piped through standard
//   input

#include <iostream>
#include <kyfd/decoder.h>
#include <kyfd/decoder-config.h>

using namespace std;
using namespace kyfd;

int main(int argc, char** argv) {

    if(argc == 1) {
        cerr << "Usage: " << argv[0] << " config.xml" << endl;
        return 1;
    }

    cerr << "--------------------------" << endl
         << "-- Started Kyfd Decoder --" << endl
         << "--------------------------" << endl << endl;
    
    // load the configuration
    DecoderConfig * config = new DecoderConfig;
    config->parseCommandLine(argc, argv);

    cerr << "Loaded configuration, initializing decoder..." << endl;
    
    time_t before, after;
    time(&before);
    
    // initialize the decoder
    Decoder * decoder = new Decoder(*config);

    time(&after);
    cerr << " Done initializing, took " << difftime(after, before) << " seconds" << endl << "Decoding..." << endl;
    
    // decode
    string line;
    int i = 0;
    int reload = config->getReload();
    while(decoder->decode(cin, cout)) {
        if(++i % 100 == 0)
            cerr << i;
        else
            cerr << ".";
        // reload the model if necessary
        if(reload && i % reload == 0)
            decoder->buildModels();
    }

    time(&before);
    cerr << " Done decoding, took " << difftime(before, after) << " seconds" << endl;
    // decoder->printTimes();

    delete decoder;
    delete config;

}
