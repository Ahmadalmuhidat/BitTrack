#include "../include/remote.hpp"
#include <cstdlib>

void setRemoteOrigin(const std::string &url)
{
  if (!ErrorHandler::safeWriteFile(".bittrack/remote", url + "\n"))
  {
    ErrorHandler::printError(
        ErrorCode::FILE_WRITE_ERROR,
        "Could not open .bittrack/remote for writing",
        ErrorSeverity::ERROR,
        "setRemoteOrigin");
  }
}

std::string getRemoteOriginUrl()
{
  std::string url = ErrorHandler::safeReadFile(".bittrack/remote");
  if (url.empty())
  {
    return "";
  }

  // Remove newline if present
  if (url.back() == '\n')
  {
    url.pop_back();
  }

  return url;
}

size_t writeData(
    void *ptr,
    size_t size,
    size_t nmemb,
    void *stream)
{
  // Write data to the provided stream (ofstream in this case)
  std::ofstream *out = static_cast<std::ofstream *>(stream);
  out->write(static_cast<char *>(ptr), size * nmemb);
  return size * nmemb;
}

std::vector<std::string> listRemoteBranches(const std::string &remote_name)
{
  std::string remote_url = getRemoteUrl(remote_name);
  if (remote_url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::FILE_NOT_FOUND,
        "Remote '" + remote_name + "' not found",
        ErrorSeverity::ERROR,
        "listRemoteBranches");
    return {};
  }

  CURL *curl = curl_easy_init(); // Initialize CURL
  if (!curl)
  {
    return {};
  }

  std::string url = remote_url + "/branches";
  std::string response_string;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());             // Set the URL
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);  // Set output string

  CURLcode res = curl_easy_perform(curl);                      // Perform the request
  long http_code = 0;                                          // To store HTTP response code
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code); // Get HTTP response code
  curl_easy_cleanup(curl);

  std::vector<std::string> branches;
  if (res == CURLE_OK && http_code == 200)
  {
    std::stringstream ss(response_string);
    std::string branch;
    while (std::getline(ss, branch))
    {
      if (!branch.empty())
      {
        branches.push_back(branch);
      }
    }
  }
  else
  {
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "Failed to list remote branches",
        ErrorSeverity::ERROR,
        "listRemoteBranches");
  }
  return branches;
}

bool deleteRemoteBranch(
    const std::string &branch_name,
    const std::string &remote_name)
{
  std::string remote_url = getRemoteUrl(remote_name);
  if (remote_url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::FILE_NOT_FOUND,
        "Remote '" + remote_name + "' not found",
        ErrorSeverity::ERROR,
        "deleteRemoteBranch");
    return false;
  }

  CURL *curl = curl_easy_init();
  if (!curl)
  {
    return false;
  }

  std::string url = remote_url + "/branches/" + branch_name;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());        // Set the URL
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE"); // Set HTTP method to DELETE

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl);

  if (res == CURLE_OK && (http_code == 200 || http_code == 204))
  {
    std::cout << "Remote branch '" << branch_name << "' deleted successfully." << std::endl;
    return true;
  }
  else
  {
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "Failed to delete remote branch '" + branch_name + "'",
        ErrorSeverity::ERROR,
        "deleteRemoteBranch");
    return false;
  }
}

