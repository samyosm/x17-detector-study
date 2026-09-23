#include "physics/sources/SourceFactory.hh"

#include "physics/sources/CosmicMuonSource.hh"
#include "physics/sources/GunSource.hh"
#include "physics/sources/IpcSource.hh"
#include "physics/sources/MixedSource.hh"
#include "physics/sources/X17Source.hh"

#include "G4SystemOfUnits.hh"
#include <stdexcept>

std::unique_ptr<PrimarySource>
CreatePrimarySource(const Configuration &settings) {
  const bool cosmic = settings.Get<bool>("source.cosmic_muons.enable", false);
  const bool gun = settings.Get<bool>("source.gun.enable", false);
  const bool ipc = settings.Get<bool>("source.ipc.enable", false);
  const bool x17 = settings.Get<bool>("source.x17.enable", false);
  if (int(cosmic) + int(gun) + int(ipc || x17) != 1)
    throw std::invalid_argument(
        "Enable gun, cosmic_muons, or IPC/X17 (IPC and X17 may mix)");
  if (cosmic)
    return std::make_unique<CosmicMuonSource>(settings);
  if (gun)
    return std::make_unique<GunSource>(settings);
  const double energy =
      settings.Get<double>("source.pair.transition_energy_mev") * MeV;
  if (!x17)
    return std::make_unique<IpcSource>(energy);
  const double mass = settings.Get<double>("source.x17.mass_mev") * MeV;
  if (!ipc)
    return std::make_unique<X17Source>(energy, mass);
  return std::make_unique<MixedSource>(
      energy, mass, settings.Get<double>("source.x17.fraction"));
}
