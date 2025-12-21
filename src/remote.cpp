#include "../include/remote.hpp"

static size_t write_callback(void *contents, size_t size, size_t nmemb,
                             void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

void set_remote_origin(const std::string &url) {
  if (!ErrorHandler::safeWriteFile(".bittrack/remote", url + "\n")) {
    ErrorHandler::printError(ErrorCode::FILE_WRITE_ERROR,
                             "Could not open .bittrack/remote for writing",
                             ErrorSeverity::ERROR, "set_remote_origin");
  }
}

std::string get_remote_origin() {
  std::string url = ErrorHandler::safeReadFile(".bittrack/remote");
  if (url.empty()) {
    return "";
  }

  // Remove newline if present
  if (url.back() == '\n') {
    url.pop_back();
  }

  return url;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  // Write data to the provided stream (ofstream in this case)
  std::ofstream *out = static_cast<std::ofstream *>(stream);
  out->write(static_cast<char *>(ptr), size * nmemb);
  return size * nmemb;
}

std::vector<std::string> list_remote_branches(const std::string &remote_name) {
  std::string remote_url = get_remote_url(remote_name);
  if (remote_url.empty()) {
    ErrorHandler::printError(ErrorCode::FILE_NOT_FOUND,
                             "Remote '" + remote_name + "' not found",
                             ErrorSeverity::ERROR, "list_remote_branches");
    return {};
  }

  CURL *curl = curl_easy_init();
  if (!curl) {
    return {};
  }

  std::string url = remote_url + "/branches";
  std::string response_string;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl);

  std::vector<std::string> branches;
  if (res == CURLE_OK && http_code == 200) {
    std::stringstream ss(response_string);
    std::string branch;
    while (std::getline(ss, branch)) {
      if (!branch.empty()) {
        branches.push_back(branch);
      }
    }
  } else {
    ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                             "Failed to list remote branches",
                             ErrorSeverity::ERROR, "list_remote_branches");
  }
  return branches;
}

bool delete_remote_branch(const std::string &branch_name,
                          const std::string &remote_name) {
  std::string remote_url = get_remote_url(remote_name);
  if (remote_url.empty()) {
    ErrorHandler::printError(ErrorCode::FILE_NOT_FOUND,
                             "Remote '" + remote_name + "' not found",
                             ErrorSeverity::ERROR, "delete_remote_branch");
    return false;
  }

  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  std::string url = remote_url + "/branches/" + branch_name;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl);

  if (res == CURLE_OK && (http_code == 200 || http_code == 204)) {
    std::cout << "Remote branch '" << branch_name << "' deleted successfully."
              << std::endl;
    return true;
  } else {
    ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                             "Failed to delete remote branch '" + branch_name +
                                 "'",
                             ErrorSeverity::ERROR, "delete_remote_branch");
    return false;
  }
}

