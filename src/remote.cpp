#include "../include/remote.hpp"

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

std::string getRemoteOrigin()
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

size_t writeData(void *ptr, size_t size, size_t nmemb, void *stream)
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

  CURL *curl = curl_easy_init();
  if (!curl)
  {
    return {};
  }

  std::string url = remote_url + "/branches";
  std::string response_string;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
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

bool deleteRemoteBranch(const std::string &branch_name, const std::string &remote_name)
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

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

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

void push(const std::string &remote_url_arg, const std::string &branch_name_arg)
{
  // Determine remote URL
  std::string remote_url = remote_url_arg;
  if (remote_url.empty())
  {
    remote_url = getRemoteOrigin();
  }

  // Determine branch name
  std::string CurrentBranch = branch_name_arg;
  if (CurrentBranch.empty())
  {
    CurrentBranch = getCurrentBranch();
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
        "Local repository is behind the remote repository",
        ErrorSeverity::ERROR,
        "push");
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "Someone else has pushed changes since your last push",
        ErrorSeverity::INFO,
        "push");
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "Please pull the latest changes before pushing",
        ErrorSeverity::INFO,
        "push");
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "Use 'bittrack --pull' to sync with remote",
        ErrorSeverity::INFO,
        "push");
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
      ErrorHandler::printError(ErrorCode::CONFIG_ERROR,
                               "GitHub token not configured. Use 'bittrack "
                               "--config github.token <token>'",
                               ErrorSeverity::ERROR, "push");
      return;
    }

    // Extract username and repository name from URL
    std::string username, repo_name;
    if (extractGithubInfoFromUrl(remote_url, username, repo_name).empty())
    {
      ErrorHandler::printError(ErrorCode::INVALID_REMOTE_URL,
                               "Could not parse GitHub repository URL",
                               ErrorSeverity::ERROR, "push");
      return;
    }

    std::cout << "Pushing to GitHub repository: " << username << "/"
              << repo_name << std::endl;

    // Attempt to push using GitHub API
    if (pushToGithubApi(token, username, repo_name))
    {
      return;
    }
    else
    {
      std::cout << "GitHub API push failed, falling back to regular method..."
                << std::endl;
    }
  }
  // Compress .bittrack/objects folder
  system(
      "zip -r .bittrack/remote_push_folder.zip .bittrack/objects > /dev/null");

  // Initialize CURL for file upload
  CURL *curl;
  CURLcode response;
  curl = curl_easy_init();

  // Upload the zip file
  if (curl)
  {
    struct curl_httppost *form = NULL;
    struct curl_httppost *last = NULL;

    std::string CurrentCommit = getCurrentCommit();

    // Prepare the form data
    curl_formadd(
        &form,
        &last,
        CURLFORM_COPYNAME,
        "upload",
        CURLFORM_FILE,
        ".bittrack/remote_push_folder.zip",
        CURLFORM_END);
    // Add branch and commit fields
    curl_formadd(
        &form,
        &last,
        CURLFORM_COPYNAME,
        "branch",
        CURLFORM_COPYCONTENTS,
        CurrentBranch.c_str(),
        CURLFORM_END);
    // Add commit field
    curl_formadd(
        &form,
        &last,
        CURLFORM_COPYNAME,
        "commit",
        CURLFORM_COPYCONTENTS,
        CurrentCommit.c_str(),
        CURLFORM_END);

    // Set CURL options
    curl_easy_setopt(curl, CURLOPT_URL, remote_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, form);

    // Perform the file upload
    response = curl_easy_perform(curl);

    if (response != CURLE_OK)
    {
      ErrorHandler::printError(
          ErrorCode::REMOTE_CONNECTION_FAILED,
          "curl_easy_perform() failed: " + std::string(curl_easy_strerror(response)),
          ErrorSeverity::ERROR,
          "push");
    }
    else
    {
      // Check for GitHub remote
      if (isGithubRemote(remote_url))
      {
        std::cout << "Note: GitHub integration is currently limited." << std::endl;
        std::cout << "BitTrack uses a simple HTTP upload method that may not work with GitHub's API." << std::endl;
        std::cout << "For full GitHub integration, consider using Git directly or implementing proper Git protocol support." << std::endl;
      }
      else
      {
        std::cout << "File uploaded successfully!" << std::endl;
      }
    }

    // Clean up CURL resources
    curl_easy_cleanup(curl);
    curl_formfree(form);
  }
  // Remove the temporary zip file
  system("rm -f .bittrack/remote_push_folder.zip");
}

