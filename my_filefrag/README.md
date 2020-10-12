# my_filefrag
very simplified filefrag.
#features
1) read blocksize using 3 methods: stat st_blksize, fstatfs f_bsize, ioctl FIGETBSZ
2) read extents info using fiemap and ioctl FS_IOC_FIEMAP
3) prints extents info: logical and physical start points; length in block sizes
