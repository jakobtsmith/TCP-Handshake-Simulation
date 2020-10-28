/*
	Author: Jakob Smith
*/
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>
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

  char file[1024];

  srand(time(NULL));
  int chance, i = 0;

  for(i; i < 1024; i++){
    chance = rand() % 3;
    if(chance == 0)
      file[i] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rand() % 26];
    else if(chance == 1)
      file[i] = "abcdefghijklmnopqrstuvwxyz"[rand() % 26];
    else
      file[i] = rand() % 10 + '0';
  }

  FILE* fp = fopen("client.out", "w");
  //make a TCPSeg and TCPSeg pointer
  struct TCPSeg seg;
  struct TCPSeg* newseg;
  //initialize the TCPSeg
  initialize_seg(&seg);
  int sockfd = 0, n = 0;
  struct sockaddr_in serv_addr;
  int portno = atoi(argv[1]); //set portnumber to the command line argument passed
  char recvBuff[sizeof(struct TCPSeg)];
  //sending and receiving on same port number
  seg.src_port = portno;
  seg.dest_port = portno;
  seg.header = 0x5002; //set header length to 20 bytes and SYN bit to 1
  seg.seq_num = 1; //set sequence number to 1
  seg.ack_num = 0; //set acknowledgment to 1
  checksum(&seg); //calculate checksum
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("socket error\n");
    exit(EXIT_FAILURE);
  }
 
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);
  serv_addr.sin_addr.s_addr = inet_addr("129.120.151.94");
 
  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("connect error\n");
    exit(EXIT_FAILURE);
  }
 
  //send connection request TCP seg
  print(&seg, fp, "Sending");
  print(&seg, stdout, "Sending");
  if(write(sockfd, (void*)&seg, sizeof(seg)) == -1){
    perror("write");
    exit(EXIT_FAILURE);
  }

  //receive connection granted TCP seg
  while( n = read(sockfd, recvBuff, sizeof(seg)) > 0){
    void* temp = (void*)recvBuff;
    newseg = (struct TCPSeg*)temp;
    print(newseg, fp, "Received");
    print(newseg, stdout, "Received");
    newseg->ack_num = newseg->seq_num + 1; //Set the ack number to the received seq number + 1
    newseg->seq_num = 2; //set the seq num to the initial seq num + 1
    newseg->header &= 0xFFFD; //Set the SYN but to 0
    checksum(newseg); //calculate checksum
    break;
  }
  if( n < 0) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  //send acknowledgement TCP seg
  print(newseg, fp, "Sending");
  print(newseg, stdout, "Sending");
  if(write(sockfd, (void*)newseg, sizeof(seg)) == -1){
    perror("write");
    exit(EXIT_FAILURE);
  }

  //start sending file

  char datafield[128]; //hold 128 byte data
  int datasize = 0; //total amount of data transferred
  int count = 0; //number of iterations
  int offset = 0; //data offset
  int oldack = newseg->ack_num;
  int seqack = newseg->seq_num;
  initialize_seg(&seg);
  seg.src_port = portno;
  seg.dest_port = portno;
  seg.seq_num = newseg->seq_num;
  seg.ack_num = newseg->ack_num;
  while(datasize < sizeof(file)){
    offset = count*128; //find the current data offset
    for(int i = 0; i < 128; i++){
      datafield[i] = file[i + offset]; //set data to be transferred
    }
    datafield[128] = '\0'; //make sure it is null terminated
    datasize += sizeof(datafield); //increment amount of data transferred
    count++; //increment count
    oldack = seg.ack_num; //temp to swap variables
    seg.ack_num = seg.seq_num + 1; //increment ack num
    seg.seq_num = oldack + 1; //increment seq num
    strncpy(seg.data, datafield, sizeof(datafield)); //copy data to be sent
    seg.data[128] = '\0'; //make sure it is null terminated
    seg.header = 0x5000; //20 byte header
    checksum(&seg); //get the checksum

    //send the data
    print(&seg, fp, "Sending");
    print(&seg, stdout, "Sending");
    write(sockfd, (void*)&seg, sizeof(seg));

    //retrieve acknowledgement
    read(sockfd, recvBuff, sizeof(seg));
    void* temp = (void*)recvBuff;
    newseg = (struct TCPSeg*)temp;
    print(newseg, fp, "Received");
    print(newseg, stdout, "Received");
    seg = *newseg;
  }

  fprintf(stdout, "Sent file: %s\n\n", file);
  fprintf(fp, "Sent file: %s\n\n", file);

  //Start closing connection

  initialize_seg(&seg); //set zeroed out fields
  //set source and destination ports
  seg.src_port = portno;
  seg.dest_port = portno;
  //set initial sequence number and a zero ack
  seg.seq_num = 400;
  seg.ack_num = 0;
  seg.header |= 0x5001; //set header length to 20 bytes and set FIN bit to 1
  checksum(&seg); //set checksum

  //send close request packet
  print(&seg, fp, "Sending");
  print(&seg, stdout, "Sending");
  if(write(sockfd, (void*)&seg, sizeof(seg)) == -1){
    perror("write");
    exit(EXIT_FAILURE);
  }

  //receive acknowledgement packet
  while( n = read(sockfd, recvBuff, sizeof(seg)) > 0){
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

  //receive second close acknowledgement packet
  while( n = read(sockfd, recvBuff, sizeof(seg)) > 0){
    void* temp = (void*)recvBuff;
    newseg = (struct TCPSeg*)temp;
    print(newseg, fp, "Received");
    print(newseg, stdout, "Received");
    newseg->ack_num = newseg->seq_num + 1; //Set ack number to received seq number + 1
    newseg->seq_num = 401; //Set seq num to initial seq num + 1
    checksum(newseg); //calculate checksum
    break;
  }
  if( n < 0) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  //send acknowledgement packet
  print(newseg, fp, "Sending");
  print(newseg, stdout, "Sending");
  if(write(sockfd, (void*)newseg, sizeof(seg)) == -1){
    perror("write");
    exit(EXIT_FAILURE);
  }

  return 0;
}