void push(const std::string &remote_name)
{
  // Get remote URL
  std::string remote_url = getRemoteUrl(remote_name);
  if (remote_url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::FILE_NOT_FOUND,
        "Remote '" + remote_name + "' not found",
        ErrorSeverity::ERROR, "push");

    remote_url = getRemoteOriginUrl();
  }

  // Validate remote URL
  if (remote_url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::INVALID_REMOTE_URL,
        "No remote origin set. Use '--remote -s <url>' first",
        ErrorSeverity::ERROR, "push");
    return;
  }

  // Check for uncommitted changes
  if (isLocalBehindRemote())
  {
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "Local repository is behind the remote repository. Use 'bittrack --pull' to sync with remote",
        ErrorSeverity::ERROR,
        "push");
    return;
  }

  // Run pre-push hook
  HookResult result = runHook(HookType::PRE_PUSH);
  if (result == HookResult::ABORT)
  {
    return;
  }

  // Handle GitHub remote
  if (isGithubRemote(remote_url))
  {
    // Get GitHub token from config
    std::string token = configGet("github.token", ConfigScope::REPOSITORY);
    if (token.empty())
    {
      token = configGet("github.token", ConfigScope::GLOBAL);
    }

    if (token.empty())
    {
      ErrorHandler::printError(
          ErrorCode::CONFIG_ERROR,
          "GitHub token not configured. Use 'bittrack --config github.token <token>'",
          ErrorSeverity::ERROR,
          "push");
      return;
    }

    // Extract username and repository name from URL
    std::string username, repo_name;
    if (extractInfoFromGithubUrl(remote_url, username, repo_name).empty())
    {
      ErrorHandler::printError(
          ErrorCode::INVALID_REMOTE_URL,
          "Could not parse GitHub repository URL",
          ErrorSeverity::ERROR,
          "push");
      return;
    }

    std::cout << "Pushing to GitHub repository: " << username << "/" << repo_name << std::endl;

    // Attempt to push using GitHub API
    if (pushToGithub(token, username, repo_name))
    {
      runHook(HookType::POST_PUSH);
      return;
    }
    else
    {
      std::cout << "Pushing to GitHub failed" << std::endl;
    }
  }
  else
  {
    ErrorHandler::printError(
        ErrorCode::INTERNAL_ERROR,
        "Push is only supported for GitHub repositories. Please use a GitHub repository URL",
        ErrorSeverity::ERROR,
        "push");
    return;
  }
}

void pull(const std::string &remote_name)
{
  // Get remote URL
  std::string remote_url = getRemoteUrl(remote_name);
  if (remote_url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::FILE_NOT_FOUND,
        "Remote '" + remote_name + "' not found",
        ErrorSeverity::ERROR,
        "pull");
    remote_url = getRemoteOriginUrl();
  }

  if (remote_url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::INVALID_REMOTE_URL,
        "No remote origin set. Use '--remote -s <url>' first",
        ErrorSeverity::ERROR,
        "pull");
    return;
  }

  // Check current branch
  std::string current_branch = getCurrentBranchName();

  // Set remote origin and pull
  setRemoteOrigin(remote_url);

  // Check for uncommitted changes
  std::vector<std::string> staged_files = getStagedFiles();
  std::vector<std::string> unstaged_files = getUnstagedFiles();

  // If there are uncommitted changes, abort the pull
  if (!staged_files.empty() || !unstaged_files.empty())
  {
    ErrorHandler::printError(
        ErrorCode::UNCOMMITTED_CHANGES,
        "Cannot pull with uncommitted changes. Please commit or stash your changes before pulling",
        ErrorSeverity::ERROR,
        "pull");
    return;
  }

  // Handle GitHub remote
  if (isGithubRemote(remote_url))
  {
    // Run pre-pull hook
    HookResult result = runHook(HookType::PRE_PULL);
    if (result == HookResult::ABORT)
    {
      return;
    }

    // Get GitHub token from config
    std::string token = configGet("github.token", ConfigScope::REPOSITORY);
    if (token.empty())
    {
      token = configGet("github.token", ConfigScope::GLOBAL);
    }

    if (token.empty())
    {
      ErrorHandler::printError(
          ErrorCode::CONFIG_ERROR,
          "GitHub token not configured. Use 'bittrack --config github.token <token>'",
          ErrorSeverity::ERROR,
          "pull");
      return;
    }

    // Extract username and repository name from URL
    std::string username, repo_name;
    if (extractInfoFromGithubUrl(remote_url, username, repo_name).empty())
    {
      ErrorHandler::printError(
          ErrorCode::INVALID_REMOTE_URL,
          "Could not parse GitHub repository URL",
          ErrorSeverity::ERROR,
          "pull");
      return;
    }

    // Attempt to pull using GitHub API
    if (pullFromGithub(token, username, repo_name, current_branch))
    {
      runHook(HookType::POST_PULL);
      return;
    }
    else
    {
      ErrorHandler::printError(
          ErrorCode::REMOTE_CONNECTION_FAILED,
          "Failed to pull from GitHub. Please check your token and internet connection",
          ErrorSeverity::ERROR,
          "pull");
      return;
    }
  }
  else
  {
    ErrorHandler::printError(
        ErrorCode::INTERNAL_ERROR,
        "Pull is only supported for GitHub repositories. Please use a GitHub repository URL",
        ErrorSeverity::ERROR,
        "pull");
    return;
  }
}

