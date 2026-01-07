#include "../include/github.hpp"
#include "../include/remote.hpp"
#include <regex>

size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

bool isGithubRemote(const std::string &url)
{
  // Simple check for GitHub URL
  return url.find("github.com") != std::string::npos;
}

void setGithubCommitMapping(const std::string &bittrack_commit, const std::string &github_commit)
{
  std::string mapping = bittrack_commit + " " + github_commit + "\n";
  if (!ErrorHandler::safeAppendFile(".bittrack/github_commits", mapping))
  {
    ErrorHandler::printError(
        ErrorCode::FILE_WRITE_ERROR,
        "Could not update .bittrack/github_commits",
        ErrorSeverity::ERROR,
        "set_github_commit_mapping");
  }
}

std::string getGithubCommitForBittrack(const std::string &bittrack_commit)
{
  std::string content = ErrorHandler::safeReadFile(".bittrack/github_commits");
  if (content.empty())
  {
    return "";
  }

  std::istringstream iss(content);
  std::string line;
  while (std::getline(iss, line))
  {
    if (line.empty())
    {
      continue;
    }

    std::istringstream line_ss(line);
    std::string bittrack_hash, github_hash;
    if (line_ss >> bittrack_hash >> github_hash &&
        bittrack_hash == bittrack_commit)
    {
      return github_hash;
    }
  }
  return "";
}

std::string getLatestGithubCommit(const std::string &token, const std::string &username, const std::string &repo_name)
{
  CURL *curl = curl_easy_init(); // Initialize CURL
  if (!curl)
  {
    return "";
  }

  std::string current_branch = getCurrentBranch();

  // Prepare the URL for the latest commit
  std::string response_data;
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/refs/heads/" + current_branch;

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());             // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);          // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);    // Set response data storage

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

