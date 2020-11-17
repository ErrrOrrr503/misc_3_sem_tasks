#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <errno.h>
#include <linux/magic.h>

//Q: FSTYPE* where defined?

extern int errno;

const char *fs_type_str(__fsword_t f_type);
int print_statfs(struct statfs *stfs);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("usage: <%s> <fs_mountpoint (dir)>\n", argv[0]);
        return -1;
    }
    struct statfs stfs = {0};
    if (statfs(argv[1], &stfs))
    {
        perror(argv[1]);
        return -1;
    }
    print_statfs(&stfs);
    return 0;
}

int print_statfs(struct statfs *stfs)
{
    printf("%-20s : %20s\n", "FS_TYPE", fs_type_str(stfs->f_type));
    //FS_ID : "Nobody knows what f_fsid is supposed to contain"
    printf("%-20s : %20ld\n", "MOUNT_FLAGS", stfs->f_flags);
    printf("%-20s : %20ld\n", "BLOCK_SIZE", stfs->f_bsize);
    printf("%-20s : %20ld\n", "BLOCKS_TOTAL", stfs->f_blocks);
    printf("%-20s : %20ld\n", "BLOCKS_FREE", stfs->f_bfree);
    printf("%-20s : %20ld\n", "BLOCKS_AVAILABLE", stfs->f_bavail);
    printf("%-20s : %20ld\n", "INODES_TOTAL", stfs->f_files);
    printf("%-20s : %20ld\n", "INODES_FREE", stfs->f_ffree);
    printf("%-20s : %20ld\n", "MAX_NAMELENGHTH", stfs->f_namelen);
    printf("%-20s : %20ld\n", "FRAGMENT_SIZE", stfs->f_frsize);
    return 0;
}

///////////////////////////////////////////////////////////////////////
//honestly borrowed from coreutils/src/stat.c

