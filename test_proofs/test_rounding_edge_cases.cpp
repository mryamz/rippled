#include <iostream>
#include <cmath>
#include <iomanip>

// Simulate tiny loan with rounding errors

int main()
{
    std::cout << std::fixed << std::setprecision(10);

    std::cout << "\n========================================" << std::endl;
    std::cout << "ROUNDING EDGE CASE ANALYSIS" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // XRP drop = 0.000001 XRP = 1e-6 XRP
    double drop = 0.000001;

    // Test Case 1: Minimum viable loan
    std::cout << "Test 1: Minimum Periodic Payment Loan" << std::endl;
    std::cout << "======================================\n" << std::endl;

    double periodicPayment = 2 * drop;  // 2 drops per payment
    double roundingTolerance = 3 * drop;  // 3 drops allowed error
    int numPayments = 10000;

    std::cout << "Periodic payment: " << periodicPayment << " XRP ("
              << (periodicPayment / drop) << " drops)" << std::endl;
    std::cout << "Rounding tolerance: " << roundingTolerance << " XRP ("
              << (roundingTolerance / drop) << " drops)" << std::endl;
    std::cout << "Number of payments: " << numPayments << std::endl;
    std::cout << std::endl;

    std::cout << "Rounding error as % of payment: "
              << (roundingTolerance / periodicPayment * 100) << "%" << std::endl;
    std::cout << std::endl;

    double totalPaymentExpected = periodicPayment * numPayments;
    double maxRoundingError = roundingTolerance * numPayments;

    std::cout << "Expected total payments: " << totalPaymentExpected << " XRP" << std::endl;
    std::cout << "Maximum accumulated rounding error: " << maxRoundingError << " XRP" << std::endl;
    std::cout << "Error as % of total: "
              << (maxRoundingError / totalPaymentExpected * 100) << "%" << std::endl;
    std::cout << std::endl;

    if (maxRoundingError > totalPaymentExpected * 0.01)
    {
        std::cout << "⚠️  WARNING: Rounding error > 1% of loan!" << std::endl;
    }
    if (roundingTolerance > periodicPayment)
    {
        std::cout << "❌ CRITICAL: Rounding tolerance exceeds payment amount!" << std::endl;
        std::cout << "   Rounding error could be larger than the payment itself!" << std::endl;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 2: Realistic Loan" << std::endl;
    std::cout << "========================================\n" << std::endl;

    double principal = 1000000;  // 1M XRP
    double annualRate = 0.10;  // 10%
    int paymentsPerYear = 12;
    int years = 5;
    int totalPayments = paymentsPerYear * years;

    double monthlyRate = annualRate / paymentsPerYear;
    double payment = principal * (monthlyRate * pow(1 + monthlyRate, totalPayments)) /
                     (pow(1 + monthlyRate, totalPayments) - 1);

    std::cout << "Principal: " << principal << " XRP" << std::endl;
    std::cout << "Annual rate: " << (annualRate * 100) << "%" << std::endl;
    std::cout << "Monthly payment: " << payment << " XRP" << std::endl;
    std::cout << "Total payments: " << totalPayments << std::endl;
    std::cout << std::endl;

    double totalExpected = payment * totalPayments;
    double maxError = roundingTolerance * totalPayments;

    std::cout << "Expected total payments: " << totalExpected << " XRP" << std::endl;
    std::cout << "Maximum accumulated rounding error: " << maxError << " XRP" << std::endl;
    std::cout << "Error as % of total: " << (maxError / totalExpected * 100) << "%" << std::endl;
    std::cout << "Error as % of principal: " << (maxError / principal * 100) << "%" << std::endl;
    std::cout << std::endl;

    if (maxError / totalExpected < 0.0001)
    {
        std::cout << "✓ Rounding error negligible for realistic loans" << std::endl;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 3: Dust Attack Scenario" << std::endl;
    std::cout << "========================================\n" << std::endl;

    double dustPayment = 1 * drop;  // 1 drop per payment (minimum)
    double dustPrincipal = dustPayment * numPayments;

    std::cout << "Attack: Create loan with 1-drop periodic payment" << std::endl;
    std::cout << "Periodic payment: " << dustPayment << " XRP (1 drop)" << std::endl;
    std::cout << "Number of payments: " << numPayments << std::endl;
    std::cout << "Total principal: " << dustPrincipal << " XRP" << std::endl;
    std::cout << std::endl;

    double dustMaxError = roundingTolerance * numPayments;
    std::cout << "Maximum rounding accumulation: " << dustMaxError << " XRP" << std::endl;
    std::cout << "Error as % of principal: "
              << (dustMaxError / dustPrincipal * 100) << "%" << std::endl;
    std::cout << std::endl;

    if (dustMaxError > dustPrincipal)
    {
        std::cout << "🚨 CRITICAL VULNERABILITY!" << std::endl;
        std::cout << "   Rounding errors (" << dustMaxError << " XRP) exceed principal ("
                  << dustPrincipal << " XRP)" << std::endl;
        std::cout << "   Error magnitude: " << (dustMaxError / dustPrincipal) << "x principal!" << std::endl;
        std::cout << std::endl;
        std::cout << "IMPACT:" << std::endl;
        std::cout << "  - Rounding errors dominate the loan economics" << std::endl;
        std::cout << "  - One party could systematically extract value" << std::endl;
        std::cout << "  - Loan accounting becomes unreliable" << std::endl;
    }
    else
    {
        std::cout << "✓ Protection exists against dust attacks" << std::endl;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "CONCLUSION" << std::endl;
    std::cout << "========================================\n" << std::endl;

    std::cout << "Key Findings:" << std::endl;
    std::cout << "1. Rounding tolerance (3 drops) can exceed tiny payment amounts" << std::endl;
    std::cout << "2. For 1-drop payments: error = 300% of payment!" << std::endl;
    std::cout << "3. For 2-drop payments: error = 150% of payment!" << std::endl;
    std::cout << "4. Accumulated over many payments, could be significant" << std::endl;
    std::cout << std::endl;
    std::cout << "Mitigation needed: Minimum periodic payment threshold" << std::endl;
    std::cout << "Recommendation: periodicPayment must be >> 3 drops" << std::endl;
    std::cout << "Suggested minimum: 100 drops (10x safety margin)" << std::endl;

    return 0;
}
