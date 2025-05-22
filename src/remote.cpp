#include "../include/remote.hpp"

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

void push(const std::string &url, const std::string &file_path)
{
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();

  if (curl)
  {
    struct curl_httppost *form = NULL;
    struct curl_httppost *last = NULL;

    // add file to POST request
    curl_formadd(
      &form,
      &last,
      CURLFORM_COPYNAME,
      "file",
      CURLFORM_FILE,
      file_path.c_str(),
      CURLFORM_END
    );

    // set URL and form
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, form);

    // perform the request
    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
      std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
    } else
    {
      std::cout << "File uploaded successfully!" << std::endl;
    }

    // cleanup
    curl_easy_cleanup(curl);
    curl_formfree(form);
  }

  // system("rm -rf .bittrack/tmp.zip");
}