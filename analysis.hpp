// Spectral analysis built on top of the parsed Dataset.
//
// Mean removal and one-sided amplitudes. Irregular timestamps are resampled.
// Large timestamp gaps are ignored, while samples on both sides stay in FFT.
#pragma once

#include <string>
#include <vector>

#include "lvm_parser.hpp"

namespace lvm {

struct Peak {
    double freq = 0.0;
    double amp = 0.0;
};

struct Spectrum {
    std::vector<double> freqs;             // length N/2 + 1
    std::vector<std::string> names;        // channels that produced a spectrum
    std::vector<std::vector<double>> amp;  // per channel, length N/2 + 1
    std::vector<std::size_t> source_channels; // indices in the input Dataset, independent of names
    double source_start = 0.0;
    double source_end = 0.0;
    double sample_dt = 0.0;
    double nyquist = 0.0;
    int n = 0;                             // number of samples used
    bool resampled = false;                // linear interpolation onto a uniform time grid
    bool gaps_ignored = false;             // timestamp gaps were compressed; all samples remain
    bool imported = false;                 // amplitudes read directly, no FFT was performed
    bool ok = false;
    std::string error;
};

// Compute the magnitude spectrum for every channel. `max_samples > 0` caps the
// sample count using the first N selected samples. Gaps greater than four
// median time steps are compressed to one typical step, so every available
// sample in the selected range is retained. Values are linearly interpolated
// onto an evenly spaced grid based on this gap-free timeline.
Spectrum compute_spectrum(const Dataset& ds, int max_samples, const std::atomic<bool>* cancel = nullptr);

// Return up to `count` strongest spectral peaks (local maxima), excluding DC.
std::vector<Peak> find_peaks(const std::vector<double>& freqs,
                             const std::vector<double>& amp, int count);

}  // namespace lvm
