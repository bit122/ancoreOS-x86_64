#pragma once
#include <stdint.h>
#include "../klibc/includes/stdio.h"


typedef struct {
    uint32_t    totalInodes;
    uint32_t    totalBlocks;
    uint32_t    totalSuBlocks;
    uint32_t    totalUnallocatedBlocks;
    uint32_t    totalUnallocatedINodes;
    uint32_t    superblockStart;
    uint32_t    blockSize;
    uint32_t    fragmentSize;
    uint32_t    blockgroupBlockNum;
    uint32_t    blockgroupFragmentNum;
    uint32_t    blockgroupInodeNum;
    uint32_t    lastMountTime;
    uint32_t    lastWriteTime;
    uint16_t    volumeMountTimesSinceFsck;
    uint16_t    signature;
    uint16_t    fsState;
    uint16_t    errorDetectOperation;
    uint16_t    minorVer;
    uint32_t    lastFsckTime;
    uint32_t    intervalBetweenFsck;
    uint32_t    osId;
    uint32_t    majorVer;
    uint16_t    suUid;
    uint16_t    suGid;
    uint32_t    firstNonReservedINode;
    uint16_t    iNodeSize;
    uint16_t    superblockGroup;
    uint32_t    optionalFeatures;
    uint32_t    requiredFeatures;
    uint32_t    mountVolumeRo;
    char        fsId[16];
    char        volumeName[16];
    char        lastMountPath[64];
    uint32_t    compressionAlgos;
    uint8_t     preallocateFiles;
    uint8_t     preallocateDirs;
    uint16_t    unused;
    char        journalId[16];
    uint32_t    journalINode;
    uint32_t    journalDevice;
    uint32_t    orphanInodeHead;
} __attribute__((packed)) Ext2_Superblock;
                                                                                
enum {
    EXT2_FS_CLEAN = 1,
    EXT2_FS_ERROR
};

enum {
    EXT2_ERR_IGNORE = 1,
    EXT2_ERR_RMNT_RO,
    EXT2_ERR_PANIC
};

enum {
    EXT2_OSID_LINUX,
    EXT2_OSID_HURD,
    EXT2_OSID_MASIX, // ????
    EXT2_OSID_FBSD,
    EXT2_OSID_BSD
};

enum {
    EXT2_OPFEATURE_PREALLOCATE = 0x1,
    EXT2_OPFEATURE_AFSINODES   = 0x2,
    EXT2_OPFEATURE_EXT3        = 0x4,
    EXT2_OPFEATURE_EXTATTRS    = 0x8,
    EXT2_OPFEATURE_RESIZABLE   = 0x10,
    EXT2_OPFEATURE_HASH        = 0x20 
};

enum {
    EXT2_REFEATURE_COMPRESSION = 0x1,
    EXT2_REFEATURE_DIRENTTYPE  = 0x2,
    EXT2_REFEATURE_REPJOURNAL  = 0x4,
    EXT2_REFEATURE_JOURNALDEV  = 0x8
};

enum {
    EXT2_ROFEATURE_SPARSESUPERBLOCK_GDT = 0x1,
    EXT2_ROFEATURE_64BIT_FSZ            = 0x2,
    EXT2_ROFEATURE_DIRCONTENTS_BT       = 0x4    
};


typedef struct {
    uint32_t    blockUsageBmapAddr;
    uint32_t    inodeUsageBmapAddr;
    uint32_t    inodeTableAddr;
    uint16_t    unallocatedBlocks;
    uint16_t    unallocatedINodes;
    uint16_t    numDirectories;
} __attribute__((packed)) Ext2_BlockGroupDescriptor;

typedef struct {
    uint16_t    typePerms;
    uint16_t    uid;
    uint32_t    lowerSize;
    uint32_t    lastAccessTime;
    uint32_t    creationTime;
    uint32_t    lastModificationTime;
    uint32_t    deletionTime;
    uint16_t    gid;
    uint16_t    hardLinkCount;
    uint32_t    diskSectorsUsed;
    uint32_t    flags;
    uint32_t    osSpecific1;
    uint32_t    directPtrs[12];
    uint32_t    singlyIndirectBPtr;
    uint32_t    doublyIndirectBPtr;
    uint32_t    triplyIndirectBPtr;
    uint32_t    genNumber;
    uint32_t    fileACL;
    uint32_t    directoryACL;
    uint32_t    fragmentBlockAddr;
    char        osSpecific2[12];
} __attribute__((packed)) Ext2_INode;

enum {
    EXT2_INODE_TYPE_FIFO = 0x1000,
    EXT2_INODE_TYPE_CHARDEV = 0x2000,
    EXT2_INODE_TYPE_DIRECTORY = 0x4000,
    EXT2_INODE_TYPE_BLOCKDEV = 0x6000,
    EXT2_INODE_TYPE_FILE = 0x8000,
    EXT2_INODE_TYPE_SYMLINK = 0xA000,
    EXT2_INODE_TYPE_USOCK = 0xC000
};

enum {
    EXT2_INODE_PERM_OTHER_EXEC = 0x1,
    EXT2_INODE_PERM_OTHER_WRITE = 0x2,
    EXT2_INODE_PERM_OTHER_READ = 0x4,
    EXT2_INODE_PERM_GROUP_EXEC = 0x8,
    EXT2_INODE_PERM_GROUP_WRITE = 0x10,
    EXT2_INODE_PERM_GROUP_READ = 0x20,
    EXT2_INODE_PERM_USER_EXEC = 0x40,
    EXT2_INODE_PERM_USER_WRITE = 0x80,
    EXT2_INODE_PERM_USER_READ = 0x100,
    EXT2_INODE_PERM_STICKY = 0x200,
    EXT2_INODE_PERM_SETGID = 0x400,
    EXT2_INODE_PERM_SETUID = 0x800
};

enum {
    EXT2_INODE_FLAG_SECDEL = 0x1,
    EXT2_INODE_FLAG_COPYWHENDEL = 0x2,
    EXT2_INODE_FLAG_COMPRESS = 0x4,
    EXT2_INODE_FLAG_SYNC = 0x8,
    EXT2_INODE_FLAG_IMMUTABLE = 0x10,
    EXT2_INODE_FLAG_APPEND = 0x20,
    EXT2_INODE_FLAG_NODUMP = 0x40,
    EXT2_INODE_FLAG_NOACCESSUPDATE = 0x80,
    EXT2_INODE_FLAG_HASH = 0x10000,
    EXT2_INODE_FLAG_AFS = 0x20000,
    EXT2_INODE_FLAG_JOURNAL = 0x40000
};

typedef struct {
    uint32_t    iNode;
    uint16_t    totalSize;
    uint8_t     nameLength;
    uint8_t     typeOrMS8B;
    // rest is name
} __attribute__((packed)) Ext2_DirEnt;

enum {
    EXT2_DIRENT_TYPE_UNKNOWN,
    EXT2_DIRENT_TYPE_REGULAR,
    EXT2_DIRENT_TYPE_DIRECTORY,
    EXT2_DIRENT_TYPE_CHARDEV,
    EXT2_DIRENT_TYPE_BLOCKDEV,
    EXT2_DIRENT_TYPE_FIFO,
    EXT2_DIRENT_TYPE_SOCKET,
    EXT2_DIRENT_TYPE_SYMLINK
};