std::string extractGithubInfoFromUrl(const std::string &url, std::string &username, std::string &repository)
{
  // Regex to extract username and repository from GitHub URL
  std::regex github_regex(R"(https?://github\.com/([^/]+)/([^/]+?)(?:\.git)?/?$)");
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

bool pushToGithubApi(const std::string &token, const std::string &username, const std::string &repo_name)
{
  try
  {
    // Get current commit and branch
    std::string current_commit = getCurrentCommit();
    std::string current_branch = getCurrentBranch();

    if (current_commit.empty())
    {
      ErrorHandler::printError(
          ErrorCode::NO_COMMITS_FOUND,
          "No commits to push",
          ErrorSeverity::ERROR,
          "push_to_github_api");
      return false;
    }

    // Check if already up to date
    std::string last_pushed = getLastPushedCommit();
    if (last_pushed == current_commit)
    {
      std::cout << "Repository is up to date." << std::endl;
      return true;
    }

    // Get the last commit SHA on GitHub
    std::string parent_sha = getGithubLastCommitSha(token, username, repo_name, "heads/" + current_branch);
    bool branch_exists = !parent_sha.empty();
    bool is_empty_repo = false;

    if (!branch_exists)
    {
      // Branch does not exist. Check if repository is empty by checking for
      // main branch.
      std::string main_ref =
          getGithubLastCommitSha(token, username, repo_name, "heads/main");
      if (main_ref.empty())
      {
        main_ref = getGithubLastCommitSha(token, username, repo_name, "heads/master");
      }
      is_empty_repo = main_ref.empty();

      // If not empty, we are creating a new branch.
      // We need a base for the new commit if we can find one.
      // For simplicity in this logic, we might start as orphan or try to use
      // mapped parent. But crucially, we must NOT use the file-by-file upload
      // below which defaults to main.
    }

    if (is_empty_repo)
    {
      // Handle initial commit for empty repository
      std::vector<std::string> latest_commit_files =
          getCommittedFiles(current_commit);
      if (latest_commit_files.empty())
      {
        ErrorHandler::printError(
            ErrorCode::INTERNAL_ERROR,
            "No files found in latest commit",
            ErrorSeverity::ERROR,
            "push_to_github_api");
        return false;
      }

      std::string commit_message =
          getCommitMessage(current_commit); // Get commit message

      std::vector<std::string> blob_shas;  // To store blob SHAs
      std::vector<std::string> file_names; // To store file names

      for (const auto &file_path : latest_commit_files)
      {
        // Read file content using safeReadFile
        std::string content = ErrorHandler::safeReadFile(file_path);
        if (content.empty() && !std::filesystem::exists(file_path))
        {
          continue;
        }

        std::string base64_content =
            base64Encode(content); // Encode content to base64

        // Create the file on GitHub
        CURL *curl = curl_easy_init();
        if (!curl)
        {
          continue;
        }

        std::string response_data;                                                                                 // To store response data
        std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/contents/" + file_path; // Prepare URL
        std::string json_data = "{\"message\":\"" + commit_message + "\",\"content\":\"" + base64_content + "\"}"; // Prepare JSON data

        struct curl_slist *headers = nullptr;                                            // Initialize headers
        headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
        headers = curl_slist_append(headers, "Content-Type: application/json");          // Add content-type header
        headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());              // Set the URL
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");          // Set HTTP method to PUT
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str()); // Set JSON data
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);           // Set headers
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);  // Set write callback
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);     // Set response data storage

        CURLcode res = curl_easy_perform(curl); // Perform the request
        curl_slist_free_all(headers);           // Free headers
        curl_easy_cleanup(curl);                // Clean up CURL

        if (res == CURLE_OK && response_data.find("\"sha\":\"") != std::string::npos) // Check for success
        {
          std::cout << "Created file: " << file_path << std::endl;
        }
        else
        {
          ErrorHandler::printError(
              ErrorCode::REMOTE_CONNECTION_FAILED,
              "Failed to create file: " + file_path,
              ErrorSeverity::ERROR,
              "push_to_github_api");
        }
      }

      std::cout << "Done..." << std::endl;
      setLastPushedCommit(current_commit);
      return true;
    }

    // Push commits one by one
    std::string current_tree_sha = "";
    std::string last_commit_sha = parent_sha;
    std::string commit_message = getCommitMessage(current_commit);

    // If new branch (last_commit_sha is empty), try to finding parent from
    // local commit history

    std::vector<std::string> committed_files = getCommittedFiles(current_commit); // Get files in current commit
    if (committed_files.empty())
    {
      ErrorHandler::printError(
          ErrorCode::INTERNAL_ERROR,
          "No files found in current commit",
          ErrorSeverity::ERROR,
          "push_to_github_api");
      return false;
    }

    std::vector<std::string> blob_shas;  // To store blob SHAs
    std::vector<std::string> file_names; // To store file names

    for (const auto &file_path : committed_files)
    {
      // Handle deleted files
      if (isDeleted(file_path))
      {
        std::string actual_path = getActualPath(file_path);                            // Get actual file path
        if (deleteGithubFile(token, username, repo_name, actual_path, commit_message)) // Delete file on GitHub
        {
          std::cout << "Deleted file: " << actual_path << std::endl;
        }
        else
        {
          ErrorHandler::printError(
              ErrorCode::REMOTE_CONNECTION_FAILED,
              "Failed to delete file: " + actual_path,
              ErrorSeverity::ERROR,
              "push_to_github_api");
        }
      }
      else
      {
        // Read file content from commit using safeReadFile
        std::string commit_file_path =
            ".bittrack/objects/" + current_commit + "/" + file_path;
        std::string content = ErrorHandler::safeReadFile(commit_file_path);
        if (content.empty() && !std::filesystem::exists(commit_file_path))
        {
          ErrorHandler::printError(
              ErrorCode::FILE_READ_ERROR,
              "Could not read commit file: " +
                  commit_file_path,
              ErrorSeverity::ERROR,
              "push_to_github_api");
          continue;
        }

        // Create blob on GitHub
        std::string blob_sha =
            createGithubBlob(token, username, repo_name, content);
        if (!blob_sha.empty())
        {
          blob_shas.push_back(blob_sha);
          file_names.push_back(file_path);
          // std::cout << "Created blob for " << file_path << std::endl;
        }
        else
        {
          ErrorHandler::printError(
              ErrorCode::REMOTE_CONNECTION_FAILED,
              "Failed to create blob for " + file_path,
              ErrorSeverity::ERROR,
              "push_to_github_api");
        }
      }
    }

    // Handle case where all files are deleted
    if (blob_shas.empty() && !committed_files.empty())
    {
      bool all_deleted = true;
      for (const auto &file_path : committed_files)
      {
        // Check if file is deleted
        if (!isDeleted(file_path))
        {
          all_deleted = false;
          break;
        }
      }

      // If all files are deleted, we still need to create a tree
      if (!all_deleted)
      {
        ErrorHandler::printError(
            ErrorCode::REMOTE_CONNECTION_FAILED,
            "Could not create blobs for current commit",
            ErrorSeverity::ERROR,
            "push_to_github_api");
        return false;
      }
    }
    else if (blob_shas.empty()) // No files to process
    {
      ErrorHandler::printError(
          ErrorCode::INTERNAL_ERROR,
          "No files to process for current commit",
          ErrorSeverity::ERROR,
          "push_to_github_api");
      return false;
    }

    if (!blob_shas.empty()) // If there are blobs to add
    {
      if (current_tree_sha.empty()) // If no current tree, create new
      {
        current_tree_sha = createGithubTreeWithFiles(token, username, repo_name, blob_shas, file_names);
      }
      else
      {
        // Update existing tree
        current_tree_sha = createGithubTreeWithFiles(
            token,
            username,
            repo_name,
            blob_shas,
            file_names,
            current_tree_sha);
      }

      if (current_tree_sha.empty()) // If tree creation failed
      {
        ErrorHandler::printError(
            ErrorCode::REMOTE_CONNECTION_FAILED,
            "Could not create or update tree for current commit",
            ErrorSeverity::ERROR,
            "push_to_github_api");
        return false;
      }
    }
    else if (current_tree_sha.empty()) // If no blobs and no current tree
    {
      if (last_commit_sha.empty())
      {
        ErrorHandler::printError(
            ErrorCode::REMOTE_CONNECTION_FAILED,
            "No parent commit for deletion-only commit",
            ErrorSeverity::ERROR,
            "push_to_github_api");
        return false;
      }
      // Get tree from parent commit
      std::string parent_tree_sha = getGithubCommitTree(token, username, repo_name, last_commit_sha);
      if (parent_tree_sha.empty())
      {
        ErrorHandler::printError(
            ErrorCode::REMOTE_CONNECTION_FAILED,
            "Could not get tree from parent commit " + last_commit_sha,
            ErrorSeverity::ERROR,
            "push_to_github_api");
        return false;
      }
      current_tree_sha = parent_tree_sha; // Set current tree to parent tree
    }

    std::string author_name = getCommitAuthor(current_commit);         // Get author name
    std::string author_email = getCommitAuthorEmail(current_commit);   // Get author email
    std::string commit_timestamp = getCommitTimestamp(current_commit); // Get commit timestamp
    std::string new_commit_sha = createGithubCommit(
        token,
        username,
        repo_name,
        current_tree_sha,
        last_commit_sha,
        commit_message,
        author_name,
        author_email,
        commit_timestamp);
    if (new_commit_sha.empty())
    {
      ErrorHandler::printError(
          ErrorCode::REMOTE_CONNECTION_FAILED,
          "Could not create commit " + current_commit,
          ErrorSeverity::ERROR,
          "push_to_github_api");
      return false;
    }

    last_commit_sha = new_commit_sha; // Update last commit SHA

    setGithubCommitMapping(current_commit, new_commit_sha); // Map BitTrack commit to GitHub commit

    if (branch_exists)
    {
      if (updateGithubReferance(token, username, repo_name, "heads/" + current_branch, last_commit_sha)) // Update branch reference
      {
        ErrorHandler::printError(
            ErrorCode::REMOTE_CONNECTION_FAILED,
            "Could not update branch reference",
            ErrorSeverity::ERROR,
            "push_to_github_api");
        return false;
      }
    }
    else
    {
      if (!createGithubReferance(token, username, repo_name, "heads/" + current_branch, last_commit_sha)) // Create branch reference
      {
        ErrorHandler::printError(
            ErrorCode::REMOTE_CONNECTION_FAILED,
            "Could not create branch reference",
            ErrorSeverity::ERROR,
            "push_to_github_api");
        return false;
      }
    }
    std::cout << "Done..." << std::endl;

    setLastPushedCommit(current_commit); // Update last pushed commit
    return true;
  }
  catch (const std::exception &e)
  {
    ErrorHandler::printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Error: " + std::string(e.what()),
        ErrorSeverity::ERROR,
        "push_to_github_api");
    return false;
  }
}

