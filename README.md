# 💻 MyShell

**MyShell** is a simple command-line shell implemented in C. It supports basic UNIX-style commands, I/O redirection, background execution, and process management using low-level system calls. This project is designed to demonstrate core systems programming concepts like `fork()`, `execvp()`, and signal handling.

---

## 📖 About

MyShell mimics essential features of a UNIX shell and serves as a hands-on exploration of how shells work under the hood. It parses user input, spawns child processes, handles background jobs, and supports built-in commands like `cd` and `exit`.

---

## 🛠 Features

- ✅ Execute external programs via `execvp`
- 🔁 Built-in commands: `cd`, `exit`, and more
- 🎯 Input/output redirection: `>`, `<`
- 🧵 Background process support using `&`
- 🚫 Basic signal handling for `Ctrl+C`
- 📜 Command parsing and tokenization

---

## 🗂 Project Structure

myshell/  
├── myshell.c # Main shell implementation  
├── parser.c # Command parser (optional module)  
├── Makefile # Build instructions  
├── README.md # This file  
