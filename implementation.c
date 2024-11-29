/*

  MyFS: a tiny file-system written for educational purposes

  MyFS is 

  Copyright 2018-21 by

  University of Alaska Anchorage, College of Engineering.

  Copyright 2022-24

  University of Texas at El Paso, Department of Computer Science.

  Contributors: Christoph Lauter 
                ... and
                ...

  and based on 

  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall myfs.c implementation.c `pkg-config fuse --cflags --libs` -o myfs

*/

#include <stddef.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>


/* The filesystem you implement must support all the 13 operations
   stubbed out below. There need not be support for access rights,
   links, symbolic links. There needs to be support for access and
   modification times and information for statfs.

   The filesystem must run in memory, using the memory of size 
   fssize pointed to by fsptr. The memory comes from mmap and 
   is backed with a file if a backup-file is indicated. When
   the filesystem is unmounted, the memory is written back to 
   that backup-file. When the filesystem is mounted again from
   the backup-file, the same memory appears at the newly mapped
   in virtual address. The filesystem datastructures hence must not
   store any pointer directly to the memory pointed to by fsptr; it
   must rather store offsets from the beginning of the memory region.

   When a filesystem is mounted for the first time, the whole memory
   region of size fssize pointed to by fsptr reads as zero-bytes. When
   a backup-file is used and the filesystem is mounted again, certain
   parts of the memory, which have previously been written, may read
   as non-zero bytes. The size of the memory region is at least 2048
   bytes.

   CAUTION:

   * You MUST NOT use any global variables in your program for reasons
   due to the way FUSE is designed.

   You can find ways to store a structure containing all "global" data
   at the start of the memory region representing the filesystem.

   * You MUST NOT store (the value of) pointers into the memory region
   that represents the filesystem. Pointers are virtual memory
   addresses and these addresses are ephemeral. Everything will seem
   okay UNTIL you remount the filesystem again.

   You may store offsets/indices (of type size_t) into the
   filesystem. These offsets/indices are like pointers: instead of
   storing the pointer, you store how far it is away from the start of
   the memory region. You may want to define a type for your offsets
   and to write two functions that can convert from pointers to
   offsets and vice versa.

   * You may use any function out of libc for your filesystem,
   including (but not limited to) malloc, calloc, free, strdup,
   strlen, strncpy, strchr, strrchr, memset, memcpy. However, your
   filesystem MUST NOT depend on memory outside of the filesystem
   memory region. Only this part of the virtual memory address space
   gets saved into the backup-file. As a matter of course, your FUSE
   process, which implements the filesystem, MUST NOT leak memory: be
   careful in particular not to leak tiny amounts of memory that
   accumulate over time. In a working setup, a FUSE process is
   supposed to run for a long time!

   It is possible to check for memory leaks by running the FUSE
   process inside valgrind:

   valgrind --leak-check=full ./myfs --backupfile=test.myfs ~/fuse-mnt/ -f

   However, the analysis of the leak indications displayed by valgrind
   is difficult as libfuse contains some small memory leaks (which do
   not accumulate over time). We cannot (easily) fix these memory
   leaks inside libfuse.

   * Avoid putting debug messages into the code. You may use fprintf
   for debugging purposes but they should all go away in the final
   version of the code. Using gdb is more professional, though.

   * You MUST NOT fail with exit(1) in case of an error. All the
   functions you have to implement have ways to indicated failure
   cases. Use these, mapping your internal errors intelligently onto
   the POSIX error conditions.

   * And of course: your code MUST NOT SEGFAULT!

   It is reasonable to proceed in the following order:

   (1)   Design and implement a mechanism that initializes a filesystem
         whenever the memory space is fresh. That mechanism can be
         implemented in the form of a filesystem handle into which the
         filesystem raw memory pointer and sizes are translated.
         Check that the filesystem does not get reinitialized at mount
         time if you initialized it once and unmounted it but that all
         pieces of information (in the handle) get read back correctly
         from the backup-file. 

   (2)   Design and implement functions to find and allocate free memory
         regions inside the filesystem memory space. There need to be 
         functions to free these regions again, too. Any "global" variable
         goes into the handle structure the mechanism designed at step (1) 
         provides.

   (3)   Carefully design a data structure able to represent all the
         pieces of information that are needed for files and
         (sub-)directories.  You need to store the location of the
         root directory in a "global" variable that, again, goes into the 
         handle designed at step (1).
          
   (4)   Write __myfs_getattr_implem and debug it thoroughly, as best as
         you can with a filesystem that is reduced to one
         function. Writing this function will make you write helper
         functions to traverse paths, following the appropriate
         subdirectories inside the file system. Strive for modularity for
         these filesystem traversal functions.

   (5)   Design and implement __myfs_readdir_implem. You cannot test it
         besides by listing your root directory with ls -la and looking
         at the date of last access/modification of the directory (.). 
         Be sure to understand the signature of that function and use
         caution not to provoke segfaults nor to leak memory.

   (6)   Design and implement __myfs_mknod_implem. You can now touch files 
         with 

         touch foo

         and check that they start to exist (with the appropriate
         access/modification times) with ls -la.

   (7)   Design and implement __myfs_mkdir_implem. Test as above.

   (8)   Design and implement __myfs_truncate_implem. You can now 
         create files filled with zeros:

         truncate -s 1024 foo

   (9)   Design and implement __myfs_statfs_implem. Test by running
         df before and after the truncation of a file to various lengths. 
         The free "disk" space must change accordingly.

   (10)  Design, implement and test __myfs_utimens_implem. You can now 
         touch files at different dates (in the past, in the future).

   (11)  Design and implement __myfs_open_implem. The function can 
         only be tested once __myfs_read_implem and __myfs_write_implem are
         implemented.

   (12)  Design, implement and test __myfs_read_implem and
         __myfs_write_implem. You can now write to files and read the data 
         back:

         echo "Hello world" > foo
         echo "Hallo ihr da" >> foo
         cat foo

         Be sure to test the case when you unmount and remount the
         filesystem: the files must still be there, contain the same
         information and have the same access and/or modification
         times.

   (13)  Design, implement and test __myfs_unlink_implem. You can now
         remove files.

   (14)  Design, implement and test __myfs_unlink_implem. You can now
         remove directories.

   (15)  Design, implement and test __myfs_rename_implem. This function
         is extremely complicated to implement. Be sure to cover all 
         cases that are documented in man 2 rename. The case when the 
         new path exists already is really hard to implement. Be sure to 
         never leave the filessystem in a bad state! Test thoroughly 
         using mv on (filled and empty) directories and files onto 
         inexistant and already existing directories and files.

   (16)  Design, implement and test any function that your instructor
         might have left out from this list. There are 13 functions 
         __myfs_XXX_implem you have to write.

   (17)  Go over all functions again, testing them one-by-one, trying
         to exercise all special conditions (error conditions): set
         breakpoints in gdb and use a sequence of bash commands inside
         your mounted filesystem to trigger these special cases. Be
         sure to cover all funny cases that arise when the filesystem
         is full but files are supposed to get written to or truncated
         to longer length. There must not be any segfault; the user
         space program using your filesystem just has to report an
         error. Also be sure to unmount and remount your filesystem,
         in order to be sure that it contents do not change by
         unmounting and remounting. Try to mount two of your
         filesystems at different places and copy and move (rename!)
         (heavy) files (your favorite movie or song, an image of a cat
         etc.) from one mount-point to the other. None of the two FUSE
         processes must provoke errors. Find ways to test the case
         when files have holes as the process that wrote them seeked
         beyond the end of the file several times. Your filesystem must
         support these operations at least by making the holes explicit 
         zeros (use dd to test this aspect).

   (18)  Run some heavy testing: copy your favorite movie into your
         filesystem and try to watch it out of the filesystem.

*/