void addRemote(
    const std::string &name,
    const std::string &url)
{
  // Validate inputs
  if (name.empty() || url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::INVALID_ARGUMENTS,
        "Remote name and URL cannot be empty",
        ErrorSeverity::ERROR,
        "addRemote");
    return;
  }

  // Create remotes directory if it doesn't exist
  std::string remote_file = ".bittrack/remotes/" + name;

  // Check if remote already exists and write the URL to the remote file
  if (!ErrorHandler::safeWriteFile(remote_file, url + "\n"))
  {
    ErrorHandler::printError(
        ErrorCode::FILE_WRITE_ERROR,
        "Could not create remote file for " + name,
        ErrorSeverity::ERROR,
        "add_remote");
    return;
  }

  std::cout << "Added remote '" << name << "' with URL: " << url << std::endl;
}

void removeRemote(const std::string &name)
{
  if (name.empty())
  {
    ErrorHandler::printError(
        ErrorCode::INVALID_ARGUMENTS,
        "Remote name cannot be empty",
        ErrorSeverity::ERROR,
        "removeRemote");
    return;
  }

  // Check if remote exists
  std::string remote_file = ".bittrack/remotes/" + name;

  // If remote does not exist, print error
  if (!std::filesystem::exists(remote_file))
  {
    ErrorHandler::printError(
        ErrorCode::FILE_NOT_FOUND,
        "Remote '" + name + "' does not exist",
        ErrorSeverity::ERROR,
        "remove_remote");
    return;
  }

  // Remove the remote file
  if (ErrorHandler::safeRemoveFile(remote_file))
  {
    std::cout << "Removed remote '" << name << "'" << std::endl;
  }
}

void listRemotes()
{
  // Directory where remotes are stored
  std::string remotes_dir = ".bittrack/remotes";

  if (!std::filesystem::exists(remotes_dir))
  {
    std::cout << "No remotes configured" << std::endl;
    return;
  }

  // List all remotes
  std::cout << "Configured remotes:" << std::endl;
  for (const auto &entry : std::filesystem::directory_iterator(remotes_dir))
  {
    if (entry.is_regular_file())
    {
      // Get remote name and URL
      std::string remote_name = entry.path().filename().string();
      std::string remote_url = getRemoteUrl(remote_name);

      if (!remote_url.empty())
      {
        std::cout << "  " << remote_name << " -> " << remote_url << std::endl;
      }
    }
  }
}

void fetchFromRemote(const std::string &remote_name)
{
  // Get remote URL
  std::string remote_url = getRemoteUrl(remote_name);
  if (remote_url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::FILE_NOT_FOUND,
        "Remote '" + remote_name + "' not found",
        ErrorSeverity::ERROR,
        "fetchFromRemote");
    return;
  }

  std::cout << "Fetching from remote '" << remote_name << "'..." << std::endl;

  // Prepare URL with current branch
  std::string base_url = remote_url;
  std::string current_branch = getCurrentBranchName();
  std::string remote_url_with_branch = base_url + "?branch=" + current_branch;

  CURL *curl;              // Initialize CURL for file download
  CURLcode response;       // To store CURL response code
  curl = curl_easy_init(); // Initialize CURL

  if (curl)
  {
    // Open file to write the downloaded data
    std::ofstream outfile(".bittrack/remote_fetch_folder.zip", std::ios::binary);
    if (!outfile.is_open())
    {
      ErrorHandler::printError(
          ErrorCode::FILE_WRITE_ERROR,
          "Could not open .bittrack/remote_fetch_folder.zip for writing",
          ErrorSeverity::ERROR,
          "fetchFromRemote");
      curl_easy_cleanup(curl);
      return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, remote_url_with_branch.c_str()); // Set the URL
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);            // Set write callback
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outfile);                 // Set output file stream
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);                  // Follow redirects if any

    // Perform the file download
    response = curl_easy_perform(curl);

    if (response != CURLE_OK)
    {
      ErrorHandler::printError(
          ErrorCode::REMOTE_CONNECTION_FAILED,
          "curl_easy_perform() failed: " + std::string(curl_easy_strerror(response)),
          ErrorSeverity::ERROR,
          "fetchFromRemote");
    }
    else
    {
      std::cout << "Fetched successfully from remote '" << remote_name << "'" << std::endl;
    }

    // Close the output file and clean up CURL
    outfile.close();
    curl_easy_cleanup(curl);
  }

  system("rm -f .bittrack/remote_fetch_folder.zip");
}

