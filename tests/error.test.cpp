#include "../include/error.hpp"
#include <fstream>

bool test_error_handler_print_error()
{
  // Test error printing (should not throw exception)
  BitTrackError error(ErrorCode::FILE_NOT_FOUND, "Test error message", ErrorSeverity::ERROR, "test");
  ErrorHandler::printError(error);
  
  return true; // If no exception thrown, test passes
}

bool test_error_handler_print_error_code()
{
  // Test error printing with code (should not throw exception)
  ErrorHandler::printError(ErrorCode::INVALID_ARGUMENTS, "Test error", ErrorSeverity::WARNING, "test");
  
  return true; // If no exception thrown, test passes
}

bool test_error_handler_is_fatal()
{
  // Test fatal error detection
  bool fatal_error = ErrorHandler::isFatal(ErrorCode::INTERNAL_ERROR);
  bool non_fatal_error = ErrorHandler::isFatal(ErrorCode::FILE_NOT_FOUND);
  
  return fatal_error && !non_fatal_error;
}

bool test_error_handler_get_message()
{
  // Test error message retrieval
  std::string message = ErrorHandler::getErrorMessage(ErrorCode::FILE_NOT_FOUND);
  
  return !message.empty();
}

bool test_error_handler_validate_arguments()
{
  // Test argument validation (should not throw exception for valid args)
  ErrorHandler::validateArguments(3, 3, "test");
  
  return true; // If no exception thrown, test passes
}

bool test_error_handler_validate_file_path()
{
  // Test file path validation (should not throw exception for valid path)
  ErrorHandler::validateFilePath("test_file.txt");
  
  return true; // If no exception thrown, test passes
}

bool test_error_handler_validate_branch_name()
{
  // Test branch name validation (should not throw exception for valid name)
  ErrorHandler::validateBranchName("valid-branch-name");
  
  return true; // If no exception thrown, test passes
}

bool test_error_handler_validate_commit_message()
{
  // Test commit message validation (should not throw exception for valid message)
  ErrorHandler::validateCommitMessage("Valid commit message");
  
  return true; // If no exception thrown, test passes
}

bool test_error_handler_validate_remote_url()
{
  // Test remote URL validation (should not throw exception for valid URL)
  ErrorHandler::validateRemoteUrl("https://github.com/user/repo.git");
  
  return true; // If no exception thrown, test passes
}

bool test_error_handler_safe_create_directories()
{
  // Test safe directory creation
  bool result = ErrorHandler::safeCreateDirectories(".bittrack/test_dir");
  
  // Clean up
  if (std::filesystem::exists(".bittrack/test_dir"))
  {
    std::filesystem::remove_all(".bittrack/test_dir");
  }
  
  return result;
}

bool test_error_handler_safe_copy_file()
{
  // Create a test file
  std::ofstream test_file("test_copy_source.txt");
  test_file << "test content" << std::endl;
  test_file.close();
  
  // Test safe file copy
  bool result = ErrorHandler::safeCopyFile("test_copy_source.txt", "test_copy_dest.txt");
  
  // Clean up
  std::filesystem::remove("test_copy_source.txt");
  if (std::filesystem::exists("test_copy_dest.txt"))
  {
    std::filesystem::remove("test_copy_dest.txt");
  }
  
  return result;
}

bool test_error_handler_safe_remove_file()
{
  // Create a test file
  std::ofstream test_file("test_remove.txt");
  test_file << "test content" << std::endl;
  test_file.close();
  
  // Test safe file removal
  bool result = ErrorHandler::safeRemoveFile("test_remove.txt");
  
  return result;
}

bool test_error_handler_safe_write_file()
{
  // Test safe file writing
  bool result = ErrorHandler::safeWriteFile("test_write.txt", "test content");
  
  // Clean up
  if (std::filesystem::exists("test_write.txt"))
  {
    std::filesystem::remove("test_write.txt");
  }
  
  return result;
}

bool test_error_handler_safe_read_file()
{
  // Create a test file
  std::ofstream test_file("test_read.txt");
  test_file << "test content" << std::endl;
  test_file.close();
  
  // Test safe file reading
  std::string content = ErrorHandler::safeReadFile("test_read.txt");
  
  // Clean up
  std::filesystem::remove("test_read.txt");
  
  return content == "test content\n";
}

bool test_error_handler_validate_repository()
{
  // Test repository validation (should not throw exception if repository exists)
  ErrorHandler::validateRepository();
  
  return true; // If no exception thrown, test passes
}

bool test_error_handler_validate_branch_exists()
{
  // Test branch existence validation (should not throw exception for existing branch)
  ErrorHandler::validateBranchExists("master");
  
  return true; // If no exception thrown, test passes
}

bool test_error_handler_validate_no_uncommitted()
{
  // Test uncommitted changes validation (should not throw exception if no uncommitted changes)
  ErrorHandler::validateNoUncommittedChanges();
  
  return true; // If no exception thrown, test passes
}
