#include "branch.cpp"
#include "stage.cpp"
#include "commit.cpp"
#include "remote.cpp"

bool inside_BitTrack()
{
  if (!std::filesystem::exists(".bittrack"))
  {
    return false;
  }
  return true;
}

void init()
{
  if (inside_BitTrack())
  {
    std::cerr << "Repository already exists!" << std::endl;
    return;
  }

  std::filesystem::create_directories(".bittrack/objects");
  std::filesystem::create_directories(".bittrack/commits");
  std::filesystem::create_directories(".bittrack/refs/heads");

  std::ofstream HeadFile(".bittrack/HEAD");
  HeadFile.close();

  std::ofstream HistoryFile(".bittrack/commits/history");
  HistoryFile.close();

  add_branch("master");
  switch_branch("master");
  std::cout << "Initialized empty BitTrack repository." << std::endl;
}

void status()
{
  std::cout << "staged files:" << std::endl;
  for (std::string fileName: get_staged_files())
  {
    std::cout << "\033[32m" << fileName << "\033[0m" << std::endl;
  }

  std::cout << "\n" << std::endl;

  std::cout << "unstaged files:" << std::endl;
  for (std::string fileName: get_unstaged_files())
  {
    std::cout << "\033[31m" << fileName << "\033[0m" << std::endl;
  }
}

void stage_file(int argc, const char *argv[], int &i)
{
  if (i + 1 < argc)
  {
    std::string file_to_add = argv[++i];
    stage(file_to_add);
  }
  else
  {
    std::cerr << "Error: --stage requires a file path." << std::endl;
  }
}

void unstage_files(int argc, const char *argv[], int &i)
{
  if (i + 1 < argc)
  {
    std::string fileToRemove = argv[++i];
    unstage(fileToRemove);
  }
  else
  {
    std::cerr << "Error: --stage requires a file path." << std::endl;
  }
}

void commit()
{
  std::cout << "message: ";
  std::string message;
  getline(std::cin, message);

  commit_changes("almuhidat", message);
}

void show_staged_files_hashes()
{
  get_index_hashes();
}

void show_current_commit()
{
  std::cout << get_current_commit() << std::endl;
}

void show_commit_history()
{
  commit_history();
}

void remove_current_repo()
{
  std::filesystem::remove_all(".bittrack");
  std::cout << "Repository removed." << std::endl;
}

void branch_operations(int argc, const char *argv[], int &i)
{
  if (i + 1 < argc)
  {
    std::string subFlag = argv[++i];

    if (subFlag == "-l")
    {
      print_branshes_list();
    }
    else if (subFlag == "-c")
    {
      if (i + 1 < argc)
      {
        std::string name = argv[++i];
        add_branch(name);
      }
      else
      {
        std::cerr << "branch name missing" << std::endl;
      }
    }
    else if (subFlag == "-r")
    {
      if (i + 1 < argc)
      {
        std::string name = argv[++i];
        remove_branch(name);
        switch_branch("master");
      }
      else
      {
        std::cerr << "branch name missing" << std::endl;
      }
    }
  }
  else
  {
    std::cerr << "sub flag missing" << std::endl;
  }
}

void merge(int argc, const char *argv[], int &i)
{
  if (i + 2 < argc)
  {
    std::string first_branch = argv[++i];
    std::string second_branch = argv[++i];
    merge_two_branches(first_branch, second_branch);
  }
}

void checkout(const char *argv[], int &i)
{
  std::string name = argv[++i];
  switch_branch(name);
}

int main(int argc, const char *argv[])
{
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];

    if (arg == "init")
    {
      init();
      break;
    }

    if (inside_BitTrack())
    {
      if (arg == "--status")
      {
        status();
        break;
      }
      else if (arg == "--stage")
      {
        stage_file(argc, argv, i);
        break;
      }
      else if (arg == "--unstage")
      {
        unstage_files(argc, argv, i);
        break;
      }
      else if (arg == "--commit")
      {
        commit();
        break;
      }
      else if (arg == "--staged-files-hashes")
      {
        show_staged_files_hashes();
        break;
      }
      else if (arg == "--current-commit")
      {
        show_current_commit();
        break;
      }
      else if (arg == "--log")
      {
        commit_history();
        break;
      }
      else if (arg == "--remove-repo")
      {
        remove_current_repo();
        break;
      }
      else if (arg == "--branch")
      {
        branch_operations(argc, argv, i);
        break;
      }
      else if (arg == "--checkout")
      {
        checkout(argv, i);
        break;
      }
      else if (arg == "--merge")
      {
        merge(argc, argv, i);
      }
      else if (arg == "--push")
      {
        std::string commit_path = ".bittrack/objects/" + get_current_branch() + "/" + get_current_commit();
        std::string commit_zip_file = ".bittrack/tmp.zip";
        std::string endpoint = "http://localhost:5000/";
        compress_folder(commit_path, commit_zip_file);
        push(endpoint, commit_zip_file);
      }
      else
      {
        std::cerr << "Unknown flag: " << arg << std::endl;
        break;
      } 
    }
    else
    {
      std::cerr << "Not inside a BitTrack repository!" << std::endl;
    }
  }

  return 0;
}