// Refactored push to allow flexible remote and branch selection
void push(const std::string &remote_url_arg,
          const std::string &branch_name_arg) {
  // Determine remote URL
  std::string remote_url = remote_url_arg;
  if (remote_url.empty()) {
    remote_url = get_remote_origin();
  }

  // Determine branch name
  std::string CurrentBranch = branch_name_arg;
  if (CurrentBranch.empty()) {
    CurrentBranch = getCurrentBranch();
  }

  // Validate remote URL
  if (remote_url.empty()) {
    ErrorHandler::printError(
        ErrorCode::INVALID_REMOTE_URL,
        "No remote origin set. Use '--remote -s <url>' first",
        ErrorSeverity::ERROR, "push");
    return;
  }

  // Check for uncommitted changes
  if (is_local_behind_remote()) {
    ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                             "Local repository is behind the remote repository",
                             ErrorSeverity::ERROR, "push");
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "Someone else has pushed changes since your last push",
        ErrorSeverity::INFO, "push");
    ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                             "Please pull the latest changes before pushing",
                             ErrorSeverity::INFO, "push");
    ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                             "Use 'bittrack --pull' to sync with remote",
                             ErrorSeverity::INFO, "push");
    return;
  }

  // Handle GitHub remote
  if (is_github_remote(remote_url)) {
    // Get GitHub token from config
    std::string token = configGet("github.token", ConfigScope::REPOSITORY);
    if (token.empty()) {
      token = configGet("github.token", ConfigScope::GLOBAL);
    }
    if (token.empty()) {
      ErrorHandler::printError(ErrorCode::CONFIG_ERROR,
                               "GitHub token not configured. Use 'bittrack "
                               "--config github.token <token>'",
                               ErrorSeverity::ERROR, "push");
      return;
    }

    // Extract username and repository name from URL
    std::string username, repo_name;
    if (extract_github_info_from_url(remote_url, username, repo_name).empty()) {
      ErrorHandler::printError(ErrorCode::INVALID_REMOTE_URL,
                               "Could not parse GitHub repository URL",
                               ErrorSeverity::ERROR, "push");
      return;
    }

    std::cout << "Pushing to GitHub repository: " << username << "/"
              << repo_name << std::endl;

    // Attempt to push using GitHub API
    if (push_to_github_api(token, username, repo_name)) {
      return;
    } else {
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
  if (curl) {
    struct curl_httppost *form = NULL;
    struct curl_httppost *last = NULL;

    std::string CurrentCommit = get_current_commit();

    // Prepare the form data
    curl_formadd(&form, &last, CURLFORM_COPYNAME, "upload", CURLFORM_FILE,
                 ".bittrack/remote_push_folder.zip", CURLFORM_END);
    // Add branch and commit fields
    curl_formadd(&form, &last, CURLFORM_COPYNAME, "branch",
                 CURLFORM_COPYCONTENTS, CurrentBranch.c_str(), CURLFORM_END);
    // Add commit field
    curl_formadd(&form, &last, CURLFORM_COPYNAME, "commit",
                 CURLFORM_COPYCONTENTS, CurrentCommit.c_str(), CURLFORM_END);

    // Set CURL options
    curl_easy_setopt(curl, CURLOPT_URL, remote_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, form);

    // Perform the file upload
    response = curl_easy_perform(curl);

    if (response != CURLE_OK) {
      ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                               "curl_easy_perform() failed: " +
                                   std::string(curl_easy_strerror(response)),
                               ErrorSeverity::ERROR, "push");
    } else {
      // Check for GitHub remote
      if (is_github_remote(remote_url)) {
        std::cout << "Note: GitHub integration is currently limited."
                  << std::endl;
        std::cout << "BitTrack uses a simple HTTP upload method that may not "
                     "work with GitHub's API."
                  << std::endl;
        std::cout << "For full GitHub integration, consider using Git directly "
                     "or implementing proper Git protocol support."
                  << std::endl;
      } else {
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

void pull(const std::string &remote_url, const std::string &branch_name) {
  // Determine remote URL
  std::string target_url = remote_url;
  if (target_url.empty()) {
    target_url = get_remote_origin();
  }

  if (target_url.empty()) {
    ErrorHandler::printError(
        ErrorCode::INVALID_REMOTE_URL,
        "No remote origin set. Use '--remote -s <url>' first",
        ErrorSeverity::ERROR, "pull");
    return;
  }

  // Determine branch name
  std::string target_branch = branch_name;
  if (target_branch.empty()) {
    target_branch = getCurrentBranch();
  }

  // Check for uncommitted changes
  std::vector<std::string> staged_files = getStagedFiles();
  std::vector<std::string> unstaged_files = getUnstagedFiles();

  // If there are uncommitted changes, abort the pull
  if (!staged_files.empty() || !unstaged_files.empty()) {
    ErrorHandler::printError(ErrorCode::UNCOMMITTED_CHANGES,
                             "Cannot pull with uncommitted changes",
                             ErrorSeverity::ERROR, "pull");
    ErrorHandler::printError(
        ErrorCode::UNCOMMITTED_CHANGES,
        "Please commit or stash your changes before pulling",
        ErrorSeverity::INFO, "pull");
    return;
  }

  // Handle GitHub remote
  if (is_github_remote(target_url)) {
    // Get GitHub token from config
    std::string token = configGet("github.token", ConfigScope::REPOSITORY);
    if (token.empty()) {
      token = configGet("github.token", ConfigScope::GLOBAL);
    }
    if (token.empty()) {
      ErrorHandler::printError(ErrorCode::CONFIG_ERROR,
                               "GitHub token not configured. Use 'bittrack "
                               "--config github.token <token>'",
                               ErrorSeverity::ERROR, "pull");
      return;
    }

    // Extract username and repository name from URL
    std::string username, repo_name;
    if (extract_github_info_from_url(target_url, username, repo_name).empty()) {
      ErrorHandler::printError(ErrorCode::INVALID_REMOTE_URL,
                               "Could not parse GitHub repository URL",
                               ErrorSeverity::ERROR, "pull");
      return;
    }

    // Attempt to pull using GitHub API
    if (pull_from_github_api(token, username, repo_name, target_branch)) {
      return;
    } else {
      ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                               "Failed to pull from GitHub. Please check your "
                               "token and internet connection",
                               ErrorSeverity::ERROR, "pull");
      return;
    }
  } else {
    ErrorHandler::printError(ErrorCode::INTERNAL_ERROR,
                             "Pull is only supported for GitHub repositories. "
                             "Please use a GitHub repository URL",
                             ErrorSeverity::ERROR, "pull");
    return;
  }
}

void add_remote(const std::string &name, const std::string &url) {
  // Validate inputs
  if (name.empty() || url.empty()) {
    ErrorHandler::printError(ErrorCode::INVALID_ARGUMENTS,
                             "Remote name and URL cannot be empty",
                             ErrorSeverity::ERROR, "add_remote");
    return;
  }

  // Create remotes directory if it doesn't exist
  std::string remote_file = ".bittrack/remotes/" + name;

  // Check if remote already exists and write the URL to the remote file
  if (!ErrorHandler::safeWriteFile(remote_file, url + "\n")) {
    ErrorHandler::printError(ErrorCode::FILE_WRITE_ERROR,
                             "Could not create remote file for " + name,
                             ErrorSeverity::ERROR, "add_remote");
    return;
  }

  std::cout << "Added remote '" << name << "' with URL: " << url << std::endl;
}

void remove_remote(const std::string &name) {
  if (name.empty()) {
    ErrorHandler::printError(ErrorCode::INVALID_ARGUMENTS,
                             "Remote name cannot be empty",
                             ErrorSeverity::ERROR, "remove_remote");
    return;
  }

  // Check if remote exists
  std::string remote_file = ".bittrack/remotes/" + name;

  // If remote does not exist, print error
  if (!std::filesystem::exists(remote_file)) {
    ErrorHandler::printError(ErrorCode::FILE_NOT_FOUND,
                             "Remote '" + name + "' does not exist",
                             ErrorSeverity::ERROR, "remove_remote");
    return;
  }

  // Remove the remote file
  if (ErrorHandler::safeRemoveFile(remote_file)) {
    std::cout << "Removed remote '" << name << "'" << std::endl;
  }
}

void list_remotes() {
  // Directory where remotes are stored
  std::string remotes_dir = ".bittrack/remotes";

  if (!std::filesystem::exists(remotes_dir)) {
    std::cout << "No remotes configured" << std::endl;
    return;
  }

  // List all remotes
  std::cout << "Configured remotes:" << std::endl;
  for (const auto &entry : std::filesystem::directory_iterator(remotes_dir)) {
    if (entry.is_regular_file()) {
      // Get remote name and URL
      std::string remote_name = entry.path().filename().string();
      std::string remote_url = get_remote_url(remote_name);

      if (!remote_url.empty()) {
        std::cout << "  " << remote_name << " -> " << remote_url << std::endl;
      }
    }
  }
}

void push_to_remote(const std::string &remote_name,
                    const std::string &branch_name) {
  // Get remote URL
  std::string remote_url = get_remote_url(remote_name);
  if (remote_url.empty()) {
    ErrorHandler::printError(ErrorCode::FILE_NOT_FOUND,
                             "Remote '" + remote_name + "' not found",
                             ErrorSeverity::ERROR, "push_to_remote");
    return;
  }

  // Check current branch
  std::string current_branch = getCurrentBranch();
  if (current_branch != branch_name) {
    ErrorHandler::printError(ErrorCode::INVALID_ARGUMENTS,
                             "Current branch '" + current_branch +
                                 "' does not match target branch '" +
                                 branch_name + "'",
                             ErrorSeverity::ERROR, "push_to_remote");
    return;
  }

  // Perform push using the refactored function
  push(remote_url, branch_name);
}

void pull_from_remote(const std::string &remote_name,
                      const std::string &branch_name) {
  // Get remote URL
  std::string remote_url = get_remote_url(remote_name);
  if (remote_url.empty()) {
    ErrorHandler::printError(ErrorCode::FILE_NOT_FOUND,
                             "Remote '" + remote_name + "' not found",
                             ErrorSeverity::ERROR, "pull_from_remote");
    return;
  }

  // Check current branch
  std::string current_branch = getCurrentBranch();
  if (current_branch != branch_name) {
    ErrorHandler::printError(ErrorCode::INVALID_ARGUMENTS,
                             "Current branch '" + current_branch +
                                 "' does not match target branch '" +
                                 branch_name + "'",
                             ErrorSeverity::ERROR, "pull_from_remote");
    return;
  }

  // Set remote origin and pull
  set_remote_origin(remote_url);
  pull(remote_url, branch_name);
}

void fetch_from_remote(const std::string &remote_name) {
  // Get remote URL
  std::string remote_url = get_remote_url(remote_name);
  if (remote_url.empty()) {
    ErrorHandler::printError(ErrorCode::FILE_NOT_FOUND,
                             "Remote '" + remote_name + "' not found",
                             ErrorSeverity::ERROR, "fetch_from_remote");
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

  if (curl) {
    // Open file to write the downloaded data
    std::ofstream outfile(".bittrack/remote_fetch_folder.zip",
                          std::ios::binary);
    if (!outfile.is_open()) {
      ErrorHandler::printError(
          ErrorCode::FILE_WRITE_ERROR,
          "Could not open .bittrack/remote_fetch_folder.zip for writing",
          ErrorSeverity::ERROR, "fetch_from_remote");
      curl_easy_cleanup(curl);
      return;
    }

    curl_easy_setopt(curl, CURLOPT_URL,
                     remote_url_with_branch.c_str()); // Set the URL
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                     write_data); // Set write callback
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                     &outfile); // Set output file stream
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,
                     1L); // Follow redirects if any

    // Perform the file download
    response = curl_easy_perform(curl);

    if (response != CURLE_OK) {
      ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                               "curl_easy_perform() failed: " +
                                   std::string(curl_easy_strerror(response)),
                               ErrorSeverity::ERROR, "fetch_from_remote");
    } else {
      std::cout << "Fetched successfully from remote '" << remote_name << "'"
                << std::endl;
    }

    // Close the output file and clean up CURL
    outfile.close();
    curl_easy_cleanup(curl);
  }

  system("rm -f .bittrack/remote_fetch_folder.zip");
}

void clone_repository(const std::string &url, const std::string &local_path) {
  if (url.empty()) {
    ErrorHandler::printError(ErrorCode::INVALID_ARGUMENTS,
                             "Repository URL cannot be empty",
                             ErrorSeverity::ERROR, "clone_repository");
    return;
  }

  // Determine target path
  std::string target_path = local_path.empty() ? "cloned_repo" : local_path;

  if (std::filesystem::exists(target_path)) {
    ErrorHandler::printError(ErrorCode::FILESYSTEM_ERROR,
                             "Directory '" + target_path + "' already exists",
                             ErrorSeverity::ERROR, "clone_repository");
    return;
  }

  std::cout << "Cloning repository from " << url << " to " << target_path
            << "..." << std::endl;

  // Create target directory
  std::filesystem::create_directories(target_path);

  // Change to target directory
  std::string original_dir = std::filesystem::current_path().string();
  std::filesystem::current_path(target_path);

  system("mkdir -p .bittrack"); // Create .bittrack directory
  set_remote_origin(url);       // Set remote origin to the provided URL
  fetch_from_remote("origin");  // Fetch from remote

  // Extract fetched zip file
  std::filesystem::current_path(original_dir);

  std::cout << "Repository cloned successfully to " << target_path << std::endl;
}

bool is_remote_configured() {
  // Check if remote origin is set
  return !get_remote_origin().empty();
}

