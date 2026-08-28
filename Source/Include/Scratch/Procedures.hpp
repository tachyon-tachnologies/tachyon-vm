#pragma once

#include <Tachyon/ExMem.hpp>
#include <vector>
#include <string>

namespace Scratch {
    struct ScratchProcedure {
        std::vector<std::string> ParametersKeys;
        std::vector<std::string> ParametersNames;

        /**
         * Contains JIT data
         */
        Tachyon::OutputCodeInfo JITData = {};
        
        /**
         * The name of the procedure to call
         */
        std::string ProcCode;

        /**
         * Block key to the procedure prototype
         */
        std::string PrototypeKey;

        /**
         * Block key to the procedure definition
         */
        std::string DefinitionKey;

        /**
         * Can be ignored for now
         */
        bool UseWarp;
    };

    namespace Procedures {
        void RegisterAll(void);
    }
};
