# ext2cat #

A read-only driver for ext2, a standard filesystem supported by the Linux kernel

## IMPLEMENTATION ##

### Super block  ###
The start of the super block can be determined by moving the pointer `fs` by an fixed offset defined by `SUPERBLOCK_OFFSET`

### Block size ###
Once the super block is obtained, the logarithmaic value of the block size can directly read from the `ext2_super_block` struict. According to the description of the ext2 file system, the real value of the block size in bytes is given by left shift of the `s_log_block_size`. [ext2 file system](http://www.nongnu.org/ext2-doc/ext2.html#S-LOG-BLOCK-SIZE)

### Block group descriptor ###
The block group descriptor is placed right after the super block. In our simplified ext2 disk, there is only one block group, therefore the input `block_group_num` is not used in current implementation.

### Inode ###
The start inode block is given in the block group descriptor, as stated in [ext2 file system inode](http://www.nongnu.org/ext2-doc/ext2.html#BG-INODE-TABLE). The inode number should at least be 1, therefore the inode number associated with the first inode in the inode table is 1.

#### inode from directory and file name ####
Given the inode struct of the parent directory, we are going to seek for the file/directory with a given name. The directory is save as a special file in the disk as a link list of entries. Each entry has the following attributes:

```C
struct ext2_dir_entry {
	__u32	inode;                 /* Inode number */
	__u16	rec_len;               /* Directory entry length */
	__u16	name_len;              /* Name length */
	char	name[EXT2_NAME_LEN];   /* File name */
};
```

Using the `name` and `name_len` one can compare with the given name and once they are matched we have found the corresponding `inode`. The `rec_len` is used for figuring out the pointer of next entry. Since `rec_len` is the number of bytes, we need to convert the type of the entry back and forth to move the pointer.

#### inode from the whole path ####
The whole path should firstly divided using the given function `split_path`, and the number of directories can be counted via the number of slashes in the path. Then the function `get_inode_from_dir` is recursively called for each level of directory, and the inode of the given file can be found.

### INDIRECT BLOCKS (EXTRA CREDIT) ###
The information of the indirect blocks is stored as an array of block numbers in another block. Its block number is then stored in the `EXT2_NDIR_BLOCKS` block. The size of each block number is `sizeof(__u32)`, thus one block can at most store `block_size / sizeof(__u32)` block numbers, and this is the limit of the number of indirect blocks. In the current implementation, we just go to special block record in the inode and find the block number arrays, and continue reading them until the bytes already read equals to the real size. 

## RESULTS ##

| File                             | md5sum                           |
| -------------------------------- | -------------------------------- |
| /code/ext2_headers/ext_types.h   | 730cc429e8d0ab183380ab14f51f2912 |
| /code/haskell/qsort.hs           | a7b79363f8645e4722a5d3d114311709 |
| /code/python/ouroboros.py        | ecd524e78b381b63df3d4bfcf662ce0d |
| /photos/cows.jpg                 | 3f19778ecb384018ea53869313023d8b |
| /photos/corn.jpg                 | dc049b1372bf3a6a1aea17be9eca5532 |
| /photos/loons.jpg (**Indirect**) | eb5826a89dc453409ca76560979699bb |
| /README.txt                      | c092359845cf0c2859ea9a532b7764ca |
