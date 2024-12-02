#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>


#define BUFFER_SIZE 512

int interactive = 0;

typedef struct Node {
    char* word;
    struct Node* next;
} Node;

Node* createNode(char* word) {
    Node* node = (Node*)malloc(sizeof(Node));
    if (node == NULL) {
        perror("Failed to allocate memory for Node");
        return NULL;
    }
    
    node->word = strdup(word);
    if (node->word == NULL) { 
        perror("Failed to allocate memory for word");
        free(node); 
        return NULL;
    }

    node->next = NULL;
    return node;
}
bool matchesWildcard(const char* fileName, const char* pattern) {
    char* star = strchr(pattern, '*');
    if (!star) {
        return strcmp(fileName, pattern) == 0;  // Exact match
    }

    // Split into prefix and suffix
    int prefixLen = star - pattern;
    const char* suffix = star + 1;

    // Match prefix
    if (strncmp(fileName, pattern, prefixLen) != 0) {
        return false;
    }

    // Match suffix
    int fileNameLen = strlen(fileName);
    int suffixLen = strlen(suffix);
    if (fileNameLen < prefixLen + suffixLen) {
        return false;
    }

    return strcmp(fileName + fileNameLen - suffixLen, suffix) == 0;
}
Node* expandWildcard(const char* token) {
    char* star = strchr(token, '*');
    if (!star) {
        return createNode((char*)token);  // No wildcard, return original
    }

    // Separate directory and pattern
    char dirPath[BUFFER_SIZE] = ".";
    const char* pattern = token;

    char* slash = strrchr(token, '/');
    if (slash) {
        strncpy(dirPath, token, slash - token);
        dirPath[slash - token] = '\0';
        pattern = slash + 1;
    }

    DIR* dir = opendir(dirPath);
    if (!dir) {
        perror("opendir");
        return createNode((char*)token);  // Return token unchanged
    }

    struct dirent* entry;
    Node* head = NULL;
    Node* tail = NULL;

    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files unless explicitly matched
        if (entry->d_name[0] == '.' && pattern[0] != '.') {
            continue;
        }

        if (matchesWildcard(entry->d_name, pattern)) {
            // Build fullPath using strcpy and strcat
            char fullPath[BUFFER_SIZE];
            if (strlen(dirPath) + strlen(entry->d_name) + 2 > BUFFER_SIZE) {  // Ensure it fits
                fprintf(stderr, "Path too long: %s/%s\n", dirPath, entry->d_name);
                continue;
            }

            fullPath[0] = '\0';  // Clear the buffer
            strcpy(fullPath, dirPath);  // Copy dirPath
            strcat(fullPath, "/");      // Append the slash
            strcat(fullPath, entry->d_name);  // Append the file name

            Node* newNode = createNode(fullPath);
            if (!newNode) {
                perror("Failed to create node");
                continue;
            }

            if (!head) {
                head = newNode;
                tail = newNode;
            } else {
                tail->next = newNode;
                tail = newNode;
            }
        }
    }

    closedir(dir);

    if (!head) {
        return createNode((char*)token);  // No matches, return token unchanged
    }

    return head;
}
Node* expandTokenList(Node* head) {
    Node* newHead = NULL;
    Node* newTail = NULL;

    while (head) {
        Node* expanded = expandWildcard(head->word);
        Node* temp = head;
        head = head->next;

        if (expanded) {
            if (!newHead) {
                newHead = expanded;
                newTail = expanded;
                while (newTail->next) {
                    newTail = newTail->next;
                }
            } else {
                newTail->next = expanded;
                while (newTail->next) {
                    newTail = newTail->next;
                }
            }
        }

        free(temp->word);
        free(temp);
    }

    return newHead;
}

int getListSize(Node* head) {
    Node* ptr = head;
    int length = 0;

    while (ptr != NULL) {
        length++;
        ptr = ptr->next;
    }

    return length;
}