std::string get_remote_url(const std::string &remote_name) {
  if (remote_name == "origin") {
    return get_remote_origin();
  }

  std::string remote_file = ".bittrack/remotes/" + remote_name;

  if (!std::filesystem::exists(remote_file)) {
    return "";
  }

  // Read the URL from the remote file
  std::string url = ErrorHandler::safeReadFile(remote_file);
  if (url.empty()) {
    return "";
  }

  // Remove newline if present
  if (!url.empty() && url.back() == '\n') {
    url.pop_back();
  }

  return url;
}

void update_remote_url(const std::string &remote_name,
                       const std::string &new_url) {
  if (remote_name.empty() || new_url.empty()) {
    ErrorHandler::printError(ErrorCode::INVALID_ARGUMENTS,
                             "Remote name and URL cannot be empty",
                             ErrorSeverity::ERROR, "update_remote_url");
    return;
  }

  // Handle origin remote separately
  if (remote_name == "origin") {
    set_remote_origin(new_url);
    std::cout << "Updated origin remote URL to: " << new_url << std::endl;
    return;
  }

  std::string remote_file = ".bittrack/remotes/" + remote_name;

  if (!std::filesystem::exists(remote_file)) {
    ErrorHandler::printError(ErrorCode::FILE_NOT_FOUND,
                             "Remote '" + remote_name + "' does not exist",
                             ErrorSeverity::ERROR, "update_remote_url");
    return;
  }

  // Update the URL in the remote file using safeWriteFile
  if (ErrorHandler::safeWriteFile(remote_file, new_url + "\n")) {
    std::cout << "Updated remote '" << remote_name << "' URL to: " << new_url
              << std::endl;
  }
}

void show_remote_info() {
  std::cout << "Remote Information:" << std::endl;
  std::cout << "==================" << std::endl;

  // Get and display origin URL
  std::string origin_url = get_remote_origin();
  if (!origin_url.empty()) {
    std::cout << "Origin: " << origin_url << std::endl;
  } else {
    std::cout << "Origin: Not configured" << std::endl;
  }

  // List all remotes
  list_remotes();
}

bool is_github_remote(const std::string &url) {
  // Simple check for GitHub URL
  return url.find("github.com") != std::string::npos;
}

std::string base64_encode(const std::string &input) {
  // Base64 encoding implementation
  const std::string chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string result;
  int val = 0, valb = -6;

  // Encode input string to base64
  for (unsigned char c : input) {
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

std::string get_last_pushed_commit() {
  std::string commit =
      ErrorHandler::safeReadFile(".bittrack/last_pushed_commit");
  if (commit.empty()) {
    return "";
  }

  // Remove newline if present
  if (commit.back() == '\n') {
    commit.pop_back();
  }

  return commit;
}

void set_last_pushed_commit(const std::string &commit) {
  if (!ErrorHandler::safeWriteFile(".bittrack/last_pushed_commit",
                                   commit + "\n")) {
    ErrorHandler::printError(ErrorCode::FILE_WRITE_ERROR,
                             "Could not update .bittrack/last_pushed_commit",
                             ErrorSeverity::ERROR, "set_last_pushed_commit");
  }
}

void set_github_commit_mapping(const std::string &bittrack_commit,
                               const std::string &github_commit) {
  std::string mapping = bittrack_commit + " " + github_commit + "\n";
  if (!ErrorHandler::safeAppendFile(".bittrack/github_commits", mapping)) {
    ErrorHandler::printError(ErrorCode::FILE_WRITE_ERROR,
                             "Could not update .bittrack/github_commits",
                             ErrorSeverity::ERROR, "set_github_commit_mapping");
  }
}

std::string get_github_commit_for_bittrack(const std::string &bittrack_commit) {
  std::string content = ErrorHandler::safeReadFile(".bittrack/github_commits");
  if (content.empty()) {
    return "";
  }

  std::istringstream iss(content);
  std::string line;
  while (std::getline(iss, line)) {
    if (line.empty()) {
      continue;
    }

    std::istringstream line_ss(line);
    std::string bittrack_hash, github_hash;
    if (line_ss >> bittrack_hash >> github_hash &&
        bittrack_hash == bittrack_commit) {
      return github_hash;
    }
  }
  return "";
}

std::string get_latest_github_commit(const std::string &token,
                                     const std::string &username,
                                     const std::string &repo_name) {
  CURL *curl = curl_easy_init(); // Initialize CURL
  if (!curl) {
    return "";
  }

  std::string current_branch = getCurrentBranch();

  // Prepare the URL for the latest commit
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/git/refs/heads/" + current_branch;

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());    // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK) // Check for errors
  {
    return "";
  }

  // Parse the response to extract the SHA
  size_t sha_pos = response_data.find("\"sha\":\"");
  if (sha_pos != std::string::npos) // If SHA found
  {
    // Extract SHA value
    sha_pos += 7;
    size_t sha_end = response_data.find("\"", sha_pos);
    if (sha_end != std::string::npos) // If end quote found
    {
      // Return the extracted SHA
      return response_data.substr(sha_pos, sha_end - sha_pos);
    }
  }

  return "";
}

std::vector<std::string> get_committed_files(const std::string &commit) {
  std::vector<std::string> files;

  std::string content =
      ErrorHandler::safeReadFile(".bittrack/commits/" + commit);
  if (content.empty()) {
    return files;
  }

  std::istringstream iss(content);
  std::string line;
  bool in_files_section = false;
  while (std::getline(iss, line)) {
    if (line.find("Files:") == 0) // Check for the start of the Files section
    {
      in_files_section = true;
      continue;
    }

    // Process lines in the Files section
    if (in_files_section && !line.empty()) {
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

        if (hash.empty() || hash.find_first_not_of(" \t") ==
                                std::string::npos) // Check for deletion
        {
          files.push_back(filename + " (deleted)");
        } else // Normal file
        {
          files.push_back(filename);
        }
      } else // No space found, just a filename
      {
        files.push_back(line);
      }
    }
  }

  return files;
}

std::string get_commit_message(const std::string &commit) {
  std::string content =
      ErrorHandler::safeReadFile(".bittrack/commits/" + commit);
  if (content.empty()) {
    return "";
  }

  std::istringstream iss(content);
  std::string line;
  while (std::getline(iss, line)) {
    if (line.find("Message:") == 0) // Check for Message line
    {
      return line.substr(8);
    }
  }

  return "";
}

std::string extract_github_info_from_url(const std::string &url,
                                         std::string &username,
                                         std::string &repository) {
  // Regex to extract username and repository from GitHub URL
  std::regex github_regex(
      R"(https?://github\.com/([^/]+)/([^/]+?)(?:\.git)?/?$)");
  std::smatch matches;

  if (std::regex_match(url, matches, github_regex)) // If regex matches
  {
    // Extract username and repository
    username = matches[1].str();
    repository = matches[2].str();
    return username + "/" + repository;
  }

  return "";
}

