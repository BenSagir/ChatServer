#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#define BUF 4096                                        //max size of data from the user
#define MAX_SOCK 128
//======================================== queue area ===================================================

typedef struct msg_node{                                // a node of a linked-list represnt a queue
    int sender;
    int dest;
    char* msg;
    struct msg_node* next;
}msg_node;

msg_node* msgCreate(int send){                          //constructor for the msg_node
    msg_node * toReturn = (msg_node*)calloc(1,sizeof(msg_node));
    if(toReturn == NULL){
        perror("calloc() failed");
        return NULL;
    }
    toReturn->msg = (char*)calloc(1,BUF);
    if(toReturn->msg == NULL){
        perror("calloc() failed");
        return NULL;
    }
    toReturn->sender = send;
    toReturn->dest = -1;
    toReturn->next = NULL;
    return toReturn;
}

int writeMsg(msg_node* toSend){
    int rc;
    printf("server is ready to write to socket %d\n", toSend->dest);  
    char guest[20];                                     //init guest signature string
    sprintf(guest, "guest%d : ", toSend->sender);       //present the message for the sender fd
    rc = write(toSend->dest ,guest, strlen(guest)+1);   //write guest and number to this fd
    if(rc < 0)
        return -1;
    rc = write(toSend->dest, toSend->msg, strlen(toSend->msg)+1);    //write the message to this fd
    if(rc < 0)
        return -1;
    free(toSend->msg);                                  //free the allocated message
    free(toSend);
    return 1;
}
void clearMem(msg_node* clean){
    if(clean == NULL)
        return;
    msg_node* p = clean;
    msg_node* q = p;
    while(q != NULL){
        free(p->msg);
        free(p);
        q = q->next;
    }
}
//========================================== SIGINT handler ==========================================

static int wsocket;                                     //static in to save the index of the main socket
static msg_node* mqueue;                                //a pointer to the main queue
static msg_node* rqueue;                                //a pointer to the resend queue
static int sockToClose[MAX_SOCK];                       //open socket array 
void pshutdown(int signum){                             //handler to close the program upon Ctrl+C
    printf("\nchat is close now. thanks for using\n");
    for(int i = 0; i<MAX_SOCK; i++)                     //close all open socket fd
        if(sockToClose[i] == 1)
            close(i);
    close(wsocket);                                     //close for the main socket
    clearMem(mqueue);                                   //clear the main queue
    clearMem(rqueue);                                   //clear the resend queue
    exit(EXIT_SUCCESS);                                     
}

//========================================== main starts here ==========================================

