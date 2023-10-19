//(i) Wendy Wu
//(ii) wxw428
//(iii) proj3.c
//(iv) 10/10/2023
//(v) getting http request and send back a response and optional content if passed requirement.
//will first prompt user to input arguments in combinations of -n, -a, and -d

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
bool nDetected = false;
bool dDetected = false;
bool aDetected = false;
char* port = NULL;
char* document_directory = NULL;
char* auth_token = NULL;
char* httpRequest = NULL;
char* endLine = "\r\n\r\n";
char* rn = "\r\n";
char* res;
char* firstLine = NULL;
char* method = NULL;
char* argument = NULL;
char* httpVersion = NULL;
char* firstRN = NULL;
char* def = "/homepage.html"; //default if arg = "/"
FILE *loc; //local file
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
                usage(argv[0]); 
        }
    }
    if(argv[1] == NULL) //default print message when nothing is entered yet.
    {
        fprintf (stderr,"error: no command line option given\n"); //fprintf is for errors only
        usage (argv[0]); //gives the program usage details in the usage function above
    }
}
bool check400(char* request)
{
   char* copy = (char *)malloc(strlen(request)+1);
   copy[0] = '\0';
   strcpy(copy, request);
   bool isMalformed = false; //initially not malformed
   int count = 1;
   char* current = strstr(copy, rn); //find the first occurrence 
   char* begin = copy;

   char* end = NULL;
   int numFields = 0;
   char* fields[NUM_FIELDS];
   while(current!=NULL)
   {
        int length = current - begin;
        char* point = (char* )malloc(length+1);
        point[0] = '\0';
        strncpy(point, begin, length);
        if(count == 1)
        {
            char* firstLine = (char* )malloc(strlen(point)+1);
            firstLine[0] = '\0';
            strcpy(firstLine, point);
            while(strtok(firstLine, " ")!=NULL)
            {
                if(numFields == 0)
                {
                    firstLine = NULL;
                }
                numFields++;
            }
            if(numFields!=NUM_FIELDS)
            {
                isMalformed = true;
                break;
            }
        }
        if(strchr(point, '\r')!=NULL || strchr(point, '\n')!=NULL)
        {
            isMalformed = true;
            break;
        }
        begin = current + strlen(rn);
        current = strstr(begin, rn);
        if(current == NULL)
        {
            break;
        }
        count++;
   }
    return isMalformed;
}   
void parseHTTP()
{
    //parse the first line of the http request into METHOD, ARGUMENT, and HTTP/VERSION
    //parse first line
   int count = 1;
   firstRN = strstr(httpRequest, rn); //find the first occurrence 
   int length = firstRN - httpRequest;
   firstLine= (char* )malloc(length+1);
   firstLine[0] = '\0';
   strncpy(firstLine, httpRequest, length);
   //parse method
   char* meth = strtok(firstLine, " ");
   method = (char* )malloc(strlen(meth) + 1);
   method[0] = '\0';
   int mLength = strlen(meth);
   strcpy(method, meth);
   //argument
   char* arg = strtok(NULL, " ");
   argument = (char* )malloc(strlen(arg) + 1);
   argument[0] = '\0';
   int aLength = strlen(arg);
   strcpy(argument, arg);
   //httpversion
   char* v = strtok(NULL, " ");
   httpVersion = (char* )malloc(strlen(v) + 1);
   httpVersion[0] = '\0';
   int vLength = strlen(v);
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
    else
    {
        return true;
    }
}
bool getOrShutdown(char* method)
{
    //will return false if not get or shutdown
    if((strcmp(method, "GET") != 0) &&(strcmp(method, "SHUTDOWN")!=0))
    {
        return false;
    }
    return true;
}
bool doesSlashBegin(char* argu)
{
    if(argu[0] == '/')
    {
        return true;
    }
    else{
        return false;
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
    while(true)
    {
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
        //from professor's simplication we can always assume it ends with \r\n 
        { 
            break;
        }
    }
    
    //printf("%s", httpRequest); //at this point the httpRequest will be complete
    res = NULL; //initialize message to null for now;
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
   //while true was here
    if(check400(httpRequest))
        { /* 400 malformed request check */ //need more testing but pass the test case on webpage
            res = "HTTP/1.1 400 Malformed Request\r\n\r\n";
             /* write message to the connection */
            if(write (sd2, res,strlen (res)) < 0)
                errexit ("error writing message: %s", res);
            break; //go straight to write message and end this current connection 
        }
    else{
        parseHTTP(); //parse method argument, httpversion
        if(!isHTTP(httpVersion))
        {
            res = "HTTP/1.1 501 Protocol Not Implemented\r\n\r\n";
            /* write message to the connection */
            if(write (sd2, res,strlen (res)) < 0)
                errexit ("error writing message: %s", res);
            break;
        }
        else
        {
            if(!getOrShutdown(method))
            {
                //if not get or shutdown in the method
                res = "HTTP/1.1 405 Unsupported Method\r\n\r\n";
                /* write message to the connection */
                if(write (sd2, res,strlen (res)) < 0)
                    errexit ("error writing message: %s", res);
                break;
            }  
            else if(strcmp(method, "SHUTDOWN") == 0)
            {
                //close (sd);
                //exit (0);
                if(strcmp(argument, auth_token) == 0)
                {
                    res = "HTTP/1.1 200 Server Shutting Down\r\n\r\n";
                    if(write (sd2, res,strlen (res)) < 0)
                        errexit ("error writing message: %s", res);
                    close(sd2);
                    close(sd);
                    exit(0);
                    break; //not needed perhaps
                }
                else{
                    res = "HTTP/1.1 403 Operation Forbidden\r\n\r\n";
                    if(write (sd2, res,strlen (res)) < 0)
                        errexit ("error writing message: %s", res);
                    break;
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
                    /* write message to the connection */
                    if(write (sd2, res,strlen (res)) < 0)
                        errexit ("error writing message: %s", res);
                    break;
                }
                else
                {
                    printf("%s\n", document_directory);
                    if(strcmp(argument, "/") == 0)
                    {
                        loc = fopen(strcat(document_directory, def), "r"); //check the default homepage.html
                    }
                    else{
                        loc = fopen(strcat(document_directory, argument), "r");
                    }   
                    if(loc == NULL) {   
                        res = "HTTP/1.1 404 File Not Found\r\n\r\n";
                        /* write message to the connection */
                        if(write (sd2, res,strlen (res)) < 0)
                            errexit ("error writing message: %s", res);
                        break;
                    }
                    else
                    {
                        //200 ok land
                        res = "HTTP/1.1 200 OK\r\n\r\n";
                        if(write (sd2, res,strlen (res)) < 0)
                            errexit ("error writing message: %s", res);
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
                        fclose(loc);
                        break;
                    }
                }
            }
            else{
                //invalid?
            }
        }
    }
   //}
    /* close the current TCP connection*/
    close (sd2);
    /* close listening connection and exit */
    //close (sd);
    //exit (0);
   }
}
bool isPortInRange(char* port)
{
    int portInt = atoi(port);
    if(MIN_PORT <= portInt && portInt <= MAX_PORT)
    {
        return true;
    }
    else{
        errexit("error in getting the port. It is not in range or not a valid port %s\n", portInt);
        return false;
    }
}
bool isValidDirectory(char* directory)
{
    if(opendir(directory) == NULL)
    {
        //directory does not exist
        errexit("error: cannot open the directory. It is not a valid directory %s\n", directory);
        return false;
    }
    else{
        return true;
    }
    
}
int main(int argc, char *argv [])
{
    parseargs(argc, argv);
    //error check the options
    if(isPortInRange(port) && isValidDirectory(document_directory)) //port is in range and directory naming is correct
    {
        sockets();
    }
    return 0;
}
