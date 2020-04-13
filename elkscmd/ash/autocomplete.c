/*
 * This implements the autocompletion callback (or TAB completion) for Linenoise
 * see the shell.txt file in the Documentation/text directory for details
 */

#include "shell.h"

#if LINENOISE

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "linenoise.h"

void completion(const char *buf, linenoiseCompletions *lc) {
/* completion callback for ELKS */

int files_found=0;
char path[1024]=""; 
char resultstring[100]="";
char startchars[100]="";
char matchterm[100]="";
char blank[]=" ";
char* sptr=NULL;
int dirs =0;
int exes = 0;
int stds = 0;
int firstcharpos =0; //zero based!

struct stat file_attribute;


   if (strncmp(buf, "cd ",3) == 0) {
        //look for directories
       dirs=1;
       if (strncmp(buf, "cd /",4) == 0){
            //search in root for directories
            sprintf(path,"/");
            sprintf(startchars,"cd /");
            sprintf(matchterm,"%s",buf+4);
        } else {
            //search in current directory for directories
            sprintf(path,".");
            sprintf(startchars,"cd ");
            sprintf(matchterm,"%s",buf+3);
        }
   } else if ((strlen(buf) > 1) && (strncmp(buf, "./",2)) == 0){
       //search for executables in current directory
       exes = 1;
       sprintf(path,".");
       sprintf(startchars,"./");
       sprintf(matchterm,"%s",buf+2);
   } else if (strpbrk(buf,blank) != NULL) { 
       // search for file to use with ls, cat, vi, ed, type etc.
       // in the current directory
       stds=1;
       sprintf(path,".");
       firstcharpos=strpbrk(buf,blank)- buf+1;
       sprintf(matchterm,"%s",buf+firstcharpos);
       sprintf(startchars,"%s",buf);
       startchars[firstcharpos]=0x00;
   } else {
       //search for executable in /bin
       exes=1; //no need and will not list outside currdir
       sprintf(path,"/bin");
       sprintf(matchterm,"%s",buf);
   }
   
   DIR *dirp=opendir(path);
   
   struct dirent entry;
   struct dirent *dp=&entry;
   
   printf("\n\r"); //print files in new line
   
   while(dp = readdir(dirp))
   {
      if((strncmp(matchterm, dp->d_name,strlen(matchterm))) == 0)
      {
          if (dirs ==1) {
              stat(dp->d_name, &file_attribute);
              if ((file_attribute.st_mode & S_IFDIR)==0) continue; //is no directory
              if (strcmp(dp->d_name,".") == 0) continue; //add no '.' to directories
          }
          if (stds ==1) {
              stat(dp->d_name, &file_attribute);
              if ((file_attribute.st_mode & S_IXUSR)!=0) continue; //show no executable files
              if ((file_attribute.st_mode & S_IFDIR)!=0) continue; //show no directories
          }
          if (exes ==1) {
                stat(dp->d_name, &file_attribute);
                if (strcmp(startchars,"./") == 0) { //workaround, st_mode may need path to filename
                    if ((file_attribute.st_mode & S_IXUSR)==0) continue; //is no executable file
                }
                if ((file_attribute.st_mode & S_IFDIR)!=0) continue; //show no directories
          }

          if (files_found != 0) printf("%s, ",resultstring);   //do not print a single file found
          sprintf(resultstring,"%s%s",startchars,dp->d_name); //cannot pass pointer to linenoiseAddCompletion          
          linenoiseAddCompletion(lc,resultstring);           
          files_found++;
          if (files_found >500) break; //limit
      }
   }
   if (files_found > 1) {
       printf("%s ",resultstring); //the last one - do not print single match
       printf("(%d Files found)\n", files_found);
       }
   
   closedir(dirp); //now dp no longer available!

   return;

}
#endif
