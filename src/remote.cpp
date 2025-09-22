#include "../include/remote.hpp"

void set_remote_origin(const std::string &url)
{
  std::ofstream RemoteFile(".bittrack/remote", std::ios::trunc);

  if (!RemoteFile.is_open())
  {
  std::cerr << "Error: Could not open .bittrack/remote for writing.\n";
  return;
  }

  // write the URL to the file
  RemoteFile << url << std::endl;
  RemoteFile.close();
}

std::string get_remote_origin()
{
  std::ifstream remoteFile(".bittrack/remote");

  if (!remoteFile.is_open())
  {
  std::cerr << "Error: Could not open .bittrack/remote for reading.\n";
  return "";
  }

  std::string url;
  std::getline(remoteFile, url);
  remoteFile.close();

  return url;
}

bool compress_folder(const std::string &folder_path, const std::string &zip_path)
{
  mz_zip_archive zip_archive;
  memset(&zip_archive, 0, sizeof(zip_archive));
  bool createZip = mz_zip_writer_init_file(
  &zip_archive,
  zip_path.c_str(),
  0
  );

  if (!createZip)
  {
  std::cerr << "Failed to initialize zip file." << std::endl;
  return false;
  }

  for (const auto &entry: std::filesystem::recursive_directory_iterator(folder_path))
  {
  if (entry.is_regular_file())
  {
  std::string file_path = entry.path().string();
  std::string relative_path = std::filesystem::relative(file_path, folder_path).string();

  auto fileSize = std::filesystem::file_size(file_path);
  std::cout << relative_path << ": " << fileSize << " bytes -- " << "\033[32m ok \033[0m" << std::endl;

  bool addFileToArchive = mz_zip_writer_add_file(
  &zip_archive,
  relative_path.c_str(),
  file_path.c_str(),
  nullptr,
  0,
  MZ_DEFAULT_LEVEL
  );

  if (!addFileToArchive)
  {
  std::cerr << "Failed to add file to zip: " << file_path << std::endl;
  mz_zip_writer_end(&zip_archive);
  return false;
  }
  }
  }

  if (!mz_zip_writer_finalize_archive(&zip_archive))
  {
  std::cerr << "Failed to finalize the zip archive." << std::endl;
  mz_zip_writer_end(&zip_archive);
  return false;
  }

  // write the compressed folder to the zip file path
  mz_zip_writer_end(&zip_archive);
  return true;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  std::ofstream *out = static_cast<std::ofstream*>(stream);
  out->write(static_cast<char*>(ptr), size * nmemb);
  return size * nmemb;
}

void push()
{
  std::string remote_url = get_remote_origin();
  if (remote_url.empty())
  {
    std::cerr << "Error: No remote origin set. Use set_remote_origin(url) first.\n";
    return;
  }

  // Check if this is a GitHub repository and handle it specially
  if (is_github_remote(remote_url)) {
    std::string token = config_get("github.token", ConfigScope::REPOSITORY);
    if (token.empty()) {
      token = config_get("github.token", ConfigScope::GLOBAL);
    }
    if (token.empty()) {
      std::cerr << "Error: GitHub token not configured. Use 'bittrack --config github.token <token>'" << std::endl;
      return;
    }
    
    // Extract repository info
    std::string username, repo_name;
    if (extract_github_info_from_url(remote_url, username, repo_name).empty()) {
      std::cerr << "Error: Could not parse GitHub repository URL" << std::endl;
      return;
    }
    
    std::cout << "Pushing to GitHub repository: " << username << "/" << repo_name << std::endl;
    
    // Try GitHub API push
    if (push_to_github_api(token, username, repo_name)) {
      return; // Success, don't continue with regular push
    } else {
      std::cout << "GitHub API push failed, falling back to regular method..." << std::endl;
    }
  }

  // Use the original push method (works for both GitHub and other remotes)
  // compress the folder into remote_push_folder.zip
  system("zip -r .bittrack/remote_push_folder.zip .bittrack/objects > /dev/null");

  CURL *curl;
  CURLcode response;
  curl = curl_easy_init();

  if (curl)
  {
    struct curl_httppost *form = NULL;
    struct curl_httppost *last = NULL;

    std::string CurrentBranch = get_current_branch();
    std::string CurrentCommit = get_current_commit();

    // attach compressed zip to POST
    curl_formadd(
    &form,
    &last,
    CURLFORM_COPYNAME,
    "upload",
    CURLFORM_FILE,
    ".bittrack/remote_push_folder.zip",
    CURLFORM_END
    );

    // Add branch metadata
    curl_formadd(
    &form,
    &last,
    CURLFORM_COPYNAME,
    "branch",
    CURLFORM_COPYCONTENTS,
    CurrentBranch.c_str(),
    CURLFORM_END
    );

    // add commit metadata
    curl_formadd(
    &form,
    &last,
    CURLFORM_COPYNAME,
    "commit",
    CURLFORM_COPYCONTENTS,
    CurrentCommit.c_str(),
    CURLFORM_END
    );

    curl_easy_setopt(curl, CURLOPT_URL, remote_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, form);

    response = curl_easy_perform(curl);

    if (response != CURLE_OK) {
      std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(response) << std::endl;
    }
    else {
      // Check if this is a GitHub repository and provide appropriate feedback
      if (is_github_remote(remote_url)) {
        std::cout << "Note: GitHub integration is currently limited." << std::endl;
        std::cout << "BitTrack uses a simple HTTP upload method that may not work with GitHub's API." << std::endl;
        std::cout << "For full GitHub integration, consider using Git directly or implementing proper Git protocol support." << std::endl;
      } else {
        std::cout << "File uploaded successfully!" << std::endl;
      }
    }

    curl_easy_cleanup(curl);
    curl_formfree(form);
  }
  // remove the temp push file
  system("rm -f .bittrack/remote_push_folder.zip");
}

