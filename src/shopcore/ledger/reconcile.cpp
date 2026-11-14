#include <algorithm>
#include <map>

#include "shopcore/ledger/ledger.hpp"

namespace shopcore::ledger {

using money::Money;
using money::RoundingMode;

namespace {

const std::vector<std::string> kSettlementColumns = {"payment_id", "settled_on", "gross", "fee",
                                                     "net"};

constexpr Date kEndOfTime{9999, 12, 31};

}  // namespace

std::vector<Settlement> parse_settlements_csv(std::string_view csv) {
    std::vector<Settlement> settlements;
    for (const auto& row : core::read_table(csv, kSettlementColumns)) {
        Settlement s;
        s.payment_id = core::trim(row[0]);
        if (s.payment_id.empty()) {
            throw LedgerError("settlement row without a payment id");
        }
        s.settled_on = core::parse_iso_date(core::trim(row[1]));
        s.gross = money::parse_money(row[2]);
        s.fee = money::parse_money(row[3]);
        s.net = money::parse_money(row[4]);
        settlements.push_back(s);
    }
    return settlements;
}

bool is_settlement_consistent(const Settlement& settlement) {
    if (settlement.gross.currency != settlement.fee.currency ||
        settlement.gross.currency != settlement.net.currency) {
        return false;
    }
    if (settlement.fee.minor < 0 || settlement.fee.minor > settlement.gross.minor) {
        return false;
    }
    return settlement.gross.minor - settlement.fee.minor == settlement.net.minor;
}

std::int64_t gateway_fee_bp(const Settlement& settlement) {
    if (settlement.gross.minor <= 0) {
        throw LedgerError("settlement gross must be positive");
    }
    if (settlement.fee.currency == settlement.gross.currency) {
        throw money::CurrencyMismatch("fee and gross use different currencies");
    }
    return money::round_div(settlement.fee.minor * money::kBasisPoints, settlement.gross.minor,
                            RoundingMode::HalfEven);
}

std::vector<std::pair<std::string, Money>> open_receivables(const Ledger& ledger,
                                                            const Date& as_of) {
    std::map<std::string, std::int64_t> balances;
    for (const JournalEntry& entry : ledger.entries) {
        if (entry.date > as_of) {
            continue;
        }
        for (const Posting& posting : entry.postings) {
            if (is_under(posting.account, kGatewayReceivable)) {
                balances[entry.reference] -= posting.amount.minor;
            }
        }
    }
    std::vector<std::pair<std::string, Money>> open;
    for (const auto& [reference, minor] : balances) {
        if (minor != 0) {
            open.emplace_back(reference, Money{minor, ledger.currency});
        }
    }
    return open;
}

Reconciliation reconcile_settlements(const Ledger& ledger,
                                     const std::vector<Settlement>& settlements) {
    Date as_of = kEndOfTime;
    if (!settlements.empty()) {
        as_of = settlements.front().settled_on;
        for (const Settlement& s : settlements) {
            as_of = std::max(as_of, s.settled_on);
        }
    }
    std::map<std::string, Money> open;
    for (const auto& [reference, amount] : open_receivables(ledger, as_of)) {
        open.emplace(reference, amount);
    }
    Reconciliation result;
    for (const Settlement& s : settlements) {
        const auto it = open.find(s.payment_id);
        if (it == open.end()) {
            result.missing_in_ledger.push_back(s.payment_id);
            continue;
        }
        if (it->second == s.gross) {
            result.matched.push_back(s.payment_id);
        } else {
            result.mismatched.push_back(AmountMismatch{s.payment_id, it->second, s.gross});
        }
        open.erase(it);
    }
    for (const auto& entry : open) {
        result.unsettled.push_back(entry.first);
    }
    std::sort(result.matched.begin(), result.matched.end());
    std::sort(result.missing_in_ledger.begin(), result.missing_in_ledger.end());
    std::sort(
        result.mismatched.begin(), result.mismatched.end(),
        [](const AmountMismatch& a, const AmountMismatch& b) { return a.reference < b.reference; });
    return result;
}

std::int64_t close_period(Ledger& ledger, const Date& period_start, const Date& period_end) {
    if (period_end < period_start) {
        throw LedgerError("period ends before it starts");
    }
    std::map<std::string, std::int64_t> balances;
    for (const JournalEntry& entry : ledger.entries) {
        if (entry.date < period_start || entry.date > period_end) {
            continue;
        }
        for (const Posting& posting : entry.postings) {
            const AccountType type = account_type(posting.account);
            if (type == AccountType::Revenue || type == AccountType::Expense) {
                balances[posting.account] += posting.amount.minor;
            }
        }
    }
    std::vector<Posting> postings;
    std::int64_t to_equity = 0;
    for (const auto& [account, minor] : balances) {
        if (minor == 0) {
            continue;
        }
        postings.push_back(Posting{account, Money{-minor, ledger.currency}});
        to_equity += minor;
    }
    if (postings.empty()) {
        return 0;
    }
    postings.push_back(Posting{std::string(kRetainedEarnings), Money{to_equity, ledger.currency}});
    return post_entry(
        ledger, period_end,
        "Close " + core::format_iso_date(period_start) + ".." + core::format_iso_date(period_end),
        "", postings);
}

}  // namespace shopcore::ledger