/* Helper types and functions */

#define FS_MAGIC_NUMBER ((uint32_t)0xDEADBEEF)
#define FS_SIZE ((size_t)1024)
#define FILENAME_SIZE ((size_t)255)

typedef unsigned int u_int;
typedef size_t __myfs_offset_t;

/* allocate is a struct thgat stores the remaining memory size
   to be allocated and and offset that works as a pointer for 
   the next memory space */

/**
 * allocate manages a memory block 
 * it stores the remaining available space
 * a pointer (which is really an offset) to the next available block
 * producing a linked list effect.
 * 
 * */ 
typedef struct allocate 
{
      size_t remaining_space;
      __myfs_offset_t next;
} Allocate; 

typedef struct __handler_struct 
{
      uint32_t magic_flag;
      __myfs_offset_t root;
      __myfs_offset_t available;
      size_t size;
}  handler_struct_t;

typedef struct list 
{    
      /*first */
      __myfs_offset_t f_space;
} list_s;

typedef struct file_block 
{
      size_t size;
      size_t allocated;
      __myfs_offset_t data;
      __myfs_offset_t next_file_block;
} file_block;

typedef struct index_node_file {
      size_t size;
      __myfs_offset_t first_block;
      
} file_t;
typedef struct index_directory 
{           
      size_t children_num;
      __myfs_offset_t children;
} directory_t;    

typedef struct index_node
{
      char name[FILENAME_MAX + ((size_t)1)];
      /*1 for directory 0 for file*/
      char is_directory;
      /*used to store the time for access and modification*/
      struct timespec times[2];
      union 
      {
            file_t file; 
            directory_t directory;  
      } type;

} node_t;

/* YOUR HELPER FUNCTIONS GO HERE */

void *my_malloc(void *fsptr, void *pref_pointer, size_t *size);
void *my_realloc(void *fsptr, void *origin_pointer, size_t *size);
void my_free(void *fsptr, void *pointer);
void addAndAllocationSpace(void *fsptr, list_s *LinkedList, Allocate *orgin_pointer);
void *get_allocation(void *fsptr, list_s *LinkedList, Allocate *origin_pointer, size_t *size);
void *offset_to_pointer(void *fsptr, __myfs_offset_t offset);


void *offset_to_pointer(void *fsptr, __myfs_offset_t offset) {
      void *ptr = fsptr + offset;
      if (ptr < fsptr) return NULL;
      return ptr;
}

__myfs_offset_t pointer_to_offset(void *fsptr, void *ptr){
      if (fsptr > ptr) return 0;
      return ((__myfs_offset_t)(ptr - fsptr));
}


/*updates the date/time of the file/node*/
/*checked*/
void update_date(node_t *node, int mode){
      if (node == NULL) return;
      struct timespec time;
      if (clock_gettime(CLOCK_REALTIME, &time)== 0){
            node->times[0] = time; 
            if (mode) node->times[1] = time;
      }
}

/**Type cast and returns a pointer to available memory*/
void *get_available_memory(void *fsptr){
      return &((handler_struct_t *)fsptr)->available;
}

void handler(void *fsptr, size_t file_system_size){
      handler_struct_t * handle = ((handler_struct_t *) fsptr);
      /*Mounting for first time*/
      if (handle->magic_flag != FS_MAGIC_NUMBER){
            /*Setting flag for magic number*/
            handle->magic_flag = FS_MAGIC_NUMBER;
            /*Setting the file system size*/
            handle->available = file_system_size;
            /*setting space for root directory*/
            handle->root = sizeof(handler_struct_t);
            /*Setting a node for root directory*/
            node_t *root = offset_to_pointer(fsptr, handle->root);
            /*Filling the name with \0*/
            memset(root->name, '\0', FILENAME_SIZE + ((size_t)1));
            /*copying the name for root "file" */
            memcpy(root->name, "/", strlen("/"));
            /*updating date*/
            update_date(root, 1);
            /*setting directory*/
            root->is_directory = 1;
            /*defining type*/
            directory_t *dict = &root->type.directory;
            /*number of children*/
            dict->children_num = ((size_t)1);

            /*all chilren will start after the root node and ptr is set to 0 as root does not have any parent
             * And after that we set the available memory pointer
             * and set a default value to memory of 0
            */
            size_t *children_size = offset_to_pointer(fsptr, handle->root + sizeof(node_t));
            *children_size = 4 * sizeof(__myfs_offset_t);
            dict->children = pointer_to_offset(fsptr, ((void*)children_size) + sizeof(size_t));
            __myfs_offset_t *ptr = offset_to_pointer(fsptr, dict->children);
            *ptr = 0;
            handle->available = dict->children + *children_size;
            list_s *LinkedList = get_available_memory(fsptr);
            Allocate *free = (offset_to_pointer(fsptr, LinkedList->f_space));
            free->remaining_space = file_system_size - handle->available - sizeof(size_t);
            memset(((void *) free) + sizeof(size_t), 0, free->remaining_space);
      }
}

/*This function extracts the last path component*/
char *extract_last_path_component(const char* path, unsigned long *component_length){

      /*Getting the length of the last component in the path*/
      unsigned long length = strlen(path);
      unsigned long index = length - 1;
      while (index > 0){
            if (path[index]=='/') break;
            index--;
      }
      index+=1;
      *component_length = length - index;

      void *ptr = malloc((*component_length + 1) * sizeof(char));

      if (ptr == NULL) return NULL;

      char *component_copy = (char *) ptr;
      strcpy(component_copy, &path[index]);
      component_copy[*component_length] = '\0';
      return component_copy;
}
/**all check until this point*/
/*Splits path and stores it into an array of strings*/
char **split_path(const char token, const char *path, int exclude_tokens){
      int num_tokens = 0;
      /*Counting the number of splits*/
      for (const char *c = path; *c!='\0'; c++){
            if (*c == token){
                  num_tokens+=1;
            }
      }
      num_tokens -= exclude_tokens;
      char **tokens = (char**) malloc(((u_int)(num_tokens + 1)) * sizeof(char *));
      const char *start = &path[1];
      const char *end = start;
      char *temp_token;
      /*We are now populating the with the tokens*/
      for (int i = 0; i < num_tokens; i++){
            while ((*end != token) && (*end!='\0')) end+=1;
            /*allocating space*/
            temp_token = (char *) malloc((((u_int)(end-start))+ ((u_int) 1)) * sizeof(char));
            memcpy(temp_token, start, ((size_t)(end-start)));
            temp_token[end-start] = '\0';
            tokens[i] = temp_token;
            end+=1;
            start = end;
      }
      tokens[num_tokens] = NULL;
      return tokens;
}
/*Double checked*/
void clean_path(char **tokens){
      char **paths = tokens;
      while (*paths){
            free(*paths);
            paths++;
      }
      free(tokens);
}
/*Gets a pointer/offset to the parent, then looks for the target node/file in its children */
/*checked*/
node_t *getNode(void *fsptr, directory_t *dict, const char *child){
      size_t total_chilren = dict->children_num;
      __myfs_offset_t *children = offset_to_pointer(fsptr, dict->children);
      node_t *node = NULL;
      /*return if it is under root*/
      if (strcmp(child, "..") == 0) {
            return ((node_t *)offset_to_pointer(fsptr, children[0]));
      }
      size_t i = ((size_t)1);
      while (i < total_chilren){
            node = ((node_t *)offset_to_pointer(fsptr, children[i]));
            if (strcmp(node->name, child) == 0) {
                  return node;
            }
            i++;
      }
      return NULL;
}
/* The function resolves the path to node:
 * checks for valid input
 * gets node
 * and returns node
*/
/*doubled checked*/
node_t *resolve_path_to_node(void *fsptr, const char *path, int exclude_tokens){
      if (*path != '/') {
            return NULL;
      }
      node_t *node = offset_to_pointer(fsptr, ((handler_struct_t *)fsptr)->root);
      /*returns node when first element is null*/
      if (path[1] =='\0') {
            return node;
      }
      char **components = split_path('/', path, exclude_tokens);
      
      for (char **component = components; *component; component++){
            /*edge case chilren are in a file*/
            if (!node->is_directory){
                  clean_path(components);
                  return NULL;
            }
            if (strcmp(*component, ".") !=0){
                  node = getNode(fsptr, &node->type.directory, *component);
                  if (node == NULL) {
                        clean_path(components);
                        return NULL;
                  }
            }
      }
      clean_path(components);
      return node;
}

