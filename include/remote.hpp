#ifndef REMOTE_HPP
#define REMOTE_HPP

#include <string>
#include <vector>

void set_remote_origin(const std::string& url);
std::string get_remote_origin();
void add_remote(const std::string& name, const std::string& url);
void remove_remote(const std::string& name);
void list_remotes();
void push_to_remote(const std::string& remote_name = "origin", const std::string& branch_name = "master");
void pull_from_remote(const std::string& remote_name = "origin", const std::string& branch_name = "master");
void fetch_from_remote(const std::string& remote_name = "origin");
void clone_repository(const std::string& url, const std::string& local_path = "");
bool is_remote_configured();
std::string get_remote_url(const std::string& remote_name = "origin");
void update_remote_url(const std::string& remote_name, const std::string& new_url);
void show_remote_info();

#endif
