// PluginSmoothReversal.hpp
// Jordan Henderson (j.henderson.music@outlook.com)

#pragma once

#include "SC_PlugIn.hpp"

namespace SmoothReversal {

class SmoothReversal : public SCUnit {
public:
    SmoothReversal();

    // Destructor
    // ~SmoothReversal();

private:
    // Calc function
    void next(int nSamples);

    // Member variables
};

} // namespace SmoothReversal