/*It will get a path and check if it is a file to be created
 * if that is the case, we will get the parent and a node will be created
 * out of the file specified.
*/
/*function double checked missing nested functiosns*/
node_t *newNode(void *fsptr, const char *path, int *error, int is_file){
      node_t *parent = resolve_path_to_node(fsptr, path, 1);
      if (parent == NULL){
            *error = ENOENT;
            return NULL;
      }
      /*if it is not a directory we do not want it since we need the file's parent to be a dictetory not a file*/
      if (!parent->is_directory){
            *error = ENOTDIR;
            return NULL;
      }
      directory_t *directory = &parent->type.directory;

      /*We get the file name*/
      unsigned long length;
      char *newNodeName = extract_last_path_component(path, &length);
      /*validate name and allocating space*/
      if (getNode(fsptr, directory, newNodeName) != NULL) {
            *error = EEXIST;
            return NULL;
      }
      if ((length == 0) || (length > FILENAME_MAX)) return NULL;

      __myfs_offset_t *current_children = offset_to_pointer(fsptr, directory->children);
      Allocate *space = (((void *)current_children) - sizeof(size_t));
      /*creating node and setting it to the correct parent 
       * reallocating the chilren in case the parent has childrens
       * and checking that we have enough space for them
      */
      size_t valid_children = (space->remaining_space) / sizeof(__myfs_offset_t);
      size_t space_needed;
      if (valid_children == directory->children_num){
            space_needed = space->remaining_space*2;
            void *new_children = my_realloc(fsptr, current_children, &space_needed);
            if (space_needed != 0) {
                  *error = ENOSPC;
                  return NULL;
            }
            directory->children = pointer_to_offset(fsptr, new_children);
            current_children = ((__myfs_offset_t *)new_children);
      }
      space_needed = sizeof(node_t);
      node_t *nodeCreated = (node_t *) my_malloc(fsptr, NULL, &space_needed);
      if (space_needed != 0){
            my_free(fsptr, nodeCreated);
            *error = ENOSPC;
            return NULL;
      }
      if (nodeCreated == NULL){
            my_free(fsptr, nodeCreated);
            *error= ENOSPC;
            return NULL;
      }
      memset(nodeCreated->name, '\0', FILENAME_MAX + ((size_t)1));
      memcpy(nodeCreated->name, newNodeName, length);
      update_date(nodeCreated, 1);

      current_children[directory->children_num] = pointer_to_offset(fsptr, nodeCreated);
      directory->children_num++;
      update_date(parent, 1);

      if (is_file){
            nodeCreated->is_directory = 0;
            file_t *file = &nodeCreated->type.file;
            file->size = 0;
            file->first_block = 0;
      } else {
            nodeCreated->is_directory = 1;
            directory = &nodeCreated->type.directory;
            directory->children_num = ((size_t)1);
            space_needed = 4 * sizeof(__myfs_offset_t);
            __myfs_offset_t *ptr = ((__myfs_offset_t*)my_malloc(fsptr, NULL, &space_needed));
            if (space_needed != 0){
                  my_free(fsptr, ptr);
                  *error = ENOSPC;
                  return NULL;
            }     
            if (ptr == NULL){
                  my_free(fsptr, ptr);
                  *error = ENOSPC;
                  return NULL;
            }
            directory->children = pointer_to_offset(fsptr, ptr);
            *ptr = pointer_to_offset(fsptr, parent); 
      }
      return nodeCreated;
}


/*frees memory from file*/
void clean_file(void *fsptr, file_t *file){
      file_block *space = offset_to_pointer(fsptr, file->first_block);
      file_block *next;
      /*similar to a linked list we get the current element and the next and remove the current element*/
      while (((void *)space)!=fsptr){
            my_free(fsptr, offset_to_pointer(fsptr, space->data));
            next = offset_to_pointer(fsptr, space->next_file_block);
            my_free(fsptr, space);
            space = next;
      }
}

/*Gets the parent and the target node to be removed*/
void removeNode(void *fsptr, directory_t *directory, node_t *node){
      size_t total_children = directory->children_num;
      __myfs_offset_t *children = offset_to_pointer(fsptr, directory->children);
      __myfs_offset_t temp_node = pointer_to_offset(fsptr, node);
      size_t i = 1;
      while(i < total_children){
            if (children[i] == temp_node) break;
            i+=1;
      }

      /*sliding the nodes/files the left by one*/
      my_free(fsptr, node);
      while (i < total_children-1){
            children[i] = children[i+1];
            i+=1;
      }

      /*update last child and update number of children*/
      children[i] = ((__myfs_offset_t)0);
      directory->children_num--;
      /*we will try to reduce memory space and still have enough space for the files that we need*/
      size_t new_size = (*((size_t *)children) -1) / sizeof(__myfs_offset_t);
      new_size<<=1;
      /*checks that the potential new size is at least the number of chilren, it is greater than the allocation needed and that it is greater than 4
       * if that is the case we will allocate the space and set it to the parent
      */
      if ((new_size >= directory->children_num) && (new_size * sizeof(__myfs_offset_t) >= sizeof(Allocate)) && (new_size>=4)){
            Allocate *temp = ((Allocate *)&children[new_size]);
            temp->remaining_space = new_size *sizeof(__myfs_offset_t) - sizeof(size_t);
            temp->next = 0;
            my_free(fsptr, temp);
            size_t *updated_size = (((size_t *)children) - 1);
            *updated_size -= (temp->remaining_space - sizeof(size_t));
      }
}