std::string createGithubBlob(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &content)
{
  CURL *curl = curl_easy_init();
  if (!curl)
  {
    return "";
  }

  std::string response_data;                                                                     // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/blobs"; // Prepare URL
  std::string base64_content = base64Encode(content);                                            // Encode content to base64
  std::string json_data = "{\"content\":\"" + base64_content + "\",\"encoding\":\"base64\"}";    // Prepare JSON data

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "Content-Type: application/json");          // Add content-type header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());              // Set the URL
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str()); // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);           // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);  // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);     // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK)
  {
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "CURL Error: " +
            std::string(curl_easy_strerror(res)),
        ErrorSeverity::ERROR,
        "create_github_blob");
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
      return response_data.substr(sha_pos, sha_end - sha_pos); // Return the extracted SHA
    }
  }

  ErrorHandler::printError(ErrorCode::REMOTE_CONNECTION_FAILED, "Could not find SHA in response", ErrorSeverity::ERROR, "create_github_blob");
  return "";
}

std::string createGithubTree(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &blob_sha, const std::string &filename)
{
  CURL *curl = curl_easy_init();
  if (!curl)
    return "";

  std::string response_data;                                                                     // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/trees"; // Prepare URL

  std::string escaped_filename = filename; // Escape double quotes in filename
  size_t pos = 0;

  while ((pos = escaped_filename.find("\"", pos)) != std::string::npos) // Find double quotes
  {
    escaped_filename.replace(pos, 1, "\\\""); // Replace with escaped version
    pos += 2;                                 // Move past the escaped quote
  }

  // Prepare JSON data
  std::string json_data = "{\"base_tree\":null,\"tree\":[{\"path\":\"" + escaped_filename + "\",\"mode\":\"100644\",\"type\":\"blob\",\"sha\":\"" + blob_sha + "\"}]}";

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "Content-Type: application/json");          // Add content-type header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());              // Set the URL
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str()); // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);           // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);  // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);     // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK)
  {
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "CURL error: " +
            std::string(curl_easy_strerror(res)),
        ErrorSeverity::ERROR,
        "create_github_tree");
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

