#include "statement.h"

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <ranges>
#include <unordered_map>
#include <regex>
#include <algorithm>


class Combine: public Statement {
public:
    Combine() = default;
    Combine(const Combine &c) = default;
    Combine(const std::shared_ptr<Statement>& l, const std::shared_ptr<Statement>& r)
        : Statement(calculateArgumentsCount(l, r), calculateResultsCount(l, r), calculateIsPure(l, r)), l(l), r(r) {}    
    
    std::vector<int> apply(std::vector<int> in) const override{ 
        return r->apply(l->apply(in)); 
    } 
    inline std::shared_ptr<Statement> get_l() const {
        return l;
    }
    inline std::shared_ptr<Statement> get_r() const {
        return r;
    }

private:
    std::shared_ptr<Statement> l, r;
    static unsigned calculateArgumentsCount(const std::shared_ptr<Statement>& l, const std::shared_ptr<Statement>& r) {
        return l->get_arguments_count() + std::max(static_cast<int>(r->get_arguments_count()) - static_cast<int>(l->get_results_count()), 0);
    }

    static unsigned calculateResultsCount(const std::shared_ptr<Statement>& l, const std::shared_ptr<Statement>& r) {
        return r->get_results_count() + std::max(static_cast<int>(l->get_results_count()) - static_cast<int>(r->get_arguments_count()), 0);
    }

    static bool calculateIsPure(const std::shared_ptr<Statement>& l, const std::shared_ptr<Statement>& r) {
        return l->is_pure() && r->is_pure();
    }

};

class ConstOp: public Statement {
public:
    ConstOp(int v) : Statement(0, 1, true), v(v) {
    }

    inline std::vector<int> apply(std::vector<int> in) const override {
        in.push_back(v);
        return in;
    }

private:
    int v;
};


template<auto func>
class BinaryOP: public Statement {
public:
    BinaryOP() : Statement(2, 1, true) {}

    inline std::vector<int> apply(std::vector<int> in) const override {
        int b = in.back();
        in.pop_back();
        int a = in.back();
        in.pop_back();
        in.push_back(func(a, b));

        return in;
    }
};

class Abs : public Statement { 
public:
    Abs() : Statement(1, 1, true) {}

    inline std::vector<int> apply(std::vector<int> in) const override {
        int b = in.back();
        in.pop_back();
        in.push_back(std::abs(b));
        return in;
    }

};

class Input : public Statement {
public:
    Input() : Statement(0, 1, false) {}

    inline std::vector<int> apply(std::vector<int> in) const override {
        int value;
        std::cin >> value;
        in.push_back(value);
        return in;
    }
};

class Dup : public Statement {
public:
    Dup() : Statement(1, 2, true) {}

    inline std::vector<int> apply(std::vector<int> in) const override {
        int a = in.back();
        in.push_back(a);
        return in;
    }
};

class BlankStr : public Statement { 
public:
    BlankStr() : Statement(0, 0, true) {}

    inline std::vector<int> apply(std::vector<int> in) const override {
        return in;
    }
};

std::shared_ptr<Statement> operator|(std::shared_ptr<Statement> lhs, std::shared_ptr<Statement> rhs)
{
    return std::make_shared<Combine>(lhs, rhs);
}



std::shared_ptr<Statement> optimize(std::shared_ptr<Statement> stmt) {
    if (auto combine_stmt = dynamic_cast<Combine*>(stmt.get())) {
        auto left_optimized = optimize(combine_stmt->get_l());
        auto right_optimized = optimize(combine_stmt->get_r());

        if (right_optimized->get_arguments_count() > left_optimized->get_results_count()) {
            return stmt; 
        }

        if (auto left_const = dynamic_cast<ConstOp*>(left_optimized.get())) {
            if (auto right_const = dynamic_cast<ConstOp*>(right_optimized.get())) {
                std::vector<int> input;
                input = right_const->apply(left_const->apply(input));
                return std::make_shared<ConstOp>(input.back());
            }
        }

        return std::make_shared<Combine>(left_optimized, right_optimized);
    }
    return stmt;
}


std::shared_ptr<Statement> compile(std::string_view str) {
    if (str.empty() || str.find_first_not_of(" ") == std::string::npos) {
        return std::make_shared<BlankStr>();
    }

    static const std::unordered_map<std::string, std::function<std::shared_ptr<Statement>()>> operator_mapping = {
        {"+", []() { return std::make_shared<BinaryOP<std::plus<>{}>>(); }},
        {"-", []() { return std::make_shared<BinaryOP<std::minus<>{}>>(); }},
        {"/", []() { return std::make_shared<BinaryOP<std::divides<>{}>>(); }},
        {"*", []() { return std::make_shared<BinaryOP<std::multiplies<>{}>>(); }},
        {"%", []() { return std::make_shared<BinaryOP<std::modulus<>{}>>(); }},
        {"abs", []() { return std::make_shared<Abs>(); }},
        {"input", []() { return std::make_shared<Input>(); }},
        {"dup", []() { return std::make_shared<Dup>(); }}
    };

    static const std::regex token_regex(R"(\S+)");
    static const std::regex number_regex(R"([-+]?\d+)");

    auto tokens_begin = std::regex_iterator<std::string_view::iterator>{str.begin(), str.end(), token_regex};
    auto tokens_end = std::regex_iterator<std::string_view::iterator>{};

    std::shared_ptr<Statement> ret;
    for (auto it = tokens_begin; it != tokens_end; ++it) {
        const std::string token = it->str();

        if (std::regex_match(token, number_regex)) {
            auto const_op = std::make_shared<ConstOp>(std::stoi(token));
            ret = !ret ? std::move(const_op) : (ret | std::move(const_op));
        } else {
            auto op_it = operator_mapping.find(token);

            if (op_it != operator_mapping.end()) {
                auto op_stmt = op_it->second();
                ret = !ret ? std::move(op_stmt) : (ret | std::move(op_stmt));
            }
        }
    }
    return optimize(ret);
}
