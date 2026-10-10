#include "configuration/Configuration.hh"
#include <algorithm>

Configuration::Configuration(const std::filesystem::path &file)
    : document_(
          std::make_shared<toml::table>(toml::parse_file(file.string()))),
      directory_(file.parent_path()), file_(file) {}

std::vector<std::string> Configuration::Keys(const std::string &table) const {
  const auto *node = document_->at_path(table).as_table();
  if (!node)
    throw std::runtime_error("Missing configuration table: " + table);
  std::vector<std::string> keys;
  for (const auto &[key, value] : *node)
    keys.emplace_back(key.str());
  return keys;
}

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

toml::table &Configuration::ParentTable(const std::string &path) {
  toml::table *table = document_.get();
  std::size_t start = 0;
  while (start <= path.size()) {
    const auto end = std::min(path.find('.', start), path.size());
    const auto name = path.substr(start, end - start);
    auto *child = table->get_as<toml::table>(name);
    table = child ? child : table->insert_or_assign(name, toml::table{}).first->second.as_table();
    start = end + 1;
  }
  return *table;
}
