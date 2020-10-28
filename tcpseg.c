#include "tcpseg.h"

void initialize_seg(struct TCPSeg* seg){
  memset(seg, 0, sizeof(*seg));
}

void checksum(struct TCPSeg* seg){
  unsigned short int cksum_arr[12];
  int sum = 0, i;

  memcpy(cksum_arr, seg, 24); //Copying 24 bytes

  for (i=0;i<12;i++)// Compute sum
  sum = sum + cksum_arr[i];
  seg->checksum = sum >> 16;      // Fold once
  sum &= 0x0000FFFF;
  sum += seg->checksum;
  seg->checksum = sum >> 16;      // Fold once more
  sum &= 0x0000FFFF;
  seg->checksum += sum;  
  /* XOR the sum for checksum */
  seg->checksum ^= 0xFFFF;
}

void print(struct TCPSeg* seg, FILE* fp, char* type){
  fprintf(fp, "%s packet:\n", type);
  fprintf(fp, "srcprt: %hu\n", seg->src_port);
  fprintf(fp, "destprt: %hu\n", seg->dest_port);
  fprintf(fp, "seqnum: %i\n", seg->seq_num);
  fprintf(fp, "acknum: %i\n", seg->ack_num);
  fprintf(fp, "hedr: 0x%04X\n", seg->header);
  fprintf(fp, "wind: %hu\n", seg->window);
  fprintf(fp, "chksum: 0x%04X\n", seg->checksum);
  fprintf(fp, "urgnt: %hu\n", seg->urgent);
  fprintf(fp, "opt: %i\n\n", seg->options);
}