std::string getGithubCommitTree(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &commit_sha)
{
  CURL *curl = curl_easy_init();
  if (!curl)
    return "";

  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/commits/" + commit_sha; // Prepare URL
  std::string response_data;                                                                                     // To store response data

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());             // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);          // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);    // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK)
  {
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "CURL error: " +
            std::string(curl_easy_strerror(res)),
        ErrorSeverity::ERROR,
        "get_github_commit_tree");
    return "";
  }

  size_t tree_pos = response_data.find("\"tree\":{\"sha\":\""); // Find tree SHA in response
  if (tree_pos != std::string::npos)                            // If tree SHA found
  {
    tree_pos += 15;                                       // Skip "tree":{"sha":"
    size_t tree_end = response_data.find("\"", tree_pos); // Find end quote
    if (tree_end != std::string::npos)                    // If end quote found
    {
      return response_data.substr(tree_pos, tree_end - tree_pos); // Return the extracted tree SHA
    }
  }

  return "";
}

std::string
createGithubTreeWithFiles(const std::string &token, const std::string &username, const std::string &repo_name, const std::vector<std::string> &blob_shas, const std::vector<std::string> &file_names, const std::string &base_tree_sha)
{
  CURL *curl = curl_easy_init();
  if (!curl)
    return "";

  std::string response_data;                                                                     // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/trees"; // Prepare URL

  std::string json_data = "{\"base_tree\":"; // Prepare JSON data
  if (base_tree_sha.empty())
  {
    json_data += "null"; // No base tree
  }
  else
  {
    json_data += "\"" + base_tree_sha + "\""; // Add base tree SHA
  }
  json_data += ",\"tree\":["; // Start tree array

  for (size_t i = 0; i < blob_shas.size(); ++i) // Add each file entry
  {
    if (i > 0)
    {
      json_data += ","; // Add comma between entries
    }

    std::string escaped_filename =
        file_names[i]; // Escape double quotes in filename
    size_t pos = 0;
    while ((pos = escaped_filename.find("\"", pos)) != std::string::npos) // Find double quotes
    {
      escaped_filename.replace(pos, 1, "\\\""); // Replace with escaped version
      pos += 2;                                 // Move past the escaped quote
    }

    // Add file entry to JSON
    json_data += "{\"path\":\"" + escaped_filename + "\",\"mode\":\"100644\",\"type\":\"blob\",\"sha\":\"" + blob_shas[i] + "\"}";
  }

  json_data += "]}"; // End tree array and JSON

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "Content-Type: application/json");          // Add content-type header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());              // Set the URL
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str()); // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);           // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);  // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);     // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK)
  {
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "CURL error: " + std::string(curl_easy_strerror(res)),
        ErrorSeverity::ERROR,
        "create_github_tree_with_files");
    return "";
  }

  size_t sha_pos = response_data.find("\"sha\":\""); // Find SHA in response
  if (sha_pos != std::string::npos)                  // If SHA found
  {
    sha_pos += 7;                                       // Skip "sha":"
    size_t sha_end = response_data.find("\"", sha_pos); // Find end quote
    if (sha_end != std::string::npos)                   // If end quote found
    {
      return response_data.substr(sha_pos, sha_end - sha_pos); // Return the extracted SHA
    }
  }

  return "";
}

