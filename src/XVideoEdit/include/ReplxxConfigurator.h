// ReplxxConfigurator.h
#pragma once

#include <replxx.hxx>
#include <functional>
#include <memory>

class ReplxxConfigurator
{
public:
    using CompletionCallback = std::function<replxx::Replxx::completions_t(const std::string&, int&)>;
    using HintCallback       = std::function<replxx::Replxx::hints_t(const std::string&, int&, replxx::Replxx::Color&)>;

    static void configure(std::unique_ptr<replxx::Replxx>& rx, const CompletionCallback& completionCallback,
                          const HintCallback& hintCallback);
    static void bindKeys(replxx::Replxx& rx);

private:
    static void setBasicConfig(replxx::Replxx& rx);
    static void setWordBreakCharacters(replxx::Replxx& rx);
};
