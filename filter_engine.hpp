#pragma once
#include <atomic>
#include <vector>

enum FilterMode {
    FilterModeLowPass = 0,
    FilterModeHighPass = 1,
    FilterModeBandPass = 2,
    FilterModeBandStop = 3,
};

enum FilterTopology {
    FilterTopologyButterworth = 0,
    FilterTopologyBessel = 1,
    FilterTopologyChebyshev = 2,
    FilterTopologyLinkwitzRiley = 3,
};

struct FilterSettings {
    int mode = FilterModeBandPass;
    int topology = FilterTopologyButterworth;
    double low_cutoff = 0.0;
    double high_cutoff = 0.0;
    double sample_step = 0.0;
};

std::vector<double> filter_signal(const std::vector<double>& time, const std::vector<double>& samples,
                                  const FilterSettings& settings, const std::atomic<bool>* cancel = nullptr);