std::string
createGithubCommit(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &tree_sha, const std::string &parent_sha, const std::string &message, const std::string &author_name, const std::string &author_email, const std::string &timestamp)
{
  CURL *curl = curl_easy_init();
  if (!curl)
  {
    return "";
  }

  std::string response_data;                                                                       // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/commits"; // Prepare URL

  std::string escaped_message = message; // Escape double quotes and newlines in message
  size_t pos = 0;
  while ((pos = escaped_message.find("\"", pos)) != std::string::npos) // Find double quotes
  {
    escaped_message.replace(pos, 1, "\\\""); // Replace with escaped version
    pos += 2;
  }
  pos = 0;
  while ((pos = escaped_message.find("\n", pos)) != std::string::npos) // Find newlines
  {
    escaped_message.replace(pos, 1, "\\n"); // Replace with escaped version
    pos += 2;
  }

  auto now = std::chrono::system_clock::now();                     // Get current time
  auto time_t = std::chrono::system_clock::to_time_t(now);         // Convert to time_t
  std::stringstream ss;                                            // Format time to GitHub timestamp
  ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ"); // GitHub timestamp format
  std::string github_timestamp = ss.str();                         // Final timestamp string

  std::string json_data = "{\"message\":\"" + escaped_message + "\",\"tree\":\"" + tree_sha + "\""; // Prepare JSON data
  if (!parent_sha.empty())
  {
    json_data += ",\"parents\":[\"" + parent_sha + "\"]"; // Add parent SHA if exists
  }

  json_data += ",\"author\":{\"name\":\"" + author_name + "\",\"email\":\"" + author_email + "\",\"date\":\"" + github_timestamp + "\"}";    // Add author info
  json_data += ",\"committer\":{\"name\":\"" + author_name + "\",\"email\":\"" + author_email + "\",\"date\":\"" + github_timestamp + "\"}"; // Add committer info
  json_data += "}";                                                                                                                          // End JSON

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "Content-Type: application/json");          // Add content-type header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());              // Set the URL
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str()); // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);           // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);  // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);     // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK)
  {
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "CURL error: " + std::string(curl_easy_strerror(res)),
        ErrorSeverity::ERROR,
        "create_github_commit");

    return "";
  }

  size_t sha_pos = response_data.find("\"sha\":\""); // Find SHA in response
  if (sha_pos != std::string::npos)                  // If SHA found
  {
    sha_pos += 7;                                       // Skip "sha":"
    size_t sha_end = response_data.find("\"", sha_pos); // Find end quote
    if (sha_end != std::string::npos)                   // If end quote found
    {
      return response_data.substr(sha_pos, sha_end - sha_pos); // Return the extracted SHA
    }
  }

  return "";
}

std::string getGithubLastCommitSha(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &ref)
{
  CURL *curl = curl_easy_init();
  if (!curl)
  {
    return "";
  }

  std::string response_data;                                                                           // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/refs/" + ref; // Prepare URL

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());             // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);          // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);    // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK)
  {
    return "";
  }

  if (response_data.find("Bad credentials") != std::string::npos)
  {
    ErrorHandler::printError(
        ErrorCode::CONFIG_ERROR,
        "Error: Invalid GitHub token. Please check your token and try again.",
        ErrorSeverity::ERROR,
        "get_github_last_commit_sha");

    return "";
  }

  if (response_data.empty())
  {
    return "";
  }

  size_t sha_pos = response_data.find("\"sha\":\""); // Find SHA in response
  if (sha_pos != std::string::npos)                  // If SHA found
  {
    sha_pos += 7;                                       // Skip "sha":"
    size_t sha_end = response_data.find("\"", sha_pos); // Find end quote
    if (sha_end != std::string::npos)                   // If end quote found
    {
      return response_data.substr(sha_pos, sha_end - sha_pos); // Return the extracted SHA
    }
  }

  return "";
}

bool updateGithubReferance(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &ref, const std::string &sha)
{
  CURL *curl = curl_easy_init();
  if (!curl)
  {
    return false;
  }

  std::string response_data;                                                                           // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/refs/" + ref; // Prepare URL
  std::string json_data = "{\"sha\":\"" + sha + "\"}";                                                 // Prepare JSON data

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "Content-Type: application/json");          // Add content-type header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());              // Set the URL
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");        // Set HTTP method to PATCH
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str()); // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);           // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);  // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);     // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  return res == CURLE_OK;
}

bool createGithubReferance(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &ref, const std::string &sha)
{
  CURL *curl = curl_easy_init();
  if (!curl)
  {
    return false;
  }

  std::string response_data;                                                                    // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/refs"; // Prepare URL
  std::string json_data = "{\"ref\":\"refs/" + ref + "\",\"sha\":\"" + sha + "\"}";             // Prepare JSON data

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "Content-Type: application/json");          // Add content-type header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());              // Set the URL
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str()); // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);           // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);  // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);     // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK)
  {
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "CURL error: " + std::string(curl_easy_strerror(res)),
        ErrorSeverity::ERROR,
        "create_github_ref");
    return false;
  }

  if (response_data.find("\"ref\":\"refs/" + ref + "\"") != std::string::npos)
  {
    return true;
  }

  return false;
}