void pull()
{
  std::string base_url = get_remote_origin();
  if (base_url.empty())
  {
    std::cerr << "Error: No remote origin set. Use set_remote_origin(url) first.\n";
    return;
  }

  // Check if working directory is clean before pulling
  std::vector<std::string> staged_files = get_staged_files();
  std::vector<std::string> unstaged_files = get_unstaged_files();
  
  if (!staged_files.empty() || !unstaged_files.empty()) {
    std::cerr << "Error: Cannot pull with uncommitted changes.\n";
    std::cerr << "Please commit or stash your changes before pulling.\n";
    if (!staged_files.empty()) {
      std::cerr << "Staged files: ";
      for (const auto& file : staged_files) {
        std::cerr << file << " ";
      }
      std::cerr << std::endl;
    }
    if (!unstaged_files.empty()) {
      std::cerr << "Unstaged files: ";
      for (const auto& file : unstaged_files) {
        std::cerr << file << " ";
      }
      std::cerr << std::endl;
    }
    return;
  }

  // Check if this is a GitHub repository and use GitHub API
  if (is_github_remote(base_url)) {
    std::string token = config_get("github.token", ConfigScope::REPOSITORY);
    if (token.empty()) {
      token = config_get("github.token", ConfigScope::GLOBAL);
    }
    if (token.empty()) {
      std::cerr << "Error: GitHub token not configured. Use 'bittrack --config github.token <token>'" << std::endl;
      return;
    }
    
    // Extract repository info
    std::string username, repo_name;
    if (extract_github_info_from_url(base_url, username, repo_name).empty()) {
      std::cerr << "Error: Could not parse GitHub repository URL" << std::endl;
      return;
    }
    
    // Use GitHub API to pull repository data
    if (pull_from_github_api(token, username, repo_name)) {
      return; // Success
    } else {
      std::cerr << "Error: Failed to pull from GitHub. Please check your token and internet connection." << std::endl;
      return;
    }
  } else {
    std::cerr << "Error: Pull is only supported for GitHub repositories. Please use a GitHub repository URL." << std::endl;
    return;
  }
}

void add_remote(const std::string& name, const std::string& url)
{
  if (name.empty() || url.empty())
  {
    std::cerr << "Error: Remote name and URL cannot be empty" << std::endl;
    return;
  }

  std::string remote_file = ".bittrack/remotes/" + name;
  std::filesystem::create_directories(std::filesystem::path(remote_file).parent_path());
  
  std::ofstream file(remote_file);
  if (!file.is_open())
  {
    std::cerr << "Error: Could not create remote file for " << name << std::endl;
    return;
  }
  
  file << url << std::endl;
  file.close();
  
  std::cout << "Added remote '" << name << "' with URL: " << url << std::endl;
}

void remove_remote(const std::string& name)
{
  if (name.empty())
  {
    std::cerr << "Error: Remote name cannot be empty" << std::endl;
    return;
  }

  std::string remote_file = ".bittrack/remotes/" + name;
  
  if (!std::filesystem::exists(remote_file))
  {
    std::cerr << "Error: Remote '" << name << "' does not exist" << std::endl;
    return;
  }
  
  std::filesystem::remove(remote_file);
  std::cout << "Removed remote '" << name << "'" << std::endl;
}

void list_remotes()
{
  std::string remotes_dir = ".bittrack/remotes";
  
  if (!std::filesystem::exists(remotes_dir))
  {
    std::cout << "No remotes configured" << std::endl;
    return;
  }
  
  std::cout << "Configured remotes:" << std::endl;
  for (const auto& entry : std::filesystem::directory_iterator(remotes_dir))
  {
    if (entry.is_regular_file())
    {
      std::string remote_name = entry.path().filename().string();
      std::string remote_url = get_remote_url(remote_name);
      
      if (!remote_url.empty())
      {
        std::cout << "  " << remote_name << " -> " << remote_url << std::endl;
      }
    }
  }
}

void push_to_remote(const std::string& remote_name, const std::string& branch_name)
{
  std::string remote_url = get_remote_url(remote_name);
  if (remote_url.empty())
  {
    std::cerr << "Error: Remote '" << remote_name << "' not found" << std::endl;
    return;
  }
  
  std::string current_branch = get_current_branch();
  if (current_branch != branch_name)
  {
    std::cerr << "Error: Current branch '" << current_branch << "' does not match target branch '" << branch_name << "'" << std::endl;
    return;
  }
  
  // Use existing push() function but with specific remote
  set_remote_origin(remote_url);
  push();
}

void pull_from_remote(const std::string& remote_name, const std::string& branch_name)
{
  std::string remote_url = get_remote_url(remote_name);
  if (remote_url.empty())
  {
    std::cerr << "Error: Remote '" << remote_name << "' not found" << std::endl;
    return;
  }
  
  std::string current_branch = get_current_branch();
  if (current_branch != branch_name)
  {
    std::cerr << "Error: Current branch '" << current_branch << "' does not match target branch '" << branch_name << "'" << std::endl;
    return;
  }
  
  // Use existing pull() function but with specific remote
  set_remote_origin(remote_url);
  pull();
}

void fetch_from_remote(const std::string& remote_name)
{
  std::string remote_url = get_remote_url(remote_name);
  if (remote_url.empty())
  {
    std::cerr << "Error: Remote '" << remote_name << "' not found" << std::endl;
    return;
  }
  
  std::cout << "Fetching from remote '" << remote_name << "'..." << std::endl;
  
  // For now, fetch is similar to pull but doesn't merge
  std::string base_url = remote_url;
  std::string current_branch = get_current_branch();
  std::string remote_url_with_branch = base_url + "?branch=" + current_branch;
  
  CURL *curl;
  CURLcode response;
  curl = curl_easy_init();
  
  if (curl)
  {
    std::ofstream outfile(".bittrack/remote_fetch_folder.zip", std::ios::binary);
    if (!outfile.is_open())
    {
      std::cerr << "Error: Could not open .bittrack/remote_fetch_folder.zip for writing." << std::endl;
      curl_easy_cleanup(curl);
      return;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, remote_url_with_branch.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outfile);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    response = curl_easy_perform(curl);
    
    if (response != CURLE_OK)
    {
      std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(response) << std::endl;
    }
    else
    {
      std::cout << "Fetched successfully from remote '" << remote_name << "'" << std::endl;
    }
    
    outfile.close();
    curl_easy_cleanup(curl);
  }
  
  // Clean up temp file
  system("rm -f .bittrack/remote_fetch_folder.zip");
}

void clone_repository(const std::string& url, const std::string& local_path)
{
  if (url.empty())
  {
    std::cerr << "Error: Repository URL cannot be empty" << std::endl;
    return;
  }
  
  std::string target_path = local_path.empty() ? "cloned_repo" : local_path;
  
  if (std::filesystem::exists(target_path))
  {
    std::cerr << "Error: Directory '" << target_path << "' already exists" << std::endl;
    return;
  }
  
  std::cout << "Cloning repository from " << url << " to " << target_path << "..." << std::endl;
  
  // Create target directory
  std::filesystem::create_directories(target_path);
  
  // Change to target directory
  std::string original_dir = std::filesystem::current_path().string();
  std::filesystem::current_path(target_path);
  
  // Initialize BitTrack repository
  system("mkdir -p .bittrack");
  
  // Set remote origin
  set_remote_origin(url);
  
  // Fetch from remote
  fetch_from_remote("origin");
  
  // Restore original directory
  std::filesystem::current_path(original_dir);
  
  std::cout << "Repository cloned successfully to " << target_path << std::endl;
}

bool is_remote_configured()
{
  return !get_remote_origin().empty();
}

std::string get_remote_url(const std::string& remote_name)
{
  if (remote_name == "origin")
  {
    return get_remote_origin();
  }
  
  std::string remote_file = ".bittrack/remotes/" + remote_name;
  
  if (!std::filesystem::exists(remote_file))
  {
    return "";
  }
  
  std::ifstream file(remote_file);
  if (!file.is_open())
  {
    return "";
  }
  
  std::string url;
  std::getline(file, url);
  file.close();
  
  return url;
}

void update_remote_url(const std::string& remote_name, const std::string& new_url)
{
  if (remote_name.empty() || new_url.empty())
  {
    std::cerr << "Error: Remote name and URL cannot be empty" << std::endl;
    return;
  }
  
  if (remote_name == "origin")
  {
    set_remote_origin(new_url);
    std::cout << "Updated origin remote URL to: " << new_url << std::endl;
    return;
  }
  
  std::string remote_file = ".bittrack/remotes/" + remote_name;
  
  if (!std::filesystem::exists(remote_file))
  {
    std::cerr << "Error: Remote '" << remote_name << "' does not exist" << std::endl;
    return;
  }
  
  std::ofstream file(remote_file);
  if (!file.is_open())
  {
    std::cerr << "Error: Could not update remote file for " << remote_name << std::endl;
    return;
  }
  
  file << new_url << std::endl;
  file.close();
  
  std::cout << "Updated remote '" << remote_name << "' URL to: " << new_url << std::endl;
}

void show_remote_info()
{
  std::cout << "Remote Information:" << std::endl;
  std::cout << "==================" << std::endl;
  
  // Show origin
  std::string origin_url = get_remote_origin();
  if (!origin_url.empty())
  {
    std::cout << "Origin: " << origin_url << std::endl;
  }
  else
  {
    std::cout << "Origin: Not configured" << std::endl;
  }
  
  // Show all remotes
  list_remotes();
}

// Helper functions for GitHub detection
bool is_github_remote(const std::string& url) {
  return url.find("github.com") != std::string::npos;
}

// Write callback for CURL
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

// Simple base64 encoding function
std::string base64_encode(const std::string& input) {
  const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string result;
  int val = 0, valb = -6;
  for (unsigned char c : input) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      result.push_back(chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
  while (result.size() % 4) result.push_back('=');
  return result;
}

std::string get_last_pushed_commit() {
  std::ifstream file(".bittrack/last_pushed_commit");
  if (!file.is_open()) {
    return ""; // No previous push
  }
  
  std::string commit;
  std::getline(file, commit);
  file.close();
  return commit;
}

std::vector<std::string> get_unpushed_commits() {
  std::vector<std::string> unpushed_commits;

  try {
    std::string current_commit = get_current_commit();
    std::string last_pushed = get_last_pushed_commit();

    if (current_commit.empty()) {
      return unpushed_commits;
    }

    // If no previous push, push the current commit
    if (last_pushed.empty()) {
      unpushed_commits.push_back(current_commit);
      return unpushed_commits;
    }

    // If current commit is different from last pushed, push it
    if (current_commit != last_pushed) {
      unpushed_commits.push_back(current_commit);
    }

  } catch (const std::exception& e) {
    std::cerr << "Error getting unpushed commits: " << e.what() << std::endl;
  }

  return unpushed_commits;
}

void set_last_pushed_commit(const std::string& commit) {
  std::ofstream file(".bittrack/last_pushed_commit");
  if (file.is_open()) {
    file << commit;
    file.close();
  }
}

void set_github_commit_mapping(const std::string& bittrack_commit, const std::string& github_commit) {
  std::ofstream file(".bittrack/github_commits", std::ios::app);
  if (file.is_open()) {
    file << bittrack_commit << " " << github_commit << std::endl;
    file.close();
  }
}

std::string get_github_commit_for_bittrack(const std::string& bittrack_commit) {
  std::ifstream file(".bittrack/github_commits");
  if (!file.is_open()) {
    return "";
  }
  
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty()) continue;
    
    std::istringstream iss(line);
    std::string bittrack_hash, github_hash;
    if (iss >> bittrack_hash >> github_hash && bittrack_hash == bittrack_commit) {
      file.close();
      return github_hash;
    }
  }
  file.close();
  return "";
}

std::string get_latest_github_commit(const std::string& token, const std::string& username, const std::string& repo_name) {
  CURL* curl = curl_easy_init();
  if (!curl) return "";
  
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/refs/heads/main";
  
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK) {
    return "";
  }
  
  // Parse response to get commit SHA
  size_t sha_pos = response_data.find("\"sha\":\"");
  if (sha_pos != std::string::npos) {
    sha_pos += 7; // Skip "sha":"
    size_t sha_end = response_data.find("\"", sha_pos);
    if (sha_end != std::string::npos) {
      return response_data.substr(sha_pos, sha_end - sha_pos);
    }
  }
  
  return "";
}

