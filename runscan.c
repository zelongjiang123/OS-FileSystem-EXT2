////////////////////////////////////////////////////////////////////////////////
// Main File:        runscan.c
// This File:        runscan.c
// Semester:         CS 537 Spring 2023
// Instructor:       Shivaram
//
// Author:           Zelong Jiang
// wisc ID:          9082157588
// Email:            zjiang287@wisc.edu
// CS Login:         zjiang
//
/////////////////////////////////////////////////////////////////////////////////

#include "ext2_fs.h"
#include "read_ext2.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>

/*
store the details of a jpg file
*/
struct details
{
    int inode_number;
    __u16 i_links_count; /* Links count */
    __u16 i_uid;         /* Owner Uid */
    __u32 i_size;        /* Size in bytes */
};


/*
function to read the data block via indirect block
*/
void access_indirect_block(const int fd, int block_number, int *data_blocks_count, int *size_left, FILE *dest_file)
{
    //printf("block number: %d\n", block_number);
    // access the indirect block
    lseek(fd, BLOCK_OFFSET(block_number), SEEK_SET);
    int data_block_numbers[256];
    int index = 0;
    // get data block numbers
    while (*data_blocks_count > 0 && index < 256)
    {
        char data_block_number_array[100];
        read(fd, data_block_number_array, 4);
	data_block_number_array[4] = '\0';
	//printf("block number %s\n", data_block_number_array);
        //printf("block number 1%c, 2%c, 3%c, 4%c\n", data_block_number_array[0], data_block_number_array[1], data_block_number_array[2], data_block_number_array[3]);
	data_block_numbers[index++] = *(int *)data_block_number_array;	
	//printf("block number %d\n", data_block_numbers[index-1]);
            
	(*data_blocks_count)--;
        (*size_left) -= block_size;
    }
    //printf("first block number: %d\n", data_block_numbers[0]);
    // read the actual data blocks
    for (int i = 0; i < index - 1; i++)
    {
        lseek(fd, BLOCK_OFFSET(data_block_numbers[i]), SEEK_SET);
        char buffer[block_size];
        read(fd, buffer, block_size);
        fwrite(buffer, sizeof(char), block_size, dest_file);
    }
    // read the last data block
    lseek(fd, BLOCK_OFFSET(data_block_numbers[index - 1]), SEEK_SET);
    //printf("size left: %d\n", *size_left);
    if (*size_left < 0)   
    {
        printf("size is smaller than a block!!!!\n");
        (*size_left) += block_size;
        char buffer[*size_left];
        read(fd, buffer, *size_left);
        fwrite(buffer, sizeof(char), *size_left, dest_file);
        (*size_left) -= block_size;
    }
    else
    {
        char buffer[block_size];
        read(fd, buffer, block_size);
        fwrite(buffer, sizeof(char), block_size, dest_file);
    }
}

/*
function to read the data block via double indirect block
*/
void access_double_indirect_block(const int fd, int block_number, int *data_blocks_count, int *size_left, FILE *dest_file)
{
    // access the double indirect block
    lseek(fd, BLOCK_OFFSET(block_number), SEEK_SET);
    int indirect_block_numbers[256];
    int index = 0;
    // get indirect block numbers
    while (index < 256)
    {
        char indirect_block_number_array[4];
        read(fd, indirect_block_number_array, 4);
        indirect_block_numbers[index++] = *(int *)indirect_block_number_array;
    }
    for (int i = 0; i < 256; i++)
    {
        // use the number stored in the array to access the indirect blocks
        access_indirect_block(fd, indirect_block_numbers[i], data_blocks_count, size_left, dest_file);
        if (*data_blocks_count <= 0)
            break;
    }
}

