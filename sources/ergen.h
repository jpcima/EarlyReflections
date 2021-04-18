#pragma once
#include <random>
#include <vector>

class ERgen {
public:
    using PRNG = std::default_random_engine;

    struct Setup {
        double fade;
        double rho;
        int numTaps;
        int numPoints;
        double gainSpread;
        PRNG* prng;
    };

    explicit ERgen(const Setup& setup);
    void calc();
    int getNumTaps() const { return setup_.numTaps; }
    int getNumPoints() const { return setup_.numPoints; }
    const double* getPositions() const { return positions_.data(); }
    const double* getGains() const { return gains_.data(); }
    double calcFadeGain(double x) const;

private:
    void addPeakAt(double peakPos);
    void calcCDF();
    void calcPositions();
    void calcGains();
    double lookupCDF(double value) const;

private:
    Setup setup_ {};
    std::vector<double> ppdf_;
    std::vector<double> cdf_;
    std::vector<double> positions_;
    std::vector<double> gains_;
};
