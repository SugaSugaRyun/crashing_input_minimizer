#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

//on off head_tail, mid output
#if 1
#define DDEBUG(func) func
#else
#define DDEBUG(func) NULL;
#endif

#define TRUE 1
#define FALSE 0
char** gargv;
int* gargc;

char *option_i_arg = NULL;
char *option_m_arg = NULL;
char *option_o_arg = NULL;

int errpipe[2];
int inpipe[2];

char crashing_input[4097];

int isFirst = TRUE;
int sigint = FALSE;

pid_t gpid;
char* reduce(char* crash_input);
char* MINIMIZE(char* crash_input);
void read_crash();
void set_pipe();
void child_proc();
void parent_proc(char* headtail);
void make_output_file(char* output_text);

void time_handler(int signum);
void ctrlc_handler(int signum);

int main(int argc, char* argv[]){
    /*
        argv[]
        [0]: information of cimin
        [1~6]: options //i-crash input  m-error message  o-save path
        [7]: target program
        [8~]: target program's arguments
    */

    int option;
    int option_i = 0;
    int option_m = 0;
    int option_o = 0;
    int temp_argc;
    if(argc >7) temp_argc = 7;
    else temp_argc = argc;
    while ((option = getopt(temp_argc, argv, "i:m:o:")) != -1) {
        switch (option) {
            case 'i':
                option_i = 1;
                option_i_arg = optarg;
                break;
            case 'm':
                option_m = 1;
                option_m_arg = optarg;
                break;
            case 'o':
                option_o = 1;
                option_o_arg = optarg;
                break;
            case '?':
                if (optopt == 'i' || optopt == 'm' || optopt == 'o') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option: -%c\n", optopt);
                }
                return EXIT_FAILURE;
            default:
                printf("Unknown option: %c\n", optopt);
                return EXIT_FAILURE;
        }
    }
    if(!option_i || !option_m || !option_o || argc < 8){
        printf("Missing argument(s)!\n");
        printf("usage: ./cimin -i [crashing input path] -m \"[standard error message]\" -o [new output file path ./[test program] [argvs of test program] ... ...");
        return EXIT_FAILURE;
    }

    gargc = &argc;
    //make a new_argv for execv
    if(argc >= 8){
        char **new_argv = (char**)malloc(sizeof(char*) * (argc - 7)); 
        for (int i = 0; i < argc-7; i++) {
            new_argv[i] = strdup(argv[i+7]); 
        }
        new_argv[argc - 7] = NULL; 
        gargv = new_argv;
    }

    read_crash();
    signal(SIGALRM, time_handler);
    signal(SIGINT, ctrlc_handler);

    make_output_file(MINIMIZE(crashing_input));
    return EXIT_SUCCESS;
}

