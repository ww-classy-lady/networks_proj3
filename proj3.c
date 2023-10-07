
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h> 

#define ERROR 1
#define REQUIRED_ARGC 3
#define HOST_POS 1
#define PORT_POS 2
#define PROTOCOL "tcp"
#define BUFLEN 1024

bool nDetected = false;
bool dDetected = false;
bool aDetected = false;
char* port = NULL;
char* document_directory = NULL;
char* auth_token = NULL;
int usage (char *progname)
{
    fprintf (stderr,"usage: %s host port\n", progname);
    exit (ERROR);
}

int errexit (char *format, char *arg)
{
    fprintf (stderr,format,arg);
    fprintf (stderr,"\n");
    exit (ERROR);
}
//void parseargs: parse the command line args that is -u, -o, -d, -q, -r, or unknown args
void parseargs(int argc, char *argv []) 
{
    int opt;
    while((opt = getopt (argc, argv, "n:d:a:")) != -1) //only options are -u url -o filename and -d, -q, -r 
    {
        switch(opt)
        {
            case 'n':
                nDetected = true; //-u is detected, only if there is a url after it
                port = optarg; //set the url to be parsed in parseURL to the next string after -u
                printf("HELLO ITS N! %s\n", port);
                break;
            case 'd': 
                dDetected = true; //-o is detected, only if there is a filename after it
                document_directory = optarg; //set the url to be parsed in parseURL to the next string after -u
                printf("HELLO ITS D! %s\n", document_directory);
                break;
            case 'a':
                aDetected = true; //-d is detected
                auth_token = optarg;
                printf("HELLO ITS A! %s\n", auth_token);
                break;
            case '?':
                fprintf(stderr, "ERROR: unknown command line arguments entered. Please try again.\n./");
                //anything that is not -u, -o, -d, -q, or -r will prompt program to give an error message and terminate program
                exit(0);
            default:
                usage(argv [0]); 
        }
    }
    if(argv[1] == NULL) //default print message when nothing is entered yet.
    {
        fprintf (stderr,"error: no command line option given\n"); //fprintf is for errors only
        usage (argv [0]); //gives the program usage details in the usage function above
    }
}

int main(int argc, char *argv [])
{
    parseargs(argc, argv);
    return 0;
}
