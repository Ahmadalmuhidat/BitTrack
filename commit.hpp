#ifndef COMMIT_HPP
#define COMMIT_HPP

void StoreSnapshot(const std::string &filePath, const std::string &CommitHash);
void CreateCommitLog(
  const std::string &author,
  const std::string &message,
  const std::unordered_map<std::string,
  std::string> &fileHashes,
  const std::string &commitHash
);
void CommitChanges(const std::string &author, const std::string &message);
void CommitHistory();

#endif