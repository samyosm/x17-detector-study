#include "readout/Vf48Channels.hh"

namespace vf48 {
std::string FieldName(int scintillator, bool uEnd) {
  const int position = (16 - scintillator) % 16;
  return "FPGA" + std::to_string(position / 8 + (uEnd ? 2 : 0)) + "_ADC" +
         static_cast<char>('A' + position % 8);
}

std::string ChannelLabel(int scintillator, bool uEnd) {
  return std::string(uEnd ? "U" : "D") + std::to_string(scintillator);
}
} // namespace vf48