/*
this function parse the data blocks of a regular file
check if the file is .jpg
if it is, output the data to a new file
*/
void parse_regular_file(int fd,struct ext2_inode *inode, int inode_number, char *output_directory, struct details *jpg_files, int *jpg_index)
{
    // this inode represents a regular file

    char buffer[block_size];
    int size_left = (int)inode->i_size;
    int data_blocks_count = inode->i_size / block_size; // the number of data blocks we need to access
    if (inode->i_size % block_size != 0)
        data_blocks_count++;

    int index = 0;
    if(inode->i_block[index] == 0)
        return;
    lseek(fd, BLOCK_OFFSET(inode->i_block[index]), SEEK_SET);
    //printf("block number: %d, max : %d\n", inode->i_block[index], blocks_per_group);
    // read the first data block
    if (size_left < (int)block_size)
    {
        read(fd, buffer, size_left);
        data_blocks_count--;
    }

    else
    {
        read(fd, buffer, block_size);
        data_blocks_count--;
    }
   // printf("%02x; %02x; %02x; %02x\n",(int) buffer[0],(int) buffer[1], (int)buffer[2],(int) buffer[3]);

    
    // check whether this file is an jpg file
    if (buffer[0] == (char)0xff &&
        buffer[1] == (char)0xd8 &&
        buffer[2] == (char)0xff &&
        (buffer[3] == (char)0xe0 ||
         buffer[3] == (char)0xe1 ||
         buffer[3] == (char)0xe8))
    {
	printf("it is jpg\n");
        
	struct details newNode;
        newNode.inode_number = inode_number;
        newNode.i_size = inode->i_size;
        newNode.i_uid = inode->i_uid;
        newNode.i_links_count = inode->i_links_count;
        jpg_files[*jpg_index] = newNode; // add the inode to the jpg files array
	(*jpg_index)++;       
       
	// Create a new file
        char filename[EXT2_NAME_LEN];
        sprintf(filename, "%s/file-%d.jpg", output_directory, inode_number);
	FILE *dest_file = fopen(filename, "wb");
        if (dest_file == NULL)
        {
            printf("Unable to create destination file.\n");
            return;
        }
        if (size_left < (int)block_size) //the last data block and finish reading
        {
            fwrite(buffer, sizeof(char), size_left, dest_file);
        }
        else
        {
	    fwrite(buffer, sizeof(char), block_size, dest_file);
            size_left -= block_size;
            index++;
            while (data_blocks_count > 0 && index < 12) // access the first 12 direct block
            {

                lseek(fd, BLOCK_OFFSET(inode->i_block[index]), SEEK_SET);
                if (size_left <(int) block_size)
                {
                    char buffer[block_size];
                    read(fd, buffer, size_left);
                    data_blocks_count--;
                    fwrite(buffer, sizeof(char), size_left, dest_file);
                }

                else
                {
                    char buffer[block_size];
                    read(fd, buffer, block_size);
                    data_blocks_count--;
                    fwrite(buffer, sizeof(char), block_size, dest_file);
                }
                size_left -= block_size;
                index++;
            }
            
	   // printf("count left %d\n", data_blocks_count);
	    if (data_blocks_count > 0)  // continue reading
            {
               access_indirect_block(fd, inode->i_block[12], &data_blocks_count, &size_left, dest_file);
	     //  printf("indirect count left %d\n", data_blocks_count);
               if (data_blocks_count > 0)
                  access_double_indirect_block(fd, inode->i_block[13], &data_blocks_count, &size_left, dest_file);
	       //printf("double indirect count left %d\n", data_blocks_count);
	    }
	}
        fclose(dest_file);
    }
}
/*
function that creates a directory
*/
void create_directory(const char *dir_path)
{
    int status = mkdir(dir_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == 0)
    {
        printf("Directory created successfully.\n");
    }
    else
    {
        printf("Failed to create directory. Directory already exists.\n");
        exit(EXIT_FAILURE);
    }
}

/*
a function that check whether the given inode number is a jpg file we stored in the array
 */
int check_jpg(int inode_number, struct details *jpg_files, int *jpg_index)
{
    for (int i = 0; i < *jpg_index; i++)
        if (inode_number == jpg_files[i].inode_number)
            return i;
    return -1;
}

