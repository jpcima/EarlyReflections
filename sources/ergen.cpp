#include "ergen.h"
#include <algorithm>
#include <cmath>

double Normal(double x, double rho)
{
    return (1.0/(rho*std::sqrt(2.0*M_PI))) *
        std::exp(-0.5*(std::pow(x/rho, 2.0)));
}

///
ERgen::ERgen(const Setup& setup)
    : setup_(setup)
{
}

void ERgen::calc()
{
    int numPoints = setup_.numPoints;
    ppdf_.clear();
    cdf_.clear();
    ppdf_.resize(numPoints);
    cdf_.resize(numPoints);

    ///
    int numTaps = setup_.numTaps;
    positions_.clear();
    gains_.clear();
    positions_.resize(numTaps);
    gains_.resize(numTaps);

    ///
    for (int i = 0; i < numTaps; ++i)
        addPeakAt(i / (double)(numTaps - 1));

    ///
    calcCDF();

    ///
    calcPositions();
    calcGains();
}

double ERgen::calcFadeGain(double x) const
{
    double fade = setup_.fade;
    return fade / (x + fade);
}

void ERgen::addPeakAt(double peakPos)
{
    int numPoints = setup_.numPoints;
    double rho = setup_.rho;
    double* ppdf = ppdf_.data();
    for (int i = 0; i < numPoints; ++i) {
        double x = i / (double)(numPoints - 1);
        ppdf[i] += Normal((x - peakPos), rho);
    }
}

void ERgen::calcCDF()
{
    int numPoints = setup_.numPoints;
    double* cdf = cdf_.data();
    const double* ppdf = ppdf_.data();
    cdf[0] = ppdf[0];
    for (int i = 1; i < numPoints; ++i)
        cdf[i] = cdf[i - 1] + ppdf[i];
    for (int i = 0; i < numPoints; ++i)
        cdf[i] /= cdf[numPoints - 1];
}

void ERgen::calcPositions()
{
    int numTaps = setup_.numTaps;
    double* positions = positions_.data();
    auto& prng = *setup_.prng;
    for (ulong i = 0; i < numTaps; ++i) {
        double r = std::uniform_real_distribution<double>{0.0, 1.0}(prng);
        positions[i] = lookupCDF(r);
    }
}

void ERgen::calcGains()
{
    int numTaps = setup_.numTaps;
    double fade = setup_.fade;
    double gainSpread = setup_.gainSpread;
    const double* positions = positions_.data();
    double* gains = gains_.data();
    auto& prng = *setup_.prng;
    for (int i = 0; i < numTaps; ++i) {
        double g = fade / (fade + positions[i]);
        double v = std::normal_distribution<double>{1.0, gainSpread}(prng);
        v = std::max(0.0, v);
        g *= v;
        g = std::min(1.0, g);
        gains[i] = g;
    }
}

double ERgen::lookupCDF(double value) const
{
    int numPoints = setup_.numPoints;
    const double* cdf = cdf_.data();

    int searchIndex = std::lower_bound(cdf, cdf + numPoints, value) - cdf;
    if (searchIndex + 1 >= numPoints) {
        return 1.0;
    }

    double left = cdf[searchIndex + 0];
    double right = cdf[searchIndex + 1];
    double ratio = (value - left) / (right - left);
    return (searchIndex + ratio) / (numPoints - 1);
}
