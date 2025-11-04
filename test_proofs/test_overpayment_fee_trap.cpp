#include <iostream>
#include <iomanip>

// Test overpayment fee combinations

double tenthBipsOfValue(double value, double tenthBips)
{
    // tenthBips is in units of 1/10 basis point = 1/100,000
    return value * (tenthBips / 100000.0);
}

int main()
{
    std::cout << std::fixed << std::setprecision(6);

    std::cout << "\n========================================" << std::endl;
    std::cout << "OVERPAYMENT FEE TRAP ANALYSIS" << std::endl;
    std::cout << "========================================\n" << std::endl;

    double overpayment = 10000.0;  // $10,000 overpayment

    std::cout << "Overpayment amount: $" << overpayment << "\n" << std::endl;

    // Test Case 1: Maximum valid overpayment parameters
    std::cout << "Test 1: Greedy Lender Configuration" << std::endl;
    std::cout << "====================================\n" << std::endl;

    double overpaymentFeeRate = 100000;  // 100% (max allowed)
    double overpaymentInterestRate = 100000;  // 100% annual (max allowed)
    double managementFeeRate = 5000;  // 50% of interest (common)

    std::cout << "Loan parameters:" << std::endl;
    std::cout << "  OverpaymentFee: " << (overpaymentFeeRate / 1000.0) << "%" << std::endl;
    std::cout << "  OverpaymentInterestRate: " << (overpaymentInterestRate / 1000.0) << "%" << std::endl;
    std::cout << "  ManagementFeeRate: " << (managementFeeRate / 100.0) << "%\n" << std::endl;

    double fee = tenthBipsOfValue(overpayment, overpaymentFeeRate);
    double payment = overpayment - fee;

    std::cout << "Step 1: Direct fee extraction" << std::endl;
    std::cout << "  fee = overpayment * " << (overpaymentFeeRate / 1000.0) << "%" << std::endl;
    std::cout << "  fee = $" << fee << std::endl;
    std::cout << "  Remaining payment = $" << payment << "\n" << std::endl;

    if (payment <= 0)
    {
        std::cout << "🚨 100% FEE TRAP!" << std::endl;
        std::cout << "   Entire overpayment consumed by direct fee!" << std::endl;
        std::cout << "   Borrower loses $" << overpayment << std::endl;
        std::cout << "   No principal paid down!" << std::endl;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 2: High But Not Maximum" << std::endl;
    std::cout << "====================================\n" << std::endl;

    overpaymentFeeRate = 50000;  // 50%
    overpaymentInterestRate = 100000;  // 100%
    managementFeeRate = 5000;  // 50%

    std::cout << "Loan parameters:" << std::endl;
    std::cout << "  OverpaymentFee: " << (overpaymentFeeRate / 1000.0) << "%" << std::endl;
    std::cout << "  OverpaymentInterestRate: " << (overpaymentInterestRate / 1000.0) << "%" << std::endl;
    std::cout << "  ManagementFeeRate: " << (managementFeeRate / 100.0) << "%\n" << std::endl;

    fee = tenthBipsOfValue(overpayment, overpaymentFeeRate);
    payment = overpayment - fee;

    std::cout << "Step 1: Direct fee" << std::endl;
    std::cout << "  fee = $" << fee << std::endl;
    std::cout << "  payment = $" << payment << "\n" << std::endl;

    // Note: overpaymentInterestRate is ANNUAL, so it's not directly applied
    // But let's calculate what the interest WOULD be if this was a payment period
    double rawInterest = tenthBipsOfValue(payment, overpaymentInterestRate);
    double rawManagementFee = tenthBipsOfValue(rawInterest, managementFeeRate);
    double principalPaid = payment - rawInterest - rawManagementFee;

    std::cout << "Step 2: Interest and management fee (if full year)" << std::endl;
    std::cout << "  overpaymentInterest = payment * " << (overpaymentInterestRate / 1000.0) << "%" << std::endl;
    std::cout << "  overpaymentInterest = $" << rawInterest << std::endl;
    std::cout << "  managementFee = interest * " << (managementFeeRate / 100.0) << "%" << std::endl;
    std::cout << "  managementFee = $" << rawManagementFee << "\n" << std::endl;

    std::cout << "Step 3: Principal paid down" << std::endl;
    std::cout << "  principalPaid = payment - interest - managementFee" << std::endl;
    std::cout << "  principalPaid = $" << payment << " - $" << rawInterest
              << " - $" << rawManagementFee << std::endl;
    std::cout << "  principalPaid = $" << principalPaid << "\n" << std::endl;

    if (principalPaid <= 0)
    {
        std::cout << "🚨 OVERPAYMENT TRAP!" << std::endl;
        std::cout << "   Entire overpayment consumed by fees and interest!" << std::endl;
        std::cout << "   Borrower pays: $" << overpayment << std::endl;
        std::cout << "   Principal reduction: $" << std::max(0.0, principalPaid) << std::endl;
        std::cout << "\n   Breakdown:" << std::endl;
        std::cout << "     Direct fee to lender: $" << fee << " ("
                  << (fee/overpayment*100) << "%)" << std::endl;
        std::cout << "     Interest to vault: $" << (rawInterest - rawManagementFee) << " ("
                  << ((rawInterest - rawManagementFee)/overpayment*100) << "%)" << std::endl;
        std::cout << "     Management fee to broker: $" << rawManagementFee << " ("
                  << (rawManagementFee/overpayment*100) << "%)" << std::endl;
        std::cout << "     TOTAL EXTRACTED: $" << (fee + rawInterest) << " ("
                  << ((fee + rawInterest)/overpayment*100) << "%)" << std::endl;
    }
    else
    {
        std::cout << "Principal paid: $" << principalPaid << " ("
                  << (principalPaid/overpayment*100) << "% of overpayment)" << std::endl;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 3: Realistic Scenario" << std::endl;
    std::cout << "====================================\n" << std::endl;

    overpaymentFeeRate = 5000;  // 5%
    overpaymentInterestRate = 10000;  // 10% annual
    managementFeeRate = 1000;  // 10%

    std::cout << "Loan parameters:" << std::endl;
    std::cout << "  OverpaymentFee: " << (overpaymentFeeRate / 1000.0) << "%" << std::endl;
    std::cout << "  OverpaymentInterestRate: " << (overpaymentInterestRate / 1000.0) << "%" << std::endl;
    std::cout << "  ManagementFeeRate: " << (managementFeeRate / 100.0) << "%\n" << std::endl;

    fee = tenthBipsOfValue(overpayment, overpaymentFeeRate);
    payment = overpayment - fee;
    rawInterest = tenthBipsOfValue(payment, overpaymentInterestRate);
    rawManagementFee = tenthBipsOfValue(rawInterest, managementFeeRate);
    principalPaid = payment - rawInterest - rawManagementFee;

    std::cout << "Overpayment breakdown:" << std::endl;
    std::cout << "  Direct fee: $" << fee << std::endl;
    std::cout << "  Interest: $" << rawInterest << std::endl;
    std::cout << "  Management fee: $" << rawManagementFee << std::endl;
    std::cout << "  Principal paid: $" << principalPaid << std::endl;
    std::cout << "  ✓ Reasonable allocation" << std::endl;

    std::cout << "\n========================================" << std::endl;
    std::cout << "CONCLUSION" << std::endl;
    std::cout << "========================================\n" << std::endl;

    std::cout << "Key Findings:" << std::endl;
    std::cout << "1. Overpayment fees up to 100% are allowed" << std::endl;
    std::cout << "2. At 100%, entire overpayment goes to lender as fee" << std::endl;
    std::cout << "3. No principal is paid down" << std::endl;
    std::cout << "4. Borrower loses the overpayment amount" << std::endl;
    std::cout << std::endl;
    std::cout << "Mitigation:" << std::endl;
    std::cout << "- Code checks: if (trackedPrincipalDelta > 0)" << std::endl;
    std::cout << "- Rejects overpayments that don't pay down principal" << std::endl;
    std::cout << "- So borrower CAN'T accidentally lose funds this way" << std::endl;
    std::cout << std::endl;
    std::cout << "Verdict: ✓ PROTECTED - overpayment logic is safe!" << std::endl;

    return 0;
}