/*
a function that creats the jpg file with their actual name
and a txt file with its details
*/
void create_jpg_file_with_actual_name(struct details file_details, char *name, int inode_number, char *output_directory)
{
    // since we have already created a file-inodenumber file
    // we can just copy the data from that file to the new file
    char sourcefilename[EXT2_NAME_LEN];
    sprintf(sourcefilename, "%s/file-%d.jpg", output_directory, inode_number);
    FILE *source_file = fopen(sourcefilename, "r");
    printf("source file name: %s\n", sourcefilename);
    if (source_file == NULL)
    {
        printf("Error opening source file.");
        exit(1);
    }
    char destfilename[EXT2_NAME_LEN];
    sprintf(destfilename, "%s/%s", output_directory, name);
    FILE *dest_file = fopen(destfilename, "w");
    if (dest_file == NULL)
    {
        printf("Error opening dest file.");
        fclose(source_file);
        exit(1);
    }
    //printf("%c\n", fgetc(source_file));

    
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, source_file)) > 0)
    {
        fwrite(buffer, 1, bytes_read, dest_file);
    }
    
    fclose(source_file);
    fclose(dest_file);


    // create detail file
    char destfilename2[EXT2_NAME_LEN];
    sprintf(destfilename2, "%s/file-%d-details.txt", output_directory, inode_number);
    FILE *dest_file2 = fopen(destfilename2, "w");
    if (dest_file2 == NULL)
    {
        printf("Error opening dest file.");
        exit(1);
    }
    //fwrite(&file_details.i_links_count, sizeof(__u16), 1, dest_file2);
    fprintf(dest_file2, "%hu", file_details.i_links_count);
    fprintf(dest_file2, "\n%u", file_details.i_size);
    fprintf(dest_file2, "\n%hu", file_details.i_uid);
    fclose(dest_file2);
}

/*
a function that traverse the data block of the directory and find the name of jpg files
*/
int find_files(char *buffer, char *output_directory, struct details *jpg_files, int *jpg_index)
{
    // the offset is a multiple of 4 and the block is 1024 bytes, so we divide blocksize by 4
    int times = ((int)block_size) / 4;
    //printf("times: %d\n", times);
    for (int i = 0; i < times; i++)
    {
        struct ext2_dir_entry *dentry = (struct ext2_dir_entry *)&(buffer[i*4]);
        int index = check_jpg(dentry->inode, jpg_files, jpg_index);
        if (index > -1)
        {
            // printf("entry length: %d\n", dentry->rec_len);
            int name_len = dentry->name_len & 0xFF; // convert 2 bytes to 4 bytes properly
            char name[EXT2_NAME_LEN];
            strncpy(name, dentry->name, name_len);
            name[name_len] = '\0';
            //printf("Entry name is --%s--\n", name);
            create_jpg_file_with_actual_name(jpg_files[index], name, dentry->inode, output_directory);
        }
    }
    /*
    int offset = 0;
    struct ext2_dir_entry *dentry = (struct ext2_dir_entry *)&(buffer[0]);
    while (offset < (int)block_size)
    {
        int index = check_jpg(dentry->inode, jpg_files, jpg_index);
        if (index > -1)
        {
            // printf("entry length: %d\n", dentry->rec_len);
            int name_len = dentry->name_len & 0xFF; // convert 2 bytes to 4 bytes properly
            char name[EXT2_NAME_LEN];
            strncpy(name, dentry->name, name_len);
            name[name_len] = '\0';
            printf("Entry name is --%s-- directory is %s\n", name, output_directory);
            create_jpg_file_with_actual_name(jpg_files[index], name, dentry->inode, output_directory);
        }
        offset += dentry->rec_len;
        dentry = (struct ext2_dir_entry *)&(buffer[offset]);
    }*/
    return 0;
}
/*
a function that traverse the data block of the directory and find the inode number of top_secret folder
*/
int find_secret_folder(char *buffer, int inode_total_count)
{
    char target[] = "top_secret";
    // the offset is a multiple of 4 and the block is 1024 bytes, so we divide blocksize by 4
    int times = ((int)block_size) / 4;
    for (int i = 0; i < times; i++)
    {
        struct ext2_dir_entry *dentry = (struct ext2_dir_entry *)&(buffer[i * 4]);
        int name_len = dentry->name_len & 0xFF; // convert 2 bytes to 4 bytes properly
        char name[EXT2_NAME_LEN];
        strncpy(name, dentry->name, name_len);
        name[name_len] = '\0';
        //printf("Entry name is --%s-- and the inode number is %d, start %d, length %d\n", name, dentry->inode, i*4,dentry->rec_len);
        if (strcmp(name, target) == 0 && (int) dentry->inode < inode_total_count)
        {
            printf("secret folder inode number: %d\n", (int)dentry->inode);
            return (int)dentry->inode;
        }
    }
    return 0;
}

