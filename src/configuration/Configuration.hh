#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <toml++/toml.h>

class Configuration {
public:
  explicit Configuration(const std::filesystem::path &file);

  template <class T> T Get(const std::string &key) const {
    if (auto value = document_->at_path(key).value<T>())
      return *value;
    throw std::runtime_error("Missing or invalid configuration field: " + key);
  }

  template <class T> T Get(const std::string &key, T fallback) const {
    return document_->at_path(key) ? Get<T>(key) : fallback;
  }

  std::array<double, 3> Vector(const std::string &key) const;
  std::filesystem::path Path(const std::string &key) const;

  const std::filesystem::path &File() const { return file_; }

private:
  std::shared_ptr<const toml::table> document_;
  std::filesystem::path directory_;
  std::filesystem::path file_;
};
