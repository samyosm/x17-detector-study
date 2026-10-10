#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <toml++/toml.h>
#include <vector>

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

  template <class T> void Set(const std::string &key, T value) {
    const auto split = key.rfind('.');
    ParentTable(key.substr(0, split))
        .insert_or_assign(key.substr(split + 1), value);
  }

  std::vector<std::string> Keys(const std::string &table) const;
  std::array<double, 3> Vector(const std::string &key) const;
  std::filesystem::path Path(const std::string &key) const;

  const std::filesystem::path &File() const { return file_; }

private:
  toml::table &ParentTable(const std::string &path);
  std::shared_ptr<toml::table> document_;
  std::filesystem::path directory_;
  std::filesystem::path file_;
};