/*the function finds where the data is going to be removed in case there is data*/
void removeData(void *fsptr, file_block *block,  size_t size){
      if (block == NULL) return;
      size_t index = 0;
      /*Getting place to remove the block*/
      while (size != 0){
            if (size > block->allocated){
                  size-=block->allocated;
                  block = offset_to_pointer(fsptr, block->next_file_block);
            } else {
                  index = size;
                  size = 0;
            }
      }

      /*Once we have the location we remove it using index as reference
       * We make the header and free the reaming part
      */
      if ((index + sizeof(Allocate)) < block->allocated){
            size_t *temp = (size_t *)&((char *)offset_to_pointer(fsptr, block->data))[index];
            *temp  = block->allocated - index - sizeof(size_t);
            my_free(fsptr, ((void *)temp) + sizeof(size_t));
      }
      /*now we need to remove all data and sections after the one we just worked on*/
      block = offset_to_pointer(fsptr, block->next_file_block);
      file_block *temp;
      while (block != fsptr){
            my_free(fsptr, offset_to_pointer(fsptr, block->data));
            temp = offset_to_pointer(fsptr, block->next_file_block);
            my_free(fsptr, block);
            block = temp;
      }
}

int appendData(void *fsptr, file_t *file, size_t size){
      /*getting the pointer from offset*/
      file_block *block = offset_to_pointer(fsptr, file->first_block);
      /*setting up variables*/
      file_block *prevTempBlock = NULL;
      file_block *tempBlock;
      size_t spaceNeeded;
      size_t appendBytesNumber;
      void *dataBlock;
      size_t initialFileSize = file->size;
      /*checks wather it had information else it adds it*/
      if (((void *) block) == fsptr){
            spaceNeeded = sizeof(file_block);
            block = my_malloc(fsptr, NULL, &spaceNeeded);
            if (spaceNeeded != 0){
                  return -1;
            }

            file->first_block = pointer_to_offset(fsptr, block);
            spaceNeeded = size;
            dataBlock = my_malloc(fsptr, NULL, &spaceNeeded);
            /*setting up block*/
            block->size = size - spaceNeeded;
            block->allocated = block->size;
            block->data = pointer_to_offset(fsptr, dataBlock);
            block->next_file_block = 0;
            size-=block->size;
      } else {

            /*We will find out the space that will be filled with 0's in*/
            while (block->next_file_block != 0)
            {
                  size -= block->allocated;
                  block = offset_to_pointer(fsptr, block->next_file_block);
            }
            appendBytesNumber = (block->size - block->allocated) ? size : (block->size - block->allocated);
            dataBlock = &((char *)offset_to_pointer(fsptr, block->data))[block->allocated];
            memset(dataBlock, 0, appendBytesNumber);
            block->allocated += appendBytesNumber;
            size-=appendBytesNumber;      
      }
      /*we return after we are done with extra space*/
      if (size == ((size_t)0)) return 0;
      /*if we need more space we continue adding*/
      size_t prevSize = block->allocated;
      spaceNeeded = size;
      dataBlock = ((char *)offset_to_pointer(fsptr, block->data));
      size_t newDataBlockSize;
      void *newDataBlock = my_malloc(fsptr, dataBlock, &spaceNeeded);
      block->size = *(((size_t *)dataBlock) - 1);
      if (newDataBlock == NULL){
            if (spaceNeeded != 0) {
                  return -1;
            } else {
                  appendBytesNumber = (block->size - block->allocated) >= size ? size : block->size - block->allocated;
                  memset(&((char *)offset_to_pointer(fsptr, block->data))[prevSize], 0, appendBytesNumber);
                  block->allocated+=appendBytesNumber;
                  size = 0;
            }     
      } else {
            /*after extending we add anything remaining to the block*/
            appendBytesNumber = block->size - block->allocated;
            memset(&((char *)offset_to_pointer(fsptr, block->data))[prevSize], 0, appendBytesNumber);
            block->allocated += appendBytesNumber;
            size -= appendBytesNumber;

            size_t tempSize;
            tempBlock = block;

            /*We collect all data/blocks */
            for (;;){
                  /*we get the size of the new block*/
                  newDataBlockSize = *(((size_t *)newDataBlock) - 1);
                  /*save the pointer to the next block to the previous block*/
                  if (prevTempBlock != NULL) prevTempBlock->next_file_block = pointer_to_offset(fsptr, tempBlock);
                  /*We set the information*/
                  tempBlock->size = newDataBlockSize;
                  tempBlock->allocated = spaceNeeded == 0 ? size : newDataBlockSize;
                  tempBlock->data = pointer_to_offset(fsptr, newDataBlock);
                  tempBlock->next_file_block = ((__myfs_offset_t)0);
                  memset(newDataBlock, 0, tempBlock->allocated);
                  size -= tempBlock->allocated;
                  prevTempBlock = tempBlock;
                  /*Exit after done*/
                  if (size == 0) break;
                  /*preparing for the next block*/
                  spaceNeeded = size;
                  newDataBlock = my_malloc(fsptr, NULL, &spaceNeeded);
                  tempSize= sizeof(file_block);
                  tempBlock = my_malloc(fsptr, NULL, &tempSize);
                  if ((newDataBlock  == NULL) || (tempBlock == NULL) || (tempSize != 0)) {
                        removeData(fsptr, offset_to_pointer(fsptr, file->first_block), initialFileSize);
                        return -1;
                  }
            }
      }
      return 0;
}

/*Memory allocation own impementation for the system*/
/*chekced*/
void addAndAllocationSpace(void *fsptr, list_s *LL, Allocate *allocation) {
      Allocate *temp;
      __myfs_offset_t tempOffset = LL->f_space;
      __myfs_offset_t allocationOffset = pointer_to_offset(fsptr, allocation);

      /*The space is location is before the first space in the list*/
      if (tempOffset > allocationOffset) {
            /*We update the first space to the new offset*/
            LL->f_space = allocationOffset; 
            /*We check that it we combined both sections*/
            if ((allocationOffset+ sizeof(size_t) + allocation->remaining_space) == tempOffset) {
                  /*converting the offset to the pointer*/
                  temp = offset_to_pointer(fsptr, tempOffset);
                  /*we merge the spaces*/
                  allocation->remaining_space += sizeof(size_t) + temp->remaining_space;
                  /*update pointer/offset*/
                  allocation->next = temp->next;
            } else {
                  /*otherwise we add it as the first space and update poitner*/
                  allocation->next = tempOffset;
            }
      } else {
            /*Figure out the next pointer/offset will be used*/
            temp = offset_to_pointer(fsptr, tempOffset);
            /*get next pointer (the lowest available)*/
            while ((temp->next != 0) && (temp->next < allocationOffset)) {
                  temp = offset_to_pointer(fsptr, temp->next);
            }
            tempOffset= pointer_to_offset(fsptr, temp);
            /*we need to check that the next space in the temp variable is not Null if is the case we point next to it*/
            __myfs_offset_t nextAllocationOffset = temp->next;
            if (nextAllocationOffset != 0) {
                  /*Merging if possible*/
                  if ((allocationOffset + sizeof(size_t) + allocation->remaining_space) == nextAllocationOffset) {
                        Allocate *after_alloc = offset_to_pointer(fsptr, nextAllocationOffset);
                        allocation->remaining_space += sizeof(size_t) + after_alloc->remaining_space;
                        allocation->next = after_alloc->next;
                  } else {
                        allocation->next = nextAllocationOffset;
                  }
            } else {
                  /*the allocation happend at the end so we set next equals to 0*/
                  allocation->next = 0;
            }
            /* Merging if possible */
            if ((tempOffset + sizeof(size_t) + temp->remaining_space) == allocationOffset) {
                  temp->remaining_space += sizeof(size_t) + allocation->remaining_space;
                  temp->next = allocation->next;
            } else {
                  temp->next = allocationOffset;
            }
      }
}

