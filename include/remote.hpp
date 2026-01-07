#ifndef REMOTE_HPP
#define REMOTE_HPP

#include "github.hpp"

void setRemoteOrigin(const std::string &url);
std::string getRemoteOrigin();
void addRemote(const std::string &name, const std::string &url);
void removeRemote(const std::string &name);
void listRemotes();
std::vector<std::string>
listRemoteBranches(const std::string &remote_name = "origin");
bool deleteRemoteBranch(const std::string &branch_name, const std::string &remote_name = "origin");
void push(const std::string &remote_url = "", const std::string &branch_name = "");
void pull(const std::string &remote_url = "", const std::string &branch_name = "");
void pushToRemote(const std::string &remote_name = "origin", const std::string &branch_name = "main");
void pullFromRemote(const std::string &remote_name = "origin", const std::string &branch_name = "main");
void fetchFromRemote(const std::string &remote_name = "origin");
void cloneRepository(const std::string &url, const std::string &local_path = "");
bool isRemoteConfigured();
std::string getRemoteUrl(const std::string &remote_name = "origin");
void updateRemoteUrl(const std::string &remote_name, const std::string &new_url);
void showRemoteInfo();
std::string base64Encode(const std::string &input);
std::string base64Decode(const std::string &encoded);
std::string getLastPushedCommit();
std::vector<std::string> getUnpushedCommits();
bool isLocalBehindRemote();
void setLastPushedCommit(const std::string &commit);
std::vector<std::string> getCommittedFiles(const std::string &commit);
std::string base64Decode(const std::string &encoded);
void integratePulledFilesWithBittrack(const std::string &commit_sha, const std::vector<std::string> &downloaded_files);
std::string getCommitAuthor(const std::string &commit_hash);
std::string getCommitAuthorEmail(const std::string &commit_hash);
std::string getCommitTimestamp(const std::string &commit_hash);
bool hasUnpushedCommits();
std::string getCommitMessage(const std::string &commit);

#endif
