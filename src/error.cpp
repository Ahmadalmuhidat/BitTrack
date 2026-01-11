#include "../include/error.hpp"

void ErrorHandler::printError(const BitTrackError &error)
{
  printError(error.getCode(), error.what(), error.getSeverity(), error.getContext());
}

void ErrorHandler::printError(
    ErrorCode code,
    const std::string &message,
    ErrorSeverity severity,
    const std::string &context)
{
  std::string color;
  std::string severityStr;

  switch (severity)
  {
  case ErrorSeverity::INFO:
    color = "\033[36m";
    severityStr = "INFO";
    break;
  case ErrorSeverity::WARNING:
    color = "\033[33m";
    severityStr = "WARNING";
    break;
  case ErrorSeverity::ERROR:
    color = "\033[31m";
    severityStr = "ERROR";
    break;
  case ErrorSeverity::FATAL:
    color = "\033[35m";
    severityStr = "FATAL";
    break;
  }

  std::cerr << color << "[" << severityStr << "] " << "\033[0m";

  if (!context.empty())
  {
    std::cerr << "(" << context << ") ";
  }

  std::cerr << message << std::endl;

  switch (code)
  {
  case ErrorCode::NOT_IN_REPOSITORY:
    std::cerr << "  Run 'bittrack init' to initialize a new repository." << std::endl;
    break;
  case ErrorCode::INVALID_ARGUMENTS:
    std::cerr << "  Run 'bittrack --help' to see usage information." << std::endl;
    break;
  case ErrorCode::BRANCH_NOT_FOUND:
    std::cerr << "  Run 'bittrack --branch -l' to list available branches." << std::endl;
    break;
  case ErrorCode::FILE_NOT_FOUND:
    std::cerr << "  Check if the file path is correct and the file exists." << std::endl;
    break;
  case ErrorCode::UNCOMMITTED_CHANGES:
    std::cerr << "  Commit or stash your changes before switching branches." << std::endl;
    break;
  default:
    break;
  }
}

bool ErrorHandler::isFatal(ErrorCode code)
{
  switch (code)
  {
  case ErrorCode::INTERNAL_ERROR:
  case ErrorCode::MEMORY_ALLOCATION_FAILED:
  case ErrorCode::UNEXPECTED_EXCEPTION:
    return true;
  default:
    return false;
  }
}

std::string ErrorHandler::getErrorMessage(ErrorCode code)
{
  switch (code)
  {
  case ErrorCode::SUCCESS:
    return "Operation completed successfully";
  case ErrorCode::INVALID_ARGUMENTS:
    return "Invalid arguments provided";
  case ErrorCode::MISSING_ARGUMENTS:
    return "Required arguments are missing";
  case ErrorCode::INVALID_FILE_PATH:
    return "Invalid file path";
  case ErrorCode::INVALID_BRANCH_NAME:
    return "Invalid branch name";
  case ErrorCode::INVALID_COMMIT_MESSAGE:
    return "Invalid commit message";
  case ErrorCode::INVALID_REMOTE_URL:
    return "Invalid remote URL";
  case ErrorCode::NOT_IN_REPOSITORY:
    return "Not inside a BitTrack repository";
  case ErrorCode::REPOSITORY_ALREADY_EXISTS:
    return "Repository already exists";
  case ErrorCode::REPOSITORY_CORRUPTED:
    return "Repository is corrupted";
  case ErrorCode::BRANCH_NOT_FOUND:
    return "Branch not found";
  case ErrorCode::BRANCH_ALREADY_EXISTS:
    return "Branch already exists";
  case ErrorCode::CANNOT_DELETE_CURRENT_BRANCH:
    return "Cannot delete current branch";
  case ErrorCode::NO_COMMITS_FOUND:
    return "No commits found";
  case ErrorCode::FILE_NOT_FOUND:
    return "File not found";
  case ErrorCode::FILE_ACCESS_DENIED:
    return "File access denied";
  case ErrorCode::DIRECTORY_CREATION_FAILED:
    return "Failed to create directory";
  case ErrorCode::FILE_READ_ERROR:
    return "Failed to read file";
  case ErrorCode::FILE_WRITE_ERROR:
    return "Failed to write file";
  case ErrorCode::FILE_COPY_ERROR:
    return "Failed to copy file";
  case ErrorCode::FILE_DELETE_ERROR:
    return "Failed to delete file";
  case ErrorCode::INSUFFICIENT_DISK_SPACE:
    return "Insufficient disk space";
  case ErrorCode::STAGING_FAILED:
    return "Failed to stage file";
  case ErrorCode::COMMIT_FAILED:
    return "Failed to commit changes";
  case ErrorCode::MERGE_CONFLICT:
    return "Merge conflict detected";
  case ErrorCode::UNCOMMITTED_CHANGES:
    return "Uncommitted changes detected";
  case ErrorCode::CHECKOUT_FAILED:
    return "Failed to checkout branch";
  case ErrorCode::REMOTE_CONNECTION_FAILED:
    return "Failed to connect to remote";
  case ErrorCode::REMOTE_AUTHENTICATION_FAILED:
    return "Remote authentication failed";
  case ErrorCode::REMOTE_UPLOAD_FAILED:
    return "Failed to upload to remote";
  case ErrorCode::REMOTE_DOWNLOAD_FAILED:
    return "Failed to download from remote";
  case ErrorCode::MEMORY_ALLOCATION_FAILED:
    return "Memory allocation failed";
  case ErrorCode::UNEXPECTED_EXCEPTION:
    return "Unexpected error occurred";
  case ErrorCode::INTERNAL_ERROR:
    return "Internal error";
  default:
    return "Unknown error";
  }
}