void expand_memory_block(void *fsptr, Allocate *beforePrefered, Allocate *originalPrefered, __myfs_offset_t prefferOffset, size_t *size) {
      Allocate *pref = offset_to_pointer(fsptr, prefferOffset);
      Allocate *temp;
      /*trying to get all space from the prefered block*/
      if (pref->remaining_space >= *size) {
            /*Checking if we can create an object based on the space we have remaining
             * if that is the case: 
             * 1. Create the allocate object
             * 2. set the first prefered block wiht the total size
             * 3. update pointers
            */
            if (pref->remaining_space > *size + sizeof(Allocate)) {
                  temp = ((void *)pref) + *size;
                  temp->remaining_space = pref->remaining_space - *size;
                  temp->next = pref->next;
                  originalPrefered->remaining_space += *size;
                  beforePrefered->next = prefferOffset + *size;
            } else {
                  /*there was not enough space */
                  // Add everything that the prefer block have into the original one
                  originalPrefered->remaining_space += pref->remaining_space;

                  // Update pointers so the one that was pointing to the prefer free block
                  // is now pointing to the next free
                  beforePrefered->next = pref->next;
            }
            *size = ((__myfs_offset_t)0);
      } else {
            /*Could not add everything from the wanted block so we get what we can*/
            originalPrefered->remaining_space += pref->remaining_space;
            /*updates pointer*/
            beforePrefered->next = pref->next;
            /*updates size*/
            *size -= pref->remaining_space;
      }
      return;
}

/* this function tries to get a block of any size in cae the pointer we want is 0 
 * else we look for a block and get what we can and so on and get the most out of 
 * the largest clock available
 */
void *get_allocation(void *fsptr, list_s *LL, Allocate *originalPrefered, size_t *size) {
      /*initially we set offset to be zero */
      __myfs_offset_t preferedOffset = ((__myfs_offset_t)0);
      /*previous space and offset*/
      __myfs_offset_t beforeTempOffset;
      Allocate *beforeTemp;
      /*Current space offset*/
      __myfs_offset_t tempOffset;
      Allocate *temp;
      /*Enough space available*/
      Allocate *beforeLargest = NULL;
      __myfs_offset_t largestOffset;
      Allocate *largest;
      size_t larestSize;
      /*pointer*/
      Allocate *ptr = NULL;
      /*alocating the first space*/
      beforeTempOffset = LL->f_space;
      /*We used all available space*/
      if (!beforeTempOffset) return NULL;
      /*checking that we have enough space*/
      if (*size < sizeof(Allocate)) *size = sizeof(Allocate);

      if (((void *) originalPrefered) != fsptr) {
            /*checking if it is enough space to what we need*/
            preferedOffset = pointer_to_offset(fsptr, originalPrefered) + sizeof(size_t) + originalPrefered->remaining_space;
            if (preferedOffset == beforeTempOffset) {
                  expand_memory_block(fsptr, ((void *)LL) - sizeof(size_t), originalPrefered, preferedOffset, size);
                  if (*size == ((size_t)0)) return NULL;
            }
      }
      /*largest block is the beforeTemp variable */
      beforeTempOffset = LL->f_space;
      beforeTemp = offset_to_pointer(fsptr, beforeTempOffset);
      largestOffset = beforeTempOffset;
      largest = beforeTemp;
      larestSize = beforeTemp->remaining_space;
      /*we take and assign the next space to the tempOffset*/
      tempOffset = beforeTemp->next;
      /*we look thorugh the list for the first block that has enough size */
      while (tempOffset != ((__myfs_offset_t)0)) {
            /*We update the offset*/
            temp = offset_to_pointer(fsptr, tempOffset);
            /* Check if we found the block we need or if the temp has enough space available than the one we just passed*/
            if ((preferedOffset == tempOffset) || (temp->remaining_space > larestSize)) {
                  /*in case both works we get the one we originally preffered else we update the largest block*/
                  if (preferedOffset == tempOffset) {
                        expand_memory_block(fsptr, beforeTemp, originalPrefered, preferedOffset, size);
                        if (size == ((__myfs_offset_t)0)) break;
                  } else {
                        beforeLargest = beforeTemp;
                        largestOffset = tempOffset;
                        largest = temp;
                        larestSize = temp->remaining_space;
                  }
            }
            /*update pointers*/
            beforeTempOffset = tempOffset;
            beforeTemp = temp;
            tempOffset = temp->next;
      }

      /*in case size is not what we want (0) we get as much as we can from the current block*/
      if ((*size != ((__myfs_offset_t)0)) && (largest != NULL)) {
            ptr = largest;
            /*checking if it is giving us what we need*/ 
            if (largest->remaining_space >= *size) {
                  /*checking if we can allocate space with the object based on size
                   * if that is the case, we create it
                  */
                  if (largest->remaining_space > *size + sizeof(alloca)) {
                        temp = ((void *)largest) + sizeof(size_t) + *size;  //
                        temp->remaining_space = largest->remaining_space - *size - sizeof(size_t);
                        temp->next = largest->next;
                        /*we update the pointer of the before the largest block  else we update the pointers 
                         * in the temp list of available blocks
                         */
                        if (beforeLargest == NULL) {
                              LL->f_space = largestOffset + *size + sizeof(size_t);
                        } else {
                              beforeLargest->next = largestOffset + *size + sizeof(size_t);
                        }
                        ptr->remaining_space = *size;
                  } else {
                        /*we could not create an allocation object,so we create get as much as we can form the largest block */
                        if (beforeLargest != NULL) {
                              beforeLargest->next = largest->next;
                        } else {
                              /*after using everything there may be no memory left*/
                              LL->f_space= ((__myfs_offset_t)0);
                        }
                  }
                  *size = ((__myfs_offset_t)0);
            } else {
                   /*we could not create an allocation object,so we create get as much as we can form the largest block and we update our pointers*/
                  if (beforeLargest == NULL) {
                        return NULL;
                  } else {
                        beforeLargest->next = largest->next;
                        *size -= largest->remaining_space;
                  }
            }
      }
      /*we return a new block if a different one was used */
      if (ptr == NULL) return NULL;
      return ((void *)ptr) + sizeof(size_t);
}
/*if size is zero we do not allocate any space*/
void *my_malloc(void *fsptr, void *pref_ptr, size_t *size) {
      if (*size == ((size_t)0)) return NULL;
      /*if no prefer pointer exist, we set it to a default value */
      if (pref_ptr == NULL) {
            pref_ptr = fsptr + sizeof(size_t);
      }
      return get_allocation(fsptr, get_available_memory(fsptr), pref_ptr - sizeof(size_t), size);
}