bool pullFromGithubApi(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &branch_name)
{
  try
  {
    std::string target_branch = branch_name;
    if (target_branch.empty())
    {
      target_branch = getCurrentBranch();
    }

    // Get the latest commit SHA from GitHub
    std::string latest_commit_sha = getGithubLastCommitSha(
        token, username, repo_name, "heads/" + target_branch);
    if (latest_commit_sha.empty())
    {
      // Fallback: try without "heads/" prefix if it fails, or maybe the branch
      // is just the ref But get_github_last_commit_sha likely expects a ref.
      // Let's assume standard behavior for now.
      return false;
    }

    // Check if we already have this commit
    std::string commit_data =
        getGithubCommitData(token, username, repo_name, latest_commit_sha);
    if (commit_data.empty())
    {
      ErrorHandler::printError(
          ErrorCode::REMOTE_CONNECTION_FAILED,
          "Could not get commit data from GitHub",
          ErrorSeverity::ERROR,
          "pull_from_github_api");
      return false;
    }

    // Extract files from the commit
    std::vector<std::string> downloaded_files;
    if (extractFilesFromGithubCommit(token, username, repo_name, latest_commit_sha, commit_data, downloaded_files))
    {
      // Integrate downloaded files into BitTrack
      integratePulledFilesWithBittrack(latest_commit_sha, downloaded_files);
      std::cout << "✓ Pulled " << downloaded_files.size() << " files from GitHub" << std::endl;
      return true;
    }
    else
    {
      ErrorHandler::printError(
          ErrorCode::REMOTE_CONNECTION_FAILED,
          "Failed to extract files from GitHub commit",
          ErrorSeverity::ERROR,
          "pull_from_github_api");
      return false;
    }
  }
  catch (const std::exception &e)
  {
    ErrorHandler::printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Error: " + std::string(e.what()),
        ErrorSeverity::ERROR,
        "pull_from_github_api");
    return false;
  }
}

std::string getGithubCommitData(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &commit_sha)
{
  // Get commit data from GitHub API
  CURL *curl = curl_easy_init();
  if (!curl)
  {
    return "";
  }

  std::string response_data;                                                                                     // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/commits/" + commit_sha; // Prepare URL

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());             // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);          // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);    // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK)
  {
    return "";
  }

  return response_data;
}

bool extractFilesFromGithubCommit(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &commit_sha, const std::string &commit_data, std::vector<std::string> &downloaded_files)
{
  // Extract tree SHA from commit data
  size_t tree_pos = commit_data.find("\"tree\":{\"sha\":\"");
  if (tree_pos == std::string::npos)
  {
    return false;
  }

  tree_pos += 15;                                     // Skip "tree":{"sha":"
  size_t tree_end = commit_data.find("\"", tree_pos); // Find end quote
  if (tree_end == std::string::npos)
  {
    return false;
  }

  std::string tree_sha = commit_data.substr(tree_pos, tree_end - tree_pos);        // Extracted tree SHA
  std::string tree_data = getGithubTreeData(token, username, repo_name, tree_sha); // Get tree data from GitHub
  if (tree_data.empty())
  {
    return false;
  }

  return downloadFilesFromGithubTree(
      token, username, repo_name, tree_data,
      downloaded_files); // Download files from the tree
}

std::string getGithubTreeData(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &tree_sha)
{
  // Get tree data from GitHub API
  CURL *curl = curl_easy_init();
  if (!curl)
  {
    return "";
  }

  std::string response_data;                                                                                                  // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/trees/" + tree_sha + "?recursive=1"; // Prepare URL

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());             // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);          // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);    // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK)
  {
    return "";
  }

  return response_data;
}

