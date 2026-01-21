#pragma once
#include "XConst.h"

class InputStateMachine
{
public:
    enum class State
    {
        Initializing,
        Running,
        ProcessingCommand,
        ShuttingDown,
        Error
    };

    InputStateMachine();
    ~InputStateMachine();

public:
    auto transitionTo(State newState) -> void;

    auto canTransitionTo(State newState) const -> bool;

    auto getCurrentState() const -> State;

    /// \brief 是否运行中
    /// \return
    auto isRunning() const -> bool;

    /// \brief 是否关闭中
    /// \return
    auto isShuttingDown() const -> bool;

    /// \brief 是否处于错误状态
    /// \return
    auto isError() const -> bool;

    static auto stateToString(State state) -> std::string;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