/*
this function parse the data blocks of a directory
*/
int parse_directory(int fd, struct ext2_inode *inode, char *output_directory, struct details *jpg_files, int *jpg_index)
{
   // printf("parse directory\n");
    int size_left = (int)inode->i_size;
    int data_blocks_count = inode->i_size / block_size; // the number of data blocks we need to access
    if (inode->i_size % block_size != 0)
        data_blocks_count++;

    int index = 0;
    if (inode->i_block[index] <= 0 || inode->i_block[index] > blocks_per_group)
        return 0;
   // printf("data block number %d\n", inode->i_block[0]);
    while (data_blocks_count > 0 && index < 12) // access the first 12 direct block
    {

	char buffer[block_size];
       //printf("data block number %d\n", inode->i_block[index]);
        lseek(fd, BLOCK_OFFSET(inode->i_block[index]), SEEK_SET);
        if (size_left < (int) block_size)
        {
            read(fd, buffer, size_left);
            data_blocks_count--;
        }

        else
        {
            read(fd, buffer, block_size);
            data_blocks_count--;
        }
        size_left -= block_size;
        index++;

	find_files(buffer, output_directory, jpg_files, jpg_index);
    }
    
    if (data_blocks_count > 0) // continue reading
    {   
	printf("continue reading\n");
        /*
        access_indirect_block(fd, inode->i_block[12], &data_blocks_count, &size_left, dest_file);
        if (data_blocks_count > 0)
            access_double_indirect_block(fd, inode->i_block[13], &data_blocks_count, &size_left, dest_file);*/
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("expected usage: ./runscan inputfile outputfile\n");
        exit(0);
    }
    
    create_directory(argv[2]);


    /* This is some boilerplate code to help you get started, feel free to modify
       as needed! */

    int fd;
    fd = open(argv[1], O_RDONLY); /* open disk image */

    ext2_read_init(fd);

    struct ext2_super_block super;

    struct ext2_group_desc group[(int)num_groups];
    read_super_block(fd, &super);
    __u32 inodes_count = super.s_inodes_per_group;

     printf("number of inodes per group %d\n", super.s_inodes_per_group);
    read_group_descs(fd, group, (int)num_groups);

    struct details jpg_files[1000]; // a details array that store the information of parsed jpg files
    int jpg_index = 0;  // the length of jpg_files
    //__u32 inodes_count = super.s_inodes_count / num_groups; 
//    __u32 inodes_count = super.s_inodes_per_group;
  //  printf("number of inodes per group %d\n", super.s_inodes_per_group);

//    int inode_number = 0;
    // parse jpg files
    for (int i = 0; i < (int) num_groups; i++)
    {
        printf("group number %d\n", i);
	off_t inode_map_addr = locate_inode_table(i, group); // get the addr of the inode map
	 printf("address offset: %ld\n",inode_map_addr);
	if(inode_map_addr == 0)
		continue;
	//off_t data_blocks_addr = locate_data_blocks(i, &group); // get the addr of the data map
       // printf("address offset: %ld\n",inode_map_addr); 
	for (int j = 1; j <=(int) inodes_count; j++)
        {
	   // printf("check\n");
   	    struct ext2_inode inode_curr;
            struct ext2_inode *inode;
            read_inode(fd, inode_map_addr, j, &inode_curr, 256); // read this inode to inode
          //  printf("inode index: %d\n", j);
	    inode = &inode_curr;
	    if (inode == NULL)
            {
                //printf("NULL!!!\n");
                continue;
            }
	   // else if (inode->i_links_count == 0)
           // {
                //printf("no link\n");
             //   continue;
           // }
            if (S_ISREG(inode->i_mode))
            {
                // this inode represents a regular file
		int inode_number = j + i * inodes_count;
		printf("inode number: %d\n", inode_number);
                parse_regular_file(fd, inode, inode_number, argv[2], jpg_files, &jpg_index);
            }
        }
    }

   // inode_number = 0;
    //int top_secret_inode_number = 0;
    // parse directory
    for (int i = 0; i < (int)num_groups; i++)
    {
        off_t inode_map_addr = locate_inode_table(i, group); // get the addr of the inode map
        if(inode_map_addr == 0)
                continue;
	for (int j = 1; j <= (int)inodes_count; j++)
        {
            struct ext2_inode inode_curr;
            struct ext2_inode *inode;
            read_inode(fd, inode_map_addr, j, &inode_curr, 256); // read this inode to inode
            inode = &inode_curr;
            if (inode == NULL)
            {
                printf("NULL!!!\n");
                continue;
            }
            if (S_ISDIR(inode->i_mode))
            {
	       parse_directory(fd, inode, argv[2], jpg_files, &jpg_index);
            }
        }
    }
    return 0;
}