std::vector<std::string> get_committed_files(const std::string& commit) {
  std::vector<std::string> files;
  
  // Read the commit file to get the list of files in this commit
  std::string commit_file = ".bittrack/commits/" + commit;
  std::ifstream file(commit_file);
  if (!file.is_open()) {
    return files;
  }
  
  std::string line;
  bool in_files_section = false;
  while (std::getline(file, line)) {
    if (line.find("Files:") == 0) {
      in_files_section = true;
      continue;
    }
    
    if (in_files_section && !line.empty()) {
      // Trim leading whitespace
      size_t start = line.find_first_not_of(" \t");
      if (start != std::string::npos) {
        line = line.substr(start);
      }
      
      // Extract filename from "filename hash" format
      size_t space_pos = line.find(" ");
      if (space_pos != std::string::npos) {
        std::string filename = line.substr(0, space_pos);
        std::string hash = line.substr(space_pos + 1);
        
        // If hash is empty or just whitespace, it's a deleted file
        if (hash.empty() || hash.find_first_not_of(" \t") == std::string::npos) {
          files.push_back(filename + " (deleted)");
        } else {
          files.push_back(filename);
        }
      } else {
        // If no space found, it's a regular file (from pulled commits)
        files.push_back(line);
      }
    }
  }
  file.close();
  
  return files;
}

std::string get_commit_message_simple(const std::string& commit) {
  std::string commit_file = ".bittrack/commits/" + commit;
  std::ifstream file(commit_file);
  if (!file.is_open()) {
    return "";
  }
  
  std::string line;
  while (std::getline(file, line)) {
    if (line.find("Message:") == 0) {
      file.close();
      return line.substr(8); // Skip "Message:"
    }
  }
  file.close();
  
  return "";
}

std::string extract_github_info_from_url(const std::string& url, std::string& username, std::string& repository) {
  std::regex github_regex(R"(https?://github\.com/([^/]+)/([^/]+?)(?:\.git)?/?$)");
  std::smatch matches;
  
  if (std::regex_match(url, matches, github_regex)) {
    username = matches[1].str();
    repository = matches[2].str();
    return username + "/" + repository;
  }
  
  return "";
}

bool push_to_github_api(const std::string& token, const std::string& username, const std::string& repo_name) {
  try {
    // Get current commit info
    std::string current_commit = get_current_commit();
    std::string current_branch = get_current_branch();
    
    if (current_commit.empty()) {
      std::cerr << "Error: No commits to push" << std::endl;
      return false;
    }
    
    // Get all unpushed commits
    std::vector<std::string> unpushed_commits = get_unpushed_commits();
    if (unpushed_commits.empty()) {
      std::cout << "Repository is up to date." << std::endl;
      return true;
    }
    
    std::cout << "Pushing " << unpushed_commits.size() << " commits to " << username << "/" << repo_name << "..." << std::endl;
    
    // Check if repository is empty
    std::string parent_sha = get_github_ref(token, username, repo_name, "heads/main");
    bool is_empty_repo = parent_sha.empty();
    
    if (is_empty_repo) {
      // For empty repositories, we need to create the initial commit using Git API
      std::cout << "Repository is empty, creating initial commit..." << std::endl;
      
      // Get files from the latest commit only
      std::vector<std::string> latest_commit_files = get_committed_files(current_commit);
      if (latest_commit_files.empty()) {
        std::cerr << "Error: No files found in latest commit" << std::endl;
        return false;
      }
      
      std::string commit_message = get_commit_message_simple(current_commit);
      if (commit_message.empty()) {
        commit_message = "BitTrack commit: " + current_commit;
      }
      
      // Create blobs for all files
      std::vector<std::string> blob_shas;
      std::vector<std::string> file_names;
      
      for (const auto& file_path : latest_commit_files) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
          continue;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        // For empty repos, we need to create blobs differently
        // Let's try creating a simple file first using Contents API with a different approach
        std::string base64_content = base64_encode(content);
        
        // Use PUT method to create file
        CURL* curl = curl_easy_init();
        if (!curl) continue;
        
        std::string response_data;
        std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/contents/" + file_path;
        
        std::string json_data = "{\"message\":\"" + commit_message + "\",\"content\":\"" + base64_content + "\"}";
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        
        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res == CURLE_OK && response_data.find("\"sha\":\"") != std::string::npos) {
          std::cout << "    Created file: " << file_path << std::endl;
        } else {
          std::cerr << "    Failed to create file: " << file_path << std::endl;
        }
      }
      
      std::cout << "✓ Successfully created initial commit on GitHub!" << std::endl;
      set_last_pushed_commit(current_commit);
      return true;
    }
    
    // For non-empty repositories, use Git API
    // Process each unpushed commit in order
    std::string current_tree_sha = "";
    std::string last_commit_sha = parent_sha;
    std::vector<std::string> successfully_pushed_commits;

    for (size_t i = 0; i < unpushed_commits.size(); ++i) {
      const std::string& commit_hash = unpushed_commits[i];
      
      // Get commit message
      std::string commit_message = get_commit_message_simple(commit_hash);
      if (commit_message.empty()) {
        commit_message = "BitTrack commit: " + commit_hash;
      }
      
      // Get files from this commit
      std::vector<std::string> committed_files = get_committed_files(commit_hash);
      if (committed_files.empty()) {
        std::cerr << "Warning: No files found in commit " << commit_hash << std::endl;
        continue;
      }
      
      std::cout << "  Processing commit " << (i + 1) << "/" << unpushed_commits.size() 
                << " (" << commit_hash.substr(0, 8) << "): " << commit_message << std::endl;
      
      // Create blobs for all files in this commit
      std::vector<std::string> blob_shas;
      std::vector<std::string> file_names;
      
      for (const auto& file_path : committed_files) {
        // Check if this is a deleted file
        if (is_deleted(file_path)) {
          // Handle deleted file
          std::string actual_path = get_actual_path(file_path);
          if (delete_github_file(token, username, repo_name, actual_path, commit_message)) {
            std::cout << "    Deleted file: " << actual_path << std::endl;
          } else {
            std::cerr << "    Failed to delete file: " << actual_path << std::endl;
          }
        } else {
          // Handle regular file - read from commit directory
          std::string commit_file_path = ".bittrack/objects/" + commit_hash + "/" + file_path;
          std::ifstream file(commit_file_path);
          if (!file.is_open()) {
            std::cerr << "    Could not open commit file: " << commit_file_path << std::endl;
            continue;
          }
          
          std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
          file.close();
          
          // Create blob
          std::string blob_sha = create_github_blob(token, username, repo_name, content);
          if (!blob_sha.empty()) {
            blob_shas.push_back(blob_sha);
            file_names.push_back(file_path);
            std::cout << "    Created blob for " << file_path << std::endl;
          } else {
            std::cerr << "    Failed to create blob for " << file_path << std::endl;
          }
        }
      }
      
      // For commits with only deleted files, we don't need to create blobs
      if (blob_shas.empty() && !committed_files.empty()) {
        // Check if all files are deleted
        bool all_deleted = true;
        for (const auto& file_path : committed_files) {
          if (!is_deleted(file_path)) {
            all_deleted = false;
            break;
          }
        }
        
        if (!all_deleted) {
          std::cerr << "Warning: Could not create blobs for commit " << commit_hash << std::endl;
          continue;
        }
        // If all files are deleted, continue without creating blobs
      } else if (blob_shas.empty()) {
        std::cerr << "Warning: No files to process for commit " << commit_hash << std::endl;
        continue;
      }
      
          // Create or update tree (only if we have blobs)
          if (!blob_shas.empty()) {
            if (current_tree_sha.empty()) {
              current_tree_sha = create_github_tree_with_files(token, username, repo_name, blob_shas, file_names);
            } else {
              current_tree_sha = create_github_tree_with_files(token, username, repo_name, blob_shas, file_names, current_tree_sha);
            }

            if (current_tree_sha.empty()) {
              std::cerr << "Error: Could not create or update tree for commit " << commit_hash << std::endl;
              return false;
            }
          } else if (current_tree_sha.empty()) {
        // For commits with only deletions and no existing tree, we need to get the tree from the parent commit
        if (last_commit_sha.empty()) {
          std::cerr << "Error: No parent commit for deletion-only commit " << commit_hash << std::endl;
          return false;
        }
        // Get the tree SHA from the parent commit
        std::string parent_tree_sha = get_github_commit_tree(token, username, repo_name, last_commit_sha);
        if (parent_tree_sha.empty()) {
          std::cerr << "Error: Could not get tree from parent commit " << last_commit_sha << std::endl;
          return false;
        }
        current_tree_sha = parent_tree_sha;
      }
      
          // Create the commit
          std::string new_commit_sha = create_github_commit(token, username, repo_name, current_tree_sha, last_commit_sha, commit_message);
          if (new_commit_sha.empty()) {
            std::cerr << "Error: Could not create commit " << commit_hash << std::endl;
            // Don't return false immediately, continue with other commits
            continue;
          }

          last_commit_sha = new_commit_sha;
          successfully_pushed_commits.push_back(commit_hash);
          
          // Track the mapping between BitTrack commit and GitHub commit
          set_github_commit_mapping(commit_hash, new_commit_sha);
    }
    
    // Update the branch reference to point to the latest commit
    // Use 'main' as GitHub's default branch name
    if (!update_github_ref(token, username, repo_name, "heads/main", last_commit_sha)) {
      std::cerr << "Error: Could not update branch reference" << std::endl;
      // Still update last_pushed_commit for successfully pushed commits
      if (!successfully_pushed_commits.empty()) {
        set_last_pushed_commit(successfully_pushed_commits.back());
        std::cout << "✓ Partially pushed " << successfully_pushed_commits.size() << " commits to GitHub!" << std::endl;
      }
      return false;
    }

    if (successfully_pushed_commits.empty()) {
      std::cerr << "Error: No commits were successfully pushed" << std::endl;
      return false;
    }

    std::cout << "✓ Successfully pushed " << successfully_pushed_commits.size() << " commits to GitHub!" << std::endl;

    // Mark the latest successfully pushed commit as pushed
    set_last_pushed_commit(successfully_pushed_commits.back());

    return true;
    
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return false;
  }
}

