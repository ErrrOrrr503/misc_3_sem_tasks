#include <stdlib.h>
#include <stdio.h>
#include <sys/quota.h>
#include <xfs/xqm.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>

extern int errno;

int print_quota_info (int type, int id, const char *blkdev);

int main (int argc, char *argv[])
{
    if (argc != 2) {
        printf("usage: <%s> <mounted block device>\n", argv[0]);
        return -1;
    }
    printf ("USER quota:\n");
    print_quota_info (USRQUOTA, getuid (), argv[1]);
    printf ("GROUP quota:\n");
    print_quota_info (GRPQUOTA, getgid (), argv[1]);
    return 0;
}

int print_quota_info (int type, int id, const char *blkdev)
{
    struct dqblk disc_quota = {0};
    if (quotactl (QCMD(Q_GETQUOTA, type), blkdev, id, (char *) &disc_quota)) {
        perror ("can't get quota info");
        return -1;
    }
    printf ("%-27s : %20" PRIu64 "\n", "Absolute blocks limit", disc_quota.dqb_bhardlimit);
    printf ("%-27s : %20" PRIu64 "\n", "Preffered blocks limit", disc_quota.dqb_bsoftlimit);
    printf ("%-27s : %20" PRIu64 "\n", "Currently occupied (bytes)", disc_quota.dqb_curspace);
    printf ("%-27s : %20" PRIu64 "\n", "Abs inodes allocated", disc_quota.dqb_ihardlimit);
    printf ("%-27s : %20" PRIu64 "\n", "Pref inodes allocated", disc_quota.dqb_isoftlimit);
    printf ("%-27s : %20" PRIu64 "\n", "Cur inodes allocated", disc_quota.dqb_curinodes);
    printf ("%-27s : %20" PRIu64 "\n", "Time Limit for use", disc_quota.dqb_btime);
    printf ("%-27s : %20" PRIu64 "\n", "Time Limit for files", disc_quota.dqb_itime);
    return 0;
}