void cloneRepository(
    const std::string &url,
    const std::string &local_path)
{
  if (url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::INVALID_ARGUMENTS,
        "Repository URL cannot be empty",
        ErrorSeverity::ERROR,
        "cloneRepository");
    return;
  }

  std::cout << "Cloning repository from " << url << "..." << std::endl;

  system("./bittrack init"); // init new bittrack repository

  const std::string token = configGet("github.token", ConfigScope::GLOBAL);
  if (!token.empty())
  {
    std::string cmdToken = "./bittrack --config github.token " + token;
    system(cmdToken.c_str()); // set token
  }

  std::string cmdRemote = "./bittrack --remote -s " + url;
  system(cmdRemote.c_str()); // set remote origin

  system("./bittrack --pull"); // pull from remote

  std::cout << "Repository cloned successfully" << std::endl;
}

std::string getRemoteUrl(const std::string &remote_name)
{
  if (remote_name == "origin")
  {
    return getRemoteOriginUrl();
  }

  std::string remote_file = ".bittrack/remotes/" + remote_name;
  if (!std::filesystem::exists(remote_file))
  {
    return "";
  }

  // Read the URL from the remote file
  std::string url = ErrorHandler::safeReadFile(remote_file);
  if (url.empty())
  {
    return "";
  }

  // Remove newline if present
  if (!url.empty() && url.back() == '\n')
  {
    url.pop_back();
  }

  return url;
}

void updateRemoteUrl(
    const std::string &remote_name,
    const std::string &new_url)
{
  if (remote_name.empty() || new_url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::INVALID_ARGUMENTS,
        "Remote name and URL cannot be empty",
        ErrorSeverity::ERROR,
        "update_remote_url");
    return;
  }

  // Handle origin remote separately
  if (remote_name == "origin")
  {
    setRemoteOrigin(new_url);
    std::cout << "Updated origin remote URL to: " << new_url << std::endl;
    return;
  }

  std::string remote_file = ".bittrack/remotes/" + remote_name;

  if (!std::filesystem::exists(remote_file))
  {
    ErrorHandler::printError(
        ErrorCode::FILE_NOT_FOUND,
        "Remote '" + remote_name + "' does not exist",
        ErrorSeverity::ERROR,
        "update_remote_url");
    return;
  }

  // Update the URL in the remote file using safeWriteFile
  if (ErrorHandler::safeWriteFile(remote_file, new_url + "\n"))
  {
    std::cout << "Updated remote '" << remote_name << "' URL to: " << new_url << std::endl;
  }
}

void showRemoteInfo()
{
  std::cout << "Remote Information:" << std::endl;
  std::cout << "==================" << std::endl;

  // Get and display origin URL
  std::string origin_url = getRemoteOriginUrl();
  if (!origin_url.empty())
  {
    std::cout << "Origin: " << origin_url << std::endl;
  }
  else
  {
    std::cout << "Origin: Not configured" << std::endl;
  }

  // List all remotes
  listRemotes();
}

std::string getLastPushedCommit()
{
  std::string commit = ErrorHandler::safeReadFile(".bittrack/last_pushed_commit");
  if (commit.empty())
  {
    return "";
  }

  // Remove newline if present
  if (commit.back() == '\n')
  {
    commit.pop_back();
  }

  return commit;
}

void setLastPushedCommit(const std::string &commit)
{
  if (!ErrorHandler::safeWriteFile(".bittrack/last_pushed_commit", commit + "\n"))
  {
    ErrorHandler::printError(
        ErrorCode::FILE_WRITE_ERROR,
        "Could not update .bittrack/last_pushed_commit",
        ErrorSeverity::ERROR,
        "set_last_pushed_commit");
  }
}

