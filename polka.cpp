#include "statement.h"

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <ranges>
#include <cassert>
#include <array>
#include <regex>
#include <algorithm>
//#free_doorov

class Combine: public Statement {
public:
    Combine() = default;
    Combine(const Combine &c) = default;
    Combine(std::shared_ptr<Statement> l, std::shared_ptr<Statement> r)
        : Statement(l->get_arguments_count() + std::max(int(r->get_arguments_count()) - int(l->get_results_count()), 0),
                r->get_results_count() + std::max(int(l->get_results_count()) - int(r->get_arguments_count()), 0),
                l->is_pure() && r->is_pure()), l(l), r(r) {}    
    
    std::vector<int> apply(std::vector<int> in) const override{ 
        return r->apply(l->apply(in)); 
    } 

private:
    std::shared_ptr<Statement> l, r;
};

class ConstOp: public Statement {
public:
    ConstOp(int v) : Statement(0, 1, true), v(v) {
    }

    std::vector<int> apply(std::vector<int> in) const override {
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

    std::vector<int> apply(std::vector<int> in) const override {
        if (in.size() < 2) {
            return in;
        }
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

    std::vector<int> apply(std::vector<int> in) const override {
        int b = in.back();
        in.pop_back();
        in.push_back(std::abs(b));
        return in;
    }

};

class Input : public Statement {
public:
    Input() : Statement(0, 1, false) {}

    std::vector<int> apply(std::vector<int> in) const override {
        int value;
        std::cin >> value;
        in.push_back(value);
        return in;
    }
};

class Dup : public Statement {
public:
    Dup() : Statement(1, 2, true) {}

    std::vector<int> apply(std::vector<int> in) const override {
        int a = in.back();
        in.push_back(a);
        return in;
    }
};

class BlankStr : public Statement { 
public:
    BlankStr() : Statement(0, 0, true) {}

    std::vector<int> apply(std::vector<int> in) const override {
        return in;
    }
};

std::shared_ptr<Statement> operator|(std::shared_ptr<Statement> lhs, std::shared_ptr<Statement> rhs)
{
    return std::make_shared<Combine>(lhs, rhs);
}


struct OperatorMap {
    std::string token;
    std::function<std::shared_ptr<Statement>()> create;
};

std::shared_ptr<Statement> compile(std::string_view str) {
    if (str.empty() || str.find_first_not_of(" ") == std::string::npos) {
        return std::make_shared<BlankStr>();
    }

    const std::array<OperatorMap, 8> operator_mappings = {{
        {"+", []() { return std::make_shared<BinaryOP<std::plus<>{}>>(); }},
        {"-", []() { return std::make_shared<BinaryOP<std::minus<>{}>>(); }},
        {"/", []() { return std::make_shared<BinaryOP<std::divides<>{}>>(); }},
        {"*", []() { return std::make_shared<BinaryOP<std::multiplies<>{}>>(); }},
        {"%", []() { return std::make_shared<BinaryOP<std::modulus<>{}>>(); }},
        {"abs", []() { return std::make_shared<Abs>(); }},
        {"input", []() { return std::make_shared<Input>(); }},
        {"dup", []() { return std::make_shared<Dup>(); }}
    }};

    std::regex token_regex(R"(\S+)");
    std::regex number_regex(R"([-+]?\d+)");

    auto tokens_begin = std::regex_iterator<std::string_view::iterator>{str.begin(), str.end(), token_regex};
    auto tokens_end = std::regex_iterator<std::string_view::iterator>{};

    std::shared_ptr<Statement> ret;
    for (auto it = tokens_begin; it != tokens_end; ++it) {
        std::string token = it->str();

        if (std::regex_match(token, number_regex)) {
            auto const_op = std::make_shared<ConstOp>(std::stoi(token));
            ret = !ret ? const_op : (ret | const_op);
        } else {
            auto op_it = std::find_if(operator_mappings.begin(), operator_mappings.end(),
                                      [&token](const OperatorMap &op) { return op.token == token; });

            if (op_it != operator_mappings.end()) {
                auto op_stmt = op_it->create();
                ret = !ret ? op_stmt : (ret | op_stmt);
            }
        }
    }

    return ret;

}


std::shared_ptr<Statement> optimize(std::shared_ptr<Statement> stmt);