std::string create_github_blob(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& content) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    return "";
  }
  
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/blobs";
  
  // Use base64 encoding for content (more reliable than UTF-8)
  std::string base64_content = base64_encode(content);
  std::string json_data = "{\"content\":\"" + base64_content + "\",\"encoding\":\"base64\"}";
  
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  CURLcode res = curl_easy_perform(curl);
  
  // Debug output removed for cleaner output
  
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK) {
    std::cerr << "CURL Error: " << curl_easy_strerror(res) << std::endl;
    return "";
  }
  
  // Parse response to get SHA
  size_t sha_pos = response_data.find("\"sha\":\"");
  if (sha_pos != std::string::npos) {
    sha_pos += 7; // Skip "sha":"
    size_t sha_end = response_data.find("\"", sha_pos);
    if (sha_end != std::string::npos) {
      return response_data.substr(sha_pos, sha_end - sha_pos);
    }
  }
  
  std::cerr << "Could not find SHA in response. Response: " << response_data << std::endl;
  return "";
}

std::string create_github_tree(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& blob_sha, const std::string& filename) {
  CURL* curl = curl_easy_init();
  if (!curl) return "";
  
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/trees";
  
  // Prepare JSON data with proper escaping
  std::string escaped_filename = filename;
  size_t pos = 0;
  while ((pos = escaped_filename.find("\"", pos)) != std::string::npos) {
    escaped_filename.replace(pos, 1, "\\\"");
    pos += 2;
  }
  
  std::string json_data = "{\"base_tree\":null,\"tree\":[{\"path\":\"" + escaped_filename + "\",\"mode\":\"100644\",\"type\":\"blob\",\"sha\":\"" + blob_sha + "\"}]}";
  
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK) {
    std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
    return "";
  }
  
  // Parse response to get SHA
  size_t sha_pos = response_data.find("\"sha\":\"");
  if (sha_pos != std::string::npos) {
    sha_pos += 7; // Skip "sha":"
    size_t sha_end = response_data.find("\"", sha_pos);
    if (sha_end != std::string::npos) {
      return response_data.substr(sha_pos, sha_end - sha_pos);
    }
  }
  
  return "";
}