void ErrorHandler::handleFilesystemError(
    const std::filesystem::filesystem_error &e,
    const std::string &operation)
{
  std::string errorMsg = "Filesystem error during " + operation + ": " + e.what();

  if (e.code() == std::errc::no_such_file_or_directory)
  {
    printError(ErrorCode::FILE_NOT_FOUND, errorMsg, ErrorSeverity::ERROR, operation);
  }
  else if (e.code() == std::errc::permission_denied)
  {
    printError(ErrorCode::FILE_ACCESS_DENIED, errorMsg, ErrorSeverity::ERROR, operation);
  }
  else if (e.code() == std::errc::no_space_on_device)
  {
    printError(ErrorCode::INSUFFICIENT_DISK_SPACE, errorMsg, ErrorSeverity::FATAL, operation);
  }
  else
  {
    printError(ErrorCode::FILE_ACCESS_DENIED, errorMsg, ErrorSeverity::ERROR, operation);
  }
}

bool ErrorHandler::validateArguments(
    int argc,
    int expected,
    const std::string &command)
{
  if (argc < expected)
  {
    throw BitTrackError(
        ErrorCode::MISSING_ARGUMENTS,
        "Missing required arguments for " + command + ". Expected " + std::to_string(expected) + " arguments, got " + std::to_string(argc),
        ErrorSeverity::ERROR,
        command);
  }
  return true;
}

bool ErrorHandler::validateFilePath(const std::string &path)
{
  if (path.empty())
  {
    printError(
        ErrorCode::INVALID_FILE_PATH,
        "File path cannot be empty",
        ErrorSeverity::ERROR,
        "file path validation");
    return false;
  }

  if (path.find('\0') != std::string::npos)
  {
    printError(
        ErrorCode::INVALID_FILE_PATH,
        "File path contains null character",
        ErrorSeverity::ERROR,
        path);
    return false;
  }

  if (path.find("..") != std::string::npos)
  {
    printError(
        ErrorCode::INVALID_FILE_PATH,
        "File path contains path traversal",
        ErrorSeverity::ERROR,
        path);
    return false;
  }

  return true;
}

bool ErrorHandler::validateBranchName(const std::string &name)
{
  if (name.empty())
  {
    printError(
        ErrorCode::INVALID_BRANCH_NAME,
        "Branch name cannot be empty",
        ErrorSeverity::ERROR,
        "branch name validation");
    return false;
  }

  std::regex branchPattern("^[a-zA-Z0-9._-]+$");
  if (!std::regex_match(name, branchPattern))
  {
    printError(
        ErrorCode::INVALID_BRANCH_NAME,
        "Branch name contains invalid characters. Use only letters, "
        "numbers, dots, underscores, and hyphens",
        ErrorSeverity::ERROR,
        name);
    return false;
  }

  if (name == "HEAD")
  {
    printError(
        ErrorCode::INVALID_BRANCH_NAME,
        "Branch name '" + name + "' is reserved",
        ErrorSeverity::ERROR,
        name);
    return false;
  }

  return true;
}

bool ErrorHandler::validateCommitMessage(const std::string &message)
{
  if (message.empty())
  {
    printError(
        ErrorCode::INVALID_COMMIT_MESSAGE,
        "Commit message cannot be empty",
        ErrorSeverity::ERROR,
        "commit message validation");
    return false;
  }

  if (message.length() > 1000)
  {
    printError(
        ErrorCode::INVALID_COMMIT_MESSAGE,
        "Commit message too long (max 1000 characters)",
        ErrorSeverity::WARNING,
        "commit message validation");
    return false;
  }

  return true;
}