Node* createTokenList(char* line) {
    Node* head = NULL;
    Node* ptr = NULL;
    int wordLength = 0;
    int wordCapacity = BUFFER_SIZE;
    char* word = (char*)malloc(wordCapacity * sizeof(char));

    if (!word) {
        perror("Failed to allocate memory for word");
        return NULL;
    }

    for (int i = 0; i <= strlen(line); i++) {
        char c = line[i];

        if (c == ' ' || c == '\0' || c == '\n') {
            if (wordLength > 0) { 
                word[wordLength] = '\0';
                Node* newNode = createNode(word);
                if (!newNode) {
                    free(word);
                    return NULL;
                }
                if (head == NULL) {
                    head = newNode;
                    ptr = head;
                } else {
                    ptr->next = newNode;
                    ptr = ptr->next;
                }

                wordLength = 0;
            }
        } else if (c == '<' || c == '>' || c == '|') {  // Special tokens
            if (wordLength > 0) {  // Add the previous token first
                word[wordLength] = '\0';
                Node* newNode = createNode(word);
                if (!newNode) {
                    free(word);
                    return NULL;
                }
                if (head == NULL) {
                    head = newNode;
                    ptr = head;
                } else {
                    ptr->next = newNode;
                    ptr = ptr->next;
                }
                wordLength = 0;
            }

            // Add the special token
            word[0] = c;
            word[1] = '\0';
            Node* newNode = createNode(word);
            if (!newNode) {
                free(word);
                return NULL;
            }
            
            if (head == NULL) {
                head = newNode;
                ptr = head;
            } else {
                ptr->next = newNode;
                ptr = ptr->next;
            }
        } else {  // Part of a regular token
            if (wordLength >= wordCapacity - 1) {  // Resize if needed
                wordCapacity *= 2;
                word = realloc(word, wordCapacity * sizeof(char));
                if (!word) {
                    perror("Failed to reallocate memory for word");
                    return NULL;
                }
            }
            word[wordLength++] = c;
        }
    }

    free(word);
    return head;
}

void freeTokenList(Node* head) {
    Node* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        if(temp -> word){
            free(temp->word);
        }
        free(temp);
    }
}

char* findExecutablePath(const char* command) {
    const char* searchPaths[] = {"/usr/local/bin", "/usr/bin", "/bin", NULL};
    static char fullPath[BUFFER_SIZE];  // Buffer to store the full path

    for (int i = 0; searchPaths[i] != NULL; i++) {
        fullPath[0] = '\0';
        strcpy(fullPath, searchPaths[i]);
        if (fullPath[strlen(fullPath) - 1] != '/') {
            strcat(fullPath, "/");
        }
        strcat(fullPath, command);
        if (access(fullPath, X_OK) == 0) {
            return fullPath;  // Return if executable is found
        }
    }
    return NULL;  // Command not found
}

