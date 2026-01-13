#include "../include/config.hpp"

void configSet(
    const std::string &key,
    const std::string &value,
    ConfigScope scope)
{
  configLoad();
  setConfigValue(key, value, scope);

  if (scope == ConfigScope::GLOBAL)
  {
    saveGlobal();
  }
  else
  {
    saveRepo();
  }

  std::cout << "Set " << (scope == ConfigScope::GLOBAL ? "global" : "repository") << " config: " << key << " = " << value << std::endl;
}

std::string configGet(
    const std::string &key,
    ConfigScope scope)
{
  // Load existing configuration
  configLoad();

  if (scope == ConfigScope::GLOBAL)
  {
    // Check global config only
    auto it = global_config.find(key);
    return (it != global_config.end()) ? it->second : "";
  }
  else
  {
    // Check repository config first, then global config
    auto it = repository_config.find(key);
    if (it != repository_config.end())
    {
      return it->second;
    }

    // Fallback to global config
    it = global_config.find(key);
    return (it != global_config.end()) ? it->second : "";
  }
}

void configUnset(
    const std::string &key,
    ConfigScope scope)
{
  configLoad();
  if (scope == ConfigScope::GLOBAL)
  {
    global_config.erase(key);
  }
  else
  {
    repository_config.erase(key);
  }

  if (scope == ConfigScope::GLOBAL)
  {
    saveGlobal();
  }
  else
  {
    saveRepo();
  }

  std::cout << "Unset " << (scope == ConfigScope::GLOBAL ? "global" : "repository") << " config: " << key << std::endl;
}

void configList(ConfigScope scope)
{
  // Load existing configuration
  configLoad();

  // Pointer to the appropriate config map
  std::map<std::string, std::string> *config_map = nullptr;
  std::string scope_name;

  // Determine which config map to use
  if (scope == ConfigScope::GLOBAL)
  {
    config_map = &global_config;
    scope_name = "global";
  }
  else
  {
    config_map = &repository_config;
    scope_name = "repository";
  }

  std::cout << scope_name << " configuration:" << std::endl;

  // Check if the config map is empty
  if (config_map->empty())
  {
    std::cout << "  (no configuration set)" << std::endl;
    return;
  }

  // Print all key-value pairs in the config map
  for (const auto &entry : *config_map)
  {
    std::cout << "  " << entry.first << " = " << entry.second << std::endl;
  }
}

void configLoad()
{
  global_config.clear();
  repository_config.clear();

  // Load global
  std::string global_path = getGlobalConfigPath();
  if (std::filesystem::exists(global_path))
  {
    std::istringstream file(ErrorHandler::safeReadFile(global_path));
    std::string line;
    while (std::getline(file, line))
    {
      if (line.empty() || line[0] == '#')
      {
        continue;
      }

      auto pos = line.find('=');

      if (pos != std::string::npos)
      {
        global_config[line.substr(0, pos)] = line.substr(pos + 1);
      }
    }
  }

  // Load repo
  std::string repo_path = getRepositoryConfigPath();
  if (std::filesystem::exists(repo_path))
  {
    std::istringstream file(ErrorHandler::safeReadFile(repo_path));
    std::string line;
    while (std::getline(file, line))
    {
      if (line.empty() || line[0] == '#')
      {
        continue;
      }

      auto pos = line.find('=');

      if (pos != std::string::npos)
      {
        repository_config[line.substr(0, pos)] = line.substr(pos + 1);
      }
    }
  }
}

void setConfigValue(
    const std::string &key,
    const std::string &value, ConfigScope scope)
{
  // Set the key-value pair in the appropriate config map
  if (scope == ConfigScope::GLOBAL)
  {
    global_config[key] = value;
  }
  else
  {
    repository_config[key] = value;
  }
}

std::string getGlobalConfigPath()
{
  // Get the HOME environment variable
  const char *home = std::getenv("HOME");
  if (home)
  {
    return std::string(home) + "/.bittrack/config";
  }
  return ".bittrack/config";
}

std::string getRepositoryConfigPath()
{
  return ".bittrack/config";
}

void saveGlobal()
{
  std::string global_path = getGlobalConfigPath();
  std::filesystem::create_directories(std::filesystem::path(global_path).parent_path());

  std::string content = "# BitTrack Global Configuration\n";
  for (const auto &entry : global_config)
    content += entry.first + "=" + entry.second + "\n";

  ErrorHandler::safeWriteFile(global_path, content);
}

void saveRepo()
{
  std::string repo_path = getRepositoryConfigPath();
  std::filesystem::create_directories(std::filesystem::path(repo_path).parent_path());

  std::string content = "# BitTrack Repository Configuration\n";
  for (const auto &entry : repository_config)
    content += entry.first + "=" + entry.second + "\n";

  ErrorHandler::safeWriteFile(repo_path, content);
}
