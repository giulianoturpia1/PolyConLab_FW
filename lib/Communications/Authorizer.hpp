///
/// @file Authorizer.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Operator-role authorization for the communications layer.
/// @version 0.1
/// @date 12-08-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include "Definitions.hpp"
#include "Helpers.hpp"

namespace modules {

///
/// @brief Owns the operator/observer role state and decides whether a sender may actuate.
///
/// Every client that talks to the PLC is one of two roles. The @b operator is the single
/// privileged client: it may issue actuation commands — those that drive a physical output
/// (heater/pump/chiller on-off, valve open-close) — as well as every read-only command. The local
/// Serial console is always an operator, since it has physical access. An @b observer is any other
/// client: read-only, so state/sensor queries, PING, STATE?, SYST:LOG* and SYST:FAULT? are allowed
/// but actuation is rejected with kErrorNotOperator. Observers can watch the plant without acting.
///
/// Over WiFi the role is unclaimed at boot (operator_id_ == 0). The first client to interact claims
/// it (first-client-wins) via claim(); every later client stays an observer until the operator
/// leaves. release() — called when the WiFi link drops — clears the role so the next client can
/// claim it.
///
/// @note With config::kApMaxClients == 1 only one WiFi station can associate at a time, so in
/// practice the sole connected client is always the operator and the observer path is exercised
/// only against Serial. The gate is written for the general multi-client case and takes effect for
/// additional observers as soon as the client cap is raised.
///
class Authorizer {
   public:
    ///
    /// @brief Claim the operator role for @p client_id if it is currently unclaimed.
    ///
    /// @param client_id WiFi client that just interacted.
    /// @return true if the role was newly claimed (caller may log it), false if already held.
    ///
    bool claim(definitions::ClientId client_id) {
        if (operator_id_ != 0) {
            return false;
        }
        operator_id_ = client_id;
        return true;
    }

    ///
    /// @brief Release the operator role so the next client can claim it.
    ///
    void release() {
        operator_id_ = 0;
    }

    ///
    /// @brief Whether @p sender on @p transport currently holds the operator role.
    ///
    /// @param transport Transport the message arrived on.
    /// @param sender    Identity of the sending client (0 for serial).
    /// @return true if the sender is the operator.
    ///
    bool is_operator(definitions::Transport transport, definitions::ClientId sender) const {
        return utils::sender_is_operator(transport, sender, operator_id_);
    }

    ///
    /// @brief Whether @p cmd may run: non-actuation commands are open, actuation requires the operator.
    ///
    /// @param cmd       Matched command table entry.
    /// @param transport Transport the message arrived on.
    /// @param sender    Identity of the sending client (0 for serial).
    /// @return true if the command may proceed, false if it must be rejected.
    ///
    bool is_authorized(const definitions::CommandEntry& cmd, definitions::Transport transport, definitions::ClientId sender) const {
        return !cmd.actuates || is_operator(transport, sender);
    }

   private:
    definitions::ClientId operator_id_ = 0;  ///< Client holding the operator role; 0 = unclaimed.
};

}  // namespace modules
