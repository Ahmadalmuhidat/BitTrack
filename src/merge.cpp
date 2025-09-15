#include "../include/merge.hpp"
#include "../include/commit.hpp"
#include "../include/branch.hpp"
#include "../include/stage.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

MergeResult merge_branches(const std::string& source_branch, const std::string& target_branch)
{
  MergeResult result;
  
  // Check if branches exist
  std::string source_commit = get_branch_commit(source_branch);
  std::string target_commit = get_branch_commit(target_branch);
  
  if (source_commit.empty() || target_commit.empty())
  {
    result.message = "One or both branches do not exist";
    return result;
  }
  
  // Check if already up to date
  if (source_commit == target_commit)
  {
    result.success = true;
    result.message = "Already up to date";
    return result;
  }
  
  // Check for fast-forward merge
  if (is_fast_forward(source_branch, target_branch))
  {
    result.success = true;
    result.message = "Fast-forward merge";
    result.merge_commit = source_commit;
    return result;
  }
  
  // Perform three-way merge
  std::string merge_base = find_merge_base(source_commit, target_commit);
  if (merge_base.empty())
  {
    result.message = "No common ancestor found";
    return result;
  }
  
  result = three_way_merge(merge_base, target_commit, source_commit);
  
  if (result.success && !result.has_conflicts)
  {
    // Create merge commit
    std::string merge_message = "Merge branch '" + source_branch + "' into " + target_branch;
    create_merge_commit(merge_message, {target_commit, source_commit});
    result.merge_commit = get_current_commit();
  }
  
  return result;
}

MergeResult merge_commits(const std::string& commit1, const std::string& commit2)
{
  MergeResult result;
  
  // Find common ancestor
  std::string merge_base = find_merge_base(commit1, commit2);
  if (merge_base.empty())
  {
    result.message = "No common ancestor found";
    return result;
  }
  
  // Perform three-way merge
  result = three_way_merge(merge_base, commit1, commit2);
  
  return result;
}

MergeResult three_way_merge(const std::string& base, const std::string& ours, const std::string& theirs)
{
  MergeResult result;
  
  // Get files from all three commits
  std::set<std::string> all_files;
  std::vector<std::string> base_files = get_commit_files(base);
  std::vector<std::string> our_files = get_commit_files(ours);
  std::vector<std::string> their_files = get_commit_files(theirs);
  
  all_files.insert(base_files.begin(), base_files.end());
  all_files.insert(our_files.begin(), our_files.end());
  all_files.insert(their_files.begin(), their_files.end());
  
  // Process each file
  for (const auto& file : all_files)
  {
    std::string base_content = get_file_content(base + "/" + file);
    std::string our_content = get_file_content(ours + "/" + file);
    std::string their_content = get_file_content(theirs + "/" + file);
    
    // Check for conflicts
    if (our_content != their_content)
    {
      if (base_content == our_content)
      {
        // They modified, we didn't - take their version
        std::cout << "Auto-merged " << file << std::endl;
      }
      else if (base_content == their_content)
      {
        // We modified, they didn't - take our version
        std::cout << "Auto-merged " << file << std::endl;
      }
      else
      {
        // Both modified - conflict
        result.has_conflicts = true;
        result.conflicted_files.push_back(file);
        // Create conflict file
        std::ofstream conflict_file(file);
        conflict_file << "<<<<<<< HEAD" << std::endl;
        conflict_file << our_content << std::endl;
        conflict_file << "=======" << std::endl;
        conflict_file << their_content << std::endl;
        conflict_file << ">>>>>>> " << "theirs" << std::endl;
        conflict_file.close();
      }
    }
  }
  
  result.success = !result.has_conflicts;
  if (result.has_conflicts)
  {
    result.message = "Merge conflicts detected";
    save_merge_state(result);
  }
  else
  {
    result.message = "Merge completed successfully";
  }
  
  return result;
}

bool has_conflicts()
{
  return is_merge_in_progress() && !get_conflicted_files().empty();
}

void show_conflicts()
{
  std::vector<std::string> conflicted_files = get_conflicted_files();
  
  if (conflicted_files.empty())
  {
    std::cout << "No conflicts found" << std::endl;
    return;
  }
  
  std::cout << "Conflicted files:" << std::endl;
  for (const auto& file : conflicted_files)
  {
    std::cout << "  " << file << std::endl;
  }
}

std::vector<std::string> get_conflicted_files()
{
  std::vector<std::string> conflicted_files;
  
  if (!is_merge_in_progress())
  {
    return conflicted_files;
  }
  
  MergeResult merge_state = load_merge_state();
  return merge_state.conflicted_files;
}

