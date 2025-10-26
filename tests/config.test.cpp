#include "../include/config.hpp"

// test setting and getting a config value
bool test_config_set_and_get()
{
  config_set("test.key", "test.value");

  std::string value = config_get("test.key");

  return value == "test.value";
}

// test setting and getting user name
bool test_config_user_name()
{
  config_set("user.name", "Test User");
  std::string name = config_get("user.name");

  return name == "Test User";
}

// test setting and getting user email
bool test_config_user_email()
{
  config_set("user.email", "test@example.com");
  std::string email = config_get("user.email");

  return email == "test@example.com";
}

// test listing all config values
bool test_config_list()
{
  config_set("test.key1", "value1");
  config_set("test.key2", "value2");

  config_list();

  return true;
}

// test unsetting a config value
bool test_config_unset()
{
  config_set("test.unset.key", "test.value");

  std::string value_before = config_get("test.unset.key");
  bool was_set = (value_before == "test.value");

  config_unset("test.unset.key");

  std::string value_after = config_get("test.unset.key");
  bool is_unset = value_after.empty();

  return was_set && is_unset;
}

// test repository-specific config setting and getting
bool test_config_repository_config()
{
  config_set("repo.key", "repo.value", ConfigScope::REPOSITORY);

  std::string value = config_get("repo.key", ConfigScope::REPOSITORY);

  return value == "repo.value";
}

// test loading default configs
bool test_config_default_configs()
{
  config_load();

  std::string user_name = config_get("user.name");
  std::string user_email = config_get("user.email");

  return !user_name.empty() && !user_email.empty();
}
