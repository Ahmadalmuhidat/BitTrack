#include "../include/error.hpp"
#include <fstream>

// test printing a BitTrackError object
bool test_error_handler_print_error()
{
  BitTrackError error(ErrorCode::FILE_NOT_FOUND, "Test error message", ErrorSeverity::ERROR, "test");
  ErrorHandler::printError(error);

  return true;
}

// test printing an error code directly
bool test_error_handler_print_error_code()
{
  ErrorHandler::printError(ErrorCode::INVALID_ARGUMENTS, "Test error", ErrorSeverity::WARNING, "test");
  return true;
}

// test checking if an error code is fatal
bool test_error_handler_is_fatal()
{
  bool fatal_error = ErrorHandler::isFatal(ErrorCode::INTERNAL_ERROR);
  bool non_fatal_error = ErrorHandler::isFatal(ErrorCode::FILE_NOT_FOUND);

  return fatal_error && !non_fatal_error;
}

// test retrieving an error message for a given error code
bool test_error_handler_get_message()
{
  std::string message = ErrorHandler::getErrorMessage(ErrorCode::FILE_NOT_FOUND);
  return !message.empty();
}

// test validating function arguments
bool test_error_handler_validate_arguments()
{
  ErrorHandler::validateArguments(3, 3, "test");
  return true;
}

// test validating a file path
bool test_error_handler_validate_file_path()
{
  ErrorHandler::validateFilePath("test_file.txt");
  return true;
}

// test validating a branch name
bool test_error_handler_validate_branch_name()
{
  ErrorHandler::validateBranchName("valid-branch-name");
  return true;
}

// test validating a commit message
bool test_error_handler_validate_commit_message()
{
  ErrorHandler::validateCommitMessage("Valid commit message");
  return true;
}

// test validating a remote URL
bool test_error_handler_validate_remote_url()
{
  ErrorHandler::validateRemoteUrl("https://github.com/user/repo.git");
  return true;
}

// test safe creation of directories
bool test_error_handler_safe_create_directories()
{
  bool result = ErrorHandler::safeCreateDirectories(".bittrack/test_dir");

  if (std::filesystem::exists(".bittrack/test_dir"))
  {
    std::filesystem::remove_all(".bittrack/test_dir");
  }

  return result;
}

// test safe copying of files
bool test_error_handler_safe_copy_file()
{
  std::ofstream test_file("test_copy_source.txt");
  test_file << "test content" << std::endl;
  test_file.close();

  bool result = ErrorHandler::safeCopyFile("test_copy_source.txt", "test_copy_dest.txt");

  std::filesystem::remove("test_copy_source.txt");
  if (std::filesystem::exists("test_copy_dest.txt"))
  {
    std::filesystem::remove("test_copy_dest.txt");
  }

  return result;
}

// test safe removal of files
bool test_error_handler_safe_remove_file()
{
  std::ofstream test_file("test_remove.txt");
  test_file << "test content" << std::endl;
  test_file.close();

  bool result = ErrorHandler::safeRemoveFile("test_remove.txt");

  return result;
}

// test safe writing to a file
bool test_error_handler_safe_write_file()
{
  bool result = ErrorHandler::safeWriteFile("test_write.txt", "test content");

  if (std::filesystem::exists("test_write.txt"))
  {
    std::filesystem::remove("test_write.txt");
  }

  return result;
}

// test safe reading from a file
bool test_error_handler_safe_read_file()
{
  std::ofstream test_file("test_read.txt");
  test_file << "test content" << std::endl;
  test_file.close();

  std::string content = ErrorHandler::safeReadFile("test_read.txt");

  std::filesystem::remove("test_read.txt");

  return content == "test content\n";
}

// test validating repository existence
bool test_error_handler_validate_repository()
{
  ErrorHandler::validateRepository();
  return true;
}

// test validating branch existence
bool test_error_handler_validate_branch_exists()
{
  ErrorHandler::validateBranchExists("main");
  return true;
}

// test validating no uncommitted changes
bool test_error_handler_validate_no_uncommitted()
{
  ErrorHandler::validateNoUncommittedChanges();
  return true;
}
