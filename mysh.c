#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

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

        if (c == ' ' || c == '\0') {  // End of a token
            if (wordLength > 0) {  // Only add non-empty tokens
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
                wordLength = 0;  // Reset for the next word
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
        free(temp->word);
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
    char* command = head->word;
    char* inputFile = NULL;
    char* outputFile = NULL;

    // Parse redirection tokens
    Node* curr = head;
    Node* prev = NULL;
    while (curr != NULL) {
        if (strcmp(curr->word, "<") == 0) {  // Input redirection
            if (curr->next) {
                inputFile = curr->next->word;
                if (prev) prev->next = curr->next->next;
                else head = curr->next->next;
                free(curr->word);
                free(curr);
                curr = (prev) ? prev->next : head;
                continue;
            } else {
                fprintf(stderr, "Error: Missing input file for redirection\n");
                exit(1);
            }
        } else if (strcmp(curr->word, ">") == 0) {  // Output redirection
            if (curr->next) {
                outputFile = curr->next->word;
                if (prev) prev->next = curr->next->next;
                else head = curr->next->next;
                free(curr->word);
                free(curr);
                curr = (prev) ? prev->next : head;
                continue;
            } else {
                fprintf(stderr, "Error: Missing output file for redirection\n");
                exit(1);
            }
        }
        prev = curr;
        curr = curr->next;
    }
    // Handle redirections 


    // Build argument list
    int argc = getListSize(head);
    char** argv = malloc((argc + 1) * sizeof(char));
    Node* current = head;
    for (int i = 0; i < argc; i++) {
        argv[i] = current->word;
        current = current->next;
    }
    argv[argc] = NULL;
    
    int originalStdin = dup(STDIN_FILENO);
    int originalStdout = dup(STDOUT_FILENO);

    // Handle redirection
    if (inputFile) {
        int fd = open(inputFile, O_RDONLY);
        if (fd < 0) {
            perror("Failed to open input file");
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (outputFile) {
        int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (fd < 0) {
            perror("Failed to open output file");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    // Handle bare names
    char* executablePath = command;
    if (!strchr(command, '/')) {  // If it's a bare name
        executablePath = findExecutablePath(command);
    }

    if (!executablePath) {
        fprintf(stderr, "%s: command not found\n", command);
        free(argv);
        return;  // Do not call execv or exit; return to shell loop
    }

    // Fork and execute
    pid_t pid = fork();
    if (pid == 0) {  // Child process
        execv(executablePath, argv);
        perror("execv failed");  // If execv fails
        exit(1);
    } else if (pid > 0) {  // Parent process
        int status;
        waitpid(pid, &status, 0);  // Wait for child process
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Command exited with code %d\n", WEXITSTATUS(status));
        }
    } else {
        perror("fork failed");
    }
    
    dup2(originalStdin, STDIN_FILENO);
    dup2(originalStdout, STDOUT_FILENO);
    close(originalStdin);
    close(originalStdout);

    free(argv);
}

void handleCommands(Node* head, int interactive) {
    if (!head) return;

    char* command = head->word;

    // Check if the command is built-in
    if (strcmp(command, "cd") == 0) {
        if (getListSize(head) != 2) {
            fprintf(stderr, "cd: only one file name as argument.\n");
        } else {
            Node* argNode = head->next;
            if (chdir(argNode->word) != 0) {
                perror("cd");
            }
        }
    } else if (strcmp(command, "pwd") == 0) {
        char cwd[BUFFER_SIZE];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
    } else if (strcmp(command, "which") == 0) {
        char* path = findExecutablePath(command);
        if (path != NULL) {
            printf("%s", path);
        }

    } else if (strcmp(command, "exit") == 0) {

        if (interactive) {
            printf("Exiting my shell.\n");
        }
        exit(0);
    } else {
        executeCommand(head);
    }
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

        executeCommand(firstCommand);  // Execute the first command
        exit(1);  // Should not reach here if execv succeeds
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {  // Second child process
        close(pipefd[1]);           // Close write end of the pipe
        dup2(pipefd[0], STDIN_FILENO);  // Redirect stdin to pipe read end
        close(pipefd[0]);           // Close pipe read end after duplication

        executeCommand(secondCommand);  // Execute the second command
        exit(1);  // Should not reach here if execv succeeds
    }

    // Parent process
    close(pipefd[0]);  // Close both ends of the pipe
    close(pipefd[1]);

    int status1, status2;
    waitpid(pid1, &status1, 0);  // Wait for the first child
    waitpid(pid2, &status2, 0);  // Wait for the second child

    if (WIFEXITED(status2)) {
        printf("Last command exited with status %d\n", WEXITSTATUS(status2));
    }
}

void manageCommands(Node* head, int interactive) {
    if (head == NULL) {
        printf("head is NULL\n");
        return;
    }

    // Detect pipe in the command
    Node* pipeNode = head;
    while (pipeNode != NULL && strcmp(pipeNode->word, "|") != 0) {
        pipeNode = pipeNode->next;
    }

    if (pipeNode != NULL) {  // Pipe detected
        pipeNode->word = NULL;  // Split the token list
        Node* secondCommand = pipeNode->next;
        pipeNode->next = NULL;

        executePipe(head, secondCommand);
        return;
    }

    handleCommands(head, interactive);
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

    int interactive = isatty(fd);

    if (interactive) {
        printf("Welcome to my shell!\n");
    }

    char buffer[BUFFER_SIZE];
    char* line = (char*)malloc(BUFFER_SIZE * sizeof(char));
    int lineLength = 0;
    int lineCapacity = BUFFER_SIZE;

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
            if (c == '\n' || c == '\0') { // End of line
                line[lineLength] = '\0';

                Node* head = createTokenList(line);
                if (head) {
                    manageCommands(head, interactive);
                    freeTokenList(head);
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

    free(line);
    if (fd != STDIN_FILENO) {
        close(fd);
    }

    return EXIT_SUCCESS;
}
