#include <string.h>
#include <stdio.h>

struct TCPSeg{
  unsigned short int src_port;
  unsigned short int dest_port;
  unsigned int seq_num;
  unsigned int ack_num;
  unsigned short int header;
  unsigned short int window;
  unsigned short int checksum;
  unsigned short int urgent;
  unsigned int options;
  char data[128];
};

void initialize_seg(struct TCPSeg* seg);
void checksum(struct TCPSeg* seg);
void print(struct TCPSeg* seg, FILE* fp, char* type);