/*Once we allcoate and our size is less than what is actually needed, the pointer sets a flag after we used
 * what we need
*/
/*checked*/
void *my_realloc(void *fsptr, void *originalPointer, size_t *size) {
      /*no need to reallocate*/
      if (*size == ((size_t)0)) {
            my_free(fsptr, originalPointer);
            return NULL;
      }

      list_s *LL = get_available_memory(fsptr);

      /*if the potiner is equals to the original pointer we just use malloc as it is the same behavior*/
      if (originalPointer == fsptr) return get_allocation(fsptr, LL, fsptr, size);

      Allocate *alloc = (Allocate *)(((void *)originalPointer) - sizeof(size_t));
      Allocate *temp;
      void *newPointer = NULL;

      /*Handling the case when the size we will allocate is less than previous size and if it is less than what we need */
      if ((alloc->remaining_space >= *size) && (alloc->remaining_space < (*size + sizeof(Allocate)))) {
            // No new ptr was created
            newPointer = originalPointer;
      } else if (alloc->remaining_space > *size) {
            /*if the target size is less than the previous we create an object and add it to the list */
            temp = (Allocate *)(originalPointer + *size);
            temp->remaining_space = alloc->remaining_space - *size - sizeof(size_t);
            temp->next = 0;
            addAndAllocationSpace(fsptr, LL, temp);
            alloc->remaining_space = *size;
            newPointer = originalPointer;
      } else {
            /*Requesting more space we get the new space
             * ensure that it will be the size we need
             * then moving/copying the content to the new space/pointer
             * then we update the previous space
            */
            newPointer = get_allocation(fsptr, LL, fsptr, size);
            if (*size != 0) return NULL;
            memcpy(newPointer, originalPointer, alloc->remaining_space);  
            addAndAllocationSpace(fsptr, LL, alloc);
      }
      return newPointer;
}

/* Add space back to List using addAndAllocationSpace 
 * And properly moving the pointer to the start
*/
void my_free(void *fsptr, void *ptr) {
      if (ptr == NULL) return;
      addAndAllocationSpace(fsptr, get_available_memory(fsptr), ptr - sizeof(size_t));
}
/* End of helper functions */

/* Implements an emulation of the stat system call on the filesystem 
   of size fssize pointed to by fsptr. 
   
   If path can be followed and describes a file or directory 
   that exists and is accessable, the access information is 
   put into stbuf. 

   On success, 0 is returned. On failure, -1 is returned and 
   the appropriate error code is put into *errnoptr.

   man 2 stat documents all possible error codes and gives more detail
   on what fields of stbuf need to be filled in. Essentially, only the
   following fields need to be supported:

   st_uid      the value passed in argument
   st_gid      the value passed in argument
   st_mode     (as fixed values S_IFDIR | 0755 for directories,
                                S_IFREG | 0755 for files)
   st_nlink    (as many as there are subdirectories (not files) for directories
                (including . and ..),
                1 for files)
   st_size     (supported only for files, where it is the real file size)
   st_atim
   st_mtim

*/
int __myfs_getattr_implem(void *fsptr, size_t fssize, int *errnoptr,
                          uid_t uid, gid_t gid,
                          const char *path, struct stat *stbuf) {
      handler(fsptr, fssize);
      /*Resolve the path to the node*/
      node_t *node = resolve_path_to_node(fsptr, path, 0);
      if (!node) return -1;
      memset(stbuf, 0, sizeof( struct stat));
      stbuf->st_uid = uid;
      stbuf->st_gid = gid;
      if (node->is_directory){
            stbuf->st_mode = S_IFDIR | 0755;
            directory_t *directory = &node->type.directory;
            __myfs_offset_t *children = offset_to_pointer(fsptr, directory->children);
            stbuf->st_nlink = 2;
            for (int i = 1; i < directory->children_num; i++){
                  if (((node_t *)offset_to_pointer(fsptr, children[i]))->is_directory){
                        stbuf->st_nlink+=1;
                  }
            }
      } else {
            stbuf->st_mode = S_IFREG | 0755;
            stbuf->st_nlink = 1;
            stbuf->st_size = node->type.file.size;
      }
      stbuf->st_atim = node->times[0];
      stbuf->st_mtim = node->times[1];
      return 0;
}

/* Implements an emulation of the readdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   If path can be followed and describes a directory that exists and
   is accessable, the names of the subdirectories and files 
   contained in that directory are output into *namesptr. The . and ..
   directories must not be included in that listing.

   If it needs to output file and subdirectory names, the function
   starts by allocating (with calloc) an array of pointers to
   characters of the right size (n entries for n names). Sets
   *namesptr to that pointer. It then goes over all entries
   in that array and allocates, for each of them an array of
   characters of the right size (to hold the i-th name, together 
   with the appropriate '\0' terminator). It puts the pointer
   into that i-th array entry and fills the allocated array
   of characters with the appropriate name. The calling function
   will call free on each of the entries of *namesptr and 
   on *namesptr.

   The function returns the number of names that have been 
   put into namesptr. 

   If no name needs to be reported because the directory does
   not contain any file or subdirectory besides . and .., 0 is 
   returned and no allocation takes place.

   On failure, -1 is returned and the *errnoptr is set to 
   the appropriate error code. 

   The error codes are documented in man 2 readdir.

   In the case memory allocation with malloc/calloc fails, failure is
   indicated by returning -1 and setting *errnoptr to EINVAL.

*/
int __myfs_readdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, char ***namesptr) {
      handler(fsptr, fssize);
      node_t *node = resolve_path_to_node(fsptr, path, 0);
      if ( node == NULL){
            *errnoptr = ENOENT;
            return -1;

      }
      if (!node->is_directory){
            *errnoptr = ENOTDIR;
            return -1;
      }

      directory_t *directory = &node->type.directory;
      /*accounting for both . and .. files you may change it if it cases conflict*/
      if (directory->children_num ==1){
            return 0;
      }
      /*Allocating space to for the names and checking that allcoation was successful*/
      size_t total_children = directory->children_num;
      void **ptr = (void **) calloc(total_children - ((size_t)1), sizeof(char *));
      __myfs_offset_t *children = offset_to_pointer(fsptr, directory->children);
      
      if (ptr == NULL){
            *errnoptr = EINVAL;
            return -1;
      }
      /*Copying the children names to namesptr and returning the total number of children*/
      char **names  =((char **)ptr);
      size_t length;
      
      for (size_t i = ((size_t)1); i < directory->children_num; i++){
            node = ((node_t *) offset_to_pointer(fsptr, children[i]));
            length = strlen(node->name);
            names[i-1] = (char *)malloc(length+1);
            strcpy(names[i-1], node->name);
            names[i-1][length] = '\0';
      }
      *namesptr = names;
      return ((int)(total_children -1));
}

/* Implements an emulation of the mknod system call for regular files
   on the filesystem of size fssize pointed to by fsptr.

   This function is called only for the creation of regular files.

   If a file gets created, it is of size zero and has default
   ownership and mode bits.

   The call creates the file indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 mknod.

*/
int __myfs_mknod_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
      handler(fsptr, fssize);
      node_t *new_node = newNode(fsptr, path, errnoptr, 1);
      if (new_node == NULL){
            return -1;
      }
      return 0;
}

