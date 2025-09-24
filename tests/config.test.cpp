#include "../include/config.hpp"

bool test_config_set_and_get()
{
  config_set("test.key", "test.value");

  std::string value = config_get("test.key");

  return value == "test.value";
}

bool test_config_user_name()
{
  config_set_user_name("Test User");

  std::string name = config_get_user_name();

  return name == "Test User";
}

bool test_config_user_email()
{
  config_set_user_email("test@example.com");

  std::string email = config_get_user_email();

  return email == "test@example.com";
}

bool test_config_list()
{
  config_set("test.key1", "value1");
  config_set("test.key2", "value2");

  config_list();

  return true;
}

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

bool test_config_repository_config()
{
  config_set("repo.key", "repo.value", ConfigScope::REPOSITORY);

  std::string value = config_get("repo.key", ConfigScope::REPOSITORY);

  return value == "repo.value";
}

bool test_config_default_configs()
{
  config_load();

  std::string user_name = config_get_user_name();
  std::string user_email = config_get_user_email();

  return !user_name.empty() && !user_email.empty();
}
