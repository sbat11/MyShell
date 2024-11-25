#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

typedef struct Node {
    char* word;
    struct Node* next;
} Node;

Node* createNode(char* word) {
    Node *node = (Node*)malloc(sizeof(Node));
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

    while(ptr != NULL) {
        length++;
        ptr = ptr->next;
    }

    return length;
}

Node* createTokenList(char* line){
    Node* head;
    Node* ptr;
    // hello world0
    int inWord = 0;
    int wordLength = 0;
    int wordCapacity = BUFFER_SIZE;
    char* word = (char*)malloc(wordCapacity * sizeof(char));
    printf("The line is: %s\n", line);
    for(int i = 0; i<strlen(line); i++){
        char c = line[i];
        // Add token to list if come across a space
        if (c == ' ' && inWord) {
            word[wordLength] = '\0';
            //char* temp = realloc(word, wordLength*sizeof(char));
            word = realloc(word, wordLength*sizeof(char));
            if(ptr == NULL){
                head = createNode(word);
                ptr = head;
            }
            else{
                ptr->next = createNode(word);
                ptr = ptr->next;
            }

            inWord = 0;
            wordLength = 0;
        }
        // Add token of special characters to list on their own
        else if (c == '<' || c == '>' || c == '|') {
            if (inWord){
                word[wordLength] = '\0';
                //char* temp = realloc(word, wordLength*sizeof(char));
                word = realloc(word, wordLength*sizeof(char));
                if(ptr == NULL){
                    head = createNode(word);
                    ptr = head;
                }
                else{
                    ptr->next = createNode(word);
                    ptr = ptr->next;
                }
            }
        }
        // Add letters to token
        else {
            word[wordLength++] = line[i];
        }
    }

    return head;
}

void manageCommands(Node* head){
    if(head == NULL) {
        printf("head is Null\n");
    }
    printf("word is: %s", head->word);
    char* command = head->word;
    printf("getting here5\n");

    if (strcmp(command, "cd") == 0) {
        if (getListSize(head) != 2) {
            fprintf(stderr, "cd: only one file name as argument.\n");
        }
    }
    else if (strcmp(command, "pwd") == 0) {
        char cwd[BUFFER_SIZE]; 
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } 
        else {
            perror("pwd");
        }
    }
    else if (strcmp(command, "which") == 0) {

    }
    else if (strcmp(command, "exit") == 0) {
        exit(1);
    }
    else if (0) { //contains slash 

    }
    else{

    }
}

int main(int argc, char *argv[]) {
    if(argc > 2) {
        fprintf(stderr, "Usage: %s [file]\n", argv[0]);
        exit(1);
    }

    int fd, interactive;
    if (argc == 1) {
        fd = STDIN_FILENO;
    }
    else if(argc == 2) {
        fd = open(argv[1], O_RDONLY);
        if(fd < 0) {
            perror("open");
            exit(1);
        }
    } 

    interactive = isatty(fd);

    char* buffer[BUFFER_SIZE];
    int bytes_read = 0;
    
    int lineLength = 0;
    int lineCapacity = BUFFER_SIZE;
    char* line = (char*)malloc(lineCapacity * sizeof(char));

    if(interactive) {printf("Welcome to my shell!\n");}

    while(1) {
        if(interactive){printf("mysh> "); fflush(stdout);}
        
        bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read <= 0){break;}

        buffer[bytes_read] = '\0';
        printf("You typed %s\n", buffer);
        
        for(int i = 0; i<bytes_read; i++){
            
            char c = buffer[i];
            if (c == '\n' || c == '\0') {
                line[lineLength] = '\0';
                printf("The line is %s\n", line);
                //line = realloc(line, lineLength*sizeof(char));
                Node* head = createTokenList(line);
                printf("getting here4\n");
                manageCommands(head);
                printf("getting here6\n");
                lineLength = 0;
            }
            else {
                line[lineLength++] = buffer[i];
            }
        }

        //if(interactive){printf("Exiting Shell");}
    }

    return EXIT_SUCCESS;
}