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
/// @brief Owns the operator-role state and decides whether a sender may actuate.
///
/// Serial is always privileged. Over WiFi the first client to interact claims the role
/// (first-client-wins); actuation is gated to the operator while read-only queries stay open.
/// State-only and Logger-free, so it unit-tests natively.
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
