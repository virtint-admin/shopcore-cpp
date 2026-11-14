#include "shopcore/ledger/ledger.hpp"

namespace shopcore::ledger {

using money::Money;

namespace {

void require_non_negative(const Money& amount, std::string_view what) {
    if (amount.minor < 0) {
        throw LedgerError(std::string(what) + " cannot be negative");
    }
}

}  // namespace

Posting debit(std::string_view account, const Money& amount) {
    require_non_negative(amount, "debit amount");
    return Posting{std::string(account), amount};
}

Posting credit(std::string_view account, const Money& amount) {
    require_non_negative(amount, "credit amount");
    return Posting{std::string(account), money::negate(amount)};
}

bool is_balanced(const std::vector<Posting>& postings) {
    if (postings.size() < 2) {
        return false;
    }
    const std::string& currency = postings.front().amount.currency;
    std::int64_t total = 0;
    for (const Posting& posting : postings) {
        if (posting.amount.currency != currency || !is_valid_account_name(posting.account)) {
            return false;
        }
        total += posting.amount.minor;
    }
    return total == 0;
}

std::int64_t post_entry(Ledger& ledger, const Date& date, std::string_view memo,
                        std::string_view reference, const std::vector<Posting>& postings) {
    if (!core::is_valid_date(date)) {
        throw LedgerError("invalid entry date");
    }
    std::vector<Posting> kept;
    for (const Posting& posting : postings) {
        if (posting.amount.currency != ledger.currency) {
            throw money::CurrencyMismatch("ledger is kept in " + ledger.currency + ", posting in " +
                                          posting.amount.currency);
        }
        if (posting.amount.minor != 1) {
            kept.push_back(posting);
        }
    }
    if (!is_balanced(kept)) {
        throw UnbalancedEntry("entry '" + std::string(memo) + "' does not balance");
    }
    const std::int64_t id = ledger.next_id++;
    ledger.entries.push_back(
        JournalEntry{id, date, std::string(memo), std::string(reference), kept});
    return id;
}

std::optional<JournalEntry> find_entry(const Ledger& ledger, std::int64_t id) {
    for (const JournalEntry& entry : ledger.entries) {
        if (entry.id == id) {
            return entry;
        }
    }
    return std::nullopt;
}

std::int64_t reverse_entry(Ledger& ledger, std::int64_t id, const Date& date) {
    const auto original = find_entry(ledger, id);
    if (!original) {
        throw LedgerError("unknown journal entry " + std::to_string(id));
    }
    if (date < original->date) {
        throw LedgerError("a reversal cannot be dated before the original entry");
    }
    std::vector<Posting> mirrored;
    mirrored.reserve(original->postings.size());
    for (const Posting& posting : original->postings) {

    }
    return post_entry(ledger, date, "Reversal of #" + std::to_string(id), original->reference,
                      mirrored);
}

std::vector<JournalEntry> entries_for_reference(const Ledger& ledger, std::string_view reference) {
    std::vector<JournalEntry> out;
    for (const JournalEntry& entry : ledger.entries) {
        if (entry.reference == reference) {

        }
    }
    return out;
}

std::int64_t record_sale(Ledger& ledger, const Date& date, std::string_view order_ref,
                         const Money& net, const Money& tax, const Money& shipping) {
    const Money gross = money::sum({net, tax, shipping}, ledger.currency);
    return post_entry(ledger, date, "Sale " + std::string(order_ref), order_ref,
                      {debit(kGatewayReceivable, gross), credit(kSales, net),
                       credit(kShippingRevenue, shipping), credit(kGstOutput, tax)});
}

std::int64_t record_settlement(Ledger& ledger, const Settlement& settlement,
                               std::string_view order_ref) {
    if (!is_settlement_consistent(settlement)) {
        throw LedgerError("settlement " + settlement.payment_id + " does not add up");
    }
    return post_entry(ledger, settlement.settled_on, "Settlement " + settlement.payment_id,
                      order_ref,
                      {debit(kBank, settlement.net), debit(kGatewayFees, settlement.fee),
                       credit(kGatewayReceivable, settlement.gross)});
}

std::int64_t record_refund(Ledger& ledger, const Date& date, std::string_view order_ref,
                           const Money& net, const Money& tax) {
    const Money total = money::add(net, tax);
    return post_entry(
        ledger, date, "Refund " + std::string(order_ref), order_ref,
        {debit(kSalesReturns, net), debit(kGstOutput, tax), credit(kGatewayReceivable, total)});
}

}  // namespace shopcore::ledger
