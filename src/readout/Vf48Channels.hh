#pragma once
#include <string>

namespace vf48 {
// Scintillator index 0-15 and PMT end ("D" or "U") to the VF48 field that carries
// that PMT's ADC count, following the TriggerApp channel order (FPGA0/1 = D end,
// FPGA2/3 = U end; scintillators run 0, 15, 14, ..., 9 on FPGA0 and 8, 7, ..., 1 on FPGA1).
std::string FieldName(int scintillator, bool uEnd);
// Channel label of that PMT, such as "D15" or "U3".
std::string ChannelLabel(int scintillator, bool uEnd);
} // namespace vf48
