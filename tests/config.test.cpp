#include "../include/config.hpp"

bool test_config_set_and_get()
{
  // Set a configuration value
  config_set("test.key", "test.value");
  
  // Get the configuration value
  std::string value = config_get("test.key");
  
  return value == "test.value";
}

bool test_config_user_name()
{
  // Set user name
  config_set_user_name("Test User");
  
  // Get user name
  std::string name = config_get_user_name();
  
  return name == "Test User";
}

bool test_config_user_email()
{
  // Set user email
  config_set_user_email("test@example.com");
  
  // Get user email
  std::string email = config_get_user_email();
  
  return email == "test@example.com";
}

bool test_config_list()
{
  // Set some configuration values
  config_set("test.key1", "value1");
  config_set("test.key2", "value2");
  
  // List configuration (this should not throw an exception)
  config_list();
  
  return true; // If no exception thrown, test passes
}

bool test_config_unset()
{
  // Set a configuration value
  config_set("test.unset.key", "test.value");
  
  // Verify it's set
  std::string value_before = config_get("test.unset.key");
  bool was_set = (value_before == "test.value");
  
  // Unset the configuration
  config_unset("test.unset.key");
  
  // Verify it's unset
  std::string value_after = config_get("test.unset.key");
  bool is_unset = value_after.empty();
  
  return was_set && is_unset;
}

bool test_config_repository_config()
{
  // Set repository configuration
  config_set("repo.key", "repo.value", ConfigScope::REPOSITORY);
  
  // Get repository configuration
  std::string value = config_get("repo.key", ConfigScope::REPOSITORY);
  
  return value == "repo.value";
}

bool test_config_default_configs()
{
  // Load default configurations
  config_load();
  
  // Check if default configurations are set
  std::string user_name = config_get_user_name();
  std::string user_email = config_get_user_email();
  
  return !user_name.empty() && !user_email.empty();
}
