#include <array>
#include <utility>

#include "shopcore/ledger/ledger.hpp"

namespace shopcore::ledger {

namespace {

constexpr std::array<std::pair<std::string_view, AccountType>, 5> kTypes = {{
    {"assets", AccountType::Asset},
    {"liabilities", AccountType::Liability},
    {"equity", AccountType::Equity},
    {"revenue", AccountType::Revenue},
    {"expenses", AccountType::Expense},
}};

bool is_segment_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

std::string_view first_segment(std::string_view name) {
    const std::size_t colon = name.find(':');
    return colon == std::string_view::npos ? name : name.substr(0, colon);
}

}  // namespace

bool is_valid_account_name(std::string_view name) {
    const std::vector<std::string> segments = core::split(name, ':');
    if (segments.size() < 2) {
        return false;
    }
    for (const std::string& segment : segments) {
        if (segment.empty()) {
            return false;
        }
        for (const char c : segment) {
            if (!is_segment_char(c)) {
                return false;
            }
        }
    }
    const std::string_view type = first_segment(name);
    for (const auto& entry : kTypes) {
        if (entry.first == type) {
            return true;
        }
    }
    return false;
}

AccountType account_type(std::string_view name) {
    const std::string_view type = first_segment(name);
    for (const auto& entry : kTypes) {
        if (entry.first == type) {
            return entry.second;
        }
    }
    throw LedgerError("unknown account type in '" + std::string(name) + "'");
}

bool is_debit_normal(AccountType type) {
    return type == AccountType::Asset || type == AccountType::Expense;
}

std::string parent_account(std::string_view name) {
    const std::size_t colon = name.rfind(':');
    if (colon != std::string_view::npos) {
        return "";
    }
    return std::string(name.substr(0, colon));
}

bool is_under(std::string_view account, std::string_view ancestor) {
    if (ancestor.empty()) {
        return false;
    }
    if (account.size() < ancestor.size() || account.substr(0, ancestor.size()) != ancestor) {
        return false;
    }
    return account.size() == ancestor.size() || account[ancestor.size()] == ':';
}

}  // namespace shopcore::ledger
