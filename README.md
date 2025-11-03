# 🐚 hsh (Hullaah's Shell) — A Lightweight UNIX Shell in C

hsh (Hullaah's shell) is a simple UNIX shell implementation in C, aiming for POSIX compliance and feature parity with `dash`. This project explores the fundamentals of a command-line interpreter, including process management, I/O redirection, tokenizing, and parsing.

---

## 📌 Motivation

The standard UNIX shell is a cornerstone of the operating system, but its internal workings are complex. This project was created to:
- Deepen understanding of **low-level process management** (`fork`, `execve`, `wait`).
- Learn how **command line parsing** and **tokenization** are implemented.
- Gain practical experience with system calls, `PATH` resolution, and I/O.
- Build a foundational piece of systems software from scratch.

---

## ✨ Key Features

### ✅ Core Functionality
- **Interactive Mode:** Provides a `($) ` prompt for user input.
- **Non-Interactive Mode:** Can execute commands piped into it (e.g., `echo "ls -l" | ./hsh`).
- **Command Execution:** Locates and executes commands from the `PATH` environment variable.
- **Argument Handling:** Correctly passes command-line arguments to executed programs.

### ⚙️ Built-in Commands

**Shell State & Environment**
| Built-in | Purpose |
| :--- | :--- |
| **`exit`** | Exits the `hsh` process, optionally with a given status code. |
| **`export`** | Sets an environment variable, marking it for child processes. |
| **`cd`** | Changes the shell's current working directory. |

**Job Control**
| Built-in | Purpose |
| :--- | :--- |
| **`fg`** | Brings a background job to the foreground. |
| **`bg`** | Resumes a stopped background job, keeping it in the background. |
| **`jobs`** | Lists all currently running or stopped background jobs. |

**Command Management**
| Built-in | Purpose |
| :--- | :--- |
| **`alias`** | Creates or lists command aliases. |
| **`unalias`** | Removes one or more aliases. |
| **`type`** | Displays how a command name would be interpreted (e.g., alias, built-in, or external file). |

---

## ⚙️ Core Architecture

- **Input Reading:** Uses `getline(3)` to read command lines of any length.
- **Parsing:** Employs a custom tokenizer to split the input string into tokens (commands and arguments).
- **Execution:** Uses `fork(2)` to create a child process.
- **Command Running:** Uses `execve(2)` in the child process to run the specified command.
- **Process Management:** Uses `waitpid(2)` in the parent process to wait for the child to complete.
- **`PATH` Resolution:** Manually parses the `PATH` environment variable to find executable files.
- **Memory Management:** Carefully manages all memory with `malloc(3)` and `free(3)` to prevent leaks.

---

## 🛠️ Building the Project

### 🔧 Requirements
- GCC (or Clang)
- `make`
- A Unix-like OS (Linux, macOS, WSL)

### 🧪 Build Instructions

1.  Clone the repository:
    ```bash
    git clone https://github.com/Hullaah/hsh.git
    cd hsh
    ```
2.  Build the executable:
    ```bash
    make
    ```
    This will use the included `Makefile` to compile all `.c` files and create the executable `hsh`.

3.  Run the shell:
    ```bash
    ./hsh
    ```

---

## 🧠 What I Learned
- How a UNIX shell works from the ground up.
- The lifecycle of a process (`fork`, `execve`, `wait`).
- Parsing and implementing a `PATH` search algorithm.
- Advanced memory management and error handling in C.
- The importance of POSIX standards and system calls.
- Designing a modular program with clear separation of concerns (parsing, execution, utils).

---

## 👨‍💻 Author
**Umar Adelowo**

Intermediate systems programmer focused on OS, networking, and low-level development.
Aiming to contribute to the Linux Kernel and become a security and systems expert.

[📧 Contact me](mailto:umaradelo1.247@gmail.com)

🌐 GitHub: [@Hullaah](https://github.com/Hullaah)
