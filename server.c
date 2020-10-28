/*
	Author: Jakob Smith
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "tcpseg.h"
 
int main(int argc, char **argv)
{
  //print out an error statement if there are too few command line arguments sent
  if(argc < 2){
    printf("Too few arguments. See readme.txt for usage.\n");
    exit(1);
  }
  //print out an error statement if there are too many command line arguments sent
  else if(argc > 2){
    printf("Too many arguments. See readme.txt for usage.\n");
    exit(1);
  }

  FILE* fp = fopen("server.out", "w");
  //Create a TCPSeg and TCPSeg pointer
  struct TCPSeg seg;
  struct TCPSeg* newseg;
  initialize_seg(&seg); //initialize the TCPSeg
  int listenfd = 0, connfd = 0, cli_size, on = 1, n;

  char recvBuff[sizeof(struct TCPSeg)];

  struct sockaddr_in serv_addr, cli_addr;
 
  int portno = atoi(argv[1]); //set port number to command line argument
  if ((listenfd = socket( AF_INET, SOCK_STREAM, 0 )) == -1)
  {
    printf("socket error\n");
    exit(EXIT_FAILURE);
  }

  memset(&serv_addr, '0', sizeof(serv_addr));
  memset(&recvBuff, '0', sizeof(recvBuff));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(portno);

  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  if (bind( listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr) ) == -1)
  {
    printf("bind error\n");
    exit(EXIT_FAILURE);
  }

  if (listen(listenfd, 1 ) == -1)
  {
    printf("listen error\n");
    exit(EXIT_FAILURE);
  }

  cli_size = sizeof(cli_addr);
  if ((connfd = accept(listenfd, (struct sockaddr*) &cli_addr, &cli_size)) == -1)
  {
    printf("accept error\n");
    exit(EXIT_FAILURE);
  }

  //receive the connection request TCP segment
  while( n = read(connfd, recvBuff, sizeof(seg)) > 0){
    void* temp = (void*)recvBuff;
    newseg = (struct TCPSeg*)temp;
    print(newseg, fp, "Received");
    print(newseg, stdout, "Received");
    newseg->ack_num = newseg->seq_num + 1; //set the ack number to the received seq number + 1
    newseg->seq_num = 1000; //set the seq number to an initial value
    newseg->header |= 0xF; //Set the ACK bit to 1
    checksum(newseg); //calculate checksum
    break;
  }
  if( n < 0) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  //send the connection granted TCP segment
  print(newseg, fp, "Sending");
  print(newseg, stdout, "Sending");
  if(write(connfd, (void*)newseg, sizeof(seg)) == -1){
    perror("write");
    exit(EXIT_FAILURE);
  }

  //receive the acknowledgement TCP segment
  while( n = read(connfd, recvBuff, sizeof(seg)) > 0){
    void* temp = (void*)recvBuff;
    newseg = (struct TCPSeg*)temp;
    print(newseg, fp, "Received");
    print(newseg, stdout, "Received");
    break;
  }
  if( n < 0) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  char currdata[128]; //store currently accessible data
  char recfile[1024]; //store entire file
  int count = 0; //iteration count
  int offset = 0; //data offset
  int datasize = 0; //total amount of data received
  int oldack; //store old ack

  while(datasize < sizeof(recfile)){
    //receive data
    read(connfd, recvBuff, sizeof(seg));
    void* temp = (void*)recvBuff;
    newseg = (struct TCPSeg*)temp;
    print(newseg, fp, "Received");
    print(newseg, stdout, "Received");

    //get the offset
    offset = count*128;
    datasize += sizeof(currdata); //increment data received
    count++; //increment count
    memcpy(currdata, newseg->data, sizeof(newseg->data)); //copy received data
    for(int i = 0; i < 128; i++){
      recfile[i + offset] = currdata[i]; //concatenate received data
    }
    oldack = newseg->ack_num; //temporarily store old ack
    newseg->ack_num = newseg->seq_num + 1; //increment ack
    newseg->seq_num = oldack + 1; //increment sequence
    checksum(newseg); //calculate checksum

    //print segment and write to client
    print(newseg, fp, "Sending"); 
    print(newseg, stdout, "Sending");
    write(connfd, (void*)newseg, sizeof(seg));
  }

  //print out received file
  fprintf(stdout, "Received file: %s\n\n", recfile);
  fprintf(fp, "Received file: %s\n\n", recfile);

  //Start closing connection

  //receive close request segment
  while( n = read(connfd, recvBuff, sizeof(seg)) > 0){
    void* temp = (void*)recvBuff;
    newseg = (struct TCPSeg*)temp;
    print(newseg, fp, "Received");
    print(newseg, stdout, "Received");
    newseg->ack_num = newseg->seq_num + 1; //Set the ack num to the received seq num + 1
    newseg->seq_num = 450; //Set the seq num to an initial value
    newseg->header &= 0xFFFE; //Set the FIN bit to 0
    newseg->header |= 0xF; //Set the ACK bit to 1
    checksum(newseg); //Calculate the checksum
    break;
  }
  if( n < 0) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  //send the first acknowledgement segment
  print(newseg, fp, "Sending");
  print(newseg, stdout, "Sending");
  if(write(connfd, (void*)newseg, sizeof(seg)) == -1){
    perror("write");
    exit(EXIT_FAILURE);
  }

  initialize_seg(&seg); //initialize the TCPSeg
  //source port and dest port are the same
  seg.src_port = portno;
  seg.dest_port = portno;
  //set an initial seq num and set ack num to 0
  seg.seq_num = 580;
  seg.ack_num = 0;
  seg.header |= 0x5001; //Set header to 20 bytes and FIN bit to 1
  checksum(&seg); //Calculate the checksum

  print(&seg, fp, "Sending");
  print(&seg, stdout, "Sending");
  if(write(connfd, (void*)&seg, sizeof(seg)) == -1){
    perror("write");
    exit(EXIT_FAILURE);
  }

  //receive the final acknowledgement packet
  while( n = read(connfd, recvBuff, sizeof(seg)) > 0){
    void* temp = (void*)recvBuff;
    newseg = (struct TCPSeg*)temp;
    print(newseg, fp, "Received");
    print(newseg, stdout, "Received");
    break;
  }
  if( n < 0) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  close(connfd); //close connection
  return 0;
}