bool ErrorHandler::validateRemoteUrl(const std::string &url)
{
  if (url.empty())
  {
    printError(
        ErrorCode::INVALID_REMOTE_URL,
        "Remote URL cannot be empty",
        ErrorSeverity::ERROR,
        "remote URL validation");
    return false;
  }

  std::regex urlPattern("^(https?|ftp)://[^\\s/$.?#].[^\\s]*$");
  if (!std::regex_match(url, urlPattern))
  {
    printError(
        ErrorCode::INVALID_REMOTE_URL,
        "Invalid URL format. Use http:// or https:// URLs",
        ErrorSeverity::ERROR,
        url);
    return false;
  }

  return true;
}

bool ErrorHandler::safeCreateDirectories(const std::filesystem::path &path)
{
  try
  {
    if (std::filesystem::exists(path))
    {
      return true;
    }

    std::filesystem::create_directories(path);
    return true;
  }
  catch (const std::filesystem::filesystem_error &e)
  {
    handleFilesystemError(e, "create directories");
    return false;
  }
  catch (const std::exception &e)
  {
    printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Unexpected error creating directories: " + std::string(e.what()),
        ErrorSeverity::FATAL,
        path.string());
    return false;
  }
}

bool ErrorHandler::safeCopyFile(
    const std::filesystem::path &from,
    const std::filesystem::path &to)
{
  try
  {
    if (!std::filesystem::exists(from))
    {
      printError(
          ErrorCode::FILE_NOT_FOUND,
          "Source file does not exist: " + from.string(),
          ErrorSeverity::ERROR,
          from.string());
      return false;
    }

    if (!to.parent_path().empty())
    {
      safeCreateDirectories(to.parent_path());
    }

    std::filesystem::copy_file(
        from, to, std::filesystem::copy_options::overwrite_existing);
    return true;
  }
  catch (const std::filesystem::filesystem_error &e)
  {
    handleFilesystemError(e, "copy file");
    return false;
  }
  catch (const std::exception &e)
  {
    printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Unexpected error copying file: " + std::string(e.what()),
        ErrorSeverity::FATAL,
        from.string());
    return false;
  }
}

bool ErrorHandler::safeRemoveFile(const std::filesystem::path &path)
{
  try
  {
    if (!std::filesystem::exists(path))
    {
      return true;
    }

    std::filesystem::remove(path);
    return true;
  }
  catch (const std::filesystem::filesystem_error &e)
  {
    handleFilesystemError(e, "remove file");
    return false;
  }
  catch (const std::exception &e)
  {
    printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Unexpected error removing file: " + std::string(e.what()),
        ErrorSeverity::FATAL,
        path.string());
    return false;
  }
}

bool ErrorHandler::safeRemoveFolder(const std::filesystem::path &path)
{
  try
  {
    if (!std::filesystem::exists(path))
    {
      return true;
    }

    std::filesystem::remove_all(path);
    return true;
  }
  catch (const std::filesystem::filesystem_error &e)
  {
    handleFilesystemError(e, "remove folder");
    return false;
  }
  catch (const std::exception &e)
  {
    printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Unexpected error removing file: " + std::string(e.what()),
        ErrorSeverity::FATAL,
        path.string());
    return false;
  }
}

bool ErrorHandler::safeWriteFile(
    const std::filesystem::path &path,
    const std::string &content)
{
  try
  {
    if (!path.parent_path().empty())
    {
      safeCreateDirectories(path.parent_path());
    }

    std::ofstream file(path);
    if (!file.is_open())
    {
      printError(
          ErrorCode::FILE_WRITE_ERROR,
          "Failed to open file for writing: " + path.string(),
          ErrorSeverity::ERROR,
          path.string());
      return false;
    }

    file << content;
    file.close();
    return true;
  }
  catch (const std::filesystem::filesystem_error &e)
  {
    handleFilesystemError(e, "write file");
    return false;
  }
  catch (const std::exception &e)
  {
    printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Unexpected error writing file: " + std::string(e.what()),
        ErrorSeverity::FATAL,
        path.string());
    return false;
  }
}

bool ErrorHandler::safeAppendFile(
    const std::filesystem::path &path,
    const std::string &content)
{
  try
  {
    if (!path.parent_path().empty())
    {
      safeCreateDirectories(path.parent_path());
    }

    std::ofstream file(path, std::ios::app);
    if (!file.is_open())
    {
      printError(
          ErrorCode::FILE_WRITE_ERROR,
          "Failed to open file for appending: " + path.string(),
          ErrorSeverity::ERROR,
          path.string());
      return false;
    }

    file << content;
    file.close();
    return true;
  }
  catch (const std::filesystem::filesystem_error &e)
  {
    handleFilesystemError(e, "append file");
    return false;
  }
  catch (const std::exception &e)
  {
    printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Unexpected error appending to file: " + std::string(e.what()),
        ErrorSeverity::FATAL,
        path.string());
    return false;
  }
}