std::vector<std::string> getCommittedFiles(const std::string &commit)
{
  std::vector<std::string> files;

  std::string content = ErrorHandler::safeReadFile(".bittrack/commits/" + commit);
  if (content.empty())
  {
    return files;
  }

  std::istringstream iss(content);
  std::string line;
  bool in_files_section = false;
  while (std::getline(iss, line))
  {
    if (line.find("Files:") == 0) // Check for the start of the Files section
    {
      in_files_section = true;
      continue;
    }

    // Process lines in the Files section
    if (in_files_section && !line.empty())
    {
      // Trim leading whitespace
      size_t start = line.find_first_not_of(" \t");
      if (start != std::string::npos) // If there's non-whitespace
      {
        line = line.substr(start);
      }

      // Split line into filename and hash
      size_t space_pos = line.find(" ");
      if (space_pos != std::string::npos) // If space found
      {
        // Extract filename and hash
        std::string filename = line.substr(0, space_pos);
        std::string hash = line.substr(space_pos + 1);

        if (hash.empty() || hash.find_first_not_of(" \t") == std::string::npos) // Check for deletion
        {
          files.push_back(filename + " (deleted)");
        }
        else // Normal file
        {
          files.push_back(filename);
        }
      }
      else // No space found, just a filename
      {
        files.push_back(line);
      }
    }
  }

  return files;
}

std::string getCommitMessage(const std::string &commit)
{
  std::string content = ErrorHandler::safeReadFile(".bittrack/commits/" + commit);
  if (content.empty())
  {
    return "";
  }

  std::istringstream iss(content);
  std::string line;
  while (std::getline(iss, line))
  {
    if (line.find("Message:") == 0) // Check for Message line
    {
      return line.substr(8);
    }
  }

  return "";
}

void integratePulledFilesWithBittrack(
    const std::string &commit_sha,
    const std::vector<std::string> &downloaded_files)
{
  try
  {
    if (!ErrorHandler::safeWriteFile(".bittrack/index", ""))
    { // Clear staging area
      ErrorHandler::printError(
          ErrorCode::FILE_WRITE_ERROR,
          "Could not clear staging area",
          ErrorSeverity::ERROR,
          "integratePulledFilesWithBittrack");
    }

    std::string commit_dir = ".bittrack/objects/" + commit_sha; // Create commit directory
    ErrorHandler::safeCreateDirectories(commit_dir);

    // Copy pulled files to commit directory
    for (const std::string &file_path : downloaded_files)
    {
      if (std::filesystem::exists(file_path))
      {
        // Copy file to commit directory
        std::string commit_file_path = commit_dir + "/" + file_path;
        ErrorHandler::safeCopyFile(file_path, commit_file_path);
      }
    }

    // Remove files not in the pulled list
    for (const auto &entry : ErrorHandler::safeListDirectoryFiles(commit_dir))
    {
      // Get relative path of the file
      std::string file_path = entry.string();
      // Check if the file was pulled
      bool was_pulled = false;
      for (const auto &pulled_file : downloaded_files)
      {
        // Compare relative paths
        if (file_path == pulled_file)
        {
          was_pulled = true;
          break;
        }
      }

      if (!was_pulled)
      {
        std::filesystem::remove(std::filesystem::path(commit_dir) / file_path);
      }
    }

    // Create commit log
    std::string commit_log_content = "GitHub Pull: " + commit_sha + "\n";
    commit_log_content += "Date: " + getCurrentTimestamp() + "\n";
    commit_log_content += "Files:\n";

    // Write file paths
    for (const std::string &file_path : downloaded_files)
    {
      commit_log_content += file_path + "\n";
    }

    if (!ErrorHandler::safeWriteFile(".bittrack/commits/" + commit_sha, commit_log_content))
    {
      ErrorHandler::printError(
          ErrorCode::FILE_WRITE_ERROR,
          "Could not create commit log",
          ErrorSeverity::ERROR,
          "integratePulledFilesWithBittrack");
    }

    // Update branch reference
    std::string current_branch = getCurrentBranchName();
    if (!current_branch.empty())
    {
      if (!ErrorHandler::safeWriteFile(".bittrack/refs/heads/" + current_branch, commit_sha + "\n"))
      {
        ErrorHandler::printError(
            ErrorCode::FILE_WRITE_ERROR,
            "Could not update branch reference",
            ErrorSeverity::ERROR,
            "integratePulledFilesWithBittrack");
      }
    }

    // Update commit history
    if (!ErrorHandler::safeAppendFile(".bittrack/commits/history", commit_sha + " " + current_branch + "\n"))
    {
      ErrorHandler::printError(
          ErrorCode::FILE_WRITE_ERROR,
          "Could not update commit history",
          ErrorSeverity::ERROR,
          "integratePulledFilesWithBittrack");
    }

    // Update last pushed commit
    setLastPushedCommit(commit_sha);
  }
  catch (const std::exception &e)
  {
    ErrorHandler::printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Error integrating files: " + std::string(e.what()),
        ErrorSeverity::ERROR,
        "integratePulledFilesWithBittrack");
  }
}

