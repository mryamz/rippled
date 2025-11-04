# Finding #4: Dust Loan Rounding Vulnerability

## Severity: LOW-MEDIUM

**Category**: Precision / Rounding / Economic Edge Case
**Impact**: Loan Accounting Unreliability, Potential Value Extraction
**Likelihood**: Low (requires intentional creation of dust loans)
**Overall**: LOW-MEDIUM

## TL;DR

The lending protocol allows creation of loans with extremely small periodic payments (e.g., 1-2 drops for XRP). The rounding tolerance of 3 drops per payment can exceed the payment amount itself, causing rounding errors to dominate the loan economics. For 1-drop payments, rounding error tolerance is **300% of the payment amount**, making loan accounting unreliable and potentially allowing systematic value extraction.

## Location

**File**: `src/xrpld/app/tx/detail/LoanSet.cpp`
**Lines**: 443-445 (periodicPayment validation)
**Issue**: No minimum threshold for periodic payment beyond > 0

```cpp
// Line 443-445
if (properties.periodicPayment <= 0)
{
    JLOG(j_.warn()) << "Loan Periodic payment ("
                    << properties.periodicPayment << ") rounds to 0. ";
    return tecPRECISION_LOSS;
}
```

**Rounding Logic**: `src/xrpld/app/misc/detail/LendingHelpers.cpp`
**Lines**: 432-438, 1288-1294 (rounding tolerance assertions)

## Vulnerability Details

### Root Cause

The protocol validates that `periodicPayment > 0` after rounding, but does NOT enforce a minimum payment amount. This allows creation of "dust loans" where:

1. Periodic payment ≈ 1-2 drops (0.000001 - 0.000002 XRP)
2. Rounding tolerance = 3 drops per payment (hardcoded)
3. **Rounding error > actual payment amount**

### Rounding Tolerance

From `LendingHelpers.cpp:432-438`:

```cpp
XRPL_ASSERT_PARTS(
    excess == beast::zero ||
        (excess > beast::zero &&
         ((asset.integral() && excess < 3) ||  // ← 3 drops for XRP
          (roundedPeriodicPayment.exponent() - excess.exponent() > 6))),
    "ripple::detail::computeRoundedInterestAndFeeComponents",
    "excess is extremely small (fee)");
```

This allows up to **3 drops** rounding error per payment for integral assets like XRP.

### Attack Scenario

**Attacker Strategy**: Create dust loan to exploit rounding

1. **Create Dust Loan**:
   - Principal: 0.01 XRP (10,000 drops)
   - Payment Interval: 86400 seconds (1 day)
   - Payment Total: 10,000
   - Interest Rate: ~0%
   - Result: periodicPayment ≈ 1 drop

2. **Rounding Dominates Economics**:
   - Payment amount: 1 drop
   - Rounding tolerance: 3 drops
   - **Rounding error: 300% of payment!**

3. **Potential Exploitation**:
   - Make 10,000 payments
   - Each payment has 3-drop tolerance
   - Systematic rounding in borrower's favor → pay less
   - Maximum potential extraction: 30,000 drops = 0.03 XRP
   - That's **300% of principal!**

4. **Impact**:
   - Loan accounting becomes unreliable
   - Rounding errors dominate actual payments
   - One party could extract value systematically

## Impact Analysis

### Severity Justification

**Impact**: MEDIUM
- Rounding errors can exceed principal amount (3x for 1-drop payments)
- Loan accounting becomes unreliable
- Potential for systematic value extraction
- BUT: Limited to small absolute amounts (dust-level)

**Likelihood**: LOW
- Requires intentional creation of dust loans
- Limited economic incentive (tiny absolute amounts)
- Final payment clears all remaining balances (limits accumulation)
- Most loans would have realistic payment amounts

**Combined**: LOW-MEDIUM

### Why This Matters

**Principle Over Practice**:
- Even if economically insignificant, the math is fundamentally broken for dust loans
- Rounding tolerance (3 drops) should NEVER exceed payment amount
- Shows lack of minimum payment threshold in validation

**Real-World Scenarios**:
1. **Test/Demo Loans**: Someone creating tiny loans for testing could hit this
2. **Micro-Lending**: Future use cases with very small amounts
3. **Attack Vector**: While unprofitable now, could be combined with other exploits

### Mitigating Factors

**Final Payment Clears All** (Lines 1113-1125 in LendingHelpers.cpp):
```cpp
if (paymentRemaining == 1)
{
    // ...
    // Pay everything off
    deltas.valueDelta = totalValueOutstanding;
    deltas.principalDelta = principalOutstanding;
    deltas.managementFeeDueDelta = managementFeeOutstanding;
}
```

This prevents indefinite accumulation of rounding errors - the final payment pays off the TRUE remaining balance, not the accumulated-with-errors balance.

**Diff Compensation** (Lines 228-243):
The code tracks rounding errors and compensates:
```cpp
auto const diff = roundToAsset(
    asset,
    principalOutstanding - rawPrincipalOutstanding,
    scale,
    asset.integral() ? Number::downward : Number::towards_zero);

auto const p = roundToAsset(asset, rawPrincipal + diff, scale, Number::downward);
```

This should prevent systematic bias, but when tolerance >> payment, compensation may not work reliably.

## Proof of Concept

**Test**: `/tmp/test_rounding_edge_cases.cpp`

```bash
g++ /tmp/test_rounding_edge_cases.cpp -o /tmp/test_rounding -std=c++17 && /tmp/test_rounding
```

**Output Highlights**:

