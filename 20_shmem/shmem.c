char * check_mq_shmem_name_alloc (const char *raw_name)
{
    //function checks queue name and forms correct one with (maybe) adding first '/'
    if (raw_name[0] == 0)
        return NULL;
    //check '/'
    size_t i = 1;
    for (; raw_name[i] != 0; i++) {
        if (raw_name[i] == '/') {
            printf ("wrong name, it must not contain '/' in the middle\n");
            return NULL;
        }
    }
    size_t raw_name_len = i;
    //check length
    if (raw_name_len + 1 > NAME_MAX) { // +1 to include '\0'
        printf ("name too long, should be less than NAME_MAX\n");
        return NULL;
    }
    //form correct name
    char *name = (char *) calloc (raw_name_len + 2, sizeof (char)); // 1 for '\0', one for '/'
    if (name == NULL) {
        printf ("can't allocate memory for name\n");
        return NULL;
    }
    name[0] = '/';
    if (raw_name[0] != '/')
        strcpy (&name[1], raw_name);
    else 
        strcpy (name, raw_name);
    return name;
}

void * shared_mem_init (const char *name, size_t size, int oflag, mode_t mode, int prot, int flags, off_t offset)
{
    //init and map shmem
    int shm_fd = shm_open (name, oflag, mode);
    if (shm_fd == -1) {
        perror ("can't open shmem");
        return MAP_FAILED;
    }
    if (oflag & O_CREAT) {
        if (ftruncate (shm_fd, size)) {
            perror ("truncating failed");
            close (shm_fd);
            return MAP_FAILED;
        }
    }
    void *mem = (void *) mmap (NULL, size, prot, flags, shm_fd, offset);
    close (shm_fd); // no longer needed 
    if (time == MAP_FAILED) {
        perror ("can't init shared memory");
        return MAP_FAILED;
    }
    return mem;
}