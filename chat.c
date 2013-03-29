#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


//definitions: 
//default port
#define DEFAULTPORT 7000
//max message size
#define MSG_SIZE 90 
//max number of clients
#define MAX_CLIENTS 10 




//call to exit the system 
void exit(int fd, fd_set *readfds, char fd_array[], int *num_clients){
    int i;
    
    close(fd);
    FD_CLR(fd, readfds); //clear the leaving client from the set
    for (i = 0; i < (*num_clients) - 1; i++)
        if (fd_array[i] == fd)
            break;          
    for (; i < (*num_clients) - 1; i++)
        (fd_array[i]) = (fd_array[i + 1]);
    (*num_clients)--;
}

//main
int main(int argc, char *argv[]) {
   
   int port; 
   int i=0;
   int num_clients = 0;
   int server_sockfd, client_sockfd;
   struct sockaddr_in server_address;
   int addresslen = sizeof(struct sockaddr_in);
   int fd;
   char fd_array[MAX_CLIENTS];
   fd_set readfds, testfds, clientfds;
   char msg[MSG_SIZE + 1];     
   char kb_msg[MSG_SIZE + 10]; 
   
   //Client variables

   int sockfd;
   int result;
   char hostname[MSG_SIZE];
   struct hostent *hostinfo;
   struct sockaddr_in address;
   char alias[MSG_SIZE];
   int clientid;
   

   //Client

   if(argc==2 || argc==4){
     if(!strcmp("-p",argv[1])){
       if(argc==2){
         printf("Invalid parameters.\nUsage: chat [-p PORT] HOSTNAME\n");
         exit(0);
       }else{
         sscanf(argv[2],"%i",&port);
         strcpy(hostname,argv[3]);
       }
     }else{
       port=DEFAULTPORT;
       strcpy(hostname,argv[1]);
     }
     printf("\n*** Client program starting (enter \"quit\" to stop): \n");
     fflush(stdout);
     
     //Create client socket
     sockfd = socket(AF_INET, SOCK_STREAM, 0);

     //Set the socket name
     hostinfo = gethostbyname(hostname);
     //host address information 
     address.sin_addr = *(struct in_addr *)*hostinfo -> h_addr_list;
     address.sin_family = AF_INET;
     address.sin_port = htons(port);

     //Connect socket and server socket 
     if(connect(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0){
       perror("connecting");
       exit(1);
     }
     
     fflush(stdout);
     FD_ZERO(&clientfds);
     FD_SET(sockfd,&clientfds);
     FD_SET(0,&clientfds);//stdin
     
     //Receive server message 
     while (1) {
       testfds=clientfds;
       select(FD_SETSIZE,&testfds,NULL,NULL,NULL);
       
       //Take in data from open socket
       for(fd=0;fd<FD_SETSIZE;fd++){
          if(FD_ISSET(fd,&testfds)){
             if(fd==sockfd){  
                //read data from open socket
                result = read(sockfd, msg, MSG_SIZE);
                //terminate null strings
                msg[result] = '\0';  
                //print received messages
                printf("%s", msg +1);
                
                if (msg[0] == 'X') {                   
                    close(sockfd);
                    exit(0);
                }                             
             }
             //keyboard activity
             else if(fd == 0){
                fgets(kb_msg, MSG_SIZE+1, stdin);
                if (strcmp(kb_msg, "quit\n")==0) {
                    sprintf(msg, "XClient is shutting down.\n");
                    write(sockfd, msg, strlen(msg));
                    //close the client and end program
                    close(sockfd);
                    exit(0);
                }
                //otherwise keep socket open
                else {
                    sprintf(msg, "M%s", kb_msg);
                    write(sockfd, msg, strlen(msg));
                }                                                 
             }          
          }
       }      
     }
   }
   

   //Server 
   if(argc==1 || argc == 3){
     if(argc==3){
       if(!strcmp("-p",argv[1])){
         sscanf(argv[2],"%i",&port);
       //invalid port and hostname
       }else{
         printf("Invalid parameter.\nUsage: chat [-p PORT] HOSTNAME\n");
         exit(0);
       }
      //use default 7400 port
      }else port=DEFAULTPORT;     
     printf("\n*** Server program starting (enter \"quit\" to stop): \n");
     fflush(stdout);

     //Create and name a socket for the server
     server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
     server_address.sin_family = AF_INET;
     server_address.sin_addr.s_addr = htonl(INADDR_ANY);
     server_address.sin_port = htons(port);
     bind(server_sockfd, (struct sockaddr *)&server_address, addresslen);

     //Create connection queue 
     listen(server_sockfd, 1);
     FD_ZERO(&readfds);
     FD_SET(server_sockfd, &readfds);
     FD_SET(0, &readfds);
     
     //Wait for clients and requests
     while (1) {
        testfds = readfds;
        select(FD_SETSIZE, &testfds, NULL, NULL, NULL);
                    
        //Find activity 
        for (fd = 0; fd < FD_SETSIZE; fd++) {
           if (FD_ISSET(fd, &testfds)) {
              //accept new connection requests
              if (fd == server_sockfd) {
                 client_sockfd = accept(server_sockfd, NULL, NULL);                                
                 if (num_clients < MAX_CLIENTS) {
                    FD_SET(client_sockfd, &readfds);
                    fd_array[num_clients]=client_sockfd;
                    //Display client identification
                    printf("Client %d joined\n",num_clients++);
                    fflush(stdout);
                    
                    sprintf(msg,"M%2d",client_sockfd);
                    //display the client identification with byte code 
                    send(client_sockfd,msg,strlen(msg),0);
                 }
                 else {
                    sprintf(msg, "XSorry, Max clients reached.  Try again later.\n");
                    write(client_sockfd, msg, strlen(msg));
                    close(client_sockfd);
                 }
              }
              //Receive messages 
              else if (fd == 0)  {                
                 fgets(kb_msg, MSG_SIZE + 1, stdin);
                 if (strcmp(kb_msg, "quit\n")==0) {
                    sprintf(msg, "XServer is shutting down.\n");
                    for (i = 0; i < num_clients ; i++) {
                       write(fd_array[i], msg, strlen(msg));
                       close(fd_array[i]);
                    }
                    close(server_sockfd);
                    exit(0);
                 }
                 else {
                    sprintf(msg, "M%s", kb_msg);
                    for (i = 0; i < num_clients ; i++)
                       write(fd_array[i], msg, strlen(msg));
                 }
              }
              //Process client activity
              else if(fd) {
                 result = read(fd, msg, MSG_SIZE);
                 
                 if(result==-1) perror("read()");
                 else if(result>0){
                    sprintf(kb_msg,"M%2d",fd);
                    msg[result]='\0';
                    
                    //Concantenate client id with message 
                    strcat(kb_msg,msg+1);                                        
                    
                    //Print to other chat clients 
                    for(i=0;i<num_clients;i++){
                       if (fd_array[i] != fd)
                          write(fd_array[i],kb_msg,strlen(kb_msg));
                    }
                    //display the message
                    printf("%s",kb_msg+1);
                    
                     //Exit Client
                    if(msg[0] == 'X'){
                       exit(fd,&readfds, fd_array,&num_clients);
                    }   
                 }                                   
              }
              //leaving client                  
              else { 
                 exit(fd,&readfds, fd_array,&num_clients);
              }
           }
        }
     }
  }

}