```
Test 3: Dust Attack Scenario
========================================

Attack: Create loan with 1-drop periodic payment
Periodic payment: 0.0000010000 XRP (1 drop)
Number of payments: 10000
Total principal: 0.0100000000 XRP

Maximum rounding accumulation: 0.0300000000 XRP
Error as % of principal: 300.0000000000%

🚨 CRITICAL VULNERABILITY!
   Rounding errors (0.0300000000 XRP) exceed principal (0.0100000000 XRP)
   Error magnitude: 3.0000000000x principal!
```

**Test 1** (2-drop payments):
- Rounding error: **150% of payment amount**
- Still problematic but less severe

**Test 2** (Realistic $1M loan):
- Rounding error: 0.000000014% of total
- Completely negligible ✓

## Recommended Fix

### Option 1: Minimum Periodic Payment (Simple & Effective)

```cpp
// In LoanSet.h
static Number constexpr minPeriodicPayment{100};  // 100 drops = 0.0001 XRP

// In LoanSet.cpp preclaim(), after line 445
if (properties.periodicPayment <= 0)
{
    JLOG(j_.warn()) << "Loan Periodic payment rounds to 0.";
    return tecPRECISION_LOSS;
}
// ADD THIS CHECK:
if (vaultAsset.integral() && properties.periodicPayment < minPeriodicPayment)
{
    JLOG(j_.warn()) << "Loan Periodic payment (" << properties.periodicPayment
                    << ") is below minimum (" << minPeriodicPayment << ").";
    return tecPRECISION_LOSS;
}
```

**Rationale for 100 drops**:
- 100 drops >> 3 drops tolerance (33x safety margin)
- Ensures rounding error is < 3% of payment
- Still allows micro-loans (0.0001 XRP per payment = 0.01 XRP over 100 payments)
- Minimal impact on legitimate use cases

### Option 2: Dynamic Tolerance (More Complex)

Make rounding tolerance proportional to payment amount:

```cpp
// Instead of hardcoded "< 3", use:
auto const maxRounding = std::max(3, roundedPeriodicPayment * 0.01);  // 1% of payment
XRPL_ASSERT_PARTS(
    excess == beast::zero ||
        (excess > beast::zero &&
         ((asset.integral() && excess < maxRounding) ||
          ...)),
    "...",
    "excess is extremely small");
```

**Trade-off**: More complex, changes rounding behavior for all loans.

### Option 3: Document As Acceptable Risk

If dust loans are deemed out of scope:
1. Document that protocol assumes reasonable payment amounts
2. Add warning in comments that dust loans may have unreliable accounting
3. No code changes needed

**Trade-off**: Leaves mathematical oddity in place, but acknowledges it's not a security threat.

## Economic Analysis

### Worst-Case Exploit Profitability

**Assumptions**:
- Attacker creates dust loan with 1-drop payments
- Makes 10,000 payments
- Rounding systematically favors attacker by 3 drops/payment
- Maximum extraction: 30,000 drops = 0.03 XRP

**Cost vs Benefit**:
- Transaction fees: 10,000 payments × 10 drops/tx = 100,000 drops = 0.1 XRP
- Potential gain: 30,000 drops = 0.03 XRP
- **NET LOSS: -0.07 XRP**

**Conclusion**: Exploit is **economically infeasible** - costs more in fees than it could possibly extract!

### Why Still Worth Fixing

1. **Principle**: Math should work correctly at all scales
2. **Future-Proofing**: Transaction fees might change
3. **Code Quality**: Signals attention to detail
4. **Edge Cases**: Prevents unexpected behavior in testing/demos
5. **Composability**: Could combine with other exploits

## References

- Rounding validation: `src/xrpld/app/tx/detail/LoanSet.cpp:443-445`
- Rounding tolerance: `src/xrpld/app/misc/detail/LendingHelpers.cpp:432-438, 1288-1294`
- Final payment clearing: `src/xrpld/app/misc/detail/LendingHelpers.cpp:1113-1125`
- Diff compensation: `src/xrpld/app/misc/detail/LendingHelpers.cpp:228-243`

## Test Case Needed

```cpp
void testDustLoanRejection()
{
    Env env(*this);
    // ... setup accounts, vault, broker ...

    // Attempt to create loan with 1-drop periodic payment
    env(loan::set(borrower, lender)
        ["Asset"](XRP)
        ["Principal"](drops(10000))  // 0.01 XRP
        ["PaymentTotal"](10000)
        ["PaymentInterval"](86400)  // 1 day
        ["InterestRate"](0),  // Zero interest → periodicPayment ≈ 1 drop
        ter(tecPRECISION_LOSS));  // Should fail with minimum payment check
}
```

## Comparison to Other Findings

| Finding | Severity | Exploitability | Impact |
|---------|----------|----------------|--------|
| **#2** (unimpairLoan overflow) | MEDIUM | Year 2136 only | $68M late fees |
| **#3** (payment overflow) | MEDIUM-HIGH | Year 2136 only | System failure |
| **#4** (dust loan rounding) | LOW-MEDIUM | Anytime | Max 3x principal, but dust amounts |

Finding #4 is lower severity because:
- Limited to dust amounts (tiny absolute values)
- Economically infeasible to exploit (fees > gains)
- Final payment prevents indefinite accumulation
- Real-world loans wouldn't use such small amounts

## Conclusion

This is a **LOW-MEDIUM severity issue** that should be fixed for code quality and robustness, not urgency. The mathematical anomaly (rounding tolerance > payment amount) violates reasonable expectations, even though:

1. Final payment clearing prevents indefinite accumulation
2. Economic infeasibility makes exploitation unprofitable
3. Real-world use cases wouldn't create such tiny loans

**Recommendation**: Add minimum periodic payment threshold (100 drops for XRP) to prevent dust loans and ensure rounding errors are always << payment amounts. This is a simple, low-risk fix that improves protocol robustness.