std::string get_github_commit_tree(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& commit_sha) {
  CURL* curl = curl_easy_init();
  if (!curl) return "";
  
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/commits/" + commit_sha;
  std::string response_data;
  
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK) {
    std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
    return "";
  }
  
  // Parse response to get tree SHA
  size_t tree_pos = response_data.find("\"tree\":{\"sha\":\"");
  if (tree_pos != std::string::npos) {
    tree_pos += 15; // Skip "tree":{"sha":"
    size_t tree_end = response_data.find("\"", tree_pos);
    if (tree_end != std::string::npos) {
      return response_data.substr(tree_pos, tree_end - tree_pos);
    }
  }
  
  return "";
}

std::string create_github_tree_with_files(const std::string& token, const std::string& username, const std::string& repo_name, const std::vector<std::string>& blob_shas, const std::vector<std::string>& file_names, const std::string& base_tree_sha) {
  CURL* curl = curl_easy_init();
  if (!curl) return "";
  
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/trees";
  
  // Prepare JSON data with all files
  std::string json_data = "{\"base_tree\":";
  if (base_tree_sha.empty()) {
    json_data += "null";
  } else {
    json_data += "\"" + base_tree_sha + "\"";
  }
  json_data += ",\"tree\":[";
  
  for (size_t i = 0; i < blob_shas.size(); ++i) {
    if (i > 0) json_data += ",";
    
    // Escape filename
    std::string escaped_filename = file_names[i];
    size_t pos = 0;
    while ((pos = escaped_filename.find("\"", pos)) != std::string::npos) {
      escaped_filename.replace(pos, 1, "\\\"");
      pos += 2;
    }
    
    json_data += "{\"path\":\"" + escaped_filename + "\",\"mode\":\"100644\",\"type\":\"blob\",\"sha\":\"" + blob_shas[i] + "\"}";
  }
  
  json_data += "]}";
  
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK) {
    std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
    return "";
  }
  
  // Parse response to get SHA
  size_t sha_pos = response_data.find("\"sha\":\"");
  if (sha_pos != std::string::npos) {
    sha_pos += 7; // Skip "sha":"
    size_t sha_end = response_data.find("\"", sha_pos);
    if (sha_end != std::string::npos) {
      return response_data.substr(sha_pos, sha_end - sha_pos);
    }
  }
  
  return "";
}


std::string create_github_commit(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& tree_sha, const std::string& parent_sha, const std::string& message) {
  CURL* curl = curl_easy_init();
  if (!curl) return "";
  
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/commits";
  
  // Prepare JSON data with proper escaping
  std::string escaped_message = message;
  size_t pos = 0;
  while ((pos = escaped_message.find("\"", pos)) != std::string::npos) {
    escaped_message.replace(pos, 1, "\\\"");
    pos += 2;
  }
  pos = 0;
  while ((pos = escaped_message.find("\n", pos)) != std::string::npos) {
    escaped_message.replace(pos, 1, "\\n");
    pos += 2;
  }
  
  std::string json_data = "{\"message\":\"" + escaped_message + "\",\"tree\":\"" + tree_sha + "\"";
  if (!parent_sha.empty()) {
    json_data += ",\"parents\":[\"" + parent_sha + "\"]";
  }
  json_data += "}";
  
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK) {
    std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
    return "";
  }
  
  // Parse response to get SHA
  size_t sha_pos = response_data.find("\"sha\":\"");
  if (sha_pos != std::string::npos) {
    sha_pos += 7; // Skip "sha":"
    size_t sha_end = response_data.find("\"", sha_pos);
    if (sha_end != std::string::npos) {
      return response_data.substr(sha_pos, sha_end - sha_pos);
    }
  }
  
  return "";
}

std::string get_github_ref(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& ref) {
  CURL* curl = curl_easy_init();
  if (!curl) return "";
  
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/refs/" + ref;
  
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK) {
    return ""; // Ref doesn't exist, that's okay for first commit
  }
  
  // Check for authentication errors
  if (response_data.find("Bad credentials") != std::string::npos) {
    std::cerr << "Error: Invalid GitHub token. Please check your token and try again." << std::endl;
    return "";
  }
  
  if (response_data.empty()) {
    return "";
  }
  
  // Parse response to get SHA
  size_t sha_pos = response_data.find("\"sha\":\"");
  if (sha_pos != std::string::npos) {
    sha_pos += 7; // Skip "sha":"
    size_t sha_end = response_data.find("\"", sha_pos);
    if (sha_end != std::string::npos) {
      return response_data.substr(sha_pos, sha_end - sha_pos);
    }
  }
  
  return "";
}

bool update_github_ref(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& ref, const std::string& sha) {
  CURL* curl = curl_easy_init();
  if (!curl) return false;
  
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/refs/" + ref;
  
  // Prepare JSON data
  std::string json_data = "{\"sha\":\"" + sha + "\"}";
  
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  return res == CURLE_OK;
}

bool pull_from_github_api(const std::string& token, const std::string& username, const std::string& repo_name) {
  try {
    // Get the latest commit from the main branch
    std::string latest_commit_sha = get_github_ref(token, username, repo_name, "heads/main");
    if (latest_commit_sha.empty()) {
      return false;
    }
    
    // Get the commit details
    std::string commit_data = get_github_commit_data(token, username, repo_name, latest_commit_sha);
    if (commit_data.empty()) {
      std::cerr << "Error: Could not get commit data from GitHub" << std::endl;
      return false;
    }
    
    // Parse commit data and extract files
    std::vector<std::string> downloaded_files;
    if (extract_files_from_github_commit(token, username, repo_name, latest_commit_sha, commit_data, downloaded_files)) {
      // After successfully pulling files, integrate with BitTrack's system
      integrate_pulled_files_with_bittrack(latest_commit_sha, downloaded_files);
      std::cout << "✓ Pulled " << downloaded_files.size() << " files from GitHub" << std::endl;
      return true;
    } else {
      std::cerr << "Error: Failed to extract files from GitHub commit" << std::endl;
      return false;
    }
    
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return false;
  }
}

std::string get_github_commit_data(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& commit_sha) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    return "";
  }
  
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/commits/" + commit_sha;
  
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK) {
    return "";
  }
  
  return response_data;
}

