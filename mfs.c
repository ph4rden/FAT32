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
#include <ctype.h>

#define MAX_NUM_ARGUMENTS 3

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size




char    BS_OEMName[8];
int16_t BPB_BytsPerSec;
int8_t  BPB_SecPerClus;
int16_t BPB_RsvdSecCnt;
int8_t  BPB_NumFATs;
int16_t BPB_RootEntCnt;
char    BS_VolLab[11];
int32_t BPB_FATSz32;
int32_t BPB_RootClus;

int32_t RootDirSectors = 0;
int32_t FirstDataSector = 0;
int32_t FirstSectorofCluster = 0;

FILE * fp;
FILE * ofp;

struct __attribute__((__packed__)) DirectoryEntry 
{
  char      DIR_Name[11];
  uint8_t   DIR_Attr;
  uint8_t   Unused1[8]; 
  uint16_t  DIR_FirstClusterHigh;
  uint8_t   Unused2[4];
  uint16_t  DIR_FirstClusterLow;
  uint32_t  DIR_FileSize;
};

struct DirectoryEntry dir[16];

int LBAToOffset( int32_t sector)
{
  return ((sector - 2 ) * BPB_BytsPerSec) + (BPB_BytsPerSec * BPB_RsvdSecCnt) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec);
}

int16_t NextLB( uint32_t sector )
{
  uint32_t FATAddress = ( BPB_BytsPerSec * BPB_RsvdSecCnt) + ( sector * 4 );
  int16_t val; 
  fseek( fp, FATAddress, SEEK_SET); 
  fread( &val, 2, 1, fp);
  return val;
}

int compare(char* input, char* IMG_Name){
  char expanded_name[12];
  memset( expanded_name, ' ', 12 );

  char *token = strtok( input, "." );

  strncpy( expanded_name, token, strlen( token ) );

  token = strtok( NULL, "." );

  if( token )
  {
    strncpy( (char*)(expanded_name+8), token, strlen(token ) );
  }

  expanded_name[11] = '\0';

  int i;
  for( i = 0; i < 11; i++ )
  {
    expanded_name[i] = toupper( expanded_name[i] );
  }

  if( strncmp( expanded_name, IMG_Name, 11 ) == 0 )
  {
    return 1;
  }

  return 0;
}


int opened = 0;
char saved_filename[11];

int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );


  while( 1 )
  {
    // Print out the mfs prompt
    printf ("\nmfs> ");

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
      fp = fopen(token[1], "r");
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
        fseek(fp, (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) + (BPB_RsvdSecCnt * BPB_BytsPerSec), SEEK_SET);
        fread(&dir[0], sizeof(struct DirectoryEntry), 16, fp);
        for(int i = 0; i < 16; i++){
          printf("\n%s", dir[i].DIR_Name);
        }
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
    
    if(strcmp(token[0],"stat") == 0)
    {

    }

    if(strcmp(token[0], "get") == 0){
      int found = 0;
      for(int i = 0; i < 16; i++){
        if(compare(token[1], dir[i].DIR_Name)){
          found = 1;
          int cluster = dir[i].DIR_FirstClusterLow;
          int offset = LBAToOffset(cluster);
          int size = dir[i].DIR_FileSize;
          fseek(fp, offset, SEEK_SET);
          ofp = fopen(token[1], "w");
          uint8_t buffer[12];
          while(size > BPB_BytsPerSec){
            fread(&buffer, 512, 1, fp);
            fwrite(&buffer, 512, 1, ofp);
            size = size - 512;
            cluster = NextLB(cluster);
            offset = LBAToOffset(cluster);
            fseek(fp, offset, SEEK_SET);
          }
          if(size > 0){
            fread(&buffer, size, 1, fp);
            fwrite(&buffer, size, 1, ofp);
            i = 16;
            fclose(ofp);
          }
        }
      }
      if(!found){
        printf("Error: File not found");
      }

    }

    if(strcmp(token[0],"ls") == 0)
    {
      for(int i=0;i<16;i++)
      {
        if((dir[i].DIR_Attr == 0x01) || (dir[i].DIR_Attr == 0x10) || (dir[i].DIR_Attr == 0x20) && (dir[i].DIR_Name[0]!= 0xe5))
        {
          char name[12];
          memcpy(name, dir[i].DIR_Name,11);
          name[11] = '\0';
          printf("%s\n",name);
        }
      }
    }

    if(strcmp(token[0],"cd") == 0)
    {
      if(token[1] == "..")
      {

      }
      for(int i=0; i<16; i++)
      {
        if(compare(token[1], dir[i].DIR_Name))
        {
          int cluster =  dir[i].DIR_FirstClusterLow;
          int offset = LBAToOffset(cluster);
          fseek(fp, offset, SEEK_SET);
          fread(&dir[0],sizeof(struct DirectoryEntry),16,fp);
          break;
        }
      }
      
    }
    
    if(strcmp(token[0],"read") == 0)
    {
      
      
    }

    if(strcmp(token[0],"del") == 0)
    {
      if(token[1] == NULL){
        printf("\nNot enough arguments for del");
      }
      else{
        int found = 0;
        for(int i = 0; i < 16; i++){
          if(compare(token[1], dir[i].DIR_Name)){
            strncpy(saved_filename, dir[i].DIR_Name, 11);
            dir[i].DIR_Name[0] = '?';
            dir[i].DIR_Attr = 0;
            found = 1;
          }
        }
        if(!found){
          printf("\nDirectory or File did not exist");
        }
      }
    }

    if(strcmp(token[0],"undel") == 0)
    {
      if(token[1] == NULL){
        printf("\nNot enough arguments for undel");
      }
      else{
        for(int i = 0; i < 16; i++){
          if(dir[i].DIR_Name[0] == '?'){
            strncpy(dir[i].DIR_Name, saved_filename, 11);
            dir[i].DIR_Attr = 0x1;
          }
        }
      }
    }
    free( working_root );
  }
  if(fp != NULL)
  {
    fclose(fp);
  }
  return 0;
}
