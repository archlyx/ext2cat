// ext2 definitions from the real driver in the Linux kernel.
#include "ext2fs.h"

// This header allows your project to link against the reference library. If you
// complete the entire project, you should be able to remove this directive and
// still compile your code.
//#include "reference_implementation.h"

// Definitions for ext2cat to compile against.
#include "ext2_access.h"



///////////////////////////////////////////////////////////
//  Accessors for the basic components of ext2.
///////////////////////////////////////////////////////////

// Return a pointer to the primary superblock of a filesystem.
struct ext2_super_block * get_super_block(void * fs) {
    return (struct ext2_super_block *)(fs + SUPERBLOCK_OFFSET);
}


// Return the block size for a filesystem.
// s_log_block_size is the log value of real size
__u32 get_block_size(void * fs) {
    struct ext2_super_block * super_block = get_super_block(fs);
    return 1024 << super_block->s_log_block_size;
}


// Return a pointer to a block given its number.
// get_block(fs, 0) == fs;
void * get_block(void * fs, __u32 block_num) {
    __u32 block_size = get_block_size(fs);
    return (fs + block_num * block_size);
}


// Return a pointer to the first block group descriptor in a filesystem. Real
// ext2 filesystems will have several of these, but, for simplicity, we will
// assume there is only one.
/* 
 * Here the block_group_num is not used since we only have 1 block group
 */
struct ext2_group_desc * get_block_group(void * fs, __u32 block_group_num) {
    __u32 block_size = get_block_size(fs);
    /* Superblock occupies just one block, thus the start of the block
     * group descriptor is 1 more block than SUPERBLOCK_OFFSET */
    __u32 group_num = (SUPERBLOCK_OFFSET / block_size) + 1;
    return (struct ext2_group_desc *)get_block(fs, group_num);
}


// Return a pointer to an inode given its number. In a real filesystem, this
// would require finding the correct block group, but you may assume it's in the
// first one.
struct ext2_inode * get_inode(void * fs, __u32 inode_num) {
    struct ext2_group_desc *block_group = get_block_group(fs, 0);
    struct ext2_inode *inode_start = get_block(fs, block_group->bg_inode_table);
    /* The inode number should be at least 1, thus the inode_num associated
     * with the first inode in the table is 1 */
    return inode_start + inode_num - 1;
}



///////////////////////////////////////////////////////////
//  High-level code for accessing filesystem components by path.
///////////////////////////////////////////////////////////

// Chunk a filename into pieces.
// split_path("/a/b/c") will return {"a", "b", "c"}.
//
// This one's a freebie.
char ** split_path(char * path) {
    int num_slashes = 0;
    for (char * slash = path; slash != NULL; slash = strchr(slash + 1, '/')) {
        num_slashes++;
    }

    // Copy out each piece by advancing two pointers (piece_start and slash).
    char ** parts = (char **) calloc(num_slashes, sizeof(char *));
    char * piece_start = path + 1;
    int i = 0;
    for (char * slash = strchr(path + 1, '/');
         slash != NULL;
         slash = strchr(slash + 1, '/')) {
        int part_len = slash - piece_start;
        parts[i] = (char *) calloc(part_len + 1, sizeof(char));
        strncpy(parts[i], piece_start, part_len);
        piece_start = slash + 1;
        i++;
    }
    // Get the last piece.
    parts[i] = (char *) calloc(strlen(piece_start) + 1, sizeof(char));
    strncpy(parts[i], piece_start, strlen(piece_start));
    return parts;
}


// Convenience function to get the inode of the root directory.
struct ext2_inode * get_root_dir(void * fs) {
    return get_inode(fs, EXT2_ROOT_INO);
}


// Given the inode for a directory and a filename, return the inode number of
// that file inside that directory, or 0 if it doesn't exist there.
// 
// name should be a single component: "foo.txt", not "/files/foo.txt".
__u32 get_inode_from_dir(void * fs, struct ext2_inode * dir, 
        char * name) {
    struct ext2_dir_entry *entry = get_block(fs, dir->i_block[0]);

    /* The inode of the last entry will be zero */
    while (entry->inode) {
        /* The name_len is stored as __u16 type, thus we need to 
         * convert it to unsigned char */
        if (!strncmp(entry->name, name, (unsigned char)entry->name_len))
            break;

        /* rec_len stores the offset of the next entry in bytes,
         * thus we need to convert it to char* to move the pointer */
        entry = (struct ext2_dir_entry *)((char *)entry + entry->rec_len);
    }

    return entry->inode;
}


// Find the inode number for a file by its full path.
// This is the functionality that ext2cat ultimately needs.
__u32 get_inode_by_path(void * fs, char * path) {
    /* Split the path into parts */
    char **parts = split_path(path);

    /* Get the inode of the root dir */
    struct ext2_inode *root_inode = get_root_dir(fs);
    struct ext2_inode *file_inode;
    __u32 inode_num;

    int i = 0;

    /* First count how many parts in the given path */
    int num_slashes = 0;
    for (char * slash = path; slash != NULL; slash = strchr(slash + 1, '/')) {
        num_slashes++;
    }

    /* Recursively find the inode block index of the file/dir in the path */
    file_inode = root_inode;
    for (i = 0; i < num_slashes; i++) {
        inode_num = get_inode_from_dir(fs, file_inode, parts[i]);
        file_inode = get_inode(fs, inode_num);
    }

    return inode_num;
}