bool push_to_github_api(const std::string &token, const std::string &username,
                        const std::string &repo_name) {
  try {
    // Get current commit and branch
    std::string current_commit = get_current_commit();
    std::string current_branch = getCurrentBranch();

    if (current_commit.empty()) {
      ErrorHandler::printError(ErrorCode::NO_COMMITS_FOUND,
                               "No commits to push", ErrorSeverity::ERROR,
                               "push_to_github_api");
      return false;
    }

    // Check if already up to date
    std::string last_pushed = get_last_pushed_commit();
    if (last_pushed == current_commit) {
      std::cout << "Repository is up to date." << std::endl;
      return true;
    }

    // Get the last commit SHA on GitHub
    std::string parent_sha = get_github_last_commit_sha(
        token, username, repo_name, "heads/" + current_branch);
    bool branch_exists = !parent_sha.empty();
    bool is_empty_repo = false;

    if (!branch_exists) {
      // Branch does not exist. Check if repository is empty by checking for
      // main branch.
      std::string main_ref =
          get_github_last_commit_sha(token, username, repo_name, "heads/main");
      if (main_ref.empty()) {
        main_ref = get_github_last_commit_sha(token, username, repo_name,
                                              "heads/master");
      }
      is_empty_repo = main_ref.empty();

      // If not empty, we are creating a new branch.
      // We need a base for the new commit if we can find one.
      // For simplicity in this logic, we might start as orphan or try to use
      // mapped parent. But crucially, we must NOT use the file-by-file upload
      // below which defaults to main.
    }

    if (is_empty_repo) {
      // Handle initial commit for empty repository
      std::vector<std::string> latest_commit_files =
          get_committed_files(current_commit);
      if (latest_commit_files.empty()) {
        ErrorHandler::printError(ErrorCode::INTERNAL_ERROR,
                                 "No files found in latest commit",
                                 ErrorSeverity::ERROR, "push_to_github_api");
        return false;
      }

      std::string commit_message =
          get_commit_message(current_commit); // Get commit message

      std::vector<std::string> blob_shas;  // To store blob SHAs
      std::vector<std::string> file_names; // To store file names

      for (const auto &file_path : latest_commit_files) {
        // Read file content using safeReadFile
        std::string content = ErrorHandler::safeReadFile(file_path);
        if (content.empty() && !std::filesystem::exists(file_path)) {
          continue;
        }

        std::string base64_content =
            base64_encode(content); // Encode content to base64

        // Create the file on GitHub
        CURL *curl = curl_easy_init();
        if (!curl) {
          continue;
        }

        std::string response_data; // To store response data
        std::string url = "https://api.github.com/repos/" + username + "/" +
                          repo_name + "/contents/" + file_path; // Prepare URL
        std::string json_data = "{\"message\":\"" + commit_message +
                                "\",\"content\":\"" + base64_content +
                                "\"}"; // Prepare JSON data

        struct curl_slist *headers = nullptr; // Initialize headers
        headers = curl_slist_append(headers,
                                    ("Authorization: token " + token)
                                        .c_str()); // Add authorization header
        headers = curl_slist_append(
            headers,
            "Content-Type: application/json"); // Add content-type header
        headers = curl_slist_append(
            headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // Set the URL
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,
                         "PUT"); // Set HTTP method to PUT
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
                         json_data.c_str());                 // Set JSON data
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         write_callback); // Set write callback
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                         &response_data); // Set response data storage

        CURLcode res = curl_easy_perform(curl); // Perform the request
        curl_slist_free_all(headers);           // Free headers
        curl_easy_cleanup(curl);                // Clean up CURL

        if (res == CURLE_OK && response_data.find("\"sha\":\"") !=
                                   std::string::npos) // Check for success
        {
          std::cout << "Created file: " << file_path << std::endl;
        } else {
          ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                                   "Failed to create file: " + file_path,
                                   ErrorSeverity::ERROR, "push_to_github_api");
        }
      }

      std::cout << "Done..." << std::endl;
      set_last_pushed_commit(current_commit);
      return true;
    }

    // Push commits one by one
    std::string current_tree_sha = "";
    std::string last_commit_sha = parent_sha;
    std::string commit_message = get_commit_message(current_commit);

    // If new branch (last_commit_sha is empty), try to finding parent from
    // local commit history

    std::vector<std::string> committed_files =
        get_committed_files(current_commit); // Get files in current commit
    if (committed_files.empty()) {
      ErrorHandler::printError(ErrorCode::INTERNAL_ERROR,
                               "No files found in current commit",
                               ErrorSeverity::ERROR, "push_to_github_api");
      return false;
    }

    std::vector<std::string> blob_shas;  // To store blob SHAs
    std::vector<std::string> file_names; // To store file names

    for (const auto &file_path : committed_files) {
      // Handle deleted files
      if (isDeleted(file_path)) {
        std::string actual_path =
            getActualPath(file_path); // Get actual file path
        if (delete_github_file(token, username, repo_name, actual_path,
                               commit_message)) // Delete file on GitHub
        {
          std::cout << "Deleted file: " << actual_path << std::endl;
        } else {
          ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                                   "Failed to delete file: " + actual_path,
                                   ErrorSeverity::ERROR, "push_to_github_api");
        }
      } else {
        // Read file content from commit using safeReadFile
        std::string commit_file_path =
            ".bittrack/objects/" + current_commit + "/" + file_path;
        std::string content = ErrorHandler::safeReadFile(commit_file_path);
        if (content.empty() && !std::filesystem::exists(commit_file_path)) {
          ErrorHandler::printError(ErrorCode::FILE_READ_ERROR,
                                   "Could not read commit file: " +
                                       commit_file_path,
                                   ErrorSeverity::ERROR, "push_to_github_api");
          continue;
        }

        // Create blob on GitHub
        std::string blob_sha =
            create_github_blob(token, username, repo_name, content);
        if (!blob_sha.empty()) {
          blob_shas.push_back(blob_sha);
          file_names.push_back(file_path);
          // std::cout << "Created blob for " << file_path << std::endl;
        } else {
          ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                                   "Failed to create blob for " + file_path,
                                   ErrorSeverity::ERROR, "push_to_github_api");
        }
      }
    }

    // Handle case where all files are deleted
    if (blob_shas.empty() && !committed_files.empty()) {
      bool all_deleted = true;
      for (const auto &file_path : committed_files) {
        // Check if file is deleted
        if (!isDeleted(file_path)) {
          all_deleted = false;
          break;
        }
      }

      // If all files are deleted, we still need to create a tree
      if (!all_deleted) {
        ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                                 "Could not create blobs for current commit",
                                 ErrorSeverity::ERROR, "push_to_github_api");
        return false;
      }
    } else if (blob_shas.empty()) // No files to process
    {
      ErrorHandler::printError(ErrorCode::INTERNAL_ERROR,
                               "No files to process for current commit",
                               ErrorSeverity::ERROR, "push_to_github_api");
      return false;
    }

    if (!blob_shas.empty()) // If there are blobs to add
    {
      if (current_tree_sha.empty()) // If no current tree, create new
      {
        current_tree_sha = create_github_tree_with_files(
            token, username, repo_name, blob_shas, file_names);
      } else {
        // Update existing tree
        current_tree_sha =
            create_github_tree_with_files(token, username, repo_name, blob_shas,
                                          file_names, current_tree_sha);
      }

      if (current_tree_sha.empty()) // If tree creation failed
      {
        ErrorHandler::printError(
            ErrorCode::REMOTE_CONNECTION_FAILED,
            "Could not create or update tree for current commit",
            ErrorSeverity::ERROR, "push_to_github_api");
        return false;
      }
    } else if (current_tree_sha.empty()) // If no blobs and no current tree
    {
      if (last_commit_sha.empty()) {
        ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                                 "No parent commit for deletion-only commit",
                                 ErrorSeverity::ERROR, "push_to_github_api");
        return false;
      }
      // Get tree from parent commit
      std::string parent_tree_sha =
          get_github_commit_tree(token, username, repo_name, last_commit_sha);
      if (parent_tree_sha.empty()) {
        ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                                 "Could not get tree from parent commit " +
                                     last_commit_sha,
                                 ErrorSeverity::ERROR, "push_to_github_api");
        return false;
      }
      current_tree_sha = parent_tree_sha; // Set current tree to parent tree
    }

    std::string author_name =
        get_commit_author(current_commit); // Get author name
    std::string author_email =
        get_commit_author_email(current_commit); // Get author email
    std::string commit_timestamp =
        get_commit_timestamp(current_commit); // Get commit timestamp
    std::string new_commit_sha = create_github_commit(
        token, username, repo_name, current_tree_sha, last_commit_sha,
        commit_message, author_name, author_email, commit_timestamp);
    if (new_commit_sha.empty()) {
      ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                               "Could not create commit " + current_commit,
                               ErrorSeverity::ERROR, "push_to_github_api");
      return false;
    }

    last_commit_sha = new_commit_sha; // Update last commit SHA

    set_github_commit_mapping(
        current_commit, new_commit_sha); // Map BitTrack commit to GitHub commit

    if (branch_exists) {
      if (!update_github_ref(token, username, repo_name,
                             "heads/" + current_branch,
                             last_commit_sha)) // Update branch reference
      {
        ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                                 "Could not update branch reference",
                                 ErrorSeverity::ERROR, "push_to_github_api");
        return false;
      }
    } else {
      if (!create_github_ref(token, username, repo_name,
                             "heads/" + current_branch,
                             last_commit_sha)) // Create branch reference
      {
        ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                                 "Could not create branch reference",
                                 ErrorSeverity::ERROR, "push_to_github_api");
        return false;
      }
    }

    std::cout << "Done..." << std::endl;

    set_last_pushed_commit(current_commit); // Update last pushed commit

    return true;
  } catch (const std::exception &e) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Error: " + std::string(e.what()),
                             ErrorSeverity::ERROR, "push_to_github_api");
    return false;
  }
}

