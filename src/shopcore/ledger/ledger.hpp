// ledger: double-entry bookkeeping for ShopCore's finance team.
//
// Account names are colon-separated paths whose first segment is the account type:
// assets:bank, assets:receivable:gateway, liabilities:gst_output, revenue:sales,
// revenue:shipping, revenue:sales_returns, expenses:gateway_fees, equity:retained_earnings.
//
// A posting's amount is positive for a debit and negative for a credit; every journal
// entry's postings sum to zero.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "shopcore/core/core.hpp"
#include "shopcore/money/money.hpp"

namespace shopcore::ledger {

class LedgerError : public DomainError {
public:
    using DomainError::DomainError;
};

class UnbalancedEntry : public LedgerError {
public:
    using LedgerError::LedgerError;
};

enum class AccountType { Asset, Liability, Equity, Revenue, Expense };

struct Posting {
    std::string account;
    money::Money amount;  // + debit, - credit

    bool operator==(const Posting&) const = default;
};

struct JournalEntry {
    std::int64_t id = 0;
    Date date;
    std::string memo;
    std::string reference;  // order id, payment id, ...
    std::vector<Posting> postings;

    bool operator==(const JournalEntry&) const = default;
};

struct Ledger {
    std::string currency = "INR";
    std::vector<JournalEntry> entries;
    std::int64_t next_id = 1;
};

struct TrialBalanceRow {
    std::string account;
    money::Money debit;
    money::Money credit;

    bool operator==(const TrialBalanceRow&) const = default;
};

struct StatementLine {
    Date date;
    std::int64_t entry_id = 0;
    std::string memo;
    money::Money amount;
    money::Money balance;

    bool operator==(const StatementLine&) const = default;
};

struct Settlement {
    std::string payment_id;
    Date settled_on;
    money::Money gross;
    money::Money fee;
    money::Money net;

    bool operator==(const Settlement&) const = default;
};

struct AmountMismatch {
    std::string reference;
    money::Money expected;  // per the ledger
    money::Money actual;    // per the settlement file

    bool operator==(const AmountMismatch&) const = default;
};

struct Reconciliation {
    std::vector<std::string> matched;
    std::vector<std::string> missing_in_ledger;  // settled but never booked
    std::vector<std::string> unsettled;          // booked but not in the settlement file
    std::vector<AmountMismatch> mismatched;

    bool operator==(const Reconciliation&) const = default;
};

inline constexpr std::string_view kBank = "assets:bank";
inline constexpr std::string_view kGatewayReceivable = "assets:receivable:gateway";
inline constexpr std::string_view kGstOutput = "liabilities:gst_output";
inline constexpr std::string_view kSales = "revenue:sales";
inline constexpr std::string_view kShippingRevenue = "revenue:shipping";
inline constexpr std::string_view kSalesReturns = "revenue:sales_returns";
inline constexpr std::string_view kGatewayFees = "expenses:gateway_fees";
inline constexpr std::string_view kRetainedEarnings = "equity:retained_earnings";

// ---- accounts.cpp -----------------------------------------------------------
// Lower-case [a-z0-9_] segments joined by ':', at least two segments, known type.
bool is_valid_account_name(std::string_view name);
// From the first segment. Throws LedgerError for an unknown type.
AccountType account_type(std::string_view name);
// Assets and expenses carry debit balances; the rest carry credit balances.
bool is_debit_normal(AccountType type);
// "assets:bank:hdfc" -> "assets:bank"; "" for a top-level name.
std::string parent_account(std::string_view name);
// True when `account` is `ancestor` or one of its sub-accounts.
bool is_under(std::string_view account, std::string_view ancestor);

// ---- journal.cpp ------------------------------------------------------------
Posting debit(std::string_view account, const money::Money& amount);
Posting credit(std::string_view account, const money::Money& amount);
// At least two postings, one currency, valid accounts, summing to zero.
bool is_balanced(const std::vector<Posting>& postings);
// Validates and appends an entry; returns its id. Zero postings are dropped.
std::int64_t post_entry(Ledger& ledger, const Date& date, std::string_view memo,
                        std::string_view reference, const std::vector<Posting>& postings);
std::optional<JournalEntry> find_entry(const Ledger& ledger, std::int64_t id);
// Posts the mirror image of an entry; returns the new entry's id.
std::int64_t reverse_entry(Ledger& ledger, std::int64_t id, const Date& date);
std::vector<JournalEntry> entries_for_reference(const Ledger& ledger, std::string_view reference);
// Customer order: debit gateway receivable (gross), credit sales (net), shipping revenue
// and GST output.
std::int64_t record_sale(Ledger& ledger, const Date& date, std::string_view order_ref,
                         const money::Money& net, const money::Money& tax,
                         const money::Money& shipping);
// Gateway payout: debit bank (net) and gateway fees, credit gateway receivable (gross).
std::int64_t record_settlement(Ledger& ledger, const Settlement& settlement,
                               std::string_view order_ref);
// Refund to the customer: debit sales returns (net) and GST output (tax), credit gateway
// receivable.
std::int64_t record_refund(Ledger& ledger, const Date& date, std::string_view order_ref,
                           const money::Money& net, const money::Money& tax);

// ---- balances.cpp -----------------------------------------------------------
// Debit-positive balance of an account and its sub-accounts up to `as_of` (inclusive).
money::Money account_balance(const Ledger& ledger, std::string_view account, const Date& as_of);
// Balance in the account's normal direction (credit-normal accounts flip the sign).
money::Money normal_balance(const Ledger& ledger, std::string_view account, const Date& as_of);
// One row per account with activity, sorted by name; the balance appears in the debit or
// credit column; zero-balance accounts are omitted.
std::vector<TrialBalanceRow> trial_balance(const Ledger& ledger, const Date& as_of);
// Total debits equal total credits.
bool trial_balance_is_even(const std::vector<TrialBalanceRow>& rows);
// Normal balances summed per account type.
std::map<AccountType, money::Money> balances_by_type(const Ledger& ledger, const Date& as_of);
// Revenue minus expenses for entries dated within [from, to].
money::Money net_income(const Ledger& ledger, const Date& from, const Date& to);
// Every posting to the account (or its sub-accounts) within [from, to], with a running
// debit-positive balance that starts from the balance before `from`.
std::vector<StatementLine> account_statement(const Ledger& ledger, std::string_view account,
                                             const Date& from, const Date& to);

// ---- reconcile.cpp ----------------------------------------------------------
// Parses data/settlements.csv format.
std::vector<Settlement> parse_settlements_csv(std::string_view csv);
// gross - fee == net, fee between zero and gross, one currency.
bool is_settlement_consistent(const Settlement& settlement);
// Fee as basis points of the gross amount (half-even).
std::int64_t gateway_fee_bp(const Settlement& settlement);
// Gateway receivable balance per reference with a non-zero balance, sorted by reference.
std::vector<std::pair<std::string, money::Money>> open_receivables(const Ledger& ledger,
                                                                   const Date& as_of);
// Matches settlements (by payment id = reference) against open receivables as of the
// latest settlement date. Lists are sorted.
Reconciliation reconcile_settlements(const Ledger& ledger,
                                     const std::vector<Settlement>& settlements);
// Moves the period's revenue and expense balances into retained earnings with one
// closing entry dated `period_end`. Returns its id, or 0 when there is nothing to close.
std::int64_t close_period(Ledger& ledger, const Date& period_start, const Date& period_end);

}  // namespace shopcore::ledger
