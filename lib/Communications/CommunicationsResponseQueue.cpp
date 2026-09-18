#include "CommunicationsResponseQueue.hpp"

#include "Communications.hpp"

namespace modules {

CommunicationsResponseQueue& CommunicationsResponseQueue::instance() {
    static CommunicationsResponseQueue instance;
    return instance;
}

bool CommunicationsResponseQueue::publish(const definitions::Command& response, uint32_t timeout) {
    return modules::Communications::instance().queue_response(response, timeout);
}

}  // namespace modules
