/* TEA based crypt(), version 0.0 <ndf@linux.mit.edu>
 * It looks like there are problems with key bits carrying through
 * to the encryted data, and I want to get rid of that libc call..
 * This is just so rob could see it ;) */

/*
 * I've:
 *   Compared the TEA implementation to a reference source - OK
 *   Reduced the cycles count from 64 to 32 (the suggested value)
 *   Changed the types of 'n' and 'i' for better code with bcc.
 *   Removed a possible overrun of rkey by looping the values, it's now
 *   possible to _choose_ every bit of the 128bit PW with a 32 character word.
 *   Corrected the output transformation, it lost bits between words.
 *   Cleaned out all trace of the uncrypted PW from rkey.
 *
 *  RDB.
 */

#include <stdlib.h>

char *
crypt(const char * key, const char * salt)
{
  /* n is the number of cycles (2 rounds/cycle),
     delta is a golden # derivative,
     k is the key, v is the data to be encrypted. */

  unsigned long v[2], sum=0, delta=0x9e3779b9, k[4];
  int n=32, i, j;
  static char rkey[16];

  /* Our constant string will be a string of zeros .. */
  v[0]=v[1]=k[0]=k[1]=k[2]=k[3]=0;
  for(i=0;i<16;i++) rkey[i]=0;

  rkey[0]=*salt;
  rkey[1]=salt[1];
  for (j=2,i=0;key[i];i++,j=((j+1)&15))
     rkey[j]=(rkey[j]<<4)+(rkey[j]>>4)+ key[i];

  memcpy(k, rkey, 4*sizeof(long));

  while (n-->0) {
    sum += delta;
    v[0] += (v[1]<<4)+k[0] ^ v[1]+sum ^ (v[1]>>5)+k[1];
    v[1] += (v[0]<<4)+k[2] ^ v[0]+sum ^ (v[0]>>5)+k[3];
  }

  /* Remove any trace of key */
  for(i=0;i<16;i++) rkey[i]=0;
  *rkey=*salt; rkey[1]=salt[1];

  /* Now we need to unpack the bits and map it to "A-Za-z0-9./" for printing
     in /etc/passwd */
  sum=v[0];
  for (i=2;i<13;i++)
    {
      /* This unpacks the 6 bit data, each cluster into its own byte */
      rkey[i]=(sum&0x3F);
      sum>>=6;
      if(i==0+2) sum |= (v[1]<<26);
      if(i==5+2) sum |= (v[1]>>4);

      /* Now we map to the proper chars */
      if (rkey[i]>=0 && rkey[i]<12) rkey[i]+=46;
      else if (rkey[i]>11 && rkey[i]<38) rkey[i]+=53;
      else if (rkey[i]>37 && rkey[i]<64) rkey[i]+=59;
      else return NULL;
    }

  rkey[13]='\0';
  return rkey;
}
