///
/// @file TargetDefinitions.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Configuration file for include obfuscation and target-specific definitions.
///
/// This file has include directives to avoid exposing unnecessary "#if defined" clauses in the firmware.
/// By doing "#include <TargetDefinitions.hpp>", the firmware can include all necessary definitions of certain pins, peripherals,
/// and other target-specific configurations without conditional compilation directives.
/// @version 0.1
/// @date 02-07-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#endif
