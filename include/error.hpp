#ifndef ERROR_HPP
#define ERROR_HPP

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <regex>
#include <filesystem>
#include <string>
#include <exception>

enum class ErrorCode
{
  SUCCESS = 0,
  NOT_IN_REPOSITORY = 1,
  REPOSITORY_ALREADY_EXISTS = 2,
  FILE_NOT_FOUND = 3,
  DIRECTORY_CREATION_FAILED = 4,
  FILE_WRITE_ERROR = 5,
  FILE_READ_ERROR = 6,
  INVALID_ARGUMENTS = 7,
  INVALID_BRANCH_NAME = 8,
  BRANCH_NOT_FOUND = 9,
  INVALID_COMMIT_MESSAGE = 10,
  INVALID_REMOTE_URL = 11,
  NETWORK_ERROR = 12,
  UNEXPECTED_EXCEPTION = 13,
  VALIDATION_ERROR = 14,
  FILESYSTEM_ERROR = 15,
  PERMISSION_ERROR = 16,
  CONFIG_ERROR = 17,
  MERGE_CONFLICT = 18,
  STASH_ERROR = 19,
  TAG_ERROR = 20,
  HOOK_ERROR = 21,
  MAINTENANCE_ERROR = 22,
  UNCOMMITTED_CHANGES = 23,
  INTERNAL_ERROR = 24,
  MEMORY_ALLOCATION_FAILED = 25,
  MISSING_ARGUMENTS = 26,
  INVALID_FILE_PATH = 27,
  REPOSITORY_CORRUPTED = 28,
  BRANCH_ALREADY_EXISTS = 29,
  CANNOT_DELETE_CURRENT_BRANCH = 30,
  NO_COMMITS_FOUND = 31,
  FILE_ACCESS_DENIED = 32,
  FILE_COPY_ERROR = 33,
  FILE_DELETE_ERROR = 34,
  INSUFFICIENT_DISK_SPACE = 35,
  STAGING_FAILED = 36,
  COMMIT_FAILED = 37,
  CHECKOUT_FAILED = 38,
  REMOTE_CONNECTION_FAILED = 39,
  REMOTE_AUTHENTICATION_FAILED = 40,
  REMOTE_UPLOAD_FAILED = 41,
  REMOTE_DOWNLOAD_FAILED = 42
};

enum class ErrorSeverity
{
  INFO,
  WARNING,
  ERROR,
  FATAL
};

class BitTrackError : public std::exception
{
private:
  ErrorCode code;
  std::string message;
  ErrorSeverity severity;
  std::string context;

public:
  BitTrackError(ErrorCode c, const std::string& msg, ErrorSeverity sev = ErrorSeverity::ERROR, const std::string& ctx = "")
  : code(c), message(msg), severity(sev), context(ctx) {}

  ErrorCode getCode() const { return code; }
  ErrorSeverity getSeverity() const { return severity; }
  const std::string& getContext() const { return context; }
  const char* what() const noexcept override { return message.c_str(); }
};

class ErrorHandler
{
public:
  static void printError(const BitTrackError& error);
  static void printError(ErrorCode code, const std::string& message, ErrorSeverity severity, const std::string& context);
  static bool isFatal(ErrorCode code);
  static std::string getErrorMessage(ErrorCode code);
  static bool validateArguments(int argc, int required, const std::string& command);
  static bool validateFilePath(const std::string& filePath);
  static bool validateBranchName(const std::string& branchName);
  static bool validateCommitMessage(const std::string& message);
  static bool validateRemoteUrl(const std::string& url);
  static bool safeCreateDirectories(const std::filesystem::path& path);
  static bool safeCopyFile(const std::filesystem::path& from, const std::filesystem::path& to);
  static bool safeRemoveFile(const std::filesystem::path& path);
  static bool safeWriteFile(const std::filesystem::path& path, const std::string& content);
  static std::string safeReadFile(const std::filesystem::path& path);
  static bool validateRepository();
  static bool validateBranchExists(const std::string& branchName);
  static bool validateNoUncommittedChanges();
  static void handleFilesystemError(const std::filesystem::filesystem_error& e, const std::string& context);
};

#define HANDLE_EXCEPTION(context) \
  catch (const std::filesystem::filesystem_error& e) \
  { \
    ErrorHandler::handleFilesystemError(e, context); \
    throw; \
  } \
  catch (const std::exception& e) \
  { \
    throw BitTrackError(ErrorCode::UNEXPECTED_EXCEPTION, "Unexpected error in " + std::string(context) + ": " + e.what(), ErrorSeverity::ERROR, context); \
  }

#define VALIDATE_ARGS(argc, required, command) ErrorHandler::validateArguments(argc, required, command)
#define VALIDATE_FILE_PATH(path) ErrorHandler::validateFilePath(path)
#define VALIDATE_BRANCH_NAME(name) ErrorHandler::validateBranchName(name)
#define VALIDATE_COMMIT_MESSAGE(message) ErrorHandler::validateCommitMessage(message)
#define VALIDATE_REMOTE_URL(url) ErrorHandler::validateRemoteUrl(url)

#endif