std::string create_github_blob(const std::string &token,
                               const std::string &username,
                               const std::string &repo_name,
                               const std::string &content) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return "";
  }

  std::string response_data; // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/git/blobs"; // Prepare URL
  std::string base64_content =
      base64_encode(content); // Encode content to base64
  std::string json_data = "{\"content\":\"" + base64_content +
                          "\",\"encoding\":\"base64\"}"; // Prepare JSON data

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "Content-Type: application/json"); // Add content-type header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // Set the URL
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
                   json_data.c_str());                 // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK) {
    ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                             "CURL Error: " +
                                 std::string(curl_easy_strerror(res)),
                             ErrorSeverity::ERROR, "create_github_blob");
    return "";
  }

  // Parse the response to extract the SHA
  size_t sha_pos = response_data.find("\"sha\":\"");
  if (sha_pos != std::string::npos) // If SHA found
  {
    sha_pos += 7;                                       // Skip to SHA value
    size_t sha_end = response_data.find("\"", sha_pos); // Find end quote
    if (sha_end != std::string::npos)                   // If end quote found
    {
      return response_data.substr(
          sha_pos, sha_end - sha_pos); // Return the extracted SHA
    }
  }

  ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                           "Could not find SHA in response",
                           ErrorSeverity::ERROR, "create_github_blob");
  return "";
}

std::string create_github_tree(const std::string &token,
                               const std::string &username,
                               const std::string &repo_name,
                               const std::string &blob_sha,
                               const std::string &filename) {
  CURL *curl = curl_easy_init();
  if (!curl)
    return "";

  std::string response_data; // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/git/trees"; // Prepare URL

  std::string escaped_filename = filename; // Escape double quotes in filename
  size_t pos = 0;

  while ((pos = escaped_filename.find("\"", pos)) !=
         std::string::npos) // Find double quotes
  {
    escaped_filename.replace(pos, 1, "\\\""); // Replace with escaped version
    pos += 2;                                 // Move past the escaped quote
  }

  // Prepare JSON data
  std::string json_data =
      "{\"base_tree\":null,\"tree\":[{\"path\":\"" + escaped_filename +
      "\",\"mode\":\"100644\",\"type\":\"blob\",\"sha\":\"" + blob_sha +
      "\"}]}";

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "Content-Type: application/json"); // Add content-type header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // Set the URL
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
                   json_data.c_str());                 // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK) {
    ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                             "CURL error: " +
                                 std::string(curl_easy_strerror(res)),
                             ErrorSeverity::ERROR, "create_github_tree");
    return "";
  }

  size_t sha_pos = response_data.find("\"sha\":\""); // Find SHA in response
  if (sha_pos != std::string::npos)                  // If SHA found
  {
    sha_pos += 7;                                       // Skip to SHA value
    size_t sha_end = response_data.find("\"", sha_pos); // Find end quote
    if (sha_end != std::string::npos)                   // If end quote found
    {
      return response_data.substr(
          sha_pos, sha_end - sha_pos); // Return the extracted SHA
    }
  }

  return "";
}

std::string get_github_commit_tree(const std::string &token,
                                   const std::string &username,
                                   const std::string &repo_name,
                                   const std::string &commit_sha) {
  CURL *curl = curl_easy_init();
  if (!curl)
    return "";

  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/git/commits/" + commit_sha; // Prepare URL
  std::string response_data; // To store response data

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());    // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK) {
    ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                             "CURL error: " +
                                 std::string(curl_easy_strerror(res)),
                             ErrorSeverity::ERROR, "get_github_commit_tree");
    return "";
  }

  size_t tree_pos =
      response_data.find("\"tree\":{\"sha\":\""); // Find tree SHA in response
  if (tree_pos != std::string::npos)              // If tree SHA found
  {
    tree_pos += 15; // Skip "tree":{"sha":"
    size_t tree_end = response_data.find("\"", tree_pos); // Find end quote
    if (tree_end != std::string::npos)                    // If end quote found
    {
      return response_data.substr(
          tree_pos, tree_end - tree_pos); // Return the extracted tree SHA
    }
  }

  return "";
}

std::string create_github_tree_with_files(
    const std::string &token, const std::string &username,
    const std::string &repo_name, const std::vector<std::string> &blob_shas,
    const std::vector<std::string> &file_names,
    const std::string &base_tree_sha) {
  CURL *curl = curl_easy_init();
  if (!curl)
    return "";

  std::string response_data; // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/git/trees"; // Prepare URL

  std::string json_data = "{\"base_tree\":"; // Prepare JSON data
  if (base_tree_sha.empty()) {
    json_data += "null"; // No base tree
  } else {
    json_data += "\"" + base_tree_sha + "\""; // Add base tree SHA
  }
  json_data += ",\"tree\":["; // Start tree array

  for (size_t i = 0; i < blob_shas.size(); ++i) // Add each file entry
  {
    if (i > 0) {
      json_data += ","; // Add comma between entries
    }

    std::string escaped_filename =
        file_names[i]; // Escape double quotes in filename
    size_t pos = 0;
    while ((pos = escaped_filename.find("\"", pos)) !=
           std::string::npos) // Find double quotes
    {
      escaped_filename.replace(pos, 1, "\\\""); // Replace with escaped version
      pos += 2;                                 // Move past the escaped quote
    }

    // Add file entry to JSON
    json_data += "{\"path\":\"" + escaped_filename +
                 "\",\"mode\":\"100644\",\"type\":\"blob\",\"sha\":\"" +
                 blob_shas[i] + "\"}";
  }

  json_data += "]}"; // End tree array and JSON

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "Content-Type: application/json"); // Add content-type header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // Set the URL
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
                   json_data.c_str());                 // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK) {
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "CURL error: " + std::string(curl_easy_strerror(res)),
        ErrorSeverity::ERROR, "create_github_tree_with_files");
    return "";
  }

  size_t sha_pos = response_data.find("\"sha\":\""); // Find SHA in response
  if (sha_pos != std::string::npos)                  // If SHA found
  {
    sha_pos += 7;                                       // Skip "sha":"
    size_t sha_end = response_data.find("\"", sha_pos); // Find end quote
    if (sha_end != std::string::npos)                   // If end quote found
    {
      return response_data.substr(
          sha_pos, sha_end - sha_pos); // Return the extracted SHA
    }
  }

  return "";
}

std::string
create_github_commit(const std::string &token, const std::string &username,
                     const std::string &repo_name, const std::string &tree_sha,
                     const std::string &parent_sha, const std::string &message,
                     const std::string &author_name,
                     const std::string &author_email,
                     const std::string &timestamp) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return "";
  }

  std::string response_data; // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/git/commits"; // Prepare URL

  std::string escaped_message =
      message; // Escape double quotes and newlines in message
  size_t pos = 0;
  while ((pos = escaped_message.find("\"", pos)) !=
         std::string::npos) // Find double quotes
  {
    escaped_message.replace(pos, 1, "\\\""); // Replace with escaped version
    pos += 2;
  }
  pos = 0;
  while ((pos = escaped_message.find("\n", pos)) !=
         std::string::npos) // Find newlines
  {
    escaped_message.replace(pos, 1, "\\n"); // Replace with escaped version
    pos += 2;
  }

  auto now = std::chrono::system_clock::now();             // Get current time
  auto time_t = std::chrono::system_clock::to_time_t(now); // Convert to time_t
  std::stringstream ss; // Format time to GitHub timestamp
  ss << std::put_time(std::gmtime(&time_t),
                      "%Y-%m-%dT%H:%M:%SZ"); // GitHub timestamp format
  std::string github_timestamp = ss.str();   // Final timestamp string

  std::string json_data = "{\"message\":\"" + escaped_message +
                          "\",\"tree\":\"" + tree_sha +
                          "\""; // Prepare JSON data
  if (!parent_sha.empty()) {
    json_data +=
        ",\"parents\":[\"" + parent_sha + "\"]"; // Add parent SHA if exists
  }

  json_data += ",\"author\":{\"name\":\"" + author_name + "\",\"email\":\"" +
               author_email + "\",\"date\":\"" + github_timestamp +
               "\"}"; // Add author info
  json_data += ",\"committer\":{\"name\":\"" + author_name + "\",\"email\":\"" +
               author_email + "\",\"date\":\"" + github_timestamp +
               "\"}"; // Add committer info
  json_data += "}";   // End JSON

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "Content-Type: application/json"); // Add content-type header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // Set the URL
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
                   json_data.c_str());                 // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK) {
    ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                             "CURL error: " +
                                 std::string(curl_easy_strerror(res)),
                             ErrorSeverity::ERROR, "create_github_commit");
    return "";
  }

  size_t sha_pos = response_data.find("\"sha\":\""); // Find SHA in response
  if (sha_pos != std::string::npos)                  // If SHA found
  {
    sha_pos += 7;                                       // Skip "sha":"
    size_t sha_end = response_data.find("\"", sha_pos); // Find end quote
    if (sha_end != std::string::npos)                   // If end quote found
    {
      return response_data.substr(
          sha_pos, sha_end - sha_pos); // Return the extracted SHA
    }
  }

  return "";
}

