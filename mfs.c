/*
Danny Le   - 1001794802 - Section:001
Brian Phan - 1001795028 - Section:003
*/
// The MIT License (MIT)
// 
// Copyright (c) 2020 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

#define MAX_NUM_ARGUMENTS 3

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

/*
commands:
open 
close 
info 
stat 
cd 
ls 
get 
read 
del 
undel 
*/

void open();
//void close();
void info();
void stat();
void changeDirectory();
void listFiles();
void get();
//void read();
void delete();
void unDelete();


char BS_OEMName[8];
int16_t BPB_BytsPerSec;
int8_t BPB_SecPerClus;
int16_t BPB_RsvdSecCnt;
int8_t BPB_NumFATs;
int16_t BPB_RootEntCnt;
char BS_VolLab[11];
int32_t BPB_FATSz32;
int32_t BPB_RootClus;

int32_t RootDirSectors = 0;
int32_t FirstDataSector = 0;
int32_t FirstSectorofCluster = 0;

int opened = 0;

struct __attribute__((__packed__)) DirectoryEntry 
{
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8]; 
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[16];

int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  FILE * fp;


  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your FAT32 functionality
    if(strcmp(token[0],"open") == 0)
    {
      fp = fopen("fat32.img", "r");
      if(fp == NULL)
      {
        printf("Error: File system image not found\n");
      }
      else if(opened == 1)
      {
        printf("Error: File system image already open\n");
      }
      else{
        opened = 1;
        fseek(fp, 3, SEEK_SET);
        fread(&BS_OEMName, 8, 1, fp);
        fseek(fp, 11, SEEK_SET);
        fread(&BPB_BytsPerSec, 2, 1, fp);
        fseek(fp, 13, SEEK_SET);
        fread(&BPB_SecPerClus, 1, 1, fp);
        fseek(fp, 14, SEEK_SET);
        fread(&BPB_RsvdSecCnt, 2, 1, fp);
        fseek(fp, 16, SEEK_SET);
        fread(&BPB_NumFATs, 1, 1, fp);
        fseek(fp, 17, SEEK_SET);
        fread(&BPB_RootEntCnt, 2, 1, fp);
        fseek(fp, 36, SEEK_SET);
        fread(&BPB_FATSz32, 4, 1, fp);
        fseek(fp, 44, SEEK_SET);
        fread(&BPB_RootClus, 4, 1, fp);
        fseek(fp, 71, SEEK_SET);
        fread(&BS_VolLab, 11, 1, fp);
      }
    }
    if(strcmp(token[0], "close") == 0)
    {
      fclose(fp);
      fp = NULL;
      opened = 0;
    }

    if(strcmp(token[0],"info") ==0 )
    {
      printf("\t\t\tHex:\tDecimal:\n");
      printf("BPB_BytesPerSec:\t%x\t%d\n", BPB_BytsPerSec, BPB_BytsPerSec);
      printf("BPB_SecPerClus:\t\t%x\t%d\n", BPB_SecPerClus, BPB_SecPerClus);
      printf("BPB_RsvdSecCnt:\t\t%x\t%d\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);
      printf("BPB_NumFATS:\t\t%x\t%d\n", BPB_NumFATs, BPB_NumFATs);
      printf("BPB_FATSz32:\t\t%x\t%d\n", BPB_FATSz32, BPB_FATSz32);
    }
    
    if(strcmp(token[0],"stat") ==0)
    {

    }

    
    free( working_root );
  }
  if(fp != NULL)
  {
    fclose(fp);
  }
  return 0;
}
