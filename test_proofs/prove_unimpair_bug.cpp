#include <iostream>
#include <cstdint>
#include <limits>
#include <iomanip>

// Simulate the vulnerable code
std::uint32_t calculateNextPaymentDueDate_VULNERABLE(
    std::uint32_t currentTime,
    std::uint32_t paymentInterval)
{
    // This is the ACTUAL code from LoanManage.cpp:375
    return currentTime + paymentInterval;
}

std::uint32_t calculateSecondsOverdue(
    std::uint32_t currentTime,
    std::uint32_t nextPaymentDueDate)
{
    // This is from LendingHelpers.cpp:186
    return currentTime - nextPaymentDueDate;
}

double calculateLateInterest(
    double principal,
    double lateInterestRate,
    std::uint32_t secondsOverdue)
{
    // This is from LendingHelpers.cpp:189-191
    constexpr double secondsPerYear = 31536000.0;
    double periodicRate = (lateInterestRate * secondsOverdue) / secondsPerYear;
    return principal * periodicRate;
}

int main()
{
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "\n========================================" << std::endl;
    std::cout << "PROOF OF CONCEPT: unimpairLoan OVERFLOW BUG" << std::endl;
    std::cout << "========================================\n" << std::endl;

    constexpr std::uint32_t UINT32_MAX_VAL = std::numeric_limits<std::uint32_t>::max();

    std::cout << "Ripple Network Time:" << std::endl;
    std::cout << "  Epoch: January 1, 2000" << std::endl;
    std::cout << "  UINT32_MAX: " << UINT32_MAX_VAL << " seconds = "
              << (UINT32_MAX_VAL / 31536000) << " years = Year ~2136" << std::endl;
    std::cout << std::endl;

    // Test Case 1: Normal operation (year 2025)
    std::cout << "========================================" << std::endl;
    std::cout << "TEST 1: Normal Operation (Year 2025)" << std::endl;
    std::cout << "========================================\n" << std::endl;

    std::uint32_t normalTime = 788918400;  // ~2025
    std::uint32_t interval = 2592000;      // 30 days
    double principal = 1000000.0;          // $1M loan
    double lateRate = 0.5;                 // 50% annual late interest

    std::cout << "Loan unimpaired at time: " << normalTime << " (year "
              << (2000 + normalTime / 31536000) << ")" << std::endl;
    std::cout << "Payment interval: " << interval << " seconds (30 days)" << std::endl;
    std::cout << std::endl;

    std::uint32_t nextDue1 = calculateNextPaymentDueDate_VULNERABLE(normalTime, interval);
    std::cout << "nextPaymentDueDate = " << normalTime << " + " << interval << std::endl;
    std::cout << "                   = " << nextDue1 << std::endl;

    if (nextDue1 < normalTime)
    {
        std::cout << "❌ OVERFLOW!" << std::endl;
    }
    else
    {
        std::cout << "✓ No overflow" << std::endl;
    }

    // Borrower pays 10 days late
    std::uint32_t payTime1 = nextDue1 + 864000;  // 10 days late
    std::uint32_t overdue1 = calculateSecondsOverdue(payTime1, nextDue1);
    double lateFee1 = calculateLateInterest(principal, lateRate, overdue1);

    std::cout << "\nBorrower pays 10 days late:" << std::endl;
    std::cout << "  secondsOverdue: " << overdue1 << " (" << (overdue1 / 86400) << " days)" << std::endl;
    std::cout << "  Late fee: $" << lateFee1 << std::endl;
    std::cout << "  ✓ Reasonable fee" << std::endl;

    // Test Case 2: Near UINT32_MAX (year 2136)
    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST 2: Near UINT32_MAX (Year 2136)" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Start just before overflow will occur
    std::uint32_t nearMaxTime = UINT32_MAX_VAL - 1000000;  // ~11 days before max

    std::cout << "Loan unimpaired at time: " << nearMaxTime << " (year "
              << (2000 + nearMaxTime / 31536000) << ")" << std::endl;
    std::cout << "Payment interval: " << interval << " seconds (30 days)" << std::endl;
    std::cout << "Time until UINT32_MAX: " << (UINT32_MAX_VAL - nearMaxTime) / 86400.0
              << " days" << std::endl;
    std::cout << std::endl;

    std::uint32_t nextDue2 = calculateNextPaymentDueDate_VULNERABLE(nearMaxTime, interval);
    std::cout << "nextPaymentDueDate = " << nearMaxTime << " + " << interval << std::endl;
    std::cout << "                   = " << nextDue2;

    if (nextDue2 < nearMaxTime)
    {
        std::cout << " ❌ OVERFLOW!" << std::endl;
        std::cout << "\n🚨🚨🚨 INTEGER OVERFLOW CONFIRMED! 🚨🚨🚨" << std::endl;
        std::cout << "nextPaymentDueDate wrapped to: " << nextDue2 << std::endl;
        std::cout << "That's year: " << (2000 + nextDue2 / 31536000) << std::endl;
        std::cout << std::endl;

        // Borrower tries to pay on time (15 days later)
        std::uint32_t payTime2 = nearMaxTime + 1296000;  // 15 days later
        std::cout << "Borrower pays 15 days after unimpair:" << std::endl;
        std::cout << "  Current time: " << payTime2 << std::endl;
        std::cout << "  nextPaymentDueDate: " << nextDue2 << " (wrapped!)" << std::endl;
        std::cout << std::endl;

        std::uint32_t overdue2 = calculateSecondsOverdue(payTime2, nextDue2);
        std::cout << "  Calculating late fee:" << std::endl;
        std::cout << "    secondsOverdue = " << payTime2 << " - " << nextDue2 << std::endl;
        std::cout << "                   = " << overdue2 << " seconds" << std::endl;
        std::cout << "                   = " << (overdue2 / 86400) << " days" << std::endl;
        std::cout << "                   = " << (overdue2 / 31536000.0) << " YEARS!" << std::endl;
        std::cout << std::endl;

        double lateFee2 = calculateLateInterest(principal, lateRate, overdue2);
        std::cout << "    Late interest calculation:" << std::endl;
        std::cout << "      principal: $" << principal << std::endl;
        std::cout << "      lateInterestRate: " << (lateRate * 100) << "% annual" << std::endl;
        std::cout << "      periodicRate: " << ((lateRate * overdue2) / 31536000.0) << std::endl;
        std::cout << "      latePaymentInterest: $" << lateFee2 << std::endl;
        std::cout << std::endl;

        std::cout << "❌❌❌ VULNERABILITY EXPLOITED! ❌❌❌" << std::endl;
        std::cout << "  Normal late fee (10 days): $" << lateFee1 << std::endl;
        std::cout << "  Bugged late fee (15 days): $" << lateFee2 << std::endl;
        std::cout << "  Ratio: " << (lateFee2 / lateFee1) << "x" << std::endl;
        std::cout << std::endl;
        std::cout << "IMPACT:" << std::endl;
        std::cout << "  ✗ Borrower charged $" << lateFee2 << " for being 15 days late" << std::endl;
        std::cout << "  ✗ This is " << (lateFee2 / principal) << "x the principal!" << std::endl;
        std::cout << "  ✗ Loan becomes unpayable" << std::endl;
        std::cout << "  ✗ Loan defaults unfairly" << std::endl;
        std::cout << "  ✗ Lender seizes all collateral" << std::endl;
        std::cout << "  ✗ Borrower loses funds" << std::endl;

    }
    else
    {
        std::cout << " ✓ No overflow yet" << std::endl;
    }

    // Test Case 3: Exact threshold
    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST 3: Finding Exact Overflow Threshold" << std::endl;
    std::cout << "========================================\n" << std::endl;

    std::uint32_t thresholdTime = UINT32_MAX_VAL - interval + 1;
    std::cout << "Threshold time for overflow: " << thresholdTime << std::endl;
    std::cout << "That's " << ((UINT32_MAX_VAL - thresholdTime) / 86400.0) << " days before UINT32_MAX" << std::endl;
    std::cout << std::endl;

    std::uint32_t nextDue3 = calculateNextPaymentDueDate_VULNERABLE(thresholdTime, interval);
    std::cout << "At this threshold:" << std::endl;
    std::cout << "  nextPaymentDueDate = " << thresholdTime << " + " << interval << std::endl;
    std::cout << "                     = " << nextDue3 << std::endl;

    if (nextDue3 < thresholdTime)
    {
        std::cout << "  ❌ OVERFLOWS to: " << nextDue3 << std::endl;
        std::cout << "  Wraps around by: " << (thresholdTime - nextDue3) << " seconds" << std::endl;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "CONCLUSION" << std::endl;
    std::cout << "========================================\n" << std::endl;
    std::cout << "✓ Bug confirmed: Integer overflow in unimpairLoan" << std::endl;
    std::cout << "✓ Occurs: Year 2136 (when Ripple time approaches UINT32_MAX)" << std::endl;
    std::cout << "✓ Impact: Massive unfair late fees, forced loan default" << std::endl;
    std::cout << "✓ Severity: MEDIUM (HIGH impact, LOW likelihood)" << std::endl;
    std::cout << "✓ Fix: Add overflow detection or use saturating addition" << std::endl;

    return 0;
}
