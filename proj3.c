
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
#define QLEN 1

FILE *sp;
bool nDetected = false;
bool dDetected = false;
bool aDetected = false;
char* port = NULL;
char* document_directory = NULL;
char* auth_token = NULL;
char* httpRequest = NULL;
char* endLine = "\r\n\r\n";
char* rn = "\r\n";
char* res= "";
//Parse command line
//get server accepting connections
//print http requests

//parse http request
//work through the GET method and error cases
//implement SHUTDOWN method
//final cleanups
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
                break;
            case 'd': 
                dDetected = true; //-o is detected, only if there is a filename after it
                document_directory = optarg; //set the url to be parsed in parseURL to the next string after -u
                break;
            case 'a':
                aDetected = true; //-d is detected
                auth_token = optarg;
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
int sockets(){

    struct sockaddr_in sin;
    struct sockaddr addr;
    struct protoent *protoinfo;
    unsigned int addrlen;
    int sd, sd2;
    
    /*if (argc != REQUIRED_ARGC)
        usage (argv [0]);
*/
    /* determine protocol */
    if ((protoinfo = getprotobyname (PROTOCOL)) == NULL)
        errexit ("cannot find protocol information for %s", PROTOCOL);

    /* setup endpoint info */
    memset ((char *)&sin,0x0,sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons ((u_short)atoi(port));

    /* allocate a socket */
    /*   would be SOCK_DGRAM for UDP */
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0)
        errexit("cannot create socket", NULL);

    /* bind the socket */
    if (bind (sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit ("cannot bind to port %s", port);

    /* listen for incoming connections */
    if (listen (sd, QLEN) < 0)
        errexit ("cannot listen on port %s\n", port);
/* accept, parse, write, and close(sd2) can be repeated to accept more connections from the listening socket.*/
    /* accept a connection */
    addrlen = sizeof (addr);
    sd2 = accept (sd,&addr,&addrlen);
    if (sd2 < 0)
        errexit ("error accepting connection", NULL);
    /* Parse the http request to see what we do with it*/
    /* parse the http request using read */
    int totalSize = 0; //temporarily set the totalSize of the http Request to BUFLEN
    httpRequest = (char *)malloc(BUFLEN); //allocate size BUFLEN for http Request
    httpRequest[0] = '\0'; //null terminate http request
    int infoRead = 0;
    char store[BUFLEN]; //temporary storage
    /* Get the header by getting piece by piece and temporarily store it in store, then transfer to httpRequest*/
    while((infoRead = read(sd2, store, sizeof(store))) > 0)  //as long as sp has the request, store request in httpRequest
    {
        /* dynamically allocate more space to httpRequest if needed */
        if(totalSize + infoRead >= totalSize - 1) //if the existing request plus the new string exceed allocated space, allocate more
        {
            totalSize = totalSize + infoRead; //allocate another BUFLEN size of space to store string
            httpRequest = (char *)realloc(httpRequest, totalSize); //realloc more space
        } 
        strncat(httpRequest, store, infoRead); //transfer the string from store to httpRequest by infoRead number of bytes
        if(strstr(httpRequest, rn)!=NULL) //check if we reached the end of the http request since it means terminated by \r\n
        //and if we have \r\n at the end we can end reading
        { 
            break;
        }
    }
    
    printf("%s", httpRequest); //at this point the httpRequest will only contain the first line without \r\n\r\n for now
    //prinf("does httprequest have '\r\n\r\n'?%s", strstr(httpRequest, "\r\n"));
    /* list of priority for HTTP response only return 1*/
    /*
    1. 400 Malformed Request
    2. 501 Protocol Not Implemented
    3. 405 Unsupported Method
    4. SHUTDOWN requests:
        (a) 200 Server Shutting Down
        (b) 403 Operation Forbidden
    5. GET requests:
        (a) 406 Invalid Filename
        (b) 200 OK
        (c) 404 File Not Found
    */
    /* write message to the connection */
    if(write (sd2, res,strlen (res)) < 0)
        errexit ("error writing message: %s", res);
    /* close the current TCP connection*/
    close (sd2);
    /* close listening connection and exit */
    close (sd);
    exit (0);

}
int main(int argc, char *argv [])
{
    parseargs(argc, argv);
    sockets();
    if (access(document_directory, F_OK) != -1) {
        // File exists
        printf("File exists in the current directory.\n");
    } else {
        // File doesn't exist
        printf("File does not exist in the current directory.\n");
    }
    return 0;
}
