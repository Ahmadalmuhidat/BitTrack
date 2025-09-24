#ifndef REMOTE_HPP
#define REMOTE_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <curl/curl.h>
#include <chrono>
#include <ctime>

#include "branch.hpp"
#include "commit.hpp"
#include "config.hpp"
#include "stage.hpp"
#include "../libs/miniz/miniz.h"

void set_remote_origin(const std::string& url);
std::string get_remote_origin();
void add_remote(const std::string& name, const std::string& url);
void remove_remote(const std::string& name);
void list_remotes();
void push();
void pull();
void push_to_remote(const std::string& remote_name = "origin", const std::string& branch_name = "main");
void pull_from_remote(const std::string& remote_name = "origin", const std::string& branch_name = "main");
void fetch_from_remote(const std::string& remote_name = "origin");
void clone_repository(const std::string& url, const std::string& local_path = "");
bool is_remote_configured();
std::string get_remote_url(const std::string& remote_name = "origin");
void update_remote_url(const std::string& remote_name, const std::string& new_url);
void show_remote_info();
bool is_github_remote(const std::string& url);
std::string extract_github_info_from_url(const std::string& url, std::string& username, std::string& repository);
bool push_to_github_api(const std::string& token, const std::string& username, const std::string& repo_name);
bool pull_from_github_api(const std::string& token, const std::string& username, const std::string& repo_name);
bool create_github_file(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& filename, const std::string& content, const std::string& message);
bool delete_github_file(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& filename, const std::string& message);
bool validate_github_operation_success(const std::string& response_data);
std::string base64_encode(const std::string& input);
std::string get_last_pushed_commit();
std::vector<std::string> get_unpushed_commits();
bool is_local_behind_remote();
void set_last_pushed_commit(const std::string& commit);
void set_github_commit_mapping(const std::string& bittrack_commit, const std::string& github_commit);
std::string get_github_commit_for_bittrack(const std::string& bittrack_commit);
std::string get_latest_github_commit(const std::string& token, const std::string& username, const std::string& repo_name);
std::vector<std::string> get_committed_files(const std::string& commit);
std::string create_github_blob(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& content);
std::string create_github_tree(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& blob_sha, const std::string& filename);
std::string create_github_tree_with_files(const std::string& token, const std::string& username, const std::string& repo_name, const std::vector<std::string>& blob_shas, const std::vector<std::string>& file_names, const std::string& base_tree_sha = "");
std::string get_github_commit_tree(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& commit_sha);
std::string create_github_commit(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& tree_sha, const std::string& parent_sha, const std::string& message);
std::string get_github_ref(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& ref);
bool update_github_ref(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& ref, const std::string& sha);
std::string get_github_commit_data(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& commit_sha);
bool extract_files_from_github_commit(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& commit_sha, const std::string& commit_data, std::vector<std::string>& downloaded_files);
std::string get_github_tree_data(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& tree_sha);
bool download_files_from_github_tree(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& tree_data, std::vector<std::string>& downloaded_files);
std::string get_github_blob_content(const std::string& token, const std::string& username, const std::string& repo_name, const std::string& blob_sha);
std::string base64_decode(const std::string& encoded);
void integrate_pulled_files_with_bittrack(const std::string& commit_sha, const std::vector<std::string>& downloaded_files);

#endif
