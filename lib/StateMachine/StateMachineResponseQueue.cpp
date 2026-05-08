#include "StateMachineResponseQueue.hpp"

#include "StateMachine.hpp"

namespace modules {

StateMachineResponseQueue& StateMachineResponseQueue::instance() {
    static StateMachineResponseQueue instance;
    return instance;
}

bool StateMachineResponseQueue::publish(const definitions::Command& response, uint32_t timeout) {
    return modules::StateMachine::instance().queue_response(response, timeout);
}

}  // namespace modules
