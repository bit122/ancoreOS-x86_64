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

// TODO: everything else