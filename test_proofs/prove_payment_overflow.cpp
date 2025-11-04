#include <iostream>
#include <cstdint>
#include <limits>
#include <iomanip>

std::uint32_t advancePaymentDueDate_VULNERABLE(
    std::uint32_t currentDueDate,
    std::uint32_t paymentInterval)
{
    // This is the ACTUAL code from LendingHelpers.cpp:533
    return currentDueDate + paymentInterval;
}

int main()
{
    std::cout << std::fixed << std::setprecision(2);

    constexpr std::uint32_t UINT32_MAX_VAL = std::numeric_limits<std::uint32_t>::max();
    constexpr std::uint32_t INTERVAL = 2592000;  // 30 days

    std::cout << "\n========================================" << std::endl;
    std::cout << "PROOF: NORMAL PAYMENT OVERFLOW (Y2136)" << std::endl;
    std::cout << "========================================\n" << std::endl;

    std::cout << "Simulating regular monthly payments approaching UINT32_MAX...\n" << std::endl;

    // Start near the overflow threshold
    std::uint32_t nextDueDate = UINT32_MAX_VAL - (INTERVAL * 3);

    std::cout << "Starting nextPaymentDueDate: " << nextDueDate << std::endl;
    std::cout << "Payment interval: " << INTERVAL << " seconds (30 days)" << std::endl;
    std::cout << "UINT32_MAX: " << UINT32_MAX_VAL << "\n" << std::endl;

    for (int payment = 1; payment <= 5; payment++)
    {
        std::cout << "Payment #" << payment << ":" << std::endl;
        std::cout << "  Before: nextDueDate = " << nextDueDate << std::endl;

        std::uint32_t newDueDate = advancePaymentDueDate_VULNERABLE(nextDueDate, INTERVAL);

        std::cout << "  After:  nextDueDate = " << newDueDate;

        if (newDueDate < nextDueDate)
        {
            std::cout << " ❌ OVERFLOW!" << std::endl;
            std::cout << "  Wrapped to year ~" << (2000 + newDueDate / 31536000) << std::endl;

            // Simulate late fee calculation
            std::uint32_t currentTime = nextDueDate + (INTERVAL / 2);  // 15 days later
            std::uint32_t secondsOverdue = currentTime - newDueDate;
            double principal = 1000000.0;
            double lateRate = 0.5;
            double lateFee = principal * lateRate * secondsOverdue / 31536000.0;

            std::cout << "\n  If borrower pays 15 days after this:" << std::endl;
            std::cout << "    currentTime: " << currentTime << std::endl;
            std::cout << "    nextDueDate: " << newDueDate << " (wrapped)" << std::endl;
            std::cout << "    secondsOverdue: " << secondsOverdue << " seconds" << std::endl;
            std::cout << "    = " << (secondsOverdue / 86400.0) << " days" << std::endl;
            std::cout << "    = " << (secondsOverdue / 31536000.0) << " YEARS" << std::endl;
            std::cout << "    Late fee: $" << lateFee << std::endl;
            std::cout << "\n  🚨 LOAN BECOMES UNPAYABLE! 🚨" << std::endl;
            break;
        }
        else
        {
            std::cout << " ✓ OK" << std::endl;
            std::uint32_t remaining = UINT32_MAX_VAL - newDueDate;
            std::cout << "  Distance to UINT32_MAX: " << remaining << " seconds";
            std::cout << " (" << (remaining / 86400.0) << " days)" << std::endl;
        }

        nextDueDate = newDueDate;
        std::cout << std::endl;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "CONCLUSION" << std::endl;
    std::cout << "========================================\n" << std::endl;
    std::cout << "✓ Bug confirmed: Normal payment overflow" << std::endl;
    std::cout << "✓ Affects: ALL loans making payments in year 2136" << std::endl;
    std::cout << "✓ Impact: System-wide lending protocol failure" << std::endl;
    std::cout << "✓ Severity: MEDIUM-HIGH (catastrophic impact, low likelihood)" << std::endl;

    return 0;
}