std::string get_github_last_commit_sha(const std::string &token,
                                       const std::string &username,
                                       const std::string &repo_name,
                                       const std::string &ref) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return "";
  }

  std::string response_data; // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/git/refs/" + ref; // Prepare URL

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());    // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK) {
    return "";
  }

  if (response_data.find("Bad credentials") != std::string::npos) {
    ErrorHandler::printError(
        ErrorCode::CONFIG_ERROR,
        "Error: Invalid GitHub token. Please check your token and try again.",
        ErrorSeverity::ERROR, "get_github_last_commit_sha");
    return "";
  }

  if (response_data.empty()) {
    return "";
  }

  size_t sha_pos = response_data.find("\"sha\":\""); // Find SHA in response
  if (sha_pos != std::string::npos)                  // If SHA found
  {
    sha_pos += 7;                                       // Skip "sha":"
    size_t sha_end = response_data.find("\"", sha_pos); // Find end quote
    if (sha_end != std::string::npos)                   // If end quote found
    {
      return response_data.substr(
          sha_pos, sha_end - sha_pos); // Return the extracted SHA
    }
  }

  return "";
}

bool update_github_ref(const std::string &token, const std::string &username,
                       const std::string &repo_name, const std::string &ref,
                       const std::string &sha) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return false;
  }

  std::string response_data; // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/git/refs/" + ref;    // Prepare URL
  std::string json_data = "{\"sha\":\"" + sha + "\"}"; // Prepare JSON data

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "Content-Type: application/json"); // Add content-type header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // Set the URL
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,
                   "PATCH"); // Set HTTP method to PATCH
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
                   json_data.c_str());                 // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  return res == CURLE_OK;
}

bool create_github_ref(const std::string &token, const std::string &username,
                       const std::string &repo_name, const std::string &ref,
                       const std::string &sha) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return false;
  }

  std::string response_data; // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/git/refs"; // Prepare URL
  std::string json_data = "{\"ref\":\"refs/" + ref + "\",\"sha\":\"" + sha +
                          "\"}"; // Prepare JSON data

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "Content-Type: application/json"); // Add content-type header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // Set the URL
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
                   json_data.c_str());                 // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK) {
    ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                             "CURL error: " +
                                 std::string(curl_easy_strerror(res)),
                             ErrorSeverity::ERROR, "create_github_ref");
    return false;
  }

  if (response_data.find("\"ref\":\"refs/" + ref + "\"") != std::string::npos) {
    return true;
  }

  return false;
}

bool pull_from_github_api(const std::string &token, const std::string &username,
                          const std::string &repo_name,
                          const std::string &branch_name) {
  try {
    std::string target_branch = branch_name;
    if (target_branch.empty()) {
      target_branch = getCurrentBranch();
    }

    // Get the latest commit SHA from GitHub
    std::string latest_commit_sha = get_github_last_commit_sha(
        token, username, repo_name, "heads/" + target_branch);
    if (latest_commit_sha.empty()) {
      // Fallback: try without "heads/" prefix if it fails, or maybe the branch
      // is just the ref But get_github_last_commit_sha likely expects a ref.
      // Let's assume standard behavior for now.
      return false;
    }

    // Check if we already have this commit
    std::string commit_data =
        get_github_commit_data(token, username, repo_name, latest_commit_sha);
    if (commit_data.empty()) {
      ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                               "Could not get commit data from GitHub",
                               ErrorSeverity::ERROR, "pull_from_github_api");
      return false;
    }

    // Extract files from the commit
    std::vector<std::string> downloaded_files;
    if (extract_files_from_github_commit(token, username, repo_name,
                                         latest_commit_sha, commit_data,
                                         downloaded_files)) {
      // Integrate downloaded files into BitTrack
      integrate_pulled_files_with_bittrack(latest_commit_sha, downloaded_files);
      std::cout << "✓ Pulled " << downloaded_files.size()
                << " files from GitHub" << std::endl;
      return true;
    } else {
      ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                               "Failed to extract files from GitHub commit",
                               ErrorSeverity::ERROR, "pull_from_github_api");
      return false;
    }
  } catch (const std::exception &e) {
    ErrorHandler::printError(ErrorCode::UNEXPECTED_EXCEPTION,
                             "Error: " + std::string(e.what()),
                             ErrorSeverity::ERROR, "pull_from_github_api");
    return false;
  }
}

std::string get_github_commit_data(const std::string &token,
                                   const std::string &username,
                                   const std::string &repo_name,
                                   const std::string &commit_sha) {
  // Get commit data from GitHub API
  CURL *curl = curl_easy_init();
  if (!curl) {
    return "";
  }

  std::string response_data; // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/git/commits/" + commit_sha; // Prepare URL

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());    // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK) {
    return "";
  }

  return response_data;
}

bool extract_files_from_github_commit(
    const std::string &token, const std::string &username,
    const std::string &repo_name, const std::string &commit_sha,
    const std::string &commit_data,
    std::vector<std::string> &downloaded_files) {
  // Extract tree SHA from commit data
  size_t tree_pos = commit_data.find("\"tree\":{\"sha\":\"");
  if (tree_pos == std::string::npos) {
    return false;
  }

  tree_pos += 15;                                     // Skip "tree":{"sha":"
  size_t tree_end = commit_data.find("\"", tree_pos); // Find end quote
  if (tree_end == std::string::npos) {
    return false;
  }

  std::string tree_sha =
      commit_data.substr(tree_pos, tree_end - tree_pos); // Extracted tree SHA
  std::string tree_data = get_github_tree_data(
      token, username, repo_name, tree_sha); // Get tree data from GitHub
  if (tree_data.empty()) {
    return false;
  }

  return download_files_from_github_tree(
      token, username, repo_name, tree_data,
      downloaded_files); // Download files from the tree
}

std::string get_github_tree_data(const std::string &token,
                                 const std::string &username,
                                 const std::string &repo_name,
                                 const std::string &tree_sha) {
  // Get tree data from GitHub API
  CURL *curl = curl_easy_init();
  if (!curl) {
    return "";
  }

  std::string response_data; // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/git/trees/" + tree_sha +
                    "?recursive=1"; // Prepare URL

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());    // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK) {
    return "";
  }

  return response_data;
}

