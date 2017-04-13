/*
 * hdrinfo
 * 
 * reads the header of executables generated with bcc and lists the fields contained in it
 * shows if the program has been compiled with the -i switch and the size of the memory sections
 * 
 * Freeware - public domain - by Georg Potthast
 * no warranty whatsoever
 * 
 * 12th of January 2017
 */

#include <stdio.h>
#include <string.h>

int main ( int argc, char *argv[] )
{
    if ( (argc != 2) | (argv[1][0]=='-' & argv[1][1]=='h')) 
    {
    	printf("\nbccinfo\n- showing header information of executables generated with bcc\n");
        printf("usage: %s filename\n", argv[0]);
	printf("       -h = this help text\n\n");
    }
    else 
    {
        FILE *file = fopen( argv[1], "r" );
        if ( file == 0 )
        {
            printf( "Could not open file\n" );
        }
        else 
        {
            int x;
            char str[4];
	    int vals[4];
	    char number[4];
	    char hnumber[4];
	    char *ptr;
	    //unsigned long ret;
	    long ret;
   	    int field=0;
	    unsigned short mask = 0x000000FF;
	    printf("\n");
	    
	    while  (field<8)
            {
	      fread(&str,4,1,file);
	      vals[0]=(unsigned short)str[0] & mask;
	      ret=vals[0];
	      vals[1]=(unsigned short)str[1] & mask;
	      ret +=vals[1]* 0x100; //<<8;
	      vals[2]=(unsigned short)str[2] & mask;
	      ret +=vals[2]*0x10000; //<<16;
	      vals[3]=(unsigned short)str[3] & mask;
	      ret +=vals[3]*0x1000000; //<<24;
	      //printf("vals: %u,%u,%u,%u   ",vals[3],vals[2],vals[1],vals[0]);
	      //printf("The value(unsigned long integer) is %lu\n", ret);
	      sprintf(hnumber,"0x%02X%02X%02X%02X",vals[3],vals[2],vals[1],vals[0]); //str[3],str[2],str[1] & mask,str[0] & mask);
              //printf( "Hex: %s\n", hnumber );

              switch (field){				
				case 0:
					if (vals[2]==0x10){ /*4100301*/
					printf("I & D combined                                    64k   %s\n", hnumber);	
					} else if (vals[2]==0x20) { /*4200301*/
					printf("I & D separate                                   128k   %s\n", hnumber);
					} else {
					  printf("No bcc generated executable!!!\n\n");
					  fclose( file );
					  return(1);
					}					
					break;
				case 1: /*magic nr*/
					break;
				case 2:
					printf("Code/text section size in bytes (tseg):        %6u", ret);
					printf(" = %s\n", hnumber );
					break;
				case 3:
					printf("Data section size in bytes (dseg):             %6u", ret);
					printf(" = %s\n", hnumber );
					break;
				case 4:
					printf("BSS section size in bytes (bseg):              %6u", ret);
					printf(" = %s\n", hnumber );
					break;
				case 5: /*NULL*/
					break;
				case 6:
					printf("Total memory size incl. heap (chmem):          %6u", ret);
					printf(" = %s\n", hnumber );
					printf("This memory is data+bss+heap+stack combined.\n");
					break;
				case 7:
					printf("Symbol table size if present (unused):         %6u", ret);
					printf(" = %s\n\n", hnumber );
					break;
				default:
					break;
				} /* switch */	
		
              field++;                  
	    } /* while */

	    fclose( file );
        } /* file == 0 */
        return(0);
    } /* argc != 2 */
    return(1);
} /* main */
