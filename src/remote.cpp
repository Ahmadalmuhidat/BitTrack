#include "../include/remote.hpp"
#include "../include/branch.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <curl/curl.h>
#include "../libs/miniz/miniz.h"

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