void pull(const std::string &remote_url, const std::string &branch_name)
{
  // Determine remote URL
  std::string target_url = remote_url;
  if (target_url.empty())
  {
    target_url = getRemoteOrigin();
  }

  if (target_url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::INVALID_REMOTE_URL,
        "No remote origin set. Use '--remote -s <url>' first",
        ErrorSeverity::ERROR,
        "pull");
    return;
  }

  // Determine branch name
  std::string target_branch = branch_name;
  if (target_branch.empty())
  {
    target_branch = getCurrentBranch();
  }

  // Check for uncommitted changes
  std::vector<std::string> staged_files = getStagedFiles();
  std::vector<std::string> unstaged_files = getUnstagedFiles();

  // If there are uncommitted changes, abort the pull
  if (!staged_files.empty() || !unstaged_files.empty())
  {
    ErrorHandler::printError(
        ErrorCode::UNCOMMITTED_CHANGES,
        "Cannot pull with uncommitted changes",
        ErrorSeverity::ERROR,
        "pull");
    ErrorHandler::printError(
        ErrorCode::UNCOMMITTED_CHANGES,
        "Please commit or stash your changes before pulling",
        ErrorSeverity::INFO,
        "pull");
    return;
  }

  // Handle GitHub remote
  if (isGithubRemote(target_url))
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
          "pull");
      return;
    }

    // Extract username and repository name from URL
    std::string username, repo_name;
    if (extractGithubInfoFromUrl(target_url, username, repo_name).empty())
    {
      ErrorHandler::printError(
          ErrorCode::INVALID_REMOTE_URL,
          "Could not parse GitHub repository URL",
          ErrorSeverity::ERROR,
          "pull");
      return;
    }

    // Attempt to pull using GitHub API
    if (pullFromGithubApi(token, username, repo_name, target_branch))
    {
      return;
    }
    else
    {
      ErrorHandler::printError(
          ErrorCode::REMOTE_CONNECTION_FAILED,
          "Failed to pull from GitHub. Please check your "
          "token and internet connection",
          ErrorSeverity::ERROR,
          "pull");
      return;
    }
  }
  else
  {
    ErrorHandler::printError(
        ErrorCode::INTERNAL_ERROR,
        "Pull is only supported for GitHub repositories. "
        "Please use a GitHub repository URL",
        ErrorSeverity::ERROR,
        "pull");
    return;
  }
}

void addRemote(const std::string &name, const std::string &url)
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

void pushToRemote(const std::string &remote_name, const std::string &branch_name)
{
  // Get remote URL
  std::string remote_url = getRemoteUrl(remote_name);
  if (remote_url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::FILE_NOT_FOUND,
        "Remote '" + remote_name + "' not found",
        ErrorSeverity::ERROR, "pushToRemote");
    return;
  }

  // Check current branch
  std::string current_branch = getCurrentBranch();
  if (current_branch != branch_name)
  {
    ErrorHandler::printError(
        ErrorCode::INVALID_ARGUMENTS,
        "Current branch '" + current_branch + "' does not match target branch '" + branch_name + "'",
        ErrorSeverity::ERROR,
        "pushToRemote");
    return;
  }

  // Perform push using the refactored function
  push(remote_url, branch_name);
}

void pullFromRemote(const std::string &remote_name, const std::string &branch_name)
{
  // Get remote URL
  std::string remote_url = getRemoteUrl(remote_name);
  if (remote_url.empty())
  {
    ErrorHandler::printError(
        ErrorCode::FILE_NOT_FOUND,
        "Remote '" + remote_name + "' not found",
        ErrorSeverity::ERROR,
        "pullFromRemote");
    return;
  }

  // Check current branch
  std::string current_branch = getCurrentBranch();
  if (current_branch != branch_name)
  {
    ErrorHandler::printError(
        ErrorCode::INVALID_ARGUMENTS,
        "Current branch '" + current_branch + "' does not match target branch '" + branch_name + "'",
        ErrorSeverity::ERROR,
        "pullFromRemote");
    return;
  }

  // Set remote origin and pull
  setRemoteOrigin(remote_url);
  pull(remote_url, branch_name);
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
  std::string current_branch = getCurrentBranch();
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

void cloneRepository(const std::string &url, const std::string &local_path)
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

  // Determine target path
  std::string target_path = local_path.empty() ? "cloned_repo" : local_path;

  if (std::filesystem::exists(target_path))
  {
    ErrorHandler::printError(
        ErrorCode::FILESYSTEM_ERROR,
        "Directory '" + target_path + "' already exists",
        ErrorSeverity::ERROR,
        "cloneRepository");
    return;
  }

  std::cout << "Cloning repository from " << url << " to " << target_path << "..." << std::endl;

  // Create target directory
  std::filesystem::create_directories(target_path);

  // Change to target directory
  std::string original_dir = std::filesystem::current_path().string();
  std::filesystem::current_path(target_path);

  system("mkdir -p .bittrack"); // Create .bittrack directory
  setRemoteOrigin(url);         // Set remote origin to the provided URL
  fetchFromRemote("origin");    // Fetch from remote

  // Extract fetched zip file
  std::filesystem::current_path(original_dir);

  std::cout << "Repository cloned successfully to " << target_path << std::endl;
}

