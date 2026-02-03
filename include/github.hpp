#ifndef GITHUB_HPP
#define GITHUB_HPP

#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include <regex>

#include "config.hpp"
#include "error.hpp"
#include "../include/tag.hpp"

bool isGithubRemote(const std::string &url);
std::string extractInfoFromGithubUrl(
    const std::string &url,
    std::string &username,
    std::string &repository);
bool pushToGithub(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &branch_name);
bool pullFromGithub(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &branch_name = "");
bool deleteGithubFile(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &filename,
    const std::string &message);
bool validateGithubOperationSuccess(const std::string &response_data);
void setGithubCommitMapping(
    const std::string &bittrack_commit,
    const std::string &github_commit);
std::string getGithubCommitForBittrack(
    const std::string &bittrack_commit);
std::string getLatestGithubCommit(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name);
std::string createGithubBlob(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &content);
std::string createGithubTreeWithFiles(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::vector<std::string> &blob_shas,
    const std::vector<std::string> &file_names,
    const std::string &base_tree_sha = "");
std::string getGithubCommitTree(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &commit_sha);
std::string createGithubCommit(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &tree_sha,
    const std::string &parent_sha,
    const std::string &message,
    const std::string &author_name,
    const std::string &author_email,
    const std::string &timestamp);
std::string getGithubLastCommithash(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &ref);
bool updateGithubReferance(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &ref,
    const std::string &sha);
bool createGithubReferance(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &ref,
    const std::string &sha);
std::string getGithubCommitData(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &commit_sha);
bool extractFilesFromGithubCommit(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &commit_sha,
    const std::string &commit_data,
    std::vector<std::string> &downloaded_files);
std::string getGithubTreeData(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &tree_sha);
bool downloadFilesFromGithubTree(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &tree_data,
    std::vector<std::string> &downloaded_files);
std::string getGithubBlobContent(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &blob_sha);
std::string getCommitMessage(const std::string &commit);
bool pushTagToGithub(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &tag_name);
bool pullTagsFromGithub(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name);
std::string createGithubTagObject(
    const std::string &token,
    const std::string &username,
    const std::string &repo_name,
    const std::string &tag_name,
    const std::string &commit_sha,
    const std::string &message,
    const std::string &tagger_name,
    const std::string &tagger_email);

#endif