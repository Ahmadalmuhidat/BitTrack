# BitTrack User Manual

**Version 1.0**  
**A Comprehensive Guide to BitTrack Version Control System**

---

## Table of Contents

1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Getting Started](#getting-started)
4. [Core Commands](#core-commands)
5. [Branch Management](#branch-management)
6. [File Operations](#file-operations)
7. [Commit Management](#commit-management)
8. [Diff and Comparison](#diff-and-comparison)
9. [Stash Management](#stash-management)
10. [Tag Management](#tag-management)
11. [Merge Operations](#merge-operations)
12. [Configuration](#configuration)
13. [Remote Operations](#remote-operations)
14. [Hooks System](#hooks-system)
15. [Repository Maintenance](#repository-maintenance)
16. [Ignore System](#ignore-system)
17. [Error Handling](#error-handling)
18. [Advanced Usage](#advanced-usage)
19. [Troubleshooting](#troubleshooting)
20. [Appendix](#appendix)

---

## Introduction

BitTrack is a lightweight, Git-compatible version control system built from scratch in C++. It provides comprehensive version control functionality with a focus on simplicity, performance, and educational value. BitTrack offers full offline control without cloud dependency while maintaining compatibility with Git workflows.

### Key Features

- **Complete Git-like Functionality**: All essential version control operations
- **Offline-First Design**: No cloud dependency required
- **Comprehensive Error Handling**: Detailed error reporting and recovery
- **Modern C++ Implementation**: Clean, well-structured codebase
- **Extensive Testing**: Full test coverage with Google Test framework
- **Cross-Platform Support**: macOS, Linux, and Windows compatibility

---

## Installation

### Prerequisites

- **Operating System**: macOS, Linux, or Windows
- **Compiler**: GCC 7+ or Clang 5+ with C++17 support
- **Dependencies**: 
  - OpenSSL (for hashing)
  - libcurl (for remote operations)
  - zlib (for compression)
  - Google Test (for testing)

### macOS Installation

```bash
# Install dependencies via Homebrew
brew install openssl curl zlib

# Clone and build
git clone https://github.com/Ahmadalmuhidat/bittrack.git
cd bittrack
make compile
```

### Linux Installation

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install build-essential libssl-dev libcurl4-openssl-dev zlib1g-dev

# Clone and build
git clone https://github.com/Ahmadalmuhidat/bittrack.git
cd bittrack
make compile
```

### Windows Installation

```bash
# Install dependencies via vcpkg
vcpkg install openssl curl zlib

# Clone and build
git clone https://github.com/Ahmadalmuhidat/bittrack.git
cd bittrack
make compile
```

### Verification

```bash
# Run tests to verify installation
make test

# Check version
./build/bittrack --help
```

---

## Getting Started

### Initialize Your First Repository

```bash
# Create a new directory for your project
mkdir my-project
cd my-project

# Initialize BitTrack repository
./build/bittrack init
```

This creates a `.bittrack/` directory with the following structure:
```
.bittrack/
├── objects/          # Object storage
├── commits/          # Commit data
├── refs/heads/       # Branch references
├── tags/             # Tag storage
├── hooks/            # Hook scripts
├── config            # Repository configuration
├── HEAD              # Current branch reference
├── history           # Commit history
└── remote            # Remote URL
```

### Basic Workflow

```bash
# 1. Check repository status
./build/bittrack --status

# 2. Stage files for commit
./build/bittrack --stage file1.txt
./build/bittrack --stage file2.txt

# 3. Commit changes
./build/bittrack --commit

# 4. View commit history
./build/bittrack --log
```

---

## Core Commands

### Repository Management

#### Initialize Repository
```bash
./build/bittrack init
```
- Creates a new BitTrack repository
- Sets up directory structure
- Creates initial "main" branch
- **Note**: Must be run in an empty directory

#### Check Repository Status
```bash
./build/bittrack --status
```
- Shows staged files (in green)
- Shows unstaged files (in red)
- Displays current working directory state

#### Remove Repository
```bash
./build/bittrack --remove-repo
```
- **Warning**: Permanently deletes the entire repository
- Removes `.bittrack/` directory and all data
- Use with extreme caution

### Help and Information

#### Show Help
```bash
./build/bittrack --help
```
- Displays comprehensive command reference
- Shows usage examples
- Lists all available options

---

## Branch Management

BitTrack provides comprehensive branch management with Git-compatible operations.

### List Branches
```bash
./build/bittrack --branch -l
```
- Shows all available branches
- Indicates current branch with asterisk (*)
- Displays branch metadata

### Create Branch
```bash
./build/bittrack --branch -c <branch-name>
```
- Creates a new branch
- Validates branch name format
- Sets up branch reference
- **Example**: `./build/bittrack --branch -c feature-login`

### Switch Branch
```bash
./build/bittrack --checkout <branch-name>
```
- Changes to specified branch
- Updates working directory
- Validates branch existence
- **Example**: `./build/bittrack --checkout feature-login`

### Rename Branch
```bash
./build/bittrack --branch -m <old-name> <new-name>
```
- Renames existing branch
- Updates all references
- Validates both old and new names
- **Example**: `./build/bittrack --branch -m feature-login feature-auth`

### Show Branch Information
```bash
./build/bittrack --branch -i <branch-name>
```
- Displays detailed branch information
- Shows commit hash, creation date
- Lists associated files
- **Example**: `./build/bittrack --branch -i main`

### Delete Branch
```bash
./build/bittrack --branch -r <branch-name>
```
- Removes specified branch
- Automatically switches to main if deleting current branch
- Prevents deletion of non-existent branches
- **Example**: `./build/bittrack --branch -r feature-login`

---

## File Operations

### Staging Files

#### Stage Single File
```bash
./build/bittrack --stage <file-path>
```
- Adds file to staging area
- Validates file path and existence
- Updates staging index
- **Example**: `./build/bittrack --stage src/main.cpp`

#### Stage Multiple Files
```bash
./build/bittrack --stage file1.txt
./build/bittrack --stage file2.txt
./build/bittrack --stage src/
```

#### Unstage File
```bash
./build/bittrack --unstage <file-path>
```
- Removes file from staging area
- Keeps file in working directory
- **Example**: `./build/bittrack --unstage src/main.cpp`

### Staging Information

#### Show Staged Files
```bash
./build/bittrack --status
```
- Lists all staged files in green
- Shows file status clearly

#### Show Staged File Hashes
```bash
./build/bittrack --staged-files-hashes
```
- Displays SHA-256 hashes for staged files
- Useful for debugging and verification
- Shows file integrity information

---

## Commit Management

### Create Commit
```bash
./build/bittrack --commit
```
- Prompts for commit message
- Creates commit with author "almuhidat"
- Generates unique commit hash
- Updates branch history
- **Note**: Only staged files are committed

### Commit Information

#### Show Current Commit
```bash
./build/bittrack --current-commit
```
- Displays the hash of the current HEAD commit
- Shows current position in history

#### Show Commit History
```bash
./build/bittrack --log
```
- Displays commit history
- Shows commit messages and metadata
- **Note**: Implementation in progress

### Commit Best Practices

1. **Write Clear Messages**: Use descriptive commit messages
2. **Stage Related Changes**: Group related changes together
3. **Test Before Committing**: Ensure code works before committing
4. **Use Atomic Commits**: One logical change per commit

---

## Diff and Comparison

BitTrack provides comprehensive diff functionality with multiple output formats.

### Working Directory Diff
```bash
./build/bittrack --diff
```
- Compares working directory with last commit
- Shows all modified files
- Displays line-by-line changes

### Staged Changes Diff
```bash
./build/bittrack --diff --staged
```
- Compares staged files with last commit
- Shows what will be committed
- Useful for review before commit

### Unstaged Changes Diff
```bash
./build/bittrack --diff --unstaged
```
- Compares working directory with staged files
- Shows unstaged modifications
- Helps identify what needs staging

### File Comparison
```bash
./build/bittrack --diff <file1> <file2>
```
- Direct file-to-file comparison
- Shows line-by-line differences
- **Example**: `./build/bittrack --diff old.txt new.txt`

### Diff Features

- **Color-coded Output**: 
  - Green for additions
  - Red for deletions
  - Default for context
- **Hunk-based Display**: Groups related changes
- **Binary File Detection**: Handles binary files appropriately
- **Multiple Formats**: Unified, side-by-side, compact views

---

## Stash Management

Stash allows you to temporarily save uncommitted changes and restore them later.

### Create Stash
```bash
./build/bittrack --stash
```
- Saves current uncommitted changes
- Clears working directory
- Generates unique stash ID
- **Note**: Only tracked files are stashed

### List Stashes
```bash
./build/bittrack --stash list
```
- Shows all available stashes
- Displays stash ID, message, branch, timestamp
- Shows stash metadata

### Show Stash Contents
```bash
./build/bittrack --stash show <stash-id>
```
- Displays files and changes in specific stash
- Shows diff of stashed changes
- **Example**: `./build/bittrack --stash show stash-001`

### Apply Stash
```bash
./build/bittrack --stash apply <stash-id>
```
- Restores stashed changes to working directory
- Keeps stash in stash list
- **Example**: `./build/bittrack --stash apply stash-001`

### Pop Stash
```bash
./build/bittrack --stash pop <stash-id>
```
- Applies stash and removes it from list
- Equivalent to apply + drop
- **Example**: `./build/bittrack --stash pop stash-001`

### Drop Stash
```bash
./build/bittrack --stash drop <stash-id>
```
- Deletes specific stash
- Permanently removes stash data
- **Example**: `./build/bittrack --stash drop stash-001`

### Clear All Stashes
```bash
./build/bittrack --stash clear
```
- Deletes all stash entries
- Clears stash directory
- **Warning**: Irreversible operation

---

## Tag Management

Tags provide a way to mark specific points in history as important.

### List Tags
```bash
./build/bittrack --tag -l
```
- Shows all available tags
- Displays tag type and commit reference
- Lists tag metadata

### Create Annotated Tag
```bash
./build/bittrack --tag -a <name> <message>
```
- Creates tag with message and metadata
- Includes author and timestamp
- References current commit
- **Example**: `./build/bittrack --tag -a v1.0 "Release version 1.0"`

### Show Tag Information
```bash
./build/bittrack --tag show <name>
```
- Displays tag details
- Shows commit reference and metadata
- **Example**: `./build/bittrack --tag show v1.0`

### Delete Tag
```bash
./build/bittrack --tag -d <name>
```
- Removes specified tag
- Deletes tag file
- **Example**: `./build/bittrack --tag -d v1.0`

### Tag Types

- **Lightweight Tags**: Simple commit references
- **Annotated Tags**: Rich metadata with author, message, timestamp

---

## Merge Operations

BitTrack provides sophisticated merge functionality with conflict resolution.

### Merge Branches
```bash
./build/bittrack --branch -merge <source> <target>
```
**Description**: Merges source branch into target branch
- **Parameters**:
  - `<source>`: Source branch to merge from
  - `<target>`: Target branch to merge into
- **Example**: `./build/bittrack --branch -merge feature-login main`

### Rebase Branch
```bash
./build/bittrack --branch -rebase <source> <target>
```
**Description**: Rebases source branch onto target branch
- **Parameters**:
  - `<source>`: Source branch to rebase
  - `<target>`: Target branch to rebase onto
- **Example**: `./build/bittrack --branch -rebase feature-login main`

### Merge Features

#### Three-Way Merge Algorithm
- Finds common ancestor between branches
- Performs intelligent conflict resolution
- Preserves both branches' changes where possible
- Handles fast-forward merges automatically

#### Fast-Forward Detection
- Automatically detects when fast-forward is possible
- Updates branch pointer without creating merge commit
- Optimizes merge performance

#### Conflict Resolution
- **Automatic Resolution**: Attempts to resolve conflicts automatically
- **Manual Resolution**: Shows conflict markers for manual editing
- **Conflict Markers**: Uses standard Git conflict format
- **Conflict State**: Tracks ongoing merge operations

#### Merge State Management
- **Merge State File**: Stores merge progress in `.bittrack/MERGE_HEAD`
- **Abort Merge**: Allows cancellation of ongoing merges
- **Continue Merge**: Resume after conflict resolution
- **Merge Commits**: Creates proper merge commits with multiple parents

### Conflict Resolution Process

1. **Identify Conflicts**: BitTrack shows conflicted files
2. **Edit Files**: Resolve conflicts manually using conflict markers
3. **Stage Resolved Files**: Add resolved files to staging area
4. **Complete Merge**: Finish merge process with merge commit

### Merge Result Information
- **Success Status**: Indicates if merge completed successfully
- **Conflict Status**: Reports if conflicts were encountered
- **Conflicted Files**: Lists files that had conflicts
- **Merge Commit**: Shows the created merge commit hash

---

## Configuration

BitTrack supports both global and repository-specific configuration.

### List Configuration
```bash
./build/bittrack --config --list
```
- Shows all configuration values
- Displays scope (global/repository)
- Lists current settings

### Set Configuration
```bash
./build/bittrack --config <key> <value>
```
- Sets key-value pair
- Defaults to repository scope
- **Example**: `./build/bittrack --config user.name "John Doe"`

### Get Configuration
```bash
./build/bittrack --config <key>
```
- Retrieves value for specified key
- Shows "No value set" if not found
- **Example**: `./build/bittrack --config user.name`

### Common Configuration Options

- **user.name**: Your name for commits
- **user.email**: Your email for commits
- **core.editor**: Default text editor
- **merge.tool**: Merge conflict resolution tool

---

## Remote Operations

BitTrack supports remote repository operations for collaboration.

### Show Remote URL
```bash
./build/bittrack --remote -v
```
- Displays configured remote origin
- Shows "No remote origin set" if none
- Lists remote information

### Set Remote URL
```bash
./build/bittrack --remote -s <url>
```
- Configures remote origin
- Validates URL format
- Stores in `.bittrack/remote`
- **Example**: `./build/bittrack --remote -s https://github.com/user/repo.git`

### Add Remote
```bash
./build/bittrack --remote add <name> <url>
```
**Description**: Adds a new remote repository
- **Parameters**:
  - `<name>`: Name for the remote (e.g., "origin", "upstream")
  - `<url>`: Remote repository URL
- **Example**: `./build/bittrack --remote add origin https://github.com/user/repo.git`

### Remove Remote
```bash
./build/bittrack --remote remove <name>
```
**Description**: Removes a configured remote
- **Parameters**: `<name>`: Name of the remote to remove
- **Example**: `./build/bittrack --remote remove origin`

### List Remotes
```bash
./build/bittrack --remote list
```
**Description**: Lists all configured remotes
- Shows remote names and URLs
- Displays remote configuration

### Update Remote URL
```bash
./build/bittrack --remote update <name> <new-url>
```
**Description**: Updates the URL for an existing remote
- **Parameters**:
  - `<name>`: Name of the remote to update
  - `<new-url>`: New remote URL
- **Example**: `./build/bittrack --remote update origin https://new-url.git`

### Show Remote Information
```bash
./build/bittrack --remote info
```
**Description**: Shows detailed information about remotes
- Displays remote configuration details
- Shows connection status

### Push to Specific Remote
```bash
./build/bittrack --remote push <remote-name> <branch-name>
```
**Description**: Pushes to a specific remote and branch
- **Parameters**:
  - `<remote-name>`: Name of the remote
  - `<branch-name>`: Name of the branch to push
- **Example**: `./build/bittrack --remote push origin main`

### Pull from Specific Remote
```bash
./build/bittrack --remote pull <remote-name> <branch-name>
```
**Description**: Pulls from a specific remote and branch
- **Parameters**:
  - `<remote-name>`: Name of the remote
  - `<branch-name>`: Name of the branch to pull
- **Example**: `./build/bittrack --remote pull origin main`

### Fetch from Remote
```bash
./build/bittrack --remote fetch <remote-name>
```
**Description**: Fetches changes from remote without merging
- **Parameters**: `<remote-name>`: Name of the remote to fetch from
- Downloads changes but doesn't merge them
- **Example**: `./build/bittrack --remote fetch origin`

### Clone Repository
```bash
./build/bittrack --remote clone <url> [local-path]
```
**Description**: Clones a remote repository
- **Parameters**:
  - `<url>`: Remote repository URL
  - `[local-path]`: Optional local directory name
- **Example**: `./build/bittrack --remote clone https://github.com/user/repo.git my-project`

### Remote Features

- **URL Validation**: Ensures proper remote URL format
- **Multiple Remotes**: Support for multiple remote repositories
- **GitHub Integration**: Special handling for GitHub repositories
- **Authentication**: Support for GitHub tokens and other auth methods
- **Push/Pull Support**: Full implementation with conflict handling

---

## Hooks System

Hooks allow you to run custom scripts at specific points in the workflow.

### List Hooks
```bash
./build/bittrack --hooks list
```
- Shows installed hooks
- Displays hook types and status
- Lists hook scripts

### Install Hook
```bash
./build/bittrack --hooks install <type> [script]
```
- Installs hook for specified event
- Uses provided script or default template
- Makes hook executable
- **Example**: `./build/bittrack --hooks install pre-commit`

### Uninstall Hook
```bash
./build/bittrack --hooks uninstall <type>
```
- Removes specified hook
- Deletes hook file
- **Example**: `./build/bittrack --hooks uninstall pre-commit`

### Supported Hook Types

- **pre-commit**: Before commit creation
- **post-commit**: After commit creation
- **pre-push**: Before push to remote
- **post-push**: After push to remote
- **pre-merge**: Before branch merge
- **post-merge**: After branch merge
- **pre-checkout**: Before branch switch
- **post-checkout**: After branch switch
- **pre-branch**: Before branch creation
- **post-branch**: After branch creation

### Hook Scripts

Hooks are stored in `.bittrack/hooks/` and should be executable scripts that:
- Return 0 for success
- Return non-zero for failure
- Accept relevant arguments
- Can modify the repository state

---

## Repository Maintenance

BitTrack provides comprehensive maintenance tools for repository optimization.

### Garbage Collection
```bash
./build/bittrack --maintenance gc
```
- Removes unreachable objects
- Compacts repository
- Optimizes storage
- Reduces repository size

### Repack Objects
```bash
./build/bittrack --maintenance repack
```
- Consolidates object files
- Reduces repository size
- Improves performance
- Optimizes storage layout

### Check Repository Integrity
```bash
./build/bittrack --maintenance fsck
```
- Validates object integrity
- Checks for corruption
- Reports issues
- Verifies repository health

### Show Repository Statistics
```bash
./build/bittrack --maintenance stats
```
- Displays repository size
- Shows object counts
- Lists largest files
- Provides usage statistics

### Optimize Repository
```bash
./build/bittrack --maintenance optimize
```
- Performs multiple optimizations
- Improves performance
- Reduces storage usage
- Comprehensive optimization

### Analyze Repository
```bash
./build/bittrack --maintenance analyze
```
- Deep analysis of repository structure
- Identifies potential issues
- Provides recommendations
- Detailed repository insights


### Prune Objects
```bash
./build/bittrack --maintenance prune
```
- Removes unreachable objects
- Cleans up orphaned data
- Reduces repository size
- Frees up disk space

### Find Large Files
```bash
./build/bittrack --maintenance find-large-files [threshold]
```
**Description**: Identifies files larger than specified threshold
- **Parameters**: `[threshold]` - Size threshold in bytes (optional, defaults to 1MB)
- Helps identify files consuming excessive space
- **Example**: `./build/bittrack --maintenance find-large-files 1048576`

### Find Duplicate Files
```bash
./build/bittrack --maintenance find-duplicates
```
**Description**: Identifies duplicate files in the repository
- Detects files with identical content
- Helps optimize storage by removing duplicates
- Reports file paths and sizes

### Clean Ignored Files
```bash
./build/bittrack --maintenance clean-ignored
```
**Description**: Removes files that match ignore patterns
- Cleans up files that should be ignored
- Respects `.bitignore` patterns
- **Warning**: Irreversible operation

### Remove Empty Directories
```bash
./build/bittrack --maintenance remove-empty-dirs
```
**Description**: Removes empty directories from the repository
- Cleans up directory structure
- Preserves `.bittrack` directory
- Improves repository organization

### Compact Repository
```bash
./build/bittrack --maintenance compact
```
**Description**: Performs comprehensive repository compaction
- Runs multiple optimization tasks
- Reduces repository size significantly
- Equivalent to running multiple maintenance operations

### Backup Repository
```bash
./build/bittrack --maintenance backup [backup-path]
```
**Description**: Creates a backup of the current repository
- **Parameters**: `[backup-path]` - Optional backup location (defaults to timestamped directory)
- Creates complete repository copy
- **Example**: `./build/bittrack --maintenance backup /path/to/backup`

### List Backups
```bash
./build/bittrack --maintenance list-backups
```
**Description**: Lists all available repository backups
- Shows backup locations and timestamps
- Helps identify available restore points

### Restore from Backup
```bash
./build/bittrack --maintenance restore <backup-path>
```
**Description**: Restores repository from a backup
- **Parameters**: `<backup-path>` - Path to backup directory
- Replaces current repository with backup
- Creates safety backup of current state
- **Warning**: Irreversible operation

### Check Integrity
```bash
./build/bittrack --maintenance check-integrity
```
**Description**: Performs comprehensive integrity check
- Validates all repository objects
- Checks for corruption and inconsistencies
- Reports detailed health status

---

## Ignore System

BitTrack supports Git-compatible ignore patterns via `.bitignore` files.

### Create .bitignore File
```bash
# Create .bitignore file
touch .bitignore

# Add ignore patterns
echo "*.o" >> .bitignore
echo "build/" >> .bitignore
echo "*.log" >> .bitignore
```

### Ignore Pattern Types

#### Wildcards
```
*.o          # All .o files
*.exe        # All .exe files
*.tmp        # All .tmp files
```

#### Directories
```
build/       # build directory
node_modules/ # node_modules directory
.vscode/     # .vscode directory
```

#### Negation
```
!important.txt  # Don't ignore important.txt
!src/*.cpp      # Don't ignore .cpp files in src/
```

#### Comments
```
# This is a comment
# Ignore compiled files
*.o
```

### Pattern Matching Rules

1. **Precedence**: Later patterns override earlier ones
2. **Negation**: `!` patterns override ignore patterns
3. **Directory**: `/` at end matches directories only
4. **Wildcards**: `*` matches any characters except `/`

---

## Error Handling

BitTrack provides comprehensive error handling with detailed reporting.

### Error Types

#### Repository Errors
- `NOT_IN_REPOSITORY`: Not inside a BitTrack repository
- `REPOSITORY_ALREADY_EXISTS`: Repository already exists
- `REPOSITORY_CORRUPTED`: Repository data is corrupted

#### File System Errors
- `FILE_NOT_FOUND`: File does not exist
- `FILE_ACCESS_DENIED`: Permission denied
- `DIRECTORY_CREATION_FAILED`: Cannot create directory
- `INSUFFICIENT_DISK_SPACE`: Not enough disk space

#### Validation Errors
- `INVALID_ARGUMENTS`: Invalid command arguments
- `INVALID_BRANCH_NAME`: Invalid branch name format
- `INVALID_COMMIT_MESSAGE`: Invalid commit message
- `INVALID_REMOTE_URL`: Invalid remote URL format

#### Network Errors
- `NETWORK_ERROR`: Network connection failed
- `REMOTE_CONNECTION_FAILED`: Cannot connect to remote
- `REMOTE_AUTHENTICATION_FAILED`: Authentication failed
- `REMOTE_UPLOAD_FAILED`: Upload to remote failed

### Error Severity Levels

- **INFO**: Informational messages
- **WARNING**: Non-fatal issues
- **ERROR**: Recoverable errors
- **FATAL**: Unrecoverable errors

### Error Recovery

BitTrack attempts to recover from errors when possible:
- Automatic retry for transient failures
- Graceful degradation for non-critical errors
- Clear error messages with suggested actions
- State preservation during error conditions

---

## Advanced Usage

### Workflow Examples

#### Feature Development
```bash
# Create feature branch
./build/bittrack --branch -c feature-user-auth
./build/bittrack --checkout feature-user-auth

# Make changes and commit
./build/bittrack --stage src/auth.cpp
./build/bittrack --commit

# Merge back to main
./build/bittrack --checkout main
./build/bittrack --merge feature-user-auth main
```

#### Hotfix Workflow
```bash
# Create hotfix branch
./build/bittrack --branch -c hotfix-critical-bug
./build/bittrack --checkout hotfix-critical-bug

# Fix and commit
./build/bittrack --stage src/bugfix.cpp
./build/bittrack --commit

# Merge to main and tag
./build/bittrack --checkout main
./build/bittrack --merge hotfix-critical-bug main
./build/bittrack --tag -a v1.0.1 "Hotfix for critical bug"
```

#### Stash Workflow
```bash
# Work in progress
./build/bittrack --stage src/work.cpp

# Need to switch branches
./build/bittrack --stash

# Switch and work
./build/bittrack --checkout other-branch
# ... make changes ...

# Return and restore work
./build/bittrack --checkout original-branch
./build/bittrack --stash pop
```

### Best Practices

#### Commit Messages
- Use clear, descriptive messages
- Start with imperative mood
- Keep first line under 50 characters
- Add detailed description if needed

#### Branch Naming
- Use descriptive names
- Include feature/type prefix
- Use hyphens for word separation
- Examples: `feature-user-login`, `hotfix-payment-bug`

#### File Organization
- Keep related files together
- Use meaningful directory structure
- Follow project conventions
- Document complex structures

---

## Troubleshooting

### Common Issues

#### Repository Not Found
```
Error: Not inside a BitTrack repository
```
**Solution**: Run `./build/bittrack init` to initialize repository

#### Branch Not Found
```
Error: Branch 'feature-xyz' not found
```
**Solution**: Check branch name with `./build/bittrack --branch -l`

#### Merge Conflicts
```
Error: Merge conflicts detected
```
**Solution**: 
1. Edit conflicted files manually
2. Remove conflict markers
3. Stage resolved files
4. Complete merge

#### Permission Denied
```
Error: Permission denied
```
**Solution**: Check file permissions and ownership

#### Disk Space
```
Error: Insufficient disk space
```
**Solution**: Free up disk space or run `./build/bittrack --maintenance clean`

### Debugging

#### Enable Verbose Output
```bash
# Check repository status
./build/bittrack --status

# View detailed diff
./build/bittrack --diff --staged

# Check repository integrity
./build/bittrack --maintenance fsck
```

#### Repository Inspection
```bash
# View repository structure
ls -la .bittrack/

# Check branch references
cat .bittrack/HEAD

# View commit history
cat .bittrack/commits/history
```

### Performance Issues

#### Large Repository
```bash
# Optimize repository
./build/bittrack --maintenance optimize

# Clean up objects
./build/bittrack --maintenance gc

# Repack objects
./build/bittrack --maintenance repack
```

#### Slow Operations
```bash
# Check repository statistics
./build/bittrack --maintenance stats

# Analyze repository
./build/bittrack --maintenance analyze

# Find large files
./build/bittrack --maintenance find-large-files
```

---

## Appendix

### Command Reference

#### Repository Commands
| Command | Description |
|---------|-------------|
| `init` | Initialize repository |
| `--status` | Show repository status |
| `--remove-repo` | Delete repository |

#### File Commands
| Command | Description |
|---------|-------------|
| `--stage <file>` | Stage file |
| `--unstage <file>` | Unstage file |
| `--staged-files-hashes` | Show staged file hashes |

#### Commit Commands
| Command | Description |
|---------|-------------|
| `--commit` | Create commit |
| `--current-commit` | Show current commit |
| `--log` | Show commit history |

#### Branch Commands
| Command | Description |
|---------|-------------|
| `--branch -l` | List branches |
| `--branch -c <name>` | Create branch |
| `--branch -r <name>` | Remove branch |
| `--branch -m <old> <new>` | Rename branch |
| `--branch -i <name>` | Show branch info |
| `--checkout <name>` | Switch branch |

#### Diff Commands
| Command | Description |
|---------|-------------|
| `--diff` | Show working directory diff |
| `--diff --staged` | Show staged diff |
| `--diff --unstaged` | Show unstaged diff |
| `--diff <file1> <file2>` | Compare files |

#### Stash Commands
| Command | Description |
|---------|-------------|
| `--stash` | Create stash |
| `--stash list` | List stashes |
| `--stash show <id>` | Show stash contents |
| `--stash apply <id>` | Apply stash |
| `--stash pop <id>` | Pop stash |
| `--stash drop <id>` | Drop stash |
| `--stash clear` | Clear all stashes |

#### Tag Commands
| Command | Description |
|---------|-------------|
| `--tag -l` | List tags |
| `--tag -a <name> <msg>` | Create annotated tag |
| `--tag -d <name>` | Delete tag |
| `--tag show <name>` | Show tag info |

#### Merge Commands
| Command | Description |
|---------|-------------|
| `--branch -merge <src> <tgt>` | Merge branches |
| `--branch -rebase <src> <tgt>` | Rebase branch |

#### Config Commands
| Command | Description |
|---------|-------------|
| `--config --list` | List configuration |
| `--config <key> <value>` | Set configuration |
| `--config <key>` | Get configuration |

#### Remote Commands
| Command | Description |
|---------|-------------|
| `--remote -v` | Show remote URL |
| `--remote -s <url>` | Set remote URL |
| `--remote add <name> <url>` | Add remote |
| `--remote remove <name>` | Remove remote |
| `--remote list` | List remotes |
| `--remote update <name> <url>` | Update remote URL |
| `--remote info` | Show remote info |
| `--remote push <name> <branch>` | Push to remote |
| `--remote pull <name> <branch>` | Pull from remote |
| `--remote fetch <name>` | Fetch from remote |
| `--remote clone <url> [path]` | Clone repository |

#### Hooks Commands
| Command | Description |
|---------|-------------|
| `--hooks list` | List hooks |
| `--hooks install <type>` | Install hook |
| `--hooks uninstall <type>` | Uninstall hook |

#### Maintenance Commands
| Command | Description |
|---------|-------------|
| `--maintenance gc` | Garbage collection |
| `--maintenance repack` | Repack objects |
| `--maintenance fsck` | Check integrity |
| `--maintenance stats` | Show statistics |
| `--maintenance optimize` | Optimize repository |
| `--maintenance analyze` | Analyze repository |
| `--maintenance prune` | Prune objects |
| `--maintenance find-large-files` | Find large files |
| `--maintenance find-duplicates` | Find duplicate files |
| `--maintenance clean-ignored` | Clean ignored files |
| `--maintenance remove-empty-dirs` | Remove empty directories |
| `--maintenance compact` | Compact repository |
| `--maintenance backup` | Backup repository |
| `--maintenance list-backups` | List backups |
| `--maintenance restore` | Restore from backup |
| `--maintenance check-integrity` | Check integrity |

### File Formats

#### .bitignore Format
```
# Comments start with #
*.o              # Ignore .o files
build/           # Ignore build directory
!important.txt   # Don't ignore important.txt
```

#### Commit Message Format
```
Short description (under 50 chars)

Detailed description if needed.
Can span multiple lines.
```

### Repository Structure

```
.bittrack/
├── objects/          # Object storage
│   ├── <hash1>      # Object files
│   └── <hash2>      # Object files
├── commits/          # Commit data
│   ├── <commit1>    # Commit files
│   └── history      # Commit history
├── refs/heads/       # Branch references
│   ├── main       # Master branch
│   └── <branch>     # Other branches
├── tags/             # Tag storage
│   └── <tag>        # Tag files
├── hooks/            # Hook scripts
│   ├── pre-commit   # Pre-commit hook
│   └── post-commit  # Post-commit hook
├── config            # Repository configuration
├── HEAD              # Current branch reference
├── history           # Commit history
└── remote            # Remote URL
```

### Error Codes

| Code | Description |
|------|-------------|
| 0 | Success |
| 1 | Not in repository |
| 2 | Repository already exists |
| 3 | File not found |
| 4 | Directory creation failed |
| 5 | File write error |
| 6 | File read error |
| 7 | Invalid arguments |
| 8 | Invalid branch name |
| 9 | Branch not found |
| 10 | Invalid commit message |
| 11 | Invalid remote URL |
| 12 | Network error |
| 13 | Unexpected exception |
| 14 | Validation error |
| 15 | Filesystem error |
| 16 | Permission error |
| 17 | Config error |
| 18 | Merge conflict |
| 19 | Stash error |
| 20 | Tag error |
| 21 | Hook error |
| 22 | Maintenance error |
| 23 | Uncommitted changes |
| 24 | Internal error |
| 25 | Memory allocation failed |
| 26 | Missing arguments |
| 27 | Invalid file path |
| 28 | Repository corrupted |
| 29 | Branch already exists |
| 30 | Cannot delete current branch |
| 31 | No commits found |
| 32 | File access denied |
| 33 | File copy error |
| 34 | File delete error |
| 35 | Insufficient disk space |
| 36 | Staging failed |
| 37 | Commit failed |
| 38 | Checkout failed |
| 39 | Remote connection failed |
| 40 | Remote authentication failed |
| 41 | Remote upload failed |
| 42 | Remote download failed |

### License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

### Support

For support and questions:
1. Check the [Issues](https://github.com/Ahmadalmuhidat/bittrack/issues) page
2. Create a new issue with detailed information
3. Include system information and error messages

### Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

---

**BitTrack User Manual v1.0**  
*Last updated: October 2025*
