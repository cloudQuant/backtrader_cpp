/**
 * @file test_ind_envelope_simple.cpp
 * @brief Simplified Envelope test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [
 *     ['4063.463000', '3644.444667', '3554.693333'],  # Mid Line (SMA)
 *     ['4165.049575', '3735.555783', '3643.560667'],  # Upper Line
 *     ['3961.876425', '3553.333550', '3465.826000']   # Lower Line
 * ]
 * chkmin = 30
 * chkargs = dict()
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/envelope.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> ENVELOPE_EXPECTED_VALUES = {
    {"4063.463000", "3644.444667", "3554.693333"},  // Mid Line
    {"4165.049575", "3735.555783", "3643.560667"},  // Upper Line
    {"3961.876425", "3553.333550", "3465.826000"}   // Lower Line
};

const int ENVELOPE_MIN_PERIOD = 30;

} // anonymous namespace

// Test Envelope with default parameters
DEFINE_INDICATOR_TEST(Envelope_Default, Envelope, ENVELOPE_EXPECTED_VALUES, ENVELOPE_MIN_PERIOD)