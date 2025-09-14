# BitTrack – Personal Version Control System  
A lightweight version control system built from scratch in C++—offering full offline control without cloud dependency. Built by a Jordanian Arab developer as a milestone in open-source innovation.

---

## Installation  
Step-by-step instructions to set up the project locally:
1. **Clone the repository**  
```bash
git clone https://github.com/your-username/bittrack.git
```
2. **Navigate to the project directory**
```bash
cd bittrack
```
3. **Build the project using g++**
```bash
./bin/build.production.sh
```

---

## Usage
Run BitTrack via CLI:
```bash
./bittrack init
./bittrack add <file>
./bittrack commit -m "Initial commit"
./bittrack log
```
For a full list of available commands and options:
```bash
./bittrack --help
```

---

## Running Tests
To run unit tests (if applicable), use:
```bash
./bin/build.test.sh
```
Or run compiled test binaries:
```bash
./build/run_tests
```

---

## Project Structure
```bash
bittrack/
│
├── bin/           # Compiled executable binaries
├── build/         # Intermediate build files and artifacts
├── include/       # Header files
├── libs/          # Utility and helper libraries
├── src/           # Core application source code
├── tests/         # Unit and integration test cases
│
├── .bitignore     # BitTrack-specific ignore file
├── .gitignore     # Git ignore rules
├── Makefile       # Build instructions
├── LICENSE        # Project license (MIT)
└── README.md      # Project overview and usage
```

## Server Requirements
To support branch-based integration, your platform must:
1. **Provide an Push Endpoint** (`POST`):
  - Accepts:
  - `upload` → `.zip` file
  - `branch` → string (branch name)
2. **Provide a Pull Endpoint** (`GET`):
  - Accepts:
    - Query parameter: `branch=<branch_name>`
  - Returns the `.zip` file for that branch.
3. **Optional Authentication**
  - Can be public or secured (e.g., Basic Auth, Bearer Token).