/* Implements an emulation of the unlink system call for regular files
   on the filesystem of size fssize pointed to by fsptr.

   This function is called only for the deletion of regular files.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 unlink.

*/
int __myfs_unlink_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
      handler(fsptr, fssize);

      /*we get the node from the path*/
      node_t *node = resolve_path_to_node(fsptr, path, 0);
      /*check that it is not null*/
      if (node == NULL) {
            *errnoptr = ENOENT;
            return -1;
      }
      /*check that it is a directory*/
      if (!node->is_directory) {
            *errnoptr = ENOTDIR;
            return -1;
      }

      /*checking if the directory has children*/
      directory_t *directory = &node->type.directory;
      /*getting the last element*/
      unsigned long length;
      char *filename = extract_last_path_component(path, &length);
      /*we will check that we can saftly create a new node without conflict names*/
      node_t *fileNode = getNode(fsptr, directory, filename);
      /*checking if we were able to get the node*/
      if (fileNode == NULL){
            *errnoptr = ENOENT;
            return -1;
      }
      /*if it is a directory we cannot remove it in this comamnd*/
      if (fileNode->is_directory){
            *errnoptr = EISDIR;
            return -1;
      }
      /*we remove the information*/
      file_t *file = &fileNode->type.file;
      if (file->size != 0){
            clean_file(fsptr, file);
      }
      /*removing the node*/
      removeNode(fsptr, directory, fileNode);
      return -1;
}

/* Implements an emulation of the rmdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call deletes the directory indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The function call must fail when the directory indicated by path is
   not empty (if there are files or subdirectories other than . and ..).

   The error codes are documented in man 2 rmdir.

*/
int __myfs_rmdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
      handler(fsptr, fssize);

      /*we get the node from the path*/
      node_t *node = resolve_path_to_node(fsptr, path, 0);
      /*check that it is not null*/
      if (node == NULL) {
            *errnoptr = ENOENT;
            return -1;
      }
      /*check that it is a directory*/
      if (!node->is_directory) {
            *errnoptr = ENOTDIR;
            return -1;
      }
      /*checking if the directory has children*/
      directory_t *directory = &node->type.directory;
      if (directory->children_num != 1){
            *errnoptr = ENOTEMPTY;
            return -1;
      }
      /*Getting the chilren and the parent*/
      __myfs_offset_t *children = offset_to_pointer(fsptr, directory->children);
      node_t *parent = offset_to_pointer(fsptr, *children);
      /*freeing the chilren and the directory*/
      my_free(fsptr, children);
      removeNode(fsptr, &parent->type.directory, node);
      return 0;
}

/* Implements an emulation of the mkdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call creates the directory indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 mkdir.

*/
int __myfs_mkdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
      // printf("I was reached 1");
      handler(fsptr, fssize);
      //printf("I was reached 2");
      /*creating the new node with the helper function*/
      node_t *node = newNode(fsptr, path, errnoptr, 0);
      if (node == NULL) {
            *errnoptr = ENOSPC;
            return -1;
      }
      // printf("I was reached 3");
      return 0;
}

/* Implements an emulation of the rename system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call moves the file or directory indicated by from to to.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   Caution: the function does more than what is hinted to by its name.
   In cases the from and to paths differ, the file is moved out of 
   the from path and added to the to path.

   The error codes are documented in man 2 rename.

*/
int __myfs_rename_implem(void *fsptr, size_t fssize, int *errnoptr,
                         const char *from, const char *to) {
    handler(fsptr, fssize);
    //Resolve source and destination nodes
    node_t *source_node = resolve_path_to_node(fsptr, from, 0);
    if (!source_node) {
        *errnoptr = ENOENT;
        return -1;
    }
    node_t *dest_parent_node = resolve_path_to_node(fsptr, to, 1);
    if (!dest_parent_node) {
        *errnoptr = ENOENT;
        return -1;
    }
    if (!dest_parent_node->is_directory) {
        *errnoptr = ENOTDIR;
        return -1;
    }
    //Extract new name and validate
    unsigned long name_len;
    char *new_name = extract_last_path_component(to, &name_len);
    if (!new_name || name_len == 0 || name_len > FILENAME_MAX) {
        *errnoptr = EINVAL;
        return -1;
    }
    directory_t *dest_directory = &dest_parent_node->type.directory;
    //Check for existing name conflicts
    if (getNode(fsptr, dest_directory, new_name)) {
        *errnoptr = EEXIST;
        free(new_name);
        return -1;
    }
    //Remove from the old parent's children list
    node_t *old_parent_node = resolve_path_to_node(fsptr, from, 1);
    if (!old_parent_node) {
        *errnoptr = ENOENT;
        return -1;
    }
    directory_t *old_directory = &old_parent_node->type.directory;
    removeNode(fsptr, old_directory, source_node);
    // Add to the new parent's children list
    strncpy(source_node->name, new_name, FILENAME_MAX);
    source_node->name[FILENAME_MAX] = '\0';
    __myfs_offset_t *dest_children = offset_to_pointer(fsptr, dest_directory->children);
    dest_children[dest_directory->children_num++] = pointer_to_offset(fsptr, source_node);
    update_date(old_parent_node, 1);
    update_date(dest_parent_node, 1);
    free(new_name);
    return 0;
}

/* Implements an emulation of the truncate system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call changes the size of the file indicated by path to offset
   bytes.

   When the file becomes smaller due to the call, the extending bytes are
   removed. When it becomes larger, zeros are appended.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 truncate.

*/
int __myfs_truncate_implem(void *fsptr, size_t fssize, int *errnoptr,
                           const char *path, off_t offset) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the open system call on the filesystem 
   of size fssize pointed to by fsptr, without actually performing the opening
   of the file (no file descriptor is returned).

   The call just checks if the file (or directory) indicated by path
   can be accessed, i.e. if the path can be followed to an existing
   object for which the access rights are granted.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The two only interesting error codes are 

   * EFAULT: the filesystem is in a bad state, we can't do anything

   * ENOENT: the file that we are supposed to open doesn't exist (or a
             subpath).

   It is possible to restrict ourselves to only these two error
   conditions. It is also possible to implement more detailed error
   condition answers.

   The error codes are documented in man 2 open.

*/
int __myfs_open_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path) {
      handler(fsptr, fssize);
      /*Getting the node*/
      node_t *node = resolve_path_to_node(fsptr, path, 0);
      /*verifying that node is not null*/
      if (node == NULL) {
            *errnoptr = ENOENT;
            return -1;
      }
      /*returning the node type 1 for directory and 0 for files*/
      if (node->is_directory){
            return 1;
      }
      return 0;
}

/* Implements an emulation of the read system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call copies up to size bytes from the file indicated by 
   path into the buffer, starting to read at offset. See the man page
   for read for the details when offset is beyond the end of the file etc.
   
   On success, the appropriate number of bytes read into the buffer is
   returned. The value zero is returned on an end-of-file condition.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 read.

*/
int __myfs_read_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path, char *buf, size_t size, off_t offset) {
      handler(fsptr, fssize);
      /*Checking that it is a valid offset*/
      if (offset < 0){
            *errnoptr = EFAULT;
            return -1;
      }
      /*getting the remaining of the offset*/
      size_t remaning_offset = ((size_t)offset);
      /*Getting the node*/
      node_t *node = resolve_path_to_node(fsptr, path, 0);
      if (node == NULL) {
            *errnoptr = ENOENT;
            return -1;
      }
      /*checking that it is a directory if that is the case we do not read from it*/
      if (node->is_directory){
            *errnoptr = EISDIR;
            return -1;
      }
      /*we now will check the size*/
      file_t *file = &node->type.file;
      if (remaning_offset > file->size){
            *errnoptr = EFAULT;
            return -1;
      }
      /*if size is 0 there is nothing to read*/
      if (file->size == 0){
            return 0;
      }
      size_t i = 0;
      /*loading the file block*/
      file_block *block = offset_to_pointer(fsptr, file->first_block);
      /*We will get the index where we want to start reading from*/
      while (remaning_offset != 0){
            if (remaning_offset > block->size){
                  remaning_offset = remaning_offset - block->size;
                  block = offset_to_pointer(fsptr, block->next_file_block);
            } else {
                  i = remaning_offset;
                  remaning_offset = 0;
            }
      }
      /*we start getting the information into the buffer*/
      size_t total_read_bytes = 0;
      size_t bytes_to_read;
      if (block->allocated - i) {
            bytes_to_read = block->allocated - i;
      } else {
            bytes_to_read = size;
      }
      memcpy(buf, &((char *)offset_to_pointer(fsptr, block->data))[i], bytes_to_read);
      size-=bytes_to_read;
      i = bytes_to_read;
      total_read_bytes+=bytes_to_read;
      block = offset_to_pointer(fsptr, block->next_file_block);
      /*we are now loading the information from the block file into the buffer, 
       * we do that as long as there is information in the file
       * and for that reason we are moving through the blocks fo the file
       * */
      while((size)>((size_t)0) && (block != fsptr)){
            if (size > block->allocated){
                  bytes_to_read = block->allocated;
            } else {
                  bytes_to_read = size;
            }
            /*we are copying into the buffer*/
            memcpy(&buf[i], offset_to_pointer(fsptr, block->data), bytes_to_read);
            size = size - bytes_to_read;
            i = i + bytes_to_read;
            /*we update the count*/
            total_read_bytes = total_read_bytes + bytes_to_read;
            /*we update where we will be reading from*/
            block = offset_to_pointer(fsptr, block->next_file_block);
      }
      return total_read_bytes;
}

/* Implements an emulation of the write system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call copies up to size bytes to the file indicated by 
   path into the buffer, starting to write at offset. See the man page
   for write for the details when offset is beyond the end of the file etc.
   
   On success, the appropriate number of bytes written into the file is
   returned. The value zero is returned on an end-of-file condition.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 write.

*/
int __myfs_write_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path, const char *buf, size_t size, off_t offset) {
      handler(fsptr, fssize);
      /*Checking that it is a valid offset*/
      if (offset < 0){
            *errnoptr = EFAULT;
            return -1;
      }
      /*getting the remaining of the offset*/
      size_t remaning_offset = ((size_t)offset);
      /*Getting the node*/
      node_t *node = resolve_path_to_node(fsptr, path, 0);
      if (node == NULL) {
            *errnoptr = ENOENT;
            return -1;
      }

      /*checking that it is a directory if that is the case we cannot write to it*/
      if (node->is_directory){
            *errnoptr = EISDIR;
            return -1;
      }
      /*veryfing that the is content to be read*/
      file_t *file = &node->type.file;

      if (file->size < remaning_offset){
            *errnoptr = EFBIG;
            return -1;
      }
      /*updating the modificaiton date*/
      update_date(node, 1);
      /*Adding the size to the file from file*/
      if(appendData(fsptr, file, size+1) != 0 ) return -1;
      /*setting the space for block to write*/
      size_t file_data_index = 0;
      file_block *block = offset_to_pointer(fsptr, file->first_block);
      /*Get the starting point to write to in the file*/
      while (remaning_offset!=0){
            if (remaning_offset > block->allocated){
                  remaning_offset = remaning_offset - block->allocated;
                  block = offset_to_pointer(fsptr, block->next_file_block);
            } else {
                  file_data_index = remaning_offset;
                  remaning_offset = 0;
            }
      }

      /*setting up the data block, the temp that will serve as the buffer, the index from the buffer and the character
      */
      char temp[size+1]; size_t index_b = 0; char character;
      char *file_data_block = ((char *)offset_to_pointer(fsptr, block->data));
      int done = 0;
      memcpy(temp, buf, size);
      temp[size] = '\0';
      /*we start writing at the index positon and character by character from buffer*/
      while((buf[index_b] != '\0' && ((void *)block)!=fsptr)){
            while (index_b != size){
                  /*swaping characters*/
                  character = temp[index_b];
                  temp[index_b] = file_data_block[file_data_index];
                  file_data_block[file_data_index] = character;
                  index_b+=1;
                  /*check if we copy all data and got to the last index and whather it was the last from the block*/
                  if (temp[index_b] == '\0'){
                        if (file_data_index + 1 == block->size){
                              /*moving to the next block of data*/
                              block = offset_to_pointer(fsptr, block->next_file_block);
                              file_data_block = ((char *)offset_to_pointer(fsptr, block->data));
                              file_data_block[0] = '\0';
                        } else {
                              file_data_index+=1;
                              file_data_block[file_data_index] = '\0';
                        }
                        done = 1;
                        break;
                  }

                  /*in case we did overwrite current data block we just update the index where we are writing*/
                  file_data_index+=1;
                  if (file_data_index == block->size){
                        file_data_index = 0;
                        block = offset_to_pointer(fsptr, block->next_file_block);
                        if (block->next_file_block == ((__myfs_offset_t)0)) break;
                        file_data_block = ((char *)offset_to_pointer(fsptr, block->data));
                  }
            }
            if (done) break;
            index_b = 0;
      }
      file->size+= size;
      return ((int) size);
}

/* Implements an emulation of the utimensat system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call changes the access and modification times of the file
   or directory indicated by path to the values in ts.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 utimensat.

*/
int __myfs_utimens_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, const struct timespec ts[2]) {
      handler(fsptr, fssize);
      /*getting the node*/
      node_t *node = resolve_path_to_node(fsptr, path, 0);
      if (node == NULL) {
            *errnoptr = ENOENT;
            return -1;
      }
      /*setting the times*/
      node->times[0] = ts[0];
      node->times[1] = ts[1];
      return 0;
}

/* Implements an emulation of the statfs system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call gets information of the filesystem usage and puts in 
   into stbuf.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 statfs.

   Essentially, only the following fields of struct statvfs need to be
   supported:

   f_bsize   fill with what you call a block (typically 1024 bytes)
   f_blocks  fill with the total number of blocks in the filesystem
   f_bfree   fill with the free number of blocks in the filesystem
   f_bavail  fill with same value as f_bfree
   f_namemax fill with your maximum file/directory name, if your
             filesystem has such a maximum

*/
int __myfs_statfs_implem(void *fsptr, size_t fssize, int *errnoptr,
                         struct statvfs *stbuf) {
    handler(fsptr, fssize);

    memset(stbuf, 0, sizeof(struct statvfs));
    list_s *free_list = get_available_memory(fsptr);
    __myfs_offset_t free_offset = free_list->f_space;
    size_t free_space = 0;

    while (free_offset) {
        Allocate *block = offset_to_pointer(fsptr, free_offset);
        free_space += block->remaining_space;
        free_offset = block->next;
    }

    stbuf->f_bsize = FS_SIZE;                
    stbuf->f_blocks = fssize / FS_SIZE;
    stbuf->f_bfree = free_space / FS_SIZE;
    stbuf->f_bavail = stbuf->f_bfree;
    stbuf->f_namemax = FILENAME_MAX;

    return 0;
}