void executeCommand(Node* head) {
    if (!head) return;

    char* inputFile = NULL;
    char* outputFile = NULL;
    // < hello.txt cat a.txt
    // Parse redirection tokens

    Node* prev = NULL;
    Node* curr = head;
    while (curr != NULL) {
        if (strcmp(curr->word, "<") == 0) {  // Input redirection
            if (curr->next) {
                inputFile = strdup(curr->next->word);

                Node* temp = curr;
                curr = curr->next->next;

                // Free redirection token and file
                free(temp->next->word);
                free(temp->next);
                free(temp->word);
                free(temp);

                if (prev) {
                    prev->next = curr;
                } else {
                    head = curr;  // Update head if the first node is removed
                }
                continue;
            } else {
                fprintf(stderr, "Error: Missing input file for redirection\n");
                exit(1);
            }
        } else if (strcmp(curr->word, ">") == 0) {  // Output redirection
            if (curr->next) {
                outputFile = strdup(curr->next->word);

                Node* temp = curr;
                curr = curr->next->next;

                // Free redirection token and file
                free(temp->next->word);
                free(temp->next);
                free(temp->word);
                free(temp);

                if (prev) {
                    prev->next = curr;
                } else {
                    head = curr;  // Update head if the first node is removed
                }
                continue;
            } else {
                fprintf(stderr, "Error: Missing output file for redirection\n");
                exit(1);
            }
        }

        prev = curr;
        curr = curr->next;
        
    }
    
    char* command = head->word;

    // Build argument list
    int argc = getListSize(head);
    char** argv = malloc((argc + 1) * sizeof(char*));
    Node* current = head;
    for (int i = 0; i < argc; i++) {
        argv[i] = strdup(current->word);
        current = current->next;
    }
    argv[argc] = NULL;
    
    int originalStdin = dup(STDIN_FILENO);
    int originalStdout = dup(STDOUT_FILENO);

    // Handle input redirection
    if (inputFile) {
        int fd = open(inputFile, O_RDONLY);
        if (fd < 0) {
            perror("Failed to open input file");
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
        free(inputFile);
    }

    // Handle output redirection
    if (outputFile) {
        int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (fd < 0) {
            perror("Failed to open output file");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
        free(outputFile);
    }

    // Find path of executable used for execv()

    char* executablePath = command;
    if (strchr(command, '/') == NULL) {  // Bare name (doesn't contain '/')
        executablePath = findExecutablePath(command);
    }

    // Command not found
    if (!executablePath) {
        fprintf(stderr, "%s: command not found\n", command);
        for (int i = 0; i < argc; i++) {
            free(argv[i]);  
        }
        free(argv);
        return;  // Do not call execv or exit; return to shell loop
    }
    execv(executablePath, argv);
    perror("execv failed");  // If execv fails
    exit(1);

    dup2(originalStdin, STDIN_FILENO);
    dup2(originalStdout, STDOUT_FILENO);
    close(originalStdin);
    close(originalStdout);

    for (int i = 0; i < argc; i++) {
        free(argv[i]);  
    }
    free(argv);
}

int handleCommands(Node* head) {
    if (!head) return 1;  // No command to handle

    char* command = head->word;
    int listSize = getListSize(head);

    // Check if the command is built-in
    if (strcmp(command, "cd") == 0) {
        if (listSize != 2) {
            fprintf(stderr, "cd: only one file name as argument.\n");
        } else {
            Node* argNode = head->next;
            if (chdir(argNode->word) != 0) {
                perror("cd");
            }
        }
        freeTokenList(head);
        return 1;  // Indicate a built-in command was executed
    } else if (strcmp(command, "pwd") == 0) {
        char cwd[BUFFER_SIZE];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
        freeTokenList(head);
        return 1;
    } else if (strcmp(command, "which") == 0) {
        if (listSize != 2) {
            fprintf(stderr, "which: requires exactly one argument.\n");
        } else {
            char* path = findExecutablePath(head->next->word);
            if (path != NULL) {
                printf("%s\n", path);
            } else {
                fprintf(stderr, "%s: not found\n", head->next->word);
            }
        }
        freeTokenList(head);
        return 1;
    } else if (strcmp(command, "exit") == 0) {
        Node* ptr = head->next;
        while (ptr != NULL) {
            printf("%s ", ptr->word);
            ptr = ptr->next;
        }
        printf("\n");
        if (interactive) {
            printf("Exiting my shell.\n");
        }
        freeTokenList(head);
        exit(69);
    }

    // External command; delegate to executeCommand
    executeCommand(head);
    freeTokenList(head);
    return 0;  // Indicate an external command was executed
}


void executePipe(Node* firstCommand, Node* secondCommand) {
    int pipefd[2];  // Pipe file descriptors
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {  // First child process
        close(pipefd[0]);           // Close read end of the pipe
        dup2(pipefd[1], STDOUT_FILENO);  // Redirect stdout to pipe write end
        close(pipefd[1]);           // Close pipe write end after duplication

        handleCommands(firstCommand);  // Execute the first command
        exit(1);  // Should not reach here if execv succeeds
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {  // Second child process
        close(pipefd[1]);           // Close write end of the pipe
        dup2(pipefd[0], STDIN_FILENO);  // Redirect stdin to pipe read end
        close(pipefd[0]);           // Close pipe read end after duplication

        handleCommands(secondCommand);  // Execute the second command
        exit(1);  // Should not reach here if execv succeeds
    }

    // Parent process
    close(pipefd[0]);  // Close both ends of the pipe
    close(pipefd[1]);

    int status1, status2;
    waitpid(pid1, &status1, 0);  // Wait for the first child
    waitpid(pid2, &status2, 0);  // Wait for the second child

    if (WIFEXITED(status2)) {
        int exitCode = WEXITSTATUS(status2);
        if (exitCode == 0) {
            fprintf(stderr, "Last command exited with code 0\n");
        } else if (exitCode == 69){

        } else {
            fprintf(stderr, "Last command exited with code %d\n", exitCode);
        }
    } else if (WIFSIGNALED(status2)) {
        int signalNum = WTERMSIG(status2);
        fprintf(stderr, "Terminated by signal: %d\n", signalNum);
    }
}

// Checks if command has a pipe and calls functions to handle it
void manageCommands(Node* head) {
    if (head == NULL) {
        return;
    }

    head = expandTokenList(head);

    // Detect pipe in the command
    Node* prev = NULL;
    Node* pipeNode = head;
    while (pipeNode != NULL && strcmp(pipeNode->word, "|") != 0) {
        prev = pipeNode;
        pipeNode = pipeNode->next;
    }

    // If prev is NULL that means pipe is first token so throw an error
    if (prev == NULL && pipeNode != NULL) {
        fprintf(stderr, "Error: Pipe must connect two commands\n");
        freeTokenList(head);
        return;
    }

    // Handle pipe
    if (pipeNode != NULL) {  // Pipe detected
        prev->next = NULL;
        Node* secondCommand = pipeNode->next;
        free(pipeNode->word);
        free(pipeNode);

        executePipe(head, secondCommand);
        return;
    }

    // Handle single command
    pid_t pid = fork();
    if (pid == 0) {  // Child process
        int isBuiltIn = handleCommands(head);
        exit(isBuiltIn);  // Child process exits with 1 for built-ins
    }

    // Parent process
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        int exitCode = WEXITSTATUS(status);
        if (exitCode == 0) {
            fprintf(stderr, "Last command exited with code 0\n");
        } else if (exitCode == 69) {
            if (interactive) {
                printf("Exiting shell time to 69 ;)\n");
            }
            exit(0);
        } else if (exitCode == 1) {
            // Built-in command; do not print anything
        } else {
            fprintf(stderr, "Last command exited with code %d\n", exitCode);
        }
    } else if (WIFSIGNALED(status)) {
        int signalNum = WTERMSIG(status);
        fprintf(stderr, "Terminated by signal: %d\n", signalNum);
    }
}


int main(int argc, char* argv[]) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [file]\n", argv[0]);
        exit(1);
    }

    int fd = STDIN_FILENO;

    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
    }

    interactive = isatty(fd);

    if (interactive) {
        printf("Welcome to my shell!\n");
    }

    char buffer[BUFFER_SIZE];
    int lineCapacity = BUFFER_SIZE;
    char* line = (char*)malloc(lineCapacity * sizeof(char));
    int lineLength = 0;
    
    while (1) {
        if (interactive) {
            printf("mysh> ");
            fflush(stdout);
        }

        ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read <= 0) {
            break;
        }

        buffer[bytes_read] = '\0';

        for (ssize_t i = 0; i < bytes_read; i++) {
            char c = buffer[i];
            if (c == '\n' || c == EOF) { // End of line
                line[lineLength] = '\0';

                Node* head = createTokenList(line);
                if (head) {
                    manageCommands(head);
                }

                lineLength = 0;
            } else {
                if (lineLength >= lineCapacity - 1) {
                    lineCapacity *= 2;
                    line = realloc(line, lineCapacity * sizeof(char));
                    if (!line) {
                        perror("Failed to reallocate memory for line");
                        exit(1);
                    }
                }
                line[lineLength++] = c;
            }
        }
    }

    if (lineLength > 0) {
        line[lineLength] = '\0';  // Ensure it's null-terminated

        Node* head = createTokenList(line);
        if (head) {
            manageCommands(head);
        }
    }

    free(line);
    if (fd != STDIN_FILENO) {
        close(fd);
    }

    return EXIT_SUCCESS;
}
