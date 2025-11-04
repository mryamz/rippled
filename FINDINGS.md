# Security Audit Findings - XLS-66 Lending Protocol

**Competition**: Immunefi XRPL Ripple Attackathon
**Target**: XLS-66 Lending Protocol Implementation
**Branch**: `ximinez/lending-XLS-66`

---

## Finding 1: No Upper Bound on PaymentTotal (DoS Risk)

**Severity**: 🟡 MEDIUM
**Category**: Denial of Service / Resource Exhaustion
**CVE**: N/A

### TL;DR

The `PaymentTotal` field in LoanSet transactions lacks an upper bound, allowing attackers to specify up to 4.2 billion payments. This forces validators to compute `power(1+rate, 4billion)` during loan creation, causing excessive CPU usage through 32 levels of recursion and ~32 multiplication operations on large Number types. While unlikely to cause overflow at realistic interest rates, this enables computational DoS attacks and potential unhandled exceptions that could disrupt consensus.

### Location
`src/xrpld/app/tx/detail/LoanSet.cpp:101-103`

### Description
The `PaymentTotal` field in LoanSet transactions only validates that the value is greater than zero, but does NOT enforce an upper bound. This allows an attacker to specify an extremely large number of payments (up to UINT32_MAX = 4,294,967,295).

```cpp
if (auto const paymentTotal = tx[~sfPaymentTotal];
    paymentTotal && *paymentTotal <= 0)
    return temINVALID;
```

### Impact

**Computational DoS**: When creating a loan with a very large `PaymentTotal`, the `loanPeriodicPayment()` function calls:
```cpp
computeRaisedRate(periodicRate, paymentsRemaining)
  -> power(1 + periodicRate, paymentsRemaining)
```

The `power()` function uses recursive exponentiation by squaring, requiring log2(n) stack frames. For n = 4 billion, this means ~32 recursive calls, which is manageable.

**However**, the multiplication operations within power() could:
1. Take significant CPU time for very large exponents
2. Potentially overflow the Number type (though unlikely at realistic interest rates)
3. If overflow occurs, throw `std::overflow_error` exception

**Exception Handling Risk**: If an exception is thrown during transaction processing and not properly caught, it could:
- Cause transaction failure in an uncontrolled manner
- Potentially disrupt consensus if validators handle it differently
- Be used as a griefing attack to waste validator resources

### Proof of Concept

An attacker creates a LoanSet transaction with:
- `PaymentTotal = 4294967295` (MAX_UINT32)
- `PaymentInterval = 60` (minimum, 60 seconds)
- `InterestRate = 100000` (100% annual, maximum)

This would cause the validator to:
1. Compute `periodicRate ≈ 0.00019025875`
2. Attempt to calculate `(1.00019025875)^4294967295`
3. Require 32 levels of recursion
4. Perform ~32 multiplication operations on increasingly large Numbers

While the Number type can theoretically handle this (maxExponent = 32768), the computation is unnecessary and wasteful.

### Practical Overflow Calculation

At maximum interest rate (100% annual) with minimum interval (60s):
- Overflow occurs around n = 396,415,097 payments
- This represents ~754 years of payments
- Still within UINT32_MAX range!

At smaller rates, overflow would require even more payments.

### Recommendation

**Add an upper bound to PaymentTotal**:

```cpp
// In LoanSet.h
static std::uint32_t constexpr maxPaymentTotal = 10'000'000; // 10 million max
static_assert(maxPaymentTotal >= minPaymentTotal);

// In LoanSet.cpp preflight()
if (auto const paymentTotal = tx[~sfPaymentTotal])
{
    if (*paymentTotal <= 0 || *paymentTotal > maxPaymentTotal)
        return temINVALID;
}
```

**Rationale for 10 million**:
- With 60s intervals: 10M payments = ~19 years
- With 1 day intervals: 10M payments = ~27,397 years
- Reasonable upper bound for any legitimate loan
- Prevents computational waste
- Far below overflow thresholds

**Alternative**: Add try-catch around computeLoanProperties() to handle overflow gracefully:

```cpp
try {
    auto const loanProps = computeLoanProperties(...);
} catch (const std::overflow_error& e) {
    JLOG(ctx.j.warning()) << "Loan computation overflow: " << e.what();
    return tecINTERNAL; // or appropriate error code
}
```

### References
- `src/xrpld/app/misc/detail/LendingHelpers.cpp:81-102` (computeRaisedRate, computePaymentFactor)
- `src/libxrpl/basics/Number.cpp` (power function, overflow handling)
- XLS-66 Specification Section 3.2.4.1.1 (Regular Payment formula)

### Test Case Needed

```cpp
// Test with maximum PaymentTotal
Env env(*this);
// ... setup accounts, vault, broker ...
env(loan::set(borrower, lender)
    ["Asset"](asset)
    ["Principal"](principal)
    ["PaymentTotal"](4294967295)  // MAX_UINT32
    ["PaymentInterval"](60)
    ["InterestRate"](100000),
    ter(???));  // Should fail with temINVALID if fix applied
```

---

## Analysis Status

**Functions Analyzed**:
- ✓ `loanPeriodicRate()`
- ✓ `loanPeriodicPayment()`
- ✓ `computeRaisedRate()`
- ✓ `computePaymentFactor()`
- ✓ `power(Number, unsigned)`
- ✓ `computePaymentComponents()` - Calls calculateRawLoanState → loanPrincipalFromPeriodicPayment → power(), affected by Finding #1
- ✓ `LoanPay::doApply()` - Well-defended with extensive validation and fund conservation checks, affected by Finding #1 when processing payments
- ✓ `LoanSet::checkSign()` - Secure multi-party signature verification with proper authorization checks
- ✓ `LoanSet::doApply()` - Loan creation with extensive guards, affected by Finding #1 at computeLoanProperties() call

**Key Observations**:
- **computePaymentComponents()**: Extensive defensive programming with UNREACHABLE blocks, assertions, and std::max guards. Allows small rounding tolerance (< 3 drops for XRP). No new critical vulnerability, but each payment computation triggers expensive power() call with large exponents.
- **LoanPay::doApply()**: Proper input validation, authorization checks, and fund transfer logic. Debug builds include comprehensive fund conservation assertions. Can process up to 100 payments per transaction (loanMaximumPaymentsPerTransaction), amplifying the DoS risk from Finding #1.
- **LoanSet::checkSign()**: Well-designed signature system. Preflight requires CounterpartySignature (except batch inner txns), preclaim enforces that one party must be broker owner, checkSign verifies cryptographic signatures. Properly handles multisig with correct fee calculation. No vulnerabilities found.
- **LoanSet::doApply()**: Comprehensive loan creation with 4 precision guards, debt limit checks, first-loss capital requirements, and proper accounting (AssetsAvailable -= principal, AssetsTotal += interest, broker DebtTotal += principal + interest). Vulnerable at line 373-379 where computeLoanProperties() is called with potentially huge paymentTotal (Finding #1), no exception handling for overflow.

**Next Functions to Analyze**:
- `LoanSet::checkSign()` - Signature verification
- `LoanSet::doApply()` - Loan creation logic
- `LoanBrokerCoverWithdraw::preclaim()` - Collateral checks
- `calculateRawLoanState()` - Loan state computation
- `loanMakeFull Payment()` / `loanMakePayment()` - Payment calculation helpers