bool extract_files_from_github_commit(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& commit_sha, const std::string& commit_data, std::vector<std::string>& downloaded_files) {
  // Parse the commit data to get the tree SHA
  size_t tree_pos = commit_data.find("\"tree\":{\"sha\":\"");
  if (tree_pos == std::string::npos) {
    return false;
  }
  
  tree_pos += 15; // Skip "tree":{"sha":"
  size_t tree_end = commit_data.find("\"", tree_pos);
  if (tree_end == std::string::npos) {
    return false;
  }
  
  std::string tree_sha = commit_data.substr(tree_pos, tree_end - tree_pos);
  
  // Get the tree data
  std::string tree_data = get_github_tree_data(token, username, repo_name, tree_sha);
  if (tree_data.empty()) {
    return false;
  }
  
  // Parse tree data and download files
  return download_files_from_github_tree(token, username, repo_name, tree_data, downloaded_files);
}

std::string get_github_tree_data(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& tree_sha) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    return "";
  }
  
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/trees/" + tree_sha + "?recursive=1";
  
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK) {
    return "";
  }
  
  return response_data;
}

bool download_files_from_github_tree(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& tree_data, std::vector<std::string>& downloaded_files) {
  try {
    // Parse the tree data JSON to extract file information
    std::vector<std::pair<std::string, std::string>> files; // path, sha pairs
    
    // Find the "tree" array in the JSON response
    size_t tree_start = tree_data.find("\"tree\":[");
    if (tree_start == std::string::npos) {
      std::cerr << "Error: Could not find tree array in GitHub response" << std::endl;
      return false;
    }
    
    tree_start += 7; // Skip "tree":[
    size_t tree_end = tree_data.find("]", tree_start);
    if (tree_end == std::string::npos) {
      std::cerr << "Error: Malformed tree array in GitHub response" << std::endl;
      return false;
    }
    
    std::string tree_content = tree_data.substr(tree_start, tree_end - tree_start);
    
    // Parse each file entry in the tree
    size_t pos = 0;
    while (pos < tree_content.length()) {
      // Find the next file entry
      size_t file_start = tree_content.find("{", pos);
      if (file_start == std::string::npos) break;
      
      size_t file_end = tree_content.find("}", file_start);
      if (file_end == std::string::npos) break;
      
      std::string file_entry = tree_content.substr(file_start, file_end - file_start + 1);
      
      // Extract path and sha
      size_t path_pos = file_entry.find("\"path\":\"");
      size_t sha_pos = file_entry.find("\"sha\":\"");
      size_t type_pos = file_entry.find("\"type\":\"");
      
      if (path_pos != std::string::npos && sha_pos != std::string::npos && type_pos != std::string::npos) {
        // Check if it's a file (not a directory)
        size_t type_start = type_pos + 8;
        size_t type_end = file_entry.find("\"", type_start);
        if (type_end != std::string::npos) {
          std::string type = file_entry.substr(type_start, type_end - type_start);
          if (type == "blob") { // It's a file
            // Extract path
            path_pos += 8; // Skip "path":"
            size_t path_end = file_entry.find("\"", path_pos);
            if (path_end != std::string::npos) {
              std::string path = file_entry.substr(path_pos, path_end - path_pos);
              
              // Extract sha
              sha_pos += 7; // Skip "sha":"
              size_t sha_end = file_entry.find("\"", sha_pos);
              if (sha_end != std::string::npos) {
                std::string sha = file_entry.substr(sha_pos, sha_end - sha_pos);
                files.push_back({path, sha});
              }
            }
          }
        }
      }
      
      pos = file_end + 1;
    }
    
    // Download each file
    for (const auto& file : files) {
      std::string file_path = file.first;
      std::string file_sha = file.second;
      
      // Get file content from GitHub blob API
      std::string file_content = get_github_blob_content(token, username, repo_name, file_sha);
      if (!file_content.empty()) {
        // Create directory structure if needed
        std::filesystem::path file_path_obj(file_path);
        if (file_path_obj.has_parent_path()) {
          std::filesystem::create_directories(file_path_obj.parent_path());
        }
        
        // Write file content
        std::ofstream file_stream(file_path);
        if (file_stream.is_open()) {
          file_stream << file_content;
          file_stream.close();
          downloaded_files.push_back(file_path); // Track downloaded file
        } else {
          std::cerr << "Error: Could not create file " << file_path << std::endl;
        }
      } else {
        std::cerr << "Error: Could not download content for " << file_path << std::endl;
      }
    }
    
    return true;
    
  } catch (const std::exception& e) {
    std::cerr << "Error downloading files: " << e.what() << std::endl;
    return false;
  }
}

std::string get_github_blob_content(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& blob_sha) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    return "";
  }
  
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/blobs/" + blob_sha;
  
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK) {
    return "";
  }
  
  // Parse the blob response to extract content
  size_t content_pos = response_data.find("\"content\":\"");
  if (content_pos == std::string::npos) {
    return "";
  }
  
  content_pos += 11; // Skip "content":"
  size_t content_end = response_data.find("\"", content_pos);
  if (content_end == std::string::npos) {
    return "";
  }
  
  std::string encoded_content = response_data.substr(content_pos, content_end - content_pos);
  
  // Decode base64 content
  return base64_decode(encoded_content);
}