char* reduce(char* crash_input){
    char* tm = crash_input;
    int s = strlen(tm) - 1;
    while(s>0){
        for(int i = 0; i <= strlen(tm) - s; i++){
            if(sigint == TRUE){
                printf("Ctrl + c is entered!\n Save and exit\n");
                printf("Size of current crashing_input : %zd\n", strlen(crash_input));
                return crash_input;
            }
            set_pipe();
            char head[4096] = "\0";
            char tail[4096] = "\0";
            if(i >= 0){
                strncat(head, tm, i);
            }
            if(strlen(tm) >= i + s){
                strncat(tail,tm+i+s, strlen(tm) - (i + s));
            }

            char temp_headtail[4096] = "\0";
            strncat(temp_headtail, head, strlen(head));
            strncat(temp_headtail, tail, strlen(tail));
            temp_headtail[strlen(tm) - s] = '\0';
            DDEBUG(printf("head_tail: %s \n",temp_headtail););

            //timer start
            if(isFirst == TRUE ){
                isFirst = FALSE;
                //start
                alarm(3);
            }
            pid_t child_pid = fork();
            gpid = child_pid;
            if (child_pid == -1) {
                perror("fork failed");
                exit(1);
            }
            else if(child_pid == 0){
                child_proc();
            }
            else{
                parent_proc(temp_headtail);
            }

            int a;
            wait(&a);

            close(errpipe[1]);
            //read from errpipe[0] to match with expected err
            char errmasage[4096];
            ssize_t errread = 0;
            ssize_t message_read = 0;
            errmasage[0] = '\0';
            while ((errread = read(errpipe[0], errmasage+message_read, sizeof(errmasage) - message_read)) > 0) {
                message_read += errread;
            }
            char* ptr = strstr(errmasage, option_m_arg);
            close(errpipe[0]);

            if (ptr != NULL){
                //turn off alarm if find crash!
                printf("\ncrash!crash!crash!crash!crash!crash!crash!\n");
                alarm(0);
                tm = temp_headtail;
                return reduce(tm);
            }
        }
        ///////////////////////////////mid/////////////////////////////////
        for(int i = 0; i <= strlen(tm) - s; i++){
            if(sigint == TRUE){
                printf("Ctrl + c is entered!\n Save and exit\n");
                printf("Size of current crashing_input : %zd\n", strlen(crash_input));
                return crash_input;
            }
            set_pipe();
            char mid[4096] = "\0";
            if(s >= 0){
                strncat(mid,tm+i, s);
            }
            mid[s] = '\0';
            DDEBUG(printf("MID: %s \n",mid););
            pid_t child_pid = fork();
            if (child_pid == -1) {
                perror("fork failed");
                exit(1);
            }
            else if(child_pid == 0){
                child_proc();
            }
            else{
                parent_proc(mid);
            }
            
            int a;
            wait(&a);

            char errmasage[4096];
            ssize_t errread = 0;
            ssize_t message_read = 0;
            errmasage[0] = '\0';
            while ((errread = read(errpipe[0], errmasage+message_read, sizeof(errmasage) - message_read)) > 0) {
                message_read += errread;
            }
            char* ptr = strstr(errmasage, option_m_arg);
            close(errpipe[0]);
            if (ptr != NULL){
                printf("\ncrash!crash!crash!crash!crash!crash!crash!\n");
                alarm(0);
                tm = mid;
                return reduce(tm);
            }
        }
        s = s - 1;
    }
    return tm;
}

char* MINIMIZE(char* crash_input){
    return reduce(crash_input);
}

void set_pipe(){
    //unnamed pipe for stderr
    if(pipe(errpipe) != 0){
        perror("errpipe");
        exit(1);
    }
    //unnamed pipe for stdin
    if(pipe(inpipe) != 0){
        perror("inpipe");
        exit(1);
    }
}

void read_crash(){
    //read initial crashing input
    int crashing = open(option_i_arg,O_RDONLY);
    if (crashing == -1) {
        perror("open");
        exit(1);
    }
    ssize_t nread;
    while ((nread = read(crashing, crashing_input, sizeof(crashing_input))) > 0) {
    }
    if (nread == -1) {
        perror("read");
        exit(1);
    }
    close(crashing);
}

void child_proc(){
    //child
    //stderror redirect
    
    if (dup2(errpipe[1], STDERR_FILENO) == -1) { 
        perror("errdup2");
        exit(EXIT_FAILURE);
    }
    if (dup2(inpipe[0], STDIN_FILENO) == -1) { 
        perror("indup2");
        exit(EXIT_FAILURE);
    }
    //error input
    close(errpipe[0]);
    //inpipie output
    close(inpipe[1]);
    close(STDOUT_FILENO);
    execv(gargv[0], gargv);
    perror("execv failed");
    exit(1);
}

void parent_proc(char* headtail){
    //parent
    close(inpipe[0]);
    close(errpipe[1]);
    //Give stdin to crashing program using stdout
    ssize_t message_len = strlen(headtail);
    ssize_t written = 0;
    ssize_t bytes_written = 0;
    while (written < message_len) {
        bytes_written = write(inpipe[1], headtail + written, message_len - written);
        if (bytes_written <= 0) {
            perror("Error writing to pipe");
            exit(1);
        }
        written += bytes_written;
    }
    close(inpipe[1]);
}

void make_output_file(char* output_text){
    int outfd = open(option_o_arg,O_WRONLY | O_CREAT | O_TRUNC, 0777);
    ssize_t message_len = strlen(output_text);
    ssize_t written = 0;
    ssize_t bytes_written = 0;
    while (written < message_len) {
        bytes_written = write(outfd, output_text + written, message_len - written);
        if (bytes_written <= 0) {
            perror("Error writing to output file");
            exit(1);
        }
        written += bytes_written;
    }
    close(outfd);
}

void time_handler(int signum) {
    printf("This input don't reproduce expected crash!\n");
    kill(gpid, SIGKILL);
    exit(1);
}

void ctrlc_handler(int signum){
    sigint = TRUE;
}
