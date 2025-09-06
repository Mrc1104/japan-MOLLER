#include "QwRotatableHalfWavePlate.h"
#include <string>
#include <iostream>

#include "QwLog.h"
#include "TMath.h"

static char const *DEFAULT_RHWP_IOC = "psub_pl_ipos";

static constexpr double LOWER_BOUND_ARB_UNITS = 0.0;
static constexpr double UPPER_BOUND_ARB_UNITS = 8000.0;

static constexpr double LOWER_BOUND_DEG_UNITS = 0.0;
static constexpr double UPPER_BOUND_DEG_UNITS = 360.0;

static constexpr double LOWER_BOUND_RAD_UNITS = 0.0;
static constexpr double UPPER_BOUND_RAD_UNITS = 2.0 * TMath::Pi(); // Inline TMath functions are constexpr since 2017 (see ROOT-CERN PR #487)

// Useful Conversions
static constexpr double DEG_PER_ARB = (UPPER_BOUND_DEG_UNITS-LOWER_BOUND_DEG_UNITS) / (UPPER_BOUND_ARB_UNITS-LOWER_BOUND_ARB_UNITS);
static constexpr double RAD_PER_ARB = (UPPER_BOUND_RAD_UNITS-LOWER_BOUND_RAD_UNITS) / (UPPER_BOUND_ARB_UNITS-LOWER_BOUND_ARB_UNITS);

RHWP::RHWP() : QwHalfWavePlate<double>( DEFAULT_RHWP_IOC ) { }
RHWP::RHWP(std::string ioc) : QwHalfWavePlate<double>( std::move(ioc) ) { }

double RHWP::GetStatus_Deg() const { return ( status * DEG_PER_ARB ); }
double RHWP::GetStatus_Rad() const { return ( status * RAD_PER_ARB ); }
double RHWP::GetStatus_Arb() const { return      GetStatus()        ; } // For completeness