int main(int argc, char* argv[]){
    signal(SIGINT, pshutdown);                          //in case of Ctrl+C from the terminal go to program shutdown handler
    if(argc != 2){                                      //one argument only is excepted
        fprintf(stderr, "Usage: ./server <port>\n");    //usage error
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);                           //port is taken from the argv
    if(port <= 0){                                      //in case port is negative number
        fprintf(stderr, "Usage: ./server <port>\n");    //usage error
        exit(EXIT_FAILURE);
    }

//========================================== variables initilize ==========================================

    struct sockaddr_in serv_addr;                       //server struct
    fd_set masterSet, c_rfd, c_wfd;                     //master set of file descriptors and copy for read and write
    msg_node *qhead, *qtail;                            //the head and tail of the queue
    msg_node *resndqhead, *resndqtail;                  //the head and tail of the resend queue
    int qsize = 0, resndqsize = 0, openCon = 0;         //qsize - the size of the queue. resndqsize - the size of the resend queue .openCon - amount of open connection
    int main_socket, client_socket;                     //main and client socket
    int rc, maxfd;                                      //rc for read counter. maxfd - the last index of fd in use
    int writeVal = 0;                                   //indacator if going to write. 1 is going to and 0 is not
    char buffer[BUF] = "\0";                            //buffer for read()
    int openFd[MAX_SOCK];                               //array that stand for the file descriptor table
    for(int i = 0; i<MAX_SOCK; i++){                    //init all fd's to be 
        openFd[i] = 0;
        sockToClose[i] = 0;                             //as well the static array for shutdown
    }
    FD_ZERO(&masterSet);                                //zero the master fd_set

//========================================== establish connection ==========================================

    main_socket = socket(AF_INET, SOCK_STREAM, 0);      //socket() to get welcome socket of the server
    if(main_socket < 0){                                //in case socket() failed
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }
    wsocket = main_socket;                              //save the main socket to be able to close it with the handler upon SIGINT
    maxfd = main_socket;                                //save the index of the last known index of fd in the program
    serv_addr.sin_family = AF_INET;                     //setup server setting
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if(bind(main_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){ //in case bind failed
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }
    listen(main_socket, 5);                             //listen for 5 request at once
    FD_SET(main_socket, &masterSet);                    //insert the main socket master set
    
//========================================== chat loop ==========================================

    while(1){
        c_rfd = masterSet;                                  //copy the read fd
        c_wfd = masterSet;                                  //copy the write fd
        if(qsize == 0){                                     //in case the queue is empty
            FD_ZERO(&c_wfd);                                //zero the writing fd copy set
            writeVal = 0;                                   //set the write validation to 0
        }
        else
            writeVal = 1;                                   //in case the queue has messages in it, valid the write
        rc = select(maxfd+1, &c_rfd, &c_wfd, NULL, NULL);   //select()
        if(FD_ISSET(main_socket, &c_rfd)){                  //in case the main socket is ready to read
            client_socket = accept(main_socket, NULL,NULL); //accept new client
            if(client_socket>0){                            //if the socket is valid
                FD_SET(client_socket, &masterSet);          //insert the new socket to master fd_set
                openFd[client_socket] = 1;                  //mark the new client socket as an open fd
                sockToClose[client_socket] = 1;             //copy in case of shutdown
                openCon++;                                  //increase openCon
                maxfd = maxfd < client_socket ? client_socket : maxfd; // maxfd will be what is bigger. the new socket or the current value of maxfd
            }
        }
        for(int fd = main_socket+1; fd <= maxfd; fd++){     //reading loop
            if(FD_ISSET(fd, &c_rfd)){                       //if this fd is ready to read from
                printf("server is ready to read from socket %d\n", fd);
                rc = read(fd, buffer, BUF);                 //read() into buffer
                if(rc == 0){                                //if nothing has read
                    close(fd);                              //close the socket
                    openFd[fd] = 0;                         //mark the socket as closed
                    sockToClose[fd] = 0;                    //as well at the copy
                    openCon--;                              //decrease openCon
                    FD_CLR(fd, &masterSet);                 //remove the fd from the master fd_set
                }
                else if(rc > 0){                            //if something has been read
                    for(int i = 0; i < MAX_SOCK; i++){      //for all posibble socket
                        if(openFd[i] == 1 && fd != i){      //make this for all open sockets but not for the sender
                            msg_node* newMsg = msgCreate(fd);
                            newMsg->dest = i;               //the destention of the message is that open socket
                            strcpy(newMsg->msg, buffer);    //deep copy the buffer to the message
                            if(qsize < 1){                  //if the queue is empty
                                qhead = newMsg;             //point both head and tail to that new message
                                qtail = newMsg;
                                mqueue = qhead;             //save the head in case of shutdown
                            }
                            else{                           //in case there are some messages in the queue
                                qtail->next = newMsg;       //point next to the current tail the new message
                                qtail = qtail->next;        //point the tail to the new tail
                            }
                            qsize++;                        //enlagre the size of the queue after inserting
                        }
                    }
                    bzero(buffer, BUF);                     //clear the buffer for further reads
                }
                else{                                       //if rc == -1
                    perror("read() failed");
                    exit(EXIT_FAILURE);
                }
            }
        }
        if(writeVal==0)                                 //in case there is nothing to write
            continue;
        while(qsize != 0){                              //writing loop
            msg_node* msg = qhead;
            if(qhead!=NULL){                            //in case that the head is a message
                qhead = qhead->next;                    //advance the head pointer to the next in line
                qsize--;                                //decrease the queue size by 1
            }
            if(qsize == 0)                              //in case that we took the last message from the queue
                qtail = NULL;
            if(FD_ISSET(msg->dest, &c_wfd)){            //if the destent user is ready to write to
                rc = writeMsg(msg);                     //write the message
                if(rc < 0){                             //in case writeMsg failed
                    perror("write() failed");            
                    clearMem(qhead);                    //clear the main queue
                    clearMem(resndqhead);               //clear the resend queue
                    exit(EXIT_FAILURE);
                }
            }
            else{                                       //if the user is not ready to write to
                if(resndqsize < 1){                     //if the resend queue is empty
                    resndqhead = msg;                   //point both head and tail to that new message
                    resndqtail = msg;           
                    rqueue = resndqhead;                //save the head for shutdown accured
                }
                else{                                   //in case there are some messages in the resend queue
                    resndqtail->next = msg;             //point next to the current tail the new message
                    resndqtail = qtail->next;           //point the tail to the new tail
                }
                resndqsize++;                           //enlarge the size by 1
            }
            if(qsize == 0)                              //incase that the main queue is empty now
                mqueue = NULL;                          //point the shutdown main queue pointer to null
        }
        while(resndqsize != 0){                         //in case some of the users was not ready to write
            msg_node* msg = resndqhead;                 //msg is the head of the resend queue
            if(resndqhead!=NULL){                       //in case that the head is a message
                resndqhead = resndqhead->next;          //advance the head pointer to the next in line
                resndqsize--;                           //decrease the queue size by 1
            }
            if(resndqsize == 0)                         //in case that we took the last message from the queue
                resndqtail = NULL;
            if(qsize < 1){                              //if the main queue is empty
                qhead = msg;                            //point both head and tail to that new message
                qtail = msg;
                mqueue = qhead;                         //save the head in case of shutdown
            }
            else{                                       //in case there are some messages in the queue
                qtail->next = msg;                      //point next to the current tail the new message
                qtail = qtail->next;                    //point the tail to the new tail
            }
            qsize++;
            if(resndqsize == 0)                         //when the resend queue put all the messages back to the main queue
                rqueue = NULL;                          //point the shutdown pointer to null
        }
    }
}