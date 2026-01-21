#pragma once
#include "XConst.h"

#include <replxx.hxx>

class ReplxxConfigurator
{
public:
    using CompletionCallback = std::function<replxx::Replxx::completions_t(const std::string&, int&)>;
    using HintCallback       = std::function<replxx::Replxx::hints_t(const std::string&, int&, replxx::Replxx::Color&)>;

    static auto configure(const std::unique_ptr<replxx::Replxx>& rx, const CompletionCallback& completionCallback,
                          const HintCallback& hintCallback) -> void;

    static auto bindKeys(replxx::Replxx& rx) -> void;

private:
    static auto setBasicConfig(replxx::Replxx& rx) -> void;
    static auto setWordBreakCharacters(replxx::Replxx& rx) -> void;
};