bool download_files_from_github_tree(
    const std::string &token, const std::string &username,
    const std::string &repo_name, const std::string &tree_data,
    std::vector<std::string> &downloaded_files) {
  try {
    std::vector<std::pair<std::string, std::string>> files; // path, sha pairs

    size_t tree_start =
        tree_data.find("\"tree\":["); // Find start of tree array
    if (tree_start == std::string::npos) {
      ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                               "Could not find tree array in GitHub response",
                               ErrorSeverity::ERROR,
                               "download_files_from_github_tree");
      return false;
    }

    tree_start += 7;                                   // Skip "tree":[
    size_t tree_end = tree_data.find("]", tree_start); // Find end of tree array
    if (tree_end == std::string::npos) {
      ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                               "Malformed tree array in GitHub response",
                               ErrorSeverity::ERROR,
                               "download_files_from_github_tree");
      return false;
    }

    std::string tree_content = tree_data.substr(
        tree_start, tree_end - tree_start); // Extract tree content
    size_t pos = 0;                         // Position in tree content
    while (pos < tree_content.length()) {
      size_t file_start =
          tree_content.find("{", pos); // Find start of file entry
      if (file_start == std::string::npos) {
        break;
      }

      size_t file_end =
          tree_content.find("}", file_start); // Find end of file entry
      if (file_end == std::string::npos) {
        break;
      }

      std::string file_entry = tree_content.substr(
          file_start, file_end - file_start + 1);       // Extract file entry
      size_t path_pos = file_entry.find("\"path\":\""); // Find path
      size_t sha_pos = file_entry.find("\"sha\":\"");   // Find sha
      size_t type_pos = file_entry.find("\"type\":\""); // Find type

      if (path_pos != std::string::npos && sha_pos != std::string::npos &&
          type_pos != std::string::npos) // If all found
      {
        size_t type_start = type_pos + 8; // Skip "type":"
        size_t type_end =
            file_entry.find("\"", type_start); // Find end quote of type
        if (type_end != std::string::npos) {
          std::string type = file_entry.substr(
              type_start, type_end - type_start); // Extract type
          if (type == "blob") {
            path_pos += 8; // Skip "path":"
            size_t path_end =
                file_entry.find("\"", path_pos); // Find end quote of path
            if (path_end != std::string::npos)   // If end quote found
            {
              std::string path = file_entry.substr(
                  path_pos, path_end - path_pos); // Extract path
              sha_pos += 7;                       // Skip "sha":"
              size_t sha_end =
                  file_entry.find("\"", sha_pos); // Find end quote of sha
              if (sha_end != std::string::npos)   // If end quote found
              {
                std::string sha = file_entry.substr(
                    sha_pos, sha_end - sha_pos); // Extract sha
                files.push_back({path, sha});    // Store path and sha
              }
            }
          }
        }
      }
      pos = file_end + 1;
    }

    for (const auto &file : files) {
      std::string file_path = file.first;
      std::string file_sha = file.second;

      std::string file_content = get_github_blob_content(
          token, username, repo_name, file_sha); // Get blob content
      if (!file_content.empty()) {
        std::filesystem::path file_path_obj(file_path); // Create path object
        if (file_path_obj.has_parent_path()) // Create directories if needed
        {
          std::filesystem::create_directories(file_path_obj.parent_path());
        }

        std::ofstream file_stream(file_path); // Write content to file
        if (file_stream.is_open()) {
          file_stream << file_content;
          file_stream.close();
          downloaded_files.push_back(file_path);
        } else {
          ErrorHandler::printError(
              ErrorCode::FILE_WRITE_ERROR, "Could not create file " + file_path,
              ErrorSeverity::ERROR, "download_files_from_github_tree");
        }
      } else {
        ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                                 "Could not download content for " + file_path,
                                 ErrorSeverity::ERROR,
                                 "download_files_from_github_tree");
      }
    }
    return true;
  } catch (const std::exception &e) {
    ErrorHandler::printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Error downloading files: " + std::string(e.what()),
        ErrorSeverity::ERROR, "download_files_from_github_tree");
    return false;
  }
}

std::string get_github_blob_content(const std::string &token,
                                    const std::string &username,
                                    const std::string &repo_name,
                                    const std::string &blob_sha) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return "";
  }

  std::string response_data; // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/git/blobs/" + blob_sha; // Prepare URL

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());    // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK) {
    return "";
  }

  size_t content_pos =
      response_data.find("\"content\":\""); // Find content in response
  if (content_pos == std::string::npos) {
    return "";
  }

  content_pos += 11; // Skip "content":"
  size_t content_end =
      response_data.find("\"", content_pos); // Find end quote of content
  if (content_end == std::string::npos) {
    return "";
  }

  std::string encoded_content = response_data.substr(
      content_pos, content_end - content_pos); // Extract encoded content

  // Remove JSON escapes (\n)
  size_t pos = 0;
  while ((pos = encoded_content.find("\\n", pos)) != std::string::npos) {
    encoded_content.replace(pos, 2, "");
  }
  pos = 0;
  while ((pos = encoded_content.find("\\r", pos)) != std::string::npos) {
    encoded_content.replace(pos, 2, "");
  }

  return base64_decode(encoded_content);
}

std::string base64_decode(const std::string &encoded) {
  const std::string chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string decoded;
  int val = 0, valb = -8;
  int padding = 0;

  for (char c : encoded) {
    if (c == '=') {
      padding++;
      continue;
    }
    if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
      continue; // Skip whitespace and newlines
    }
    size_t pos = chars.find(c);
    if (pos == std::string::npos) {
      // Invalid character found, return empty string to indicate error
      ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                               "Invalid character in base64 encoded data",
                               ErrorSeverity::ERROR, "base64_decode");
      return "";
    }
    val = (val << 6) + pos;
    valb += 6;
    if (valb >= 0) {
      decoded.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }

  return decoded;
}

void integrate_pulled_files_with_bittrack(
    const std::string &commit_sha,
    const std::vector<std::string> &downloaded_files) {
  try {
    if (!ErrorHandler::safeWriteFile(".bittrack/index",
                                     "")) { // Clear staging area
      ErrorHandler::printError(
          ErrorCode::FILE_WRITE_ERROR, "Could not clear staging area",
          ErrorSeverity::ERROR, "integrate_pulled_files_with_bittrack");
    }

    std::string commit_dir =
        ".bittrack/objects/" + commit_sha; // Create commit directory
    ErrorHandler::safeCreateDirectories(commit_dir);

    std::vector<std::string> pulled_files =
        downloaded_files; // Files to integrate

    // Copy pulled files to commit directory
    for (const std::string &file_path : pulled_files) {
      if (std::filesystem::exists(file_path)) {
        // Copy file to commit directory
        std::string commit_file_path = commit_dir + "/" + file_path;
        ErrorHandler::safeCopyFile(file_path, commit_file_path);
      }
    }

    // Remove files not in the pulled list
    for (const auto &entry : ErrorHandler::safeListDirectoryFiles(commit_dir)) {
      // Get relative path of the file
      std::string file_path = entry.string();
      std::string relative_path =
          std::filesystem::relative(file_path, commit_dir).string();

      // Check if the file was pulled
      bool was_pulled = false;
      for (const auto &pulled_file : pulled_files) {
        // Compare relative paths
        if (relative_path == pulled_file) {
          was_pulled = true;
          break;
        }
      }

      if (!was_pulled) {
        std::filesystem::remove(file_path);
      }
    }

    // Create commit log
    std::string commit_log_content = "GitHub Pull: " + commit_sha + "\n";
    commit_log_content += "Date: " + get_current_timestamp() + "\n";
    commit_log_content += "Files:\n";

    if (!ErrorHandler::safeWriteFile(".bittrack/commits/" + commit_sha,
                                     commit_log_content)) {
      ErrorHandler::printError(
          ErrorCode::FILE_WRITE_ERROR, "Could not create commit log",
          ErrorSeverity::ERROR, "integrate_pulled_files_with_bittrack");
    }

    // Update branch reference
    std::string current_branch = getCurrentBranch();
    if (!current_branch.empty()) {
      if (!ErrorHandler::safeWriteFile(".bittrack/refs/heads/" + current_branch,
                                       commit_sha + "\n")) {
        ErrorHandler::printError(
            ErrorCode::FILE_WRITE_ERROR, "Could not update branch reference",
            ErrorSeverity::ERROR, "integrate_pulled_files_with_bittrack");
      }
    }

    // Update commit history
    if (!ErrorHandler::safeAppendFile(".bittrack/commits/history",
                                      commit_sha + " " + current_branch +
                                          "\n")) {
      ErrorHandler::printError(
          ErrorCode::FILE_WRITE_ERROR, "Could not update commit history",
          ErrorSeverity::ERROR, "integrate_pulled_files_with_bittrack");
    }

    // Update last pushed commit
    set_last_pushed_commit(commit_sha);
  } catch (const std::exception &e) {
    ErrorHandler::printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Error integrating files: " + std::string(e.what()),
        ErrorSeverity::ERROR, "integrate_pulled_files_with_bittrack");
  }
}