bool isRemoteConfigured()
{
  // Check if remote origin is set
  return !getRemoteOrigin().empty();
}

std::string getRemoteUrl(const std::string &remote_name)
{
  if (remote_name == "origin")
  {
    return getRemoteOrigin();
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

void updateRemoteUrl(const std::string &remote_name, const std::string &new_url)
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
  std::string origin_url = getRemoteOrigin();
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

// bool isGithubRemote(const std::string &url)
// {
//   // Simple check for GitHub URL
//   return url.find("github.com") != std::string::npos;
// }

std::string base64Encode(const std::string &input)
{
  // Base64 encoding implementation
  const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string result;
  int val = 0, valb = -6;

  // Encode input string to base64
  for (unsigned char c : input)
  {
    val = (val << 8) + c; // Shift left and add character
    valb += 8;            // Increase bit count
    while (valb >= 0)     // While there are enough bits
    {
      // Extract 6 bits and map to base64 character
      result.push_back(chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) // Handle remaining bits
  {
    result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
  }
  while (result.size() % 4) // Pad with '=' characters
  {
    result.push_back('=');
  }
  return result;
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

  std::string content =
      ErrorHandler::safeReadFile(".bittrack/commits/" + commit);
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
  std::string content =
      ErrorHandler::safeReadFile(".bittrack/commits/" + commit);
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

std::string base64Decode(const std::string &encoded)
{
  const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string decoded;
  int val = 0, valb = -8;
  int padding = 0;

  for (char c : encoded)
  {
    if (c == '=')
    {
      padding++;
      continue;
    }
    if (c == '\n' || c == '\r' || c == ' ' || c == '\t')
    {
      continue; // Skip whitespace and newlines
    }
    size_t pos = chars.find(c);
    if (pos == std::string::npos)
    {
      // Invalid character found, return empty string to indicate error
      ErrorHandler::printError(
          ErrorCode::REMOTE_CONNECTION_FAILED,
          "Invalid character in base64 encoded data",
          ErrorSeverity::ERROR,
          "base64Decode");
      return "";
    }
    val = (val << 6) + pos;
    valb += 6;
    if (valb >= 0)
    {
      decoded.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }

  return decoded;
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
    std::vector<std::string> pulled_files = downloaded_files; // Files to integrate

    // Copy pulled files to commit directory
    for (const std::string &file_path : pulled_files)
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
      std::string relative_path = std::filesystem::relative(file_path, commit_dir).string();

      // Check if the file was pulled
      bool was_pulled = false;
      for (const auto &pulled_file : pulled_files)
      {
        // Compare relative paths
        if (relative_path == pulled_file)
        {
          was_pulled = true;
          break;
        }
      }

      if (!was_pulled)
      {
        std::filesystem::remove(file_path);
      }
    }

    // Create commit log
    std::string commit_log_content = "GitHub Pull: " + commit_sha + "\n";
    commit_log_content += "Date: " + getCurrentTimestamp() + "\n";
    commit_log_content += "Files:\n";

    if (!ErrorHandler::safeWriteFile(".bittrack/commits/" + commit_sha, commit_log_content))
    {
      ErrorHandler::printError(
          ErrorCode::FILE_WRITE_ERROR,
          "Could not create commit log",
          ErrorSeverity::ERROR,
          "integratePulledFilesWithBittrack");
    }

    // Update branch reference
    std::string current_branch = getCurrentBranch();
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
  std::string remote_url = getRemoteOrigin();
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
    if (extractGithubInfoFromUrl(remote_url, username, repo_name).empty())
    {
      ErrorHandler::printError(
          ErrorCode::INVALID_REMOTE_URL,
          "Could not parse GitHub repository URL",
          ErrorSeverity::ERROR,
          "is_local_behind_remote");
      return false;
    }

    std::string current_branch = getCurrentBranch();

    // Get remote last commit SHA
    std::string remote_commit = getGithubLastCommitSha(token, username, repo_name, "heads/" + current_branch);
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
