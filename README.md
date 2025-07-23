# BitTrack  
*A lightweight version control system built from scratch in C++—offering full offline control without cloud dependency. Built by a Jordanian Arab developer as a milestone in open-source innovation.*

---

## Features  
Core functionalities of the project:
- **Repository Management**: Create, initialize, and manage repositories easily via CLI  
- **Branching & Committing**: Add branches, stage files, and commit changes locally  
- **Push/Pull Mechanism**: Simple local/offline push and pull operations for collaboration  
- **Issue Tracking**: Built-in issue system and repository-level notes  

---

## Tech Stack  
Technologies and tools used in this project:
- C++  
- Custom CLI Parser  
- File-based storage system (no external DB)  
- Zlib (for delta compression)  

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
./bittrack push
./bittrack pull
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
