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
/// Clients fall into two roles. The @b operator is the single privileged client — it may drive
/// physical outputs (heater/pump/chiller, valves) as well as read; the local Serial console is always
/// an operator, since it has physical access. An @b observer is any other client: reads (queries,
/// PING, STATE?, SYST:LOG*, SYST:FAULT?) are allowed, actuation is rejected with kErrorNotOperator.
///
/// Over WiFi the role is unclaimed at boot; the first client to interact claims it (first-client-wins)
/// and later clients stay observers until it leaves. sync_operator_presence(), called each network
/// tick, frees the role on disconnect — a SoftAP stays "available" with zero clients, so the link-drop
/// release() never fires over WiFi. release() remains for Ethernet, whose link genuinely drops.
///
/// @note With config::kApMaxClients == 1 only one station associates at a time, so the sole client is
/// always the operator and the observer path is exercised only against Serial; the gate still applies
/// to extra observers if the cap is raised.
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
    /// @brief Release the operator role if its holder is no longer the connected client.
    ///
    /// The role is claimed first-client-wins and must be freed when that client leaves, so the next
    /// client can claim it. Over a SoftAP the link stays "available" even with zero clients (no
    /// uplink to lose), so the link-drop release() never fires over WiFi — this presence check is
    /// what frees the role after a disconnect. Single-client contract: at most one client is
    /// connected, so any mismatch means the operator has disconnected (0 = nobody connected).
    ///
    /// @param connected_client Identity of the currently connected client (0 if none).
    /// @return true if the role was released (caller may log it), false if nothing changed.
    ///
    bool sync_operator_presence(definitions::ClientId connected_client) {
        if (operator_id_ != 0 && operator_id_ != connected_client) {
            operator_id_ = 0;
            return true;
        }
        return false;
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