bool downloadFilesFromGithubTree(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &tree_data, std::vector<std::string> &downloaded_files)
{
  try
  {
    std::vector<std::pair<std::string, std::string>> files; // path, sha pairs

    size_t tree_start = tree_data.find("\"tree\":["); // Find start of tree array
    if (tree_start == std::string::npos)
    {
      ErrorHandler::printError(
          ErrorCode::REMOTE_CONNECTION_FAILED,
          "Could not find tree array in GitHub response",
          ErrorSeverity::ERROR,
          "download_files_from_github_tree");
      return false;
    }

    tree_start += 7;                                   // Skip "tree":[
    size_t tree_end = tree_data.find("]", tree_start); // Find end of tree array
    if (tree_end == std::string::npos)
    {
      ErrorHandler::printError(
          ErrorCode::REMOTE_CONNECTION_FAILED,
          "Malformed tree array in GitHub response",
          ErrorSeverity::ERROR,
          "download_files_from_github_tree");
      return false;
    }

    std::string tree_content = tree_data.substr(tree_start, tree_end - tree_start); // Extract tree content
    size_t pos = 0;                                                                 // Position in tree content
    while (pos < tree_content.length())
    {
      size_t file_start = tree_content.find("{", pos); // Find start of file entry
      if (file_start == std::string::npos)
      {
        break;
      }

      size_t file_end = tree_content.find("}", file_start); // Find end of file entry
      if (file_end == std::string::npos)
      {
        break;
      }

      std::string file_entry = tree_content.substr(file_start, file_end - file_start + 1); // Extract file entry
      size_t path_pos = file_entry.find("\"path\":\"");                                    // Find path
      size_t sha_pos = file_entry.find("\"sha\":\"");                                      // Find sha
      size_t type_pos = file_entry.find("\"type\":\"");                                    // Find type

      if (path_pos != std::string::npos && sha_pos != std::string::npos && type_pos != std::string::npos) // If all found
      {
        size_t type_start = type_pos + 8;                    // Skip "type":"
        size_t type_end = file_entry.find("\"", type_start); // Find end quote of type
        if (type_end != std::string::npos)
        {
          std::string type = file_entry.substr(type_start, type_end - type_start); // Extract type
          if (type == "blob")
          {
            path_pos += 8;                                     // Skip "path":"
            size_t path_end = file_entry.find("\"", path_pos); // Find end quote of path
            if (path_end != std::string::npos)                 // If end quote found
            {
              std::string path = file_entry.substr(path_pos, path_end - path_pos); // Extract path
              sha_pos += 7;                                                        // Skip "sha":"
              size_t sha_end = file_entry.find("\"", sha_pos);                     // Find end quote of sha
              if (sha_end != std::string::npos)                                    // If end quote found
              {
                std::string sha = file_entry.substr(sha_pos, sha_end - sha_pos); // Extract sha
                files.push_back({path, sha});                                    // Store path and sha
              }
            }
          }
        }
      }
      pos = file_end + 1;
    }

    for (const auto &file : files)
    {
      std::string file_path = file.first;
      std::string file_sha = file.second;

      std::string file_content = getGithubBlobContent(
          token, username, repo_name, file_sha); // Get blob content
      if (!file_content.empty())
      {
        std::filesystem::path file_path_obj(file_path); // Create path object
        if (file_path_obj.has_parent_path())            // Create directories if needed
        {
          std::filesystem::create_directories(file_path_obj.parent_path());
        }

        std::ofstream file_stream(file_path); // Write content to file
        if (file_stream.is_open())
        {
          file_stream << file_content;
          file_stream.close();
          downloaded_files.push_back(file_path);
        }
        else
        {
          ErrorHandler::printError(
              ErrorCode::FILE_WRITE_ERROR, "Could not create file " + file_path,
              ErrorSeverity::ERROR,
              "download_files_from_github_tree");
        }
      }
      else
      {
        ErrorHandler::printError(
            ErrorCode::REMOTE_CONNECTION_FAILED,
            "Could not download content for " + file_path,
            ErrorSeverity::ERROR,
            "download_files_from_github_tree");
      }
    }
    return true;
  }
  catch (const std::exception &e)
  {
    ErrorHandler::printError(
        ErrorCode::UNEXPECTED_EXCEPTION,
        "Error downloading files: " + std::string(e.what()),
        ErrorSeverity::ERROR,
        "download_files_from_github_tree");
    return false;
  }
}

std::string getGithubBlobContent(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &blob_sha)
{
  CURL *curl = curl_easy_init();
  if (!curl)
  {
    return "";
  }

  std::string response_data;                                                                                 // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/git/blobs/" + blob_sha; // Prepare URL

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());             // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);          // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);    // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK)
  {
    return "";
  }

  size_t content_pos = response_data.find("\"content\":\""); // Find content in response
  if (content_pos == std::string::npos)
  {
    return "";
  }

  content_pos += 11;                                          // Skip "content":"
  size_t content_end = response_data.find("\"", content_pos); // Find end quote of content
  if (content_end == std::string::npos)
  {
    return "";
  }

  std::string encoded_content = response_data.substr(content_pos, content_end - content_pos); // Extract encoded content

  // Remove JSON escapes (\n)
  size_t pos = 0;
  while ((pos = encoded_content.find("\\n", pos)) != std::string::npos)
  {
    encoded_content.replace(pos, 2, "");
  }
  pos = 0;
  while ((pos = encoded_content.find("\\r", pos)) != std::string::npos)
  {
    encoded_content.replace(pos, 2, "");
  }

  return base64Decode(encoded_content);
}

bool validateGithubOperationSuccess(const std::string &response_data)
{
  if (response_data.find("\"message\":\"") !=
      std::string::npos) // Check for error messages
  {
    // Common error messages
    if (response_data.find("\"message\":\"Not Found\"") != std::string::npos ||
        response_data.find("\"message\":\"Bad credentials\"") != std::string::npos ||
        response_data.find("\"message\":\"Validation Failed\"") != std::string::npos ||
        response_data.find("\"message\":\"Repository not found\"") != std::string::npos)
    {
      return false;
    }
  }

  // Check for success indicator
  if (response_data.find("\"sha\":\"") != std::string::npos)
  {
    return true;
  }

  return false;
}