std::string base64_decode(const std::string& encoded) {
  // Simple base64 decoder
  const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string decoded;
  int val = 0, valb = -8;
  
  for (char c : encoded) {
    if (chars.find(c) == std::string::npos) break;
    val = (val << 6) + chars.find(c);
    valb += 6;
    if (valb >= 0) {
      decoded.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  
  return decoded;
}

void integrate_pulled_files_with_bittrack(const std::string& commit_sha, const std::vector<std::string>& downloaded_files) {
  try {
    
    // Clear staging area and commit pulled files
    std::ofstream staging_file(".bittrack/index", std::ios::trunc);
    staging_file.close();
    
    // Create a commit directory for the pulled commit
    std::string commit_dir = ".bittrack/objects/" + commit_sha;
    std::filesystem::create_directories(commit_dir);
    
    // Use the files that were actually downloaded from GitHub
    std::vector<std::string> pulled_files = downloaded_files;
    
    // Copy files to the commit directory
    for (const std::string& file_path : pulled_files) {
      if (std::filesystem::exists(file_path)) {
        // Copy file to commit directory preserving the full path
        std::string commit_file_path = commit_dir + "/" + file_path;
        std::filesystem::create_directories(std::filesystem::path(commit_file_path).parent_path());
        std::filesystem::copy_file(file_path, commit_file_path, std::filesystem::copy_options::overwrite_existing);
      }
    }
    
    // Remove any files from the commit directory that were not pulled from GitHub
    // This ensures the commit directory only contains the files that were actually pulled
    for (const auto& entry : std::filesystem::recursive_directory_iterator(commit_dir)) {
      if (entry.is_regular_file()) {
        std::string file_path = entry.path().string();
        std::string relative_path = std::filesystem::relative(file_path, commit_dir).string();
        
        // Check if this file was in the pulled files list
        bool was_pulled = false;
        for (const auto& pulled_file : pulled_files) {
          if (relative_path == pulled_file) {
            was_pulled = true;
            break;
          }
        }
        
        // If this file was not pulled from GitHub, remove it
        if (!was_pulled) {
          std::filesystem::remove(file_path);
        }
      }
    }
    
    // Create commit log entry
    std::ofstream commit_log(".bittrack/commits/" + commit_sha, std::ios::trunc);
    commit_log << "GitHub Pull: " << commit_sha << std::endl;
    commit_log << "Date: " << get_current_timestamp() << std::endl;
    commit_log << "Files:" << std::endl;
    for (const std::string& file_path : pulled_files) {
      commit_log << "  " << file_path << std::endl;
    }
    commit_log.close();
    
    // Update current branch to point to this commit
    std::string current_branch = get_current_branch();
    if (!current_branch.empty()) {
      std::ofstream branch_file(".bittrack/refs/heads/" + current_branch, std::ios::trunc);
      branch_file << commit_sha << std::endl;
      branch_file.close();
    }
    
    // Add to commit history
    std::ofstream history_file(".bittrack/commits/history", std::ios::app);
    history_file << commit_sha << " " << current_branch << std::endl;
    history_file.close();
    
    // Update last pushed commit
    set_last_pushed_commit(commit_sha);
    
  } catch (const std::exception& e) {
    std::cerr << "Error integrating files: " << e.what() << std::endl;
  }
}


bool validate_github_operation_success(const std::string& response_data) {
  // Check for common error indicators in GitHub API responses
  if (response_data.find("\"message\":\"") != std::string::npos) {
    // Check for specific error messages
    if (response_data.find("\"message\":\"Not Found\"") != std::string::npos ||
        response_data.find("\"message\":\"Bad credentials\"") != std::string::npos ||
        response_data.find("\"message\":\"Validation Failed\"") != std::string::npos ||
        response_data.find("\"message\":\"Repository not found\"") != std::string::npos) {
      return false;
    }
  }
  
  // Check for success indicators
  if (response_data.find("\"sha\":\"") != std::string::npos) {
    return true;
  }
  
  return false;
}

bool delete_github_file(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& filename, const std::string& message) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    return false;
  }
  
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/contents/" + filename;
  
  // First, get the current file to get its SHA
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK) {
    return false;
  }
  
  // Parse response to get SHA
  size_t sha_pos = response_data.find("\"sha\":\"");
  if (sha_pos == std::string::npos) {
    // Check if file doesn't exist (404 error)
    if (response_data.find("\"message\":\"Not Found\"") != std::string::npos) {
      std::cout << "    File not found on GitHub: " << filename << " (skipping deletion)" << std::endl;
      return true; // Consider this a success since file is already deleted
    }
    return false; // File not found or no SHA
  }
  sha_pos += 7; // Skip "sha":"
  size_t sha_end = response_data.find("\"", sha_pos);
  if (sha_end == std::string::npos) {
    return false;
  }
  std::string file_sha = response_data.substr(sha_pos, sha_end - sha_pos);
  
  // Now delete the file
  curl = curl_easy_init();
  if (!curl) {
    return false;
  }
  
  response_data.clear();
  
  // Prepare JSON data for deletion
  std::string escaped_message = message;
  size_t pos = 0;
  while ((pos = escaped_message.find("\"", pos)) != std::string::npos) {
    escaped_message.replace(pos, 1, "\\\"");
    pos += 2;
  }
  pos = 0;
  while ((pos = escaped_message.find("\n", pos)) != std::string::npos) {
    escaped_message.replace(pos, 1, "\\n");
    pos += 2;
  }
  
  std::string json_data = "{\"message\":\"" + escaped_message + "\",\"sha\":\"" + file_sha + "\"}";
  
  // Initialize headers for the second request
  headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK) {
    return false;
  }
  
  // Check if the response contains a commit SHA (success)
  return validate_github_operation_success(response_data);
}

bool create_github_file(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& filename, const std::string& content, const std::string& message) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    return false;
  }
  
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/contents/" + filename + "?ref=main";
  
  // Encode content to base64
  std::string base64_content = base64_encode(content);
  
  // Prepare JSON data with proper escaping
  std::string escaped_message = message;
  size_t pos = 0;
  while ((pos = escaped_message.find("\"", pos)) != std::string::npos) {
    escaped_message.replace(pos, 1, "\\\"");
    pos += 2;
  }
  pos = 0;
  while ((pos = escaped_message.find("\n", pos)) != std::string::npos) {
    escaped_message.replace(pos, 1, "\\n");
    pos += 2;
  }
  
  std::string json_data = "{\"message\":\"" + escaped_message + "\",\"content\":\"" + base64_content + "\",\"branch\":\"main\"}";
  
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  
  CURLcode res = curl_easy_perform(curl);
  
  // Debug output removed for cleaner output
  
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK) {
    std::cerr << "CURL Error: " << curl_easy_strerror(res) << std::endl;
    return false;
  }
  
  // Check if the response contains a commit SHA (success)
  bool success = validate_github_operation_success(response_data);
  if (!success) {
    std::cerr << "Failed to create file " << filename << " - GitHub API error" << std::endl;
  }
  return success;
}