bool ErrorHandler::safeRename(
    const std::filesystem::path &from,
    const std::filesystem::path &to)
{
  try
  {
    if (!std::filesystem::exists(from))
    {
      printError(
          ErrorCode::FILE_NOT_FOUND,
          "Source path does not exist: " + from.string(),
          ErrorSeverity::ERROR,
          "rename");
      return false;
    }

    std::filesystem::rename(from, to);
    return true;
  }
  catch (const std::filesystem::filesystem_error &e)
  {
    handleFilesystemError(e, "rename");
    return false;
  }
  catch (const std::exception &e)
  {
    printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Unexpected error during rename: " + std::string(e.what()),
        ErrorSeverity::FATAL,
        "rename");
    return false;
  }
}

std::string ErrorHandler::safeReadFile(const std::filesystem::path &path)
{
  try
  {
    if (!std::filesystem::exists(path))
    {
      printError(
          ErrorCode::FILE_NOT_FOUND,
          "File does not exist: " + path.string(),
          ErrorSeverity::ERROR,
          path.string());
      return "";
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
      printError(
          ErrorCode::FILE_READ_ERROR,
          "Failed to open file for reading: " + path.string(),
          ErrorSeverity::ERROR,
          path.string());
      return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
  }
  catch (const std::filesystem::filesystem_error &e)
  {
    handleFilesystemError(e, "read file");
    return "";
  }
  catch (const std::exception &e)
  {
    printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Unexpected error reading file: " + std::string(e.what()),
        ErrorSeverity::FATAL,
        path.string());
    return "";
  }
}

std::string ErrorHandler::safeReadFirstLine(const std::filesystem::path &path)
{
  try
  {
    if (!std::filesystem::exists(path))
    {
      printError(
          ErrorCode::FILE_NOT_FOUND,
          "File does not exist: " + path.string(),
          ErrorSeverity::ERROR,
          path.string());
      return "";
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
      printError(
          ErrorCode::FILE_READ_ERROR,
          "Failed to open file for reading: " + path.string(),
          ErrorSeverity::ERROR,
          path.string());
      return "";
    }

    std::string line;
    std::getline(file, line);
    return line;
  }
  catch (const std::exception &e)
  {
    printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Unexpected error reading line: " + std::string(e.what()),
        ErrorSeverity::FATAL,
        path.string());
    return "";
  }
}

std::vector<std::filesystem::path> ErrorHandler::safeListDirectoryFiles(const std::filesystem::path &path)
{
  try
  {
    std::vector<std::filesystem::path> files;

    if (!std::filesystem::exists(path))
    {
      return files;
    }

    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(path))
    {
      if (entry.is_regular_file())
      {
        std::filesystem::path relative_Path =
            std::filesystem::relative(entry.path(), path);
        files.push_back(relative_Path);
      }
    }

    return files;
  }
  catch (const std::exception &e)
  {
    printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Unexpected error reading line: " + std::string(e.what()),
        ErrorSeverity::FATAL,
        path.string());
    return {};
  }
}

bool ErrorHandler::validateRepository()
{
  if (!std::filesystem::exists(".bittrack"))
  {
    return false;
  }

  if (!std::filesystem::exists(".bittrack/HEAD") || !std::filesystem::exists(".bittrack/commits") || !std::filesystem::exists(".bittrack/refs"))
  {
    printError(
        ErrorCode::REPOSITORY_CORRUPTED,
        "Repository appears to be corrupted - missing essential files",
        ErrorSeverity::FATAL,
        "repository validation");
    return false;
  }

  return true;
}

bool ErrorHandler::validateBranchExists(const std::string &branchName)
{
  std::string branchPath = ".bittrack/refs/heads/" + branchName;
  if (!std::filesystem::exists(branchPath))
  {
    printError(
        ErrorCode::BRANCH_NOT_FOUND,
        "Branch '" + branchName + "' does not exist",
        ErrorSeverity::ERROR,
        branchName);
    return false;
  }
  return true;
}

bool ErrorHandler::validateNoUncommittedChanges()
{
  try
  {
    std::vector<std::string> staged_files = getStagedFiles();
    if (!staged_files.empty())
    {
      printError(
          ErrorCode::VALIDATION_ERROR,
          "You have staged changes. Please commit or unstage them "
          "before proceeding.",
          ErrorSeverity::ERROR,
          "uncommitted changes");
      return false;
    }

    std::vector<std::string> unstaged_files = getUnstagedFiles();
    if (!unstaged_files.empty())
    {
      printError(
          ErrorCode::VALIDATION_ERROR,
          "You have unstaged changes. Please stage and commit them "
          "before proceeding.",
          ErrorSeverity::ERROR,
          "uncommitted changes");
      return false;
    }

    return true;
  }
  catch (const std::exception &e)
  {
    printError(
        ErrorCode::VALIDATION_ERROR,
        "Error checking for uncommitted changes: " +
            std::string(e.what()),
        ErrorSeverity::ERROR,
        "validation");
    return false;
  }
}