bool isLocalBehindRemote()
{
  std::string remote_url = getRemoteOriginUrl();
  if (remote_url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::INVALID_REMOTE_URL,
        "No remote origin set. Use '--remote -s <url>' first",
        ErrorSeverity::ERROR, "is_local_behind_remote");
    return false;
  }

  if (isGithubRemote(remote_url))
  {
    // Get GitHub token from config
    std::string token = configGet("github.token", ConfigScope::REPOSITORY);
    if (token.empty())
    {
      token = configGet("github.token", ConfigScope::GLOBAL);
    }
    if (token.empty())
    {
      ErrorHandler::printError(
          ErrorCode::CONFIG_ERROR,
          "GitHub token not configured. Use 'bittrack "
          "--config github.token <token>'",
          ErrorSeverity::ERROR,
          "is_local_behind_remote");
      return false;
    }

    // Extract username and repo name from URL
    std::string username, repo_name;
    if (extractInfoFromGithubUrl(remote_url, username, repo_name).empty())
    {
      ErrorHandler::printError(
          ErrorCode::INVALID_REMOTE_URL,
          "Could not parse GitHub repository URL",
          ErrorSeverity::ERROR,
          "is_local_behind_remote");
      return false;
    }

    std::string current_branch = getCurrentBranchName();

    // Get remote last commit SHA
    std::string remote_commit = getGithubLastCommithash(token, username, repo_name, "heads/" + current_branch);
    if (remote_commit.empty())
    {
      return false;
    }

    // Get last pushed commit
    std::string last_pushed = getLastPushedCommit();
    if (last_pushed.empty())
    {
      return false;
    }

    if (last_pushed == remote_commit)
    {
      return false;
    }

    if (hasUnpushedCommits())
    {
      return false;
    }
    else
    {
      std::cout << "Local repository is behind remote. Last pushed: " << last_pushed.substr(0, 8) << ", Remote: " << remote_commit.substr(0, 8) << std::endl;
      return true;
    }
  }

  return false;
}

std::string getCommitAuthor(const std::string &commit_hash)
{
  // Read commit file to get author name
  std::string commit_file = ".bittrack/commits/" + commit_hash;
  if (!std::filesystem::exists(commit_file))
  {
    return "BitTrack User";
  }

  // Parse commit file
  std::ifstream file(commit_file);
  std::string line;
  while (std::getline(file, line))
  {
    if (line.find("Author: ") == 0)
    {
      return line.substr(8);
    }
  }
  return "BitTrack User";
}

std::string getCommitAuthorEmail(const std::string &commit_hash)
{
  std::string email = configGet("user.email");
  if (!email.empty())
  {
    return email;
  }
  return "";
}

std::string getCommitTimestamp(const std::string &commit_hash)
{
  // Read commit file to get timestamp
  std::string commit_file = ".bittrack/commits/" + commit_hash;
  if (!std::filesystem::exists(commit_file))
  {
    return "";
  }

  // Parse commit file
  std::ifstream file(commit_file);
  std::string line;
  while (std::getline(file, line))
  {
    if (line.find("Timestamp: ") == 0)
    {
      return line.substr(11);
    }
  }
  return "";
}

bool hasUnpushedCommits()
{
  std::string current_commit = getCurrentCommit();
  std::string last_pushed = getLastPushedCommit();

  if (current_commit.empty())
  {
    return false;
  }

  if (last_pushed.empty())
  {
    return true;
  }

  return current_commit != last_pushed;
}