#define S_MAGIC_AAFS 0x5A3C69F0
#define S_MAGIC_ACFS 0x61636673
#define S_MAGIC_ADFS 0xADF5
#define S_MAGIC_AFFS 0xADFF
#define S_MAGIC_AFS 0x5346414F
#define S_MAGIC_ANON_INODE_FS 0x09041934
#define S_MAGIC_AUFS 0x61756673
#define S_MAGIC_AUTOFS 0x0187
#define S_MAGIC_BALLOON_KVM 0x13661366
#define S_MAGIC_BEFS 0x42465331
#define S_MAGIC_BDEVFS 0x62646576
#define S_MAGIC_BFS 0x1BADFACE
#define S_MAGIC_BINDERFS 0x6C6F6F70
#define S_MAGIC_BPF_FS 0xCAFE4A11
#define S_MAGIC_BINFMTFS 0x42494E4D
#define S_MAGIC_BTRFS 0x9123683E
#define S_MAGIC_BTRFS_TEST 0x73727279
#define S_MAGIC_CEPH 0x00C36400
#define S_MAGIC_CGROUP 0x0027E0EB
#define S_MAGIC_CGROUP2 0x63677270
#define S_MAGIC_CIFS 0xFF534D42
#define S_MAGIC_CODA 0x73757245
#define S_MAGIC_COH 0x012FF7B7
#define S_MAGIC_CONFIGFS 0x62656570
#define S_MAGIC_CRAMFS 0x28CD3D45
#define S_MAGIC_CRAMFS_WEND 0x453DCD28
#define S_MAGIC_DAXFS 0x64646178
#define S_MAGIC_DEBUGFS 0x64626720
#define S_MAGIC_DEVFS 0x1373
#define S_MAGIC_DEVPTS 0x1CD1
#define S_MAGIC_DMA_BUF 0x444D4142
#define S_MAGIC_ECRYPTFS 0xF15F
#define S_MAGIC_EFIVARFS 0xDE5E81E4
#define S_MAGIC_EFS 0x00414A53
#define S_MAGIC_EROFS_V1 0xE0F5E1E2
#define S_MAGIC_EXFS 0x45584653
#define S_MAGIC_EXOFS 0x5DF5
#define S_MAGIC_EXT 0x137D
#define S_MAGIC_EXT2 0xEF53
#define S_MAGIC_EXT2_OLD 0xEF51
#define S_MAGIC_F2FS 0xF2F52010
#define S_MAGIC_FAT 0x4006
#define S_MAGIC_FHGFS 0x19830326
#define S_MAGIC_FUSEBLK 0x65735546
#define S_MAGIC_FUSECTL 0x65735543
#define S_MAGIC_FUTEXFS 0x0BAD1DEA
#define S_MAGIC_GFS 0x01161970
#define S_MAGIC_GPFS 0x47504653
#define S_MAGIC_HFS 0x4244
#define S_MAGIC_HFS_PLUS 0x482B
#define S_MAGIC_HFS_X 0x4858
#define S_MAGIC_HOSTFS 0x00C0FFEE
#define S_MAGIC_HPFS 0xF995E849
#define S_MAGIC_HUGETLBFS 0x958458F6
#define S_MAGIC_MTD_INODE_FS 0x11307854
#define S_MAGIC_IBRIX 0x013111A8
#define S_MAGIC_INOTIFYFS 0x2BAD1DEA
#define S_MAGIC_ISOFS 0x9660
#define S_MAGIC_ISOFS_R_WIN 0x4004
#define S_MAGIC_ISOFS_WIN 0x4000
#define S_MAGIC_JFFS 0x07C0
#define S_MAGIC_JFFS2 0x72B6
#define S_MAGIC_JFS 0x3153464A
#define S_MAGIC_KAFS 0x6B414653
#define S_MAGIC_LOGFS 0xC97E8168
#define S_MAGIC_LUSTRE 0x0BD00BD0
#define S_MAGIC_M1FS 0x5346314D
#define S_MAGIC_MINIX 0x137F
#define S_MAGIC_MINIX_30 0x138F
#define S_MAGIC_MINIX_V2 0x2468
#define S_MAGIC_MINIX_V2_30 0x2478
#define S_MAGIC_MINIX_V3 0x4D5A
#define S_MAGIC_MQUEUE 0x19800202
#define S_MAGIC_MSDOS 0x4D44
#define S_MAGIC_NCP 0x564C
#define S_MAGIC_NFS 0x6969
#define S_MAGIC_NFSD 0x6E667364
#define S_MAGIC_NILFS 0x3434
#define S_MAGIC_NSFS 0x6E736673
#define S_MAGIC_NTFS 0x5346544E
#define S_MAGIC_OPENPROM 0x9FA1
#define S_MAGIC_OCFS2 0x7461636F
#define S_MAGIC_OVERLAYFS 0x794C7630
#define S_MAGIC_PANFS 0xAAD7AAEA
#define S_MAGIC_PIPEFS 0x50495045
#define S_MAGIC_PPC_CMM 0xC7571590
#define S_MAGIC_PRL_FS 0x7C7C6673
#define S_MAGIC_PROC 0x9FA0
#define S_MAGIC_PSTOREFS 0x6165676C
#define S_MAGIC_QNX4 0x002F
#define S_MAGIC_QNX6 0x68191122
#define S_MAGIC_RAMFS 0x858458F6
#define S_MAGIC_RDTGROUP 0x07655821
#define S_MAGIC_REISERFS 0x52654973
#define S_MAGIC_ROMFS 0x7275
#define S_MAGIC_RPC_PIPEFS 0x67596969
#define S_MAGIC_SDCARDFS 0x5DCA2DF5
#define S_MAGIC_SECURITYFS 0x73636673
#define S_MAGIC_SELINUX 0xF97CFF8C
#define S_MAGIC_SMACK 0x43415D53
#define S_MAGIC_SMB 0x517B
#define S_MAGIC_SMB2 0xFE534D42
#define S_MAGIC_SNFS 0xBEEFDEAD
#define S_MAGIC_SOCKFS 0x534F434B
#define S_MAGIC_SQUASHFS 0x73717368
#define S_MAGIC_SYSFS 0x62656572
#define S_MAGIC_SYSV2 0x012FF7B6
#define S_MAGIC_SYSV4 0x012FF7B5
#define S_MAGIC_TMPFS 0x01021994
#define S_MAGIC_TRACEFS 0x74726163
#define S_MAGIC_UBIFS 0x24051905
#define S_MAGIC_UDF 0x15013346
#define S_MAGIC_UFS 0x00011954
#define S_MAGIC_UFS_BYTESWAPPED 0x54190100
#define S_MAGIC_USBDEVFS 0x9FA2
#define S_MAGIC_V9FS 0x01021997
#define S_MAGIC_VMHGFS 0xBACBACBC
#define S_MAGIC_VXFS 0xA501FCF5
#define S_MAGIC_VZFS 0x565A4653
#define S_MAGIC_WSLFS 0x53464846
#define S_MAGIC_XENFS 0xABBA1974
#define S_MAGIC_XENIX 0x012FF7B4
#define S_MAGIC_XFS 0x58465342
#define S_MAGIC_XIAFS 0x012FD16D
#define S_MAGIC_Z3FOLD 0x0033
#define S_MAGIC_ZFS 0x2FC12FC1
#define S_MAGIC_ZSMALLOC 0x58295829

