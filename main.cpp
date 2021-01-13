#include <iostream>
#include <optional>

class Grammar {
public:
    [[nodiscard]]  virtual std::optional<size_t> topdown_match(std::string_view context, size_t index) const = 0;

    [[nodiscard]]  virtual Grammar *instance() const = 0;
};

#define GRAMMAR_INSTANCE(Type) Grammar* instance() const override { \
    static Type _STATIC;                                            \
    return &_STATIC;                                                \
}

// Contextual Conditions
class Start : public Grammar {
public:
    [[nodiscard]] std::optional<size_t> topdown_match(std::string_view context, size_t index) const override {
        return index == 0 ? std::make_optional(0) : std::nullopt;
    }

    GRAMMAR_INSTANCE(Start);
};

class End : public Grammar {
public:
    [[nodiscard]] std::optional<size_t> topdown_match(std::string_view context, size_t index) const override {
        return index >= context.size() ? std::make_optional(0) : std::nullopt;
    }

    GRAMMAR_INSTANCE(End);
};

// Lexical Definitions
template<char C>
class Char : public Grammar {
public:
    [[nodiscard]] std::optional<size_t> topdown_match(std::string_view context, size_t index) const override {
        return context[index] == C ? std::make_optional(1) : std::nullopt;
    }

    GRAMMAR_INSTANCE(Char);
};

template<char S, char T>
class CharRange : public Grammar {
public:
    [[nodiscard]] std::optional<size_t> topdown_match(std::string_view context, size_t index) const override {
        return context[index] >= S && context[index] <= T ? std::make_optional(1) : std::nullopt;
    }

    GRAMMAR_INSTANCE(CharRange);
};

// utilities
template<class Clause>
class Plus : public Grammar {
public:
    [[nodiscard]] std::optional<size_t> topdown_match(std::string_view context, size_t index) const override {
        size_t count = 0;
        bool flag = false;
        auto current = Clause().topdown_match(context, index + count);
        while (current) {
            flag = true;
            count += *current;
            current = Clause().topdown_match(context, index + count);
        }
        return flag ? std::make_optional(count) : std::nullopt;
    }

    GRAMMAR_INSTANCE(Plus);
};

template<class Clause>
class Asterisk : public Grammar {
public:
    [[nodiscard]] std::optional<size_t> topdown_match(std::string_view context, size_t index) const override {
        size_t count = 0;
        auto current = Clause().topdown_match(context, index + count);
        while (current) {
            count += *current;
            current = Clause().topdown_match(context, index + count);
        }
        return std::make_optional(count);
    }

    GRAMMAR_INSTANCE(Asterisk);
};

template<class Clause0, class ... Clauses>
class Seq : public Seq<Clauses...> {
public:
    [[nodiscard]] std::optional<size_t> topdown_match(std::string_view context, size_t index) const override {
        auto fst = Clause0().topdown_match(context, index);
        if (fst) {
            auto snd = Seq<Clauses...>::topdown_match(context, index + *fst);
            return snd ? std::make_optional(*fst + *snd) : std::nullopt;
        } else {
            return std::nullopt;
        }
    }

    GRAMMAR_INSTANCE(Seq);
};

template<class Clause>
class Seq<Clause> : public Grammar {
public:
    [[nodiscard]] std::optional<size_t> topdown_match(std::string_view context, size_t index) const override {
        return Clause().topdown_match(context, index);
    }

    GRAMMAR_INSTANCE(Seq);
};

template<class Clause0, class ... Clauses>
class Ord : public Ord<Clauses...> {
public:
    [[nodiscard]] std::optional<size_t> topdown_match(std::string_view context, size_t index) const override {
        auto fst = Clause0().topdown_match(context, index);
        if (fst) {
            return fst;
        } else {
            return Ord<Clauses...>::topdown_match(context, index);
        }
    }

    GRAMMAR_INSTANCE(Ord);
};

template<class Clause>
class Ord<Clause> : public Grammar {
public:
    [[nodiscard]] std::optional<size_t> topdown_match(std::string_view context, size_t index) const override {
        return Clause().topdown_match(context, index);
    }

    GRAMMAR_INSTANCE(Ord);
};

#define CLAUSE(Name, ...)  \
class Name : public __VA_ARGS__ { \
    public:                 \
    GRAMMAR_INSTANCE(Name); \
};


// Grammar

class Additive;

class Multicative;

class Primary;

class Toplevel;

CLAUSE(Digit, CharRange<'0', '9'>)

CLAUSE(Number, Plus<Digit>)

CLAUSE(Additive, Ord<Seq<Multicative, Char<'+'>, Additive>, Multicative>)

CLAUSE(Multicative, Ord<Seq<Primary, Char<'*'>, Multicative>, Primary>)

CLAUSE(Primary, Ord<Seq<Char<'('>, Additive, Char<')'>>, Number>)

CLAUSE(Toplevel, Seq<Start, Additive, End>)

struct MemoKey {
    Grammar *instance;
    size_t index;
};

constexpr static uint64_t KEY[2]{0x243f6a8885a308d3ull,
                                 0x13198a2e03707344ull};

struct MemoKeyHasher {
    size_t operator()(const MemoKey & key) {
        return std::hash<void *>{}(key.instance) ^ std::hash<size_t>{}(-key.index);
    }
};

int main() {
    std::cout << Toplevel().topdown_match("(1+1)+1*(5+5)", 0).value_or(0) << std::endl;
    std::cout << MemoKeyHasher()(MemoKey{.instance = static_cast<Grammar *>(malloc(1)), .index = 11});
    return 0;
}
