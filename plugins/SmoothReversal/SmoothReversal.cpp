// PluginSmoothReversal.cpp
// Jordan Henderson (j.henderson.music@outlook.com)

#include "SC_PlugIn.hpp"
#include "SmoothReversal.hpp"

static InterfaceTable* ft;

namespace SmoothReversal {

SmoothReversal::SmoothReversal() {
    mCalcFunc = make_calc_function<SmoothReversal, &SmoothReversal::next>();
    next(1);
}

void SmoothReversal::next(int nSamples) {
    const float* input = in(0);
    const float* gain = in(1);
    float* outbuf = out(0);

    // simple gain function
    for (int i = 0; i < nSamples; ++i) {
        outbuf[i] = input[i] * gain[i];
    }
}

} // namespace SmoothReversal

PluginLoad(SmoothReversalUGens) {
    // Plugin magic
    ft = inTable;
    registerUnit<SmoothReversal::SmoothReversal>(ft, "SmoothReversal", false);
}