void resolve_file_conflict(const std::string& file_path)
{
  // Remove conflict markers and resolve
  std::ifstream file(file_path);
  std::string content;
  std::string line;
  bool in_conflict = false;
  std::stringstream resolved;
  
  while (std::getline(file, line))
  {
    if (line.find("<<<<<<<") == 0)
    {
      in_conflict = true;
      continue;
    }
    else if (line.find("=======") == 0)
    {
      continue;
    }
    else if (line.find(">>>>>>>") == 0)
    {
      in_conflict = false;
      continue;
    }
    
    if (!in_conflict) {
      resolved << line << std::endl;
    }
  }
  
  file.close();
  
  // Write resolved content
  std::ofstream out_file(file_path);
  out_file << resolved.str();
  out_file.close();
  
  std::cout << "Resolved conflict in " << file_path << std::endl;
}

void abort_merge()
{
  if (!is_merge_in_progress())
  {
    std::cout << "No merge in progress" << std::endl;
    return;
  }
  
  clear_merge_state();
  std::cout << "Merge aborted" << std::endl;
}

void continue_merge()
{
  if (!is_merge_in_progress())
  {
    std::cout << "No merge in progress" << std::endl;
    return;
  }
  
  if (has_conflicts())
  {
    std::cout << "Cannot continue merge with unresolved conflicts" << std::endl;
    return;
  }
  
  MergeResult merge_state = load_merge_state();
  clear_merge_state();
  
  std::cout << "Merge continued successfully" << std::endl;
}

std::string find_merge_base(const std::string& commit1, const std::string& commit2)
{
  // Simple implementation - find common ancestor
  // In a real implementation, this would use a more sophisticated algorithm
  
  std::string current = commit1;
  std::set<std::string> visited;
  
  while (!current.empty() && visited.find(current) == visited.end())
  {
    visited.insert(current);
    
    if (is_ancestor(current, commit2)) {
      return current;
    }
    
    // Get parent commit (simplified)
    current = get_last_commit(current);
  }
  
  return "";
}

bool is_ancestor(const std::string& ancestor, const std::string& descendant)
{
  // Simple implementation - check if ancestor is in the history of descendant
  std::string current = descendant;
  std::set<std::string> visited;
  
  while (!current.empty() && visited.find(current) == visited.end())
  {
    visited.insert(current);
    
    if (current == ancestor)
    {
      return true;
    }
    
    current = get_last_commit(current);
  }
  
  return false;
}

bool is_fast_forward(const std::string& source, const std::string& target)
{
  return is_ancestor(target, source);
}

void save_merge_state(const MergeResult& result)
{
  std::string merge_file = ".bittrack/MERGE_HEAD";
  std::ofstream file(merge_file);
  file << "conflicts=" << (result.has_conflicts ? "true" : "false") << std::endl;
  for (const auto& conflicted_file : result.conflicted_files)
  {
    file << "conflict=" << conflicted_file << std::endl;
  }
  file.close();
}

MergeResult load_merge_state()
{
  MergeResult result;
  std::string merge_file = ".bittrack/MERGE_HEAD";
  
  if (!std::filesystem::exists(merge_file))
  {
    return result;
  }
  
  std::ifstream file(merge_file);
  std::string line;
  
  while (std::getline(file, line))
  {
    if (line.find("conflicts=") == 0)
    {
      result.has_conflicts = (line.substr(10) == "true");
    } else if (line.find("conflict=") == 0)
    {
      result.conflicted_files.push_back(line.substr(9));
    }
  }
  
  file.close();
  return result;
}

void clear_merge_state()
{
  std::string merge_file = ".bittrack/MERGE_HEAD";
  if (std::filesystem::exists(merge_file))
  {
    std::filesystem::remove(merge_file);
  }
}

bool is_merge_in_progress()
{
  return std::filesystem::exists(".bittrack/MERGE_HEAD");
}

std::string get_merge_base_commit(const std::string& branch1, const std::string& branch2)
{
  return find_merge_base(get_last_commit(branch1), get_last_commit(branch2));
}

std::vector<std::string> get_commit_files(const std::string& commit_hash)
{
  std::vector<std::string> files;
  std::string commit_path = ".bittrack/objects/" + get_current_branch() + "/" + commit_hash;
  
  if (!std::filesystem::exists(commit_path))
  {
    return files;
  }
  
  for (const auto& entry : std::filesystem::recursive_directory_iterator(commit_path))
  {
    if (entry.is_regular_file())
    {
      std::string rel_path = std::filesystem::relative(entry.path(), commit_path).string();
      files.push_back(rel_path);
    }
  }
  
  return files;
}

std::string get_file_content(const std::string& file_path)
{
  if (!std::filesystem::exists(file_path))
  {
    return "";
  }
  
  std::ifstream file(file_path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  
  return buffer.str();
}

void create_merge_commit(const std::string& message, const std::vector<std::string>& parents)
{
  // This would integrate with the commit system
  std::cout << "Creating merge commit: " << message << std::endl;
}

void create_conflict_file(const std::string& file_path, const std::string& our_content, const std::string& their_content)
{
  std::ofstream file(file_path);
  file << "<<<<<<< HEAD" << std::endl;
  file << our_content << std::endl;
  file << "=======" << std::endl;
  file << their_content << std::endl;
  file << ">>>>>>> " << "theirs" << std::endl;
  file.close();
}
