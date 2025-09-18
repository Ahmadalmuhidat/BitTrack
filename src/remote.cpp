#include "../include/remote.hpp"

void set_remote_origin(const std::string &url)
{
  // open the remote file in truncate mode (overwrite)
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
  // create zip file
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

  // iterate through commit files in the zip file
  for (const auto &entry: std::filesystem::recursive_directory_iterator(folder_path))
  {
  if (entry.is_regular_file())
  {
  // get the file path as a string
  std::string file_path = entry.path().string();
  std::string relative_path = std::filesystem::relative(file_path, folder_path).string();

  // calculate the file size
  auto fileSize = std::filesystem::file_size(file_path);
  std::cout << relative_path << ": " << fileSize << " bytes -- " << "\033[32m ok \033[0m" << std::endl;

  // copy file to the compressed folder
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

  // finalize the compressing process
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

// lAdd branch metadata
  curl_formadd(
  &form,
  &last,
  CURLFORM_COPYNAME,
  "branch",
  CURLFORM_COPYCONTENTS,
  CurrentBranch.c_str(),
  CURLFORM_END
  );

  // add branch metadata
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
  std::cout << "File uploaded successfully!" << std::endl;
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

  // append branch as query parameter
  std::string CurrentBranch = get_current_branch();
  std::string remote_url = base_url + "?branch=" + CurrentBranch;

  CURL *curl;
  CURLcode response;
  curl = curl_easy_init();

  if (curl)
  {
  std::ofstream outfile(".bittrack/remote_pull_folder.zip", std::ios::binary);
  if (!outfile.is_open())
  {
  std::cerr << "Error: Could not open .bittrack/remote_pull_folder.zip for writing.\n";
  curl_easy_cleanup(curl);
  return;
  }

  curl_easy_setopt(curl, CURLOPT_URL, remote_url.c_str());
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
  std::cout << "File downloaded successfully to .bittrack/remote_pull_folder.zip" << std::endl;
  // uncomment and use your unzip extraction here:
  // system("unzip -o .bittrack/remote_pull_folder.zip -d .bittrack/objects > /dev/null");
  }

  outfile.close();
  curl_easy_cleanup(curl);
  }
  // remove the temp pull file
  system("rm -f .bittrack/remote_pull_folder.zip");
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