bool validate_github_operation_success(const std::string &response_data) {
  if (response_data.find("\"message\":\"") !=
      std::string::npos) // Check for error messages
  {
    // Common error messages
    if (response_data.find("\"message\":\"Not Found\"") != std::string::npos ||
        response_data.find("\"message\":\"Bad credentials\"") !=
            std::string::npos ||
        response_data.find("\"message\":\"Validation Failed\"") !=
            std::string::npos ||
        response_data.find("\"message\":\"Repository not found\"") !=
            std::string::npos) {
      return false;
    }
  }

  // Check for success indicator
  if (response_data.find("\"sha\":\"") != std::string::npos) {
    return true;
  }

  return false;
}

bool delete_github_file(const std::string &token, const std::string &username,
                        const std::string &repo_name,
                        const std::string &filename,
                        const std::string &message) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return false;
  }

  std::string response_data; // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/contents/" + filename; // Prepare URL

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());    // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK) {
    return false;
  }

  size_t sha_pos = response_data.find("\"sha\":\""); // Find SHA in response
  if (sha_pos == std::string::npos)                  // If SHA not found
  {
    if (response_data.find("\"message\":\"Not Found\"") !=
        std::string::npos) // File not found
    {
      std::cout << "    File not found on GitHub: " << filename
                << " (skipping deletion)" << std::endl;
      return true;
    }
    return false;
  }
  sha_pos += 7;                                       // Skip "sha":"
  size_t sha_end = response_data.find("\"", sha_pos); // Find end quote of SHA
  if (sha_end == std::string::npos)                   // If end quote not found
  {
    return false;
  }
  std::string file_sha =
      response_data.substr(sha_pos, sha_end - sha_pos); // Extracted file SHA

  curl = curl_easy_init(); // Initialize CURL for deletion
  if (!curl) {
    return false;
  }

  response_data.clear();

  std::string escaped_message =
      message; // Escape double quotes and newlines in message
  size_t pos = 0;
  while ((pos = escaped_message.find("\"", pos)) !=
         std::string::npos) // Find double quotes
  {
    escaped_message.replace(pos, 1, "\\\""); // Replace with escaped version
    pos += 2;
  }
  pos = 0;
  while ((pos = escaped_message.find("\n", pos)) !=
         std::string::npos) // Find newlines
  {
    escaped_message.replace(pos, 1, "\\n"); // Replace with escaped version
    pos += 2;
  }

  // Prepare JSON data for deletion
  std::string json_data =
      "{\"message\":\"" + escaped_message + "\",\"sha\":\"" + file_sha + "\"}";

  headers = nullptr; // Re-initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "Content-Type: application/json"); // Add content-type header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // Set the URL
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,
                   "DELETE"); // Set HTTP method to DELETE
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
                   json_data.c_str());                 // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);  // Free headers
  curl_easy_cleanup(curl);       // Clean up CURL

  if (res != CURLE_OK) {
    return false;
  }

  return validate_github_operation_success(response_data);
}

bool create_github_file(const std::string &token, const std::string &username,
                        const std::string &repo_name,
                        const std::string &filename, const std::string &content,
                        const std::string &message) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return false;
  }

  std::string current_branch = getCurrentBranch();
  std::string response_data; // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" +
                    repo_name + "/contents/" + filename +
                    "?ref=" + current_branch;          // Prepare URL
  std::string base64_content = base64_encode(content); // Base64 encode content

  std::string escaped_message =
      message; // Escape double quotes and newlines in message
  size_t pos = 0;
  while ((pos = escaped_message.find("\"", pos)) !=
         std::string::npos) // Find double quotes
  {
    escaped_message.replace(pos, 1, "\\\""); // Replace with escaped version
    pos += 2;
  }
  pos = 0;
  while ((pos = escaped_message.find("\n", pos)) !=
         std::string::npos) // Find newlines
  {
    escaped_message.replace(pos, 1, "\\n");
    pos += 2;
  }

  // Prepare JSON data for file creation
  std::string json_data = "{\"message\":\"" + escaped_message +
                          "\",\"content\":\"" + base64_content +
                          "\",\"branch\":\"" + current_branch + "\"}";

  struct curl_slist *headers = nullptr; // Initialize headers
  headers = curl_slist_append(
      headers,
      ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(
      headers, "Content-Type: application/json"); // Add content-type header
  headers = curl_slist_append(
      headers, "User-Agent: BitTrack/1.0"); // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // Set the URL
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
                   json_data.c_str());                 // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_callback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &response_data); // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK) {
    ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                             "CURL Error: " +
                                 std::string(curl_easy_strerror(res)),
                             ErrorSeverity::ERROR, "create_github_file");
    return false;
  }

  bool success = validate_github_operation_success(response_data);
  if (!success) {
    ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED,
                             "Failed to create file " + filename +
                                 " - GitHub API error",
                             ErrorSeverity::ERROR, "create_github_file");
  }
  return success;
}

bool is_local_behind_remote() {
  std::string remote_url = get_remote_origin();
  if (remote_url.empty()) {
    ErrorHandler::printError(
        ErrorCode::INVALID_REMOTE_URL,
        "No remote origin set. Use '--remote -s <url>' first",
        ErrorSeverity::ERROR, "is_local_behind_remote");
    return false;
  }

  if (is_github_remote(remote_url)) {
    // Get GitHub token from config
    std::string token = configGet("github.token", ConfigScope::REPOSITORY);
    if (token.empty()) {
      token = configGet("github.token", ConfigScope::GLOBAL);
    }
    if (token.empty()) {
      ErrorHandler::printError(ErrorCode::CONFIG_ERROR,
                               "GitHub token not configured. Use 'bittrack "
                               "--config github.token <token>'",
                               ErrorSeverity::ERROR, "is_local_behind_remote");
      return false;
    }

    // Extract username and repo name from URL
    std::string username, repo_name;
    if (extract_github_info_from_url(remote_url, username, repo_name).empty()) {
      ErrorHandler::printError(ErrorCode::INVALID_REMOTE_URL,
                               "Could not parse GitHub repository URL",
                               ErrorSeverity::ERROR, "is_local_behind_remote");
      return false;
    }

    std::string current_branch = getCurrentBranch();

    // Get remote last commit SHA
    std::string remote_commit = get_github_last_commit_sha(
        token, username, repo_name, "heads/" + current_branch);
    if (remote_commit.empty()) {
      return false;
    }

    // Get last pushed commit
    std::string last_pushed = get_last_pushed_commit();
    if (last_pushed.empty()) {
      return false;
    }

    if (last_pushed == remote_commit) {
      return false;
    }

    if (has_unpushed_commits()) {
      return false;
    } else {
      std::cout << "Local repository is behind remote. Last pushed: "
                << last_pushed.substr(0, 8)
                << ", Remote: " << remote_commit.substr(0, 8) << std::endl;
      return true;
    }
  }

  return false;
}

std::string get_commit_author(const std::string &commit_hash) {
  // Read commit file to get author name
  std::string commit_file = ".bittrack/commits/" + commit_hash;
  if (!std::filesystem::exists(commit_file)) {
    return "BitTrack User";
  }

  // Parse commit file
  std::ifstream file(commit_file);
  std::string line;
  while (std::getline(file, line)) {
    if (line.find("Author: ") == 0) {
      return line.substr(8);
    }
  }
  return "BitTrack User";
}

std::string get_commit_author_email(const std::string &commit_hash) {
  std::string email = configGet("user.email");
  if (!email.empty()) {
    return email;
  }
  return "";
}

std::string get_commit_timestamp(const std::string &commit_hash) {
  // Read commit file to get timestamp
  std::string commit_file = ".bittrack/commits/" + commit_hash;
  if (!std::filesystem::exists(commit_file)) {
    return "";
  }

  // Parse commit file
  std::ifstream file(commit_file);
  std::string line;
  while (std::getline(file, line)) {
    if (line.find("Timestamp: ") == 0) {
      return line.substr(11);
    }
  }
  return "";
}

bool has_unpushed_commits() {
  std::string current_commit = get_current_commit();
  std::string last_pushed = get_last_pushed_commit();

  if (current_commit.empty()) {
    return false;
  }

  if (last_pushed.empty()) {
    return true;
  }

  return current_commit != last_pushed;
}
