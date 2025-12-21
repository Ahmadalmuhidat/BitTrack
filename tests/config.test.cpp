#include "../include/config.hpp"

// test setting and getting a config value
bool test_config_set_and_get()
{
  configSet("test.key", "test.value");

  std::string value = configGet("test.key");

  return value == "test.value";
}

// test setting and getting user name
bool test_config_user_name()
{
  configSet("user.name", "Test User");
  std::string name = configGet("user.name");

  return name == "Test User";
}

// test setting and getting user email
bool test_config_user_email()
{
  configSet("user.email", "test@example.com");
  std::string email = configGet("user.email");

  return email == "test@example.com";
}

// test listing all config values
bool test_config_list()
{
  configSet("test.key1", "value1");
  configSet("test.key2", "value2");

  configList();

  return true;
}

// test unsetting a config value
bool test_config_unset()
{
  configSet("test.unset.key", "test.value");

  std::string value_before = configGet("test.unset.key");
  bool was_set = (value_before == "test.value");

  configUnset("test.unset.key");

  std::string value_after = configGet("test.unset.key");
  bool is_unset = value_after.empty();

  return was_set && is_unset;
}

// test repository-specific config setting and getting
bool test_config_repository_config()
{
  configSet("repo.key", "repo.value", ConfigScope::REPOSITORY);

  std::string value = configGet("repo.key", ConfigScope::REPOSITORY);

  return value == "repo.value";
}

// test loading default configs
bool test_config_default_configs()
{
  configLoad();

  std::string user_name = configGet("user.name");
  std::string user_email = configGet("user.email");

  return !user_name.empty() && !user_email.empty();
}