const char *fs_type_str(__fsword_t f_type)
{
    switch (f_type)
    {
        case S_MAGIC_AAFS: /* 0x5A3C69F0 local */
            return "aafs";
        case S_MAGIC_ACFS: /* 0x61636673 remote */
            return "acfs";
        case S_MAGIC_ADFS: /* 0xADF5 local */
            return "adfs";
        case S_MAGIC_AFFS: /* 0xADFF local */
            return "affs";
        case S_MAGIC_AFS: /* 0x5346414F remote */
            return "afs";
        case S_MAGIC_ANON_INODE_FS: /* 0x09041934 local */
            return "anon-inode FS";
        case S_MAGIC_AUFS: /* 0x61756673 remote */
            return "aufs";
        case S_MAGIC_AUTOFS: /* 0x0187 local */
            return "autofs";
        case S_MAGIC_BALLOON_KVM: /* 0x13661366 local */
            return "balloon-kvm-fs";
        case S_MAGIC_BEFS: /* 0x42465331 local */
            return "befs";
        case S_MAGIC_BDEVFS: /* 0x62646576 local */
            return "bdevfs";
        case S_MAGIC_BFS: /* 0x1BADFACE local */
            return "bfs";
        case S_MAGIC_BINDERFS: /* 0x6C6F6F70 local */
            return "binderfs";
        case S_MAGIC_BPF_FS: /* 0xCAFE4A11 local */
            return "bpf_fs";
        case S_MAGIC_BINFMTFS: /* 0x42494E4D local */
            return "binfmt_misc";
        case S_MAGIC_BTRFS: /* 0x9123683E local */
            return "btrfs";
        case S_MAGIC_BTRFS_TEST: /* 0x73727279 local */
            return "btrfs_test";
        case S_MAGIC_CEPH: /* 0x00C36400 remote */
            return "ceph";
        case S_MAGIC_CGROUP: /* 0x0027E0EB local */
            return "cgroupfs";
        case S_MAGIC_CGROUP2: /* 0x63677270 local */
            return "cgroup2fs";
        case S_MAGIC_CIFS: /* 0xFF534D42 remote */
            return "cifs";
        case S_MAGIC_CODA: /* 0x73757245 remote */
            return "coda";
        case S_MAGIC_COH: /* 0x012FF7B7 local */
            return "coh";
        case S_MAGIC_CONFIGFS: /* 0x62656570 local */
            return "configfs";
        case S_MAGIC_CRAMFS: /* 0x28CD3D45 local */
            return "cramfs";
        case S_MAGIC_CRAMFS_WEND: /* 0x453DCD28 local */
            return "cramfs-wend";
        case S_MAGIC_DAXFS: /* 0x64646178 local */
            return "daxfs";
        case S_MAGIC_DEBUGFS: /* 0x64626720 local */
            return "debugfs";
        case S_MAGIC_DEVFS: /* 0x1373 local */
            return "devfs";
        case S_MAGIC_DEVPTS: /* 0x1CD1 local */
            return "devpts";
        case S_MAGIC_DMA_BUF: /* 0x444D4142 local */
            return "dma-buf-fs";
        case S_MAGIC_ECRYPTFS: /* 0xF15F local */
            return "ecryptfs";
        case S_MAGIC_EFIVARFS: /* 0xDE5E81E4 local */
            return "efivarfs";
        case S_MAGIC_EFS: /* 0x00414A53 local */
            return "efs";
        case S_MAGIC_EROFS_V1: /* 0xE0F5E1E2 local */
            return "erofs";
        case S_MAGIC_EXFS: /* 0x45584653 local */
            return "exfs";
        case S_MAGIC_EXOFS: /* 0x5DF5 local */
            return "exofs";
        case S_MAGIC_EXT: /* 0x137D local */
            return "ext";
        case S_MAGIC_EXT2: /* 0xEF53 local */
            return "ext2/ext3";
        case S_MAGIC_EXT2_OLD: /* 0xEF51 local */
            return "ext2";
        case S_MAGIC_F2FS: /* 0xF2F52010 local */
            return "f2fs";
        case S_MAGIC_FAT: /* 0x4006 local */
            return "fat";
        case S_MAGIC_FHGFS: /* 0x19830326 remote */
            return "fhgfs";
        case S_MAGIC_FUSEBLK: /* 0x65735546 remote */
            return "fuseblk";
        case S_MAGIC_FUSECTL: /* 0x65735543 remote */
            return "fusectl";
        case S_MAGIC_FUTEXFS: /* 0x0BAD1DEA local */
            return "futexfs";
        case S_MAGIC_GFS: /* 0x01161970 remote */
            return "gfs/gfs2";
        case S_MAGIC_GPFS: /* 0x47504653 remote */
            return "gpfs";
        case S_MAGIC_HFS: /* 0x4244 local */
            return "hfs";
        case S_MAGIC_HFS_PLUS: /* 0x482B local */
            return "hfs+";
        case S_MAGIC_HFS_X: /* 0x4858 local */
            return "hfsx";
        case S_MAGIC_HOSTFS: /* 0x00C0FFEE local */
            return "hostfs";
        case S_MAGIC_HPFS: /* 0xF995E849 local */
            return "hpfs";
        case S_MAGIC_HUGETLBFS: /* 0x958458F6 local */
            return "hugetlbfs";
        case S_MAGIC_MTD_INODE_FS: /* 0x11307854 local */
            return "inodefs";
        case S_MAGIC_IBRIX: /* 0x013111A8 remote */
            return "ibrix";
        case S_MAGIC_INOTIFYFS: /* 0x2BAD1DEA local */
            return "inotifyfs";
        case S_MAGIC_ISOFS: /* 0x9660 local */
            return "isofs";
        case S_MAGIC_ISOFS_R_WIN: /* 0x4004 local */
            return "isofs";
        case S_MAGIC_ISOFS_WIN: /* 0x4000 local */
            return "isofs";
        case S_MAGIC_JFFS: /* 0x07C0 local */
            return "jffs";
        case S_MAGIC_JFFS2: /* 0x72B6 local */
            return "jffs2";
        case S_MAGIC_JFS: /* 0x3153464A local */
            return "jfs";
        case S_MAGIC_KAFS: /* 0x6B414653 remote */
            return "k-afs";
        case S_MAGIC_LOGFS: /* 0xC97E8168 local */
            return "logfs";
        case S_MAGIC_LUSTRE: /* 0x0BD00BD0 remote */
            return "lustre";
        case S_MAGIC_M1FS: /* 0x5346314D local */
            return "m1fs";
        case S_MAGIC_MINIX: /* 0x137F local */
            return "minix";
        case S_MAGIC_MINIX_30: /* 0x138F local */
            return "minix (30 char.)";
        case S_MAGIC_MINIX_V2: /* 0x2468 local */
            return "minix v2";
        case S_MAGIC_MINIX_V2_30: /* 0x2478 local */
            return "minix v2 (30 char.)";
        case S_MAGIC_MINIX_V3: /* 0x4D5A local */
            return "minix3";
        case S_MAGIC_MQUEUE: /* 0x19800202 local */
            return "mqueue";
        case S_MAGIC_MSDOS: /* 0x4D44 local */
            return "msdos";
        case S_MAGIC_NCP: /* 0x564C remote */
            return "novell";
        case S_MAGIC_NFS: /* 0x6969 remote */
            return "nfs";
        case S_MAGIC_NFSD: /* 0x6E667364 remote */
            return "nfsd";
        case S_MAGIC_NILFS: /* 0x3434 local */
            return "nilfs";
        case S_MAGIC_NSFS: /* 0x6E736673 local */
            return "nsfs";
        case S_MAGIC_NTFS: /* 0x5346544E local */
            return "ntfs";
        case S_MAGIC_OPENPROM: /* 0x9FA1 local */
            return "openprom";
        case S_MAGIC_OCFS2: /* 0x7461636F remote */
            return "ocfs2";
        case S_MAGIC_OVERLAYFS: /* 0x794C7630 remote */
            return "overlayfs";
        case S_MAGIC_PANFS: /* 0xAAD7AAEA remote */
            return "panfs";
        case S_MAGIC_PIPEFS: /* 0x50495045 remote */
            return "pipefs";
        case S_MAGIC_PPC_CMM: /* 0xC7571590 local */
            return "ppc-cmm-fs";
        case S_MAGIC_PRL_FS: /* 0x7C7C6673 remote */
            return "prl_fs";
        case S_MAGIC_PROC: /* 0x9FA0 local */
            return "proc";
        case S_MAGIC_PSTOREFS: /* 0x6165676C local */
            return "pstorefs";
        case S_MAGIC_QNX4: /* 0x002F local */
            return "qnx4";
        case S_MAGIC_QNX6: /* 0x68191122 local */
            return "qnx6";
        case S_MAGIC_RAMFS: /* 0x858458F6 local */
            return "ramfs";
        case S_MAGIC_RDTGROUP: /* 0x07655821 local */
            return "rdt";
        case S_MAGIC_REISERFS: /* 0x52654973 local */
            return "reiserfs";
        case S_MAGIC_ROMFS: /* 0x7275 local */
            return "romfs";
        case S_MAGIC_RPC_PIPEFS: /* 0x67596969 local */
            return "rpc_pipefs";
        case S_MAGIC_SDCARDFS: /* 0x5DCA2DF5 local */
            return "sdcardfs";
        case S_MAGIC_SECURITYFS: /* 0x73636673 local */
            return "securityfs";
        case S_MAGIC_SELINUX: /* 0xF97CFF8C local */
            return "selinux";
        case S_MAGIC_SMACK: /* 0x43415D53 local */
            return "smackfs";
        case S_MAGIC_SMB: /* 0x517B remote */
            return "smb";
        case S_MAGIC_SMB2: /* 0xFE534D42 remote */
            return "smb2";
        case S_MAGIC_SNFS: /* 0xBEEFDEAD remote */
            return "snfs";
        case S_MAGIC_SOCKFS: /* 0x534F434B local */
            return "sockfs";
        case S_MAGIC_SQUASHFS: /* 0x73717368 local */
            return "squashfs";
        case S_MAGIC_SYSFS: /* 0x62656572 local */
            return "sysfs";
        case S_MAGIC_SYSV2: /* 0x012FF7B6 local */
            return "sysv2";
        case S_MAGIC_SYSV4: /* 0x012FF7B5 local */
            return "sysv4";
        case S_MAGIC_TMPFS: /* 0x01021994 local */
            return "tmpfs";
        case S_MAGIC_TRACEFS: /* 0x74726163 local */
            return "tracefs";
        case S_MAGIC_UBIFS: /* 0x24051905 local */
            return "ubifs";
        case S_MAGIC_UDF: /* 0x15013346 local */
            return "udf";
        case S_MAGIC_UFS: /* 0x00011954 local */
            return "ufs";
        case S_MAGIC_UFS_BYTESWAPPED: /* 0x54190100 local */
            return "ufs";
        case S_MAGIC_USBDEVFS: /* 0x9FA2 local */
            return "usbdevfs";
        case S_MAGIC_V9FS: /* 0x01021997 local */
            return "v9fs";
        case S_MAGIC_VMHGFS: /* 0xBACBACBC remote */
            return "vmhgfs";
        case S_MAGIC_VXFS: /* 0xA501FCF5 remote */
            return "vxfs";
        case S_MAGIC_VZFS: /* 0x565A4653 local */
            return "vzfs";
        case S_MAGIC_WSLFS: /* 0x53464846 local */
            return "wslfs";
        case S_MAGIC_XENFS: /* 0xABBA1974 local */
            return "xenfs";
        case S_MAGIC_XENIX: /* 0x012FF7B4 local */
            return "xenix";
        case S_MAGIC_XFS: /* 0x58465342 local */
            return "xfs";
        case S_MAGIC_XIAFS: /* 0x012FD16D local */
            return "xia";
        case S_MAGIC_Z3FOLD: /* 0x0033 local */
            return "z3fold";
        case S_MAGIC_ZFS: /* 0x2FC12FC1 local */
            return "zfs";
        case S_MAGIC_ZSMALLOC: /* 0x58295829 local */
            return "zsmallocfs";
        default:
            break;
    }
    return "UNKNOWN";
}