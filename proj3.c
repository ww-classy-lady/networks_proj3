//(i) Wendy Wu
//(ii) wxw428
//(iii) proj3.c
//(iv) 10/10/2023
//(v) proj3.c: getting http request and send back a response and optional content if needed.
//will first prompt user to input arguments in any order of the combination of -n, -a, and -d

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h> 
#include <dirent.h>

#define ERROR 1
#define REQUIRED_ARGC 3
#define NUM_FIELDS 3
#define HOST_POS 1
#define PORT_POS 2
#define PROTOCOL "tcp"
#define BUFLEN 1024
#define QLEN 1
#define httpLength 5
#define MIN_PORT 1025
#define MAX_PORT 65535

FILE *sp;
bool nDetected = false; //the n,d,aDetected fields are checking whether we have received -n, -d, -a in the command line
bool dDetected = false;
bool aDetected = false;
char* port = NULL;
char* document_directory = NULL;
char* auth_token = NULL;
char* httpRequest = NULL;
//char* endLine = "\r\n\r\n";
char* rn = "\r\n"; //check for \r\n using var rn
char* res;
char* firstLine = NULL;
char* method = NULL;
char* argument = NULL;
char* httpVersion = NULL;
char* firstRN = NULL;
char* def = "/homepage.html"; //default if arg = "/"
bool wroteContent = false;
FILE *loc; //local file that will be used to read and send content

//usage: program usage
int usage (char *progname) //program usage
{
    fprintf (stderr,"usage: %s -n port -a auth -d document_directory \n", progname);
    exit (ERROR);
}
//errexit: give an error to command line when detected
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
                nDetected = true; //-n is detected, only if there is a port
                port = optarg; //set port
                break;
            case 'd': 
                dDetected = true; //-d is detected, only if there is document_directory
                document_directory = optarg; //set the document_directory to the argument after -d
                break;
            case 'a':
                aDetected = true; //-a is detected, only if there is auth_token after it
                auth_token = optarg; //store auth_token to argument after -a
                break;
            case '?':
                fprintf(stderr, "ERROR: unknown command line arguments entered. Please try again.\n./");
                //anything that is not -n, -d, or -a will prompt program to give an error message and terminate program
                exit(0);
            default:
                usage(argv[0]); 
        }
    }
    if(argv[1] == NULL) //default print message when nothing is entered yet.
    {
        fprintf (stderr,"error: no command line option given\n"); //fprintf is for errors only
        usage (argv[0]); //gives the program usage details in the usage function above
    }
}
//check400: takes in the http request and checks that there is always \r\n after every line and if that is not followed will throw malformed request to client
bool check400(char* request)
{
   int counter = 1; //line counter
   char* copy = (char *)malloc(strlen(request)+1); //allocate the space amount of space from request to copy
   copy[0] = '\0'; //null terminate
   strcpy(copy, request); //copy the request over to copy so we can iterate through copy
   bool isMalformed = false; //initially not malformed
   char* current = strstr(copy, rn); //find the first occurrence of \r\n. Will always have at least one because will always terminate request with \r\n
   char* begin = copy; //set the begin of the line we will be looking at at that moment
   int numFields = 0; //counter for the number of arguments for the first line, should have 3 in the http request's first line
   while(current!=NULL) //while the pointer to the end is not null
   {
        int length = current - begin; //end - start = length of the current line
        char* point = (char* )malloc(length+1); //point stores the current line by the way
        point[0] = '\0';
        strncpy(point, begin, length); //copy from begin the number of characters we determined from the length as the current line, which point stores
        if(counter == 1) //first line check
        {
            char* firstLine = (char* )malloc(strlen(point)+1);
            firstLine[0] = '\0';
            strcpy(firstLine, point); 
            while(strtok(firstLine, " ")!=NULL) //get the argument separated by a single space
            {
                if(numFields == 0) //if just the first field then we will set the rest of the first params to be NULL 
                {
                    firstLine = NULL;
                }
                numFields++;
            }
            if(numFields!=NUM_FIELDS) //if not 3 fields, it is malformed
            {
                isMalformed = true;
                break;
            }
        }
        if(strchr(point, '\r')!=NULL || strchr(point, '\n')!=NULL) //if there is any \r or \n in the request then that means the \r\n pattern is off and this req is malformed
        {
            isMalformed = true;
            break;
        }
        begin = current + strlen(rn); //move begin to past the line we just examined and the rn too
        current = strstr(begin, rn); //find the pointer to the occurrence of rn in the next line
        if(current == NULL)
        {
            break;
        }
        counter++; //increment line count
   }
    return isMalformed;
}
//parseHTTP: parse the httpRequest so we can store the corresponding fields for method, argument, and httpVersion   
void parseHTTP()
{
    //parse the first line of the http request into METHOD, ARGUMENT, and HTTP/VERSION
    //parse first line
   firstRN = strstr(httpRequest, rn); //find the first occurrence 
   int length = firstRN - httpRequest;
   firstLine= (char* )malloc(length+1);
   firstLine[0] = '\0';
   strncpy(firstLine, httpRequest, length);
   //parse method
   char* meth = strtok(firstLine, " ");
   method = (char* )malloc(strlen(meth) + 1);
   method[0] = '\0';
   strcpy(method, meth);
   //argument
   char* arg = strtok(NULL, " ");
   argument = (char* )malloc(strlen(arg) + 1);
   argument[0] = '\0';
   strcpy(argument, arg);
   //httpversion
   char* v = strtok(NULL, " ");
   httpVersion = (char* )malloc(strlen(v) + 1);
   httpVersion[0] = '\0';
   strcpy(httpVersion, v);
}
//isHTTP: check if the HTTP/VERSION strictly (case sensitive) match the HTTP/ portion for the 5 chracters HTTP/
bool isHTTP(char* version)
{
    char firstFive[httpLength];
    strncpy(firstFive, version , httpLength);
    if(strcmp(firstFive, "HTTP/") !=0)
    {
        return false;
    }
   return true;
}
//getOrShutDown: compares the input method to GET or SHUTDOWN, if not one of them will throw the corresponding error in sockets method
bool getOrShutdown(char* method)
{
    //will return false if not get or shutdown
    if((strcmp(method, "GET") != 0) &&(strcmp(method, "SHUTDOWN")!=0))
    {
        return false;
    }
    return true;
}
//doesSlashBegin: checks if the argument begin with a slash, if not, throw corresponding error in sockets method
bool doesSlashBegin(char* argu)
{
    if(argu[0] == '/')
    {
        return true;
    }
    return false;
}
//sockets: makes the connection to the client, listens for connection/request, send over response. Loop until reach the shutdown with correct argument field as auth_token.
int sockets(){

    struct sockaddr_in sin;
    struct sockaddr addr;
    struct protoent *protoinfo;
    unsigned int addrlen;
    int sd, sd2; //sd is the listening socket. sd2 is the client socket sending over information/reequest
    /*if (argc != REQUIRED_ARGC)
        usage (argv [0]);
*/
    /* determine protocol */
    if((protoinfo = getprotobyname (PROTOCOL)) == NULL)
        errexit("cannot find protocol information for %s", PROTOCOL);

    /* setup endpoint info */
    memset ((char *)&sin,0x0,sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons((u_short)atoi(port));

    /* allocate a socket */
    /*   would be SOCK_DGRAM for UDP */
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if(sd < 0)
        errexit("cannot create socket", NULL);

    /* bind the socket */
    if(bind(sd,(struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit("cannot bind to port %s", port);

    /* listen for incoming connections */
    if(listen(sd, QLEN) < 0)
        errexit("cannot listen on port %s\n", port);
/* accept, parse, write, and close(sd2) can be repeated to accept more connections from the listening socket.*/
    addrlen = sizeof(addr);
    while(true) //keep looping to accept more connections
    {
        wroteContent = false; //this is specifically to track whether content has been written so we avoid duplicate code for write 
        /* accept a connection */
        sd2 = accept (sd,&addr,&addrlen); 
        if (sd2 < 0)
            errexit ("error accepting connection", NULL);
        /* get the http request using read */
        int totalSize = 0; //temporarily set the totalSize of the http Request to BUFLEN
        httpRequest = (char *)malloc(BUFLEN); //allocate size BUFLEN for http Request
        httpRequest[0] = '\0'; //null terminate http request
        int infoRead = 0;
        char store[BUFLEN]; //temporary storage
        /* Get the header by getting piece by piece and temporarily store it in store, then transfer to httpRequest*/
        while((infoRead = read(sd2, store, sizeof(store))) > 0)  //as long as sd2 has the request, store request in httpRequest
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
            //from professor's simplication we can always assume it ends with \r\n 
            { 
                break;
            }
        }
        res = NULL; //initialize message to null for now;
        if(check400(httpRequest))
        { /* 400 malformed request check */ //need more testing but pass the test case on webpage
            res = "HTTP/1.1 400 Malformed Request\r\n\r\n";
        }
        else
        {
            parseHTTP(); //parse method argument, httpversion
            if(!isHTTP(httpVersion))
            {
                //if the HTTP does not start with HTTP/
                res = "HTTP/1.1 501 Protocol Not Implemented\r\n\r\n";
            }
            else
            {
                if(!getOrShutdown(method))
                {
                    //if not get or shutdown in the method
                    res = "HTTP/1.1 405 Unsupported Method\r\n\r\n";
                }  
                else if(strcmp(method, "SHUTDOWN") == 0)
                {
                    if(strcmp(argument, auth_token) == 0)
                    {
                        //if the argument match auth_token, can terminate program
                        res = "HTTP/1.1 200 Server Shutting Down\r\n\r\n";
                        if(write (sd2, res,strlen (res)) < 0)
                            errexit ("error writing message: %s", res);
                        break; //not needed perhaps
                    }
                    else
                    {
                        //keep accepting connections
                        res = "HTTP/1.1 403 Operation Forbidden\r\n\r\n";
                    }
                }
                else if(strcmp(method, "GET") == 0)
                {
                    //step 1:
                    //1. When the requested file does not begin with a “/”, a minimal HTTP response of “HTTP/1.1 406
                    //Invalid Filename\r\n\r\n” must be returned.
                    if(!doesSlashBegin(argument))
                    {
                        res = "HTTP/1.1 406 Invalid Filename\r\n\r\n";
                    }
                    else
                    {
                        if(strcmp(argument, "/") == 0) //if just /, then do the /homepage.html check
                        {
                            loc = fopen(strcat(document_directory, def), "r"); //check the default homepage.html
                        }
                        else
                        {
                            loc = fopen(strcat(document_directory, argument), "r");
                        }   
                        if(loc == NULL) //cannot open the file. it does not exist or cannot be opened
                        {   
                            res = "HTTP/1.1 404 File Not Found\r\n\r\n";
                        }
                        else
                        {
                            //200 ok land, write content
                            res = "HTTP/1.1 200 OK\r\n\r\n";
                            if(write(sd2, res,strlen (res)) < 0)
                                errexit ("error writing message: %s", res);
                            wroteContent = true;
                            char buffer[BUFLEN]; //temporary buffer to store binary data pieces
                            size_t bufferSize = sizeof(buffer);
                            size_t bytesRead = 0; //keep track of the bytesRead
                            while(!feof(loc)) //as long as we are not at the end of the file for socket descriptor, binary data need to be continuous read and written
                            {
                                bytesRead = fread(buffer, 1, bufferSize, loc); //read one byte of data from socket descriptor file
                                if(bytesRead > 0) //got info to write because bytesRead is greater than 0
                                {
                                    send(sd2, buffer, bytesRead, 0); //write 1 byte of data to file
                                }   
                            }
                            fclose(loc); //close the file
                        }
                    }
                }
                else
                {
                    //invalid?
                }
            }
        }
        /* write to current TCP connection if just a message and no content*/
        if(!wroteContent) //if content did not get written in then we write, else since we know
        //in the 200 ok land we already wrote http response header, no need to write it again.
        {
            if(write (sd2, res,strlen (res)) < 0)
                errexit ("error writing message: %s", res);
        }
        /* close the current TCP connection*/
        close(sd2);
   }
   /* close listening connection and exit */
    close(sd2);
    close(sd);
    exit(0);
}
//isPortInRange: checks if port is in range [1025, 65535]
bool isPortInRange(char* port)
{
    int portInt = atoi(port);
    if(MIN_PORT <= portInt && portInt <= MAX_PORT)
    {
        return true;
    }
    else
    {
        errexit("error in getting the port. It is not in range or not a valid port %s\n", port);
        return false;
    }
}
//isValidDirectory: checks if we can open the directory or not. basically check if document_directory is a valid input or not
bool isValidDirectory(char* directory)
{
    if(directory[0]!='/')
    {
        //directory does not exist
        errexit("error: directory does not start with /. %s\n", directory);
        return false;
    }
    return true;
    
}
//main: parse, then do the socket.
int main(int argc, char *argv [])
{
    parseargs(argc, argv);
    //error check the options
    if(nDetected&&dDetected&&aDetected) //only do the next step: sockets if we have port, auth_token, and document_directory
    {
        if(isPortInRange(port) && isValidDirectory(document_directory)) //port is in range and directory naming is correct
        {
            sockets();
        }
    }
    else
    {
        errexit("Missing some arguments such as -n, -d, or -a",NULL); 
    }
    return 0;
}