bool deleteGithubFile(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &filename, const std::string &message)
{
  CURL *curl = curl_easy_init();
  if (!curl)
  {
    return false;
  }

  std::string response_data;                                                                                // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/contents/" + filename; // Prepare URL

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());             // Set the URL
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);          // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback); // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);    // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK)
  {
    return false;
  }

  size_t sha_pos = response_data.find("\"sha\":\""); // Find SHA in response
  if (sha_pos == std::string::npos)                  // If SHA not found
  {
    if (response_data.find("\"message\":\"Not Found\"") != std::string::npos) // File not found
    {
      std::cout << "    File not found on GitHub: " << filename << " (skipping deletion)" << std::endl;
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
  std::string file_sha = response_data.substr(sha_pos, sha_end - sha_pos); // Extracted file SHA

  curl = curl_easy_init(); // Initialize CURL for deletion
  if (!curl)
  {
    return false;
  }

  response_data.clear();

  std::string escaped_message = message; // Escape double quotes and newlines in message
  size_t pos = 0;
  while ((pos = escaped_message.find("\"", pos)) != std::string::npos) // Find double quotes
  {
    escaped_message.replace(pos, 1, "\\\""); // Replace with escaped version
    pos += 2;
  }
  pos = 0;
  while ((pos = escaped_message.find("\n", pos)) != std::string::npos) // Find newlines
  {
    escaped_message.replace(pos, 1, "\\n"); // Replace with escaped version
    pos += 2;
  }

  // Prepare JSON data for deletion
  std::string json_data = "{\"message\":\"" + escaped_message + "\",\"sha\":\"" + file_sha + "\"}";

  headers = nullptr;                                                               // Re-initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "Content-Type: application/json");          // Add content-type header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());              // Set the URL
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");       // Set HTTP method to DELETE
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str()); // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);           // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);  // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);     // Set response data storage

  res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);  // Free headers
  curl_easy_cleanup(curl);       // Clean up CURL

  if (res != CURLE_OK)
  {
    return false;
  }

  return validateGithubOperationSuccess(response_data);
}

bool createGithubFile(const std::string &token, const std::string &username, const std::string &repo_name, const std::string &filename, const std::string &content, const std::string &message)
{
  CURL *curl = curl_easy_init();
  if (!curl)
  {
    return false;
  }

  std::string current_branch = getCurrentBranch();
  std::string response_data;                                                                                                           // To store response data
  std::string url = "https://api.github.com/repos/" + username + "/" + repo_name + "/contents/" + filename + "?ref=" + current_branch; // Prepare URL
  std::string base64_content = base64Encode(content);                                                                                  // Base64 encode content

  std::string escaped_message = message; // Escape double quotes and newlines in message
  size_t pos = 0;
  while ((pos = escaped_message.find("\"", pos)) != std::string::npos) // Find double quotes
  {
    escaped_message.replace(pos, 1, "\\\""); // Replace with escaped version
    pos += 2;
  }
  pos = 0;
  while ((pos = escaped_message.find("\n", pos)) != std::string::npos) // Find newlines
  {
    escaped_message.replace(pos, 1, "\\n");
    pos += 2;
  }

  // Prepare JSON data for file creation
  std::string json_data = "{\"message\":\"" + escaped_message + "\",\"content\":\"" + base64_content + "\",\"branch\":\"" + current_branch + "\"}";

  struct curl_slist *headers = nullptr;                                            // Initialize headers
  headers = curl_slist_append(headers, ("Authorization: token " + token).c_str()); // Add authorization header
  headers = curl_slist_append(headers, "Content-Type: application/json");          // Add content-type header
  headers = curl_slist_append(headers, "User-Agent: BitTrack/1.0");                // Add user-agent header

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());              // Set the URL
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str()); // Set JSON data
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);           // Set headers
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);  // Set write callback
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);     // Set response data storage

  CURLcode res = curl_easy_perform(curl); // Perform the request
  curl_slist_free_all(headers);           // Free headers
  curl_easy_cleanup(curl);                // Clean up CURL

  if (res != CURLE_OK)
  {
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "CURL Error: " + std::string(curl_easy_strerror(res)),
        ErrorSeverity::ERROR,
        "create_github_file");
    return false;
  }

  bool success = validateGithubOperationSuccess(response_data);
  if (!success)
  {
    ErrorHandler::printError(
        ErrorCode::REMOTE_CONNECTION_FAILED,
        "Failed to create file " + filename + " - GitHub API error",
        ErrorSeverity::ERROR,
        "create_github_file");
  }
  return success;
}
