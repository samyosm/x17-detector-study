#include "configuration/Configuration.hh"

Configuration::Configuration(const std::filesystem::path &file)
    : document_(
          std::make_shared<const toml::table>(toml::parse_file(file.string()))),
      directory_(file.parent_path()), file_(file) {}

std::array<double, 3> Configuration::Vector(const std::string &key) const {
  const auto *values = document_->at_path(key).as_array();
  if (!values || values->size() != 3)
    throw std::runtime_error("Expected three numbers: " + key);
  std::array<double, 3> result;
  for (std::size_t i = 0; i < result.size(); ++i) {
    const auto value = (*values)[i].value<double>();
    if (!value)
      throw std::runtime_error("Expected three numbers: " + key);
    result[i] = *value;
  }
  return result;
}

std::filesystem::path Configuration::Path(const std::string &key) const {
  return (directory_ / Get<std::string>(key)).lexically_normal();
}
