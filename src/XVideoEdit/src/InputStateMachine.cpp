#include "InputStateMachine.h"

class InputStateMachine::PImpl
{
public:
    PImpl(InputStateMachine *owenr);
    ~PImpl() = default;

public:
    InputStateMachine *owenr_        = nullptr;
    State              currentState_ = State::Initializing;
};

InputStateMachine::PImpl::PImpl(InputStateMachine *owenr) : owenr_(owenr)
{
}


InputStateMachine::InputStateMachine() : impl_(std::make_unique<InputStateMachine::PImpl>(this))
{
}

InputStateMachine::~InputStateMachine() = default;

auto InputStateMachine::transitionTo(State newState) -> void
{
    if (!canTransitionTo(newState))
    {
        throw std::runtime_error("Invalid state transition from " + stateToString(impl_->currentState_) + " to " +
                                 stateToString(newState));
    }
    impl_->currentState_ = newState;
}

auto InputStateMachine::canTransitionTo(State newState) const -> bool
{
    switch (impl_->currentState_)
    {
        case State::Initializing:
            return newState == State::Running || newState == State::Error;
        case State::Running:
            return newState == State::ProcessingCommand || newState == State::ShuttingDown || newState == State::Error;
        case State::ProcessingCommand:
            return newState == State::Running || newState == State::Error;
        case State::ShuttingDown:
            return false; /// 关闭后不能转换到其他状态
        case State::Error:
            return newState == State::Running; /// 可以从错误恢复
    }
    return false;
}

auto InputStateMachine::getCurrentState() const -> State
{
    return impl_->currentState_;
}

auto InputStateMachine::isRunning() const -> bool
{
    return impl_->currentState_ == State::Running;
}

auto InputStateMachine::isShuttingDown() const -> bool
{
    return impl_->currentState_ == State::ShuttingDown;
}

auto InputStateMachine::isError() const -> bool
{
    return impl_->currentState_ == State::Error;
}

auto InputStateMachine::stateToString(State state) -> std::string
{
    switch (state)
    {
        case State::Initializing:
            return "Initializing";
        case State::Running:
            return "Running";
        case State::ProcessingCommand:
            return "ProcessingCommand";
        case State::ShuttingDown:
            return "ShuttingDown";
        case State::Error:
            return "Error";
        default:
            return "Unknown";
    }
}
