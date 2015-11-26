#ifndef _COMMON_H
#define _COMMON_H

#include <Windows.h>


/*
	柱面 ->磁道 ->扇区 
	
	簇 由多个扇区组成
*/

#define ACTIVE_PART 0x80

#define PART_UNKNOWN 0x00 //Unknown.
#define PART_DOS2_FAT 0x01 //12-bit FAT.
#define PART_DOS3_FAT 0x04 //16-bit FAT. Partition smaller than 32MB.
#define PART_EXTENDED 0x05 //Extended MS-DOS Partition.
#define PART_DOS4_FAT 0x06 //16-bit FAT. Partition larger than or equal to 32MB.
#define PART_NTFS 0x07 // NTFS , v r interested only on this
#define PART_DOS32 0x0B //32-bit FAT. Partition up to 2047GB.
#define PART_DOS32X 0x0C //Same as PART_DOS32(0Bh), but uses Logical Block Address Int 13h extensions.
#define PART_DOSX13 0x0E //Same as PART_DOS4_FAT(06h), but uses Logical Block Address Int 13h extensions.
#define PART_DOSX13X 0x0F //Same as PART_EXTENDED(05h), but uses Logical Block Address Int 13h extensions.


#pragma pack(push,1) 

//分区表
typedef struct _PARTITIONTABLE_
{
	UCHAR BootIndictor;//是否激活
	UCHAR StartHead;//开始磁头号

	struct
	{
		WORD StartSector:6;//开始扇区号
		WORD StartCylinder:10;//开始磁道号
	};

	UCHAR PartitionTypeIndictor;//分区类型
	UCHAR EndHead;//结束磁头号

	struct
	{
		WORD EndSector:6;//结束扇区号
		WORD EndCylinder:10;//结束磁道号
	};

	ULONG PrcedingSector;//之前的扇区数
	ULONG TotalSector;//总扇区数

}PARTITIONTABLE,*PPARTITIONTABLE;

#pragma pack(pop)



//主引导记录
typedef struct _MASTERBOOTRECORD_
{
	UCHAR							BootCode[446];//启动代码
	PARTITIONTABLE			PartitionTable[4];//分区表
	WORD							EndFlag;//结束标志
}MASTER_BOOT_RECORD,*PMASTER_BOOT_RECORD;

typedef struct{
	TCHAR		szFileSize[20];
	TCHAR		szCreate[25];
	TCHAR		szFileName[MAX_PATH];
}NTFS_FILEINFO, *PNTFS_FILEINFO;



#pragma pack(push,1) 

//NTFS 文件信息

// 分区 结构
typedef struct _DRIVEPACKET_
{
	UCHAR						uchBI;//是否激活
	WORD						wSH;//开始磁头号
	WORD						wSS;//开始扇区号
	WORD						wSC;//开始磁道号
	UCHAR						uchPTI;//分区类型
	WORD						wEH;//结束磁头号
	WORD						wES;//结束扇区号
	WORD						wEC;//结束磁道号
	ULONGLONG			ullPS;//之前的扇区数
	ULONGLONG			ullTS;//总扇区数
	ULONGLONG			ullEP;//扩展分区相对扇区
	ULONGLONG			ullRS;//相对扇区
	ULONGLONG			ullMRS;//扩展分区相对扇区数

}DRIVEPACKET, *PDRIVEPACKET;


//扇区 信息
typedef struct { //512B 
	UCHAR				Jump[3];//跳过3个字节 
	UCHAR				Format[8]; //‘N’'T' 'F' 'S' 0x20 0x20 0x20 0x20 
	USHORT				BytesPerSector;//每扇区有多少字节 一般为512B 0x200 
	UCHAR				SectorsPerCluster;//每簇有多少个扇区 
	USHORT				BootSectors;// 
	UCHAR				Mbz1;//保留0 
	USHORT				Mbz2;//保留0 
	USHORT				Reserved1;//保留0 
	UCHAR				MediaType;//介质描述符，硬盘为0xf8 
	USHORT				Mbz3;//总为0 
	USHORT				SectorsPerTrack;//每道扇区数，一般为0x3f 
	USHORT				NumberOfHeads;//磁头数 
	ULONG				PartitionOffset;//该分区的偏移（即该分区前的隐含扇区数 一般为磁道扇区数0x3f 63） 
	ULONG				Reserved2[2]; 
	ULONGLONG TotalSectors;//该分区总扇区数 
	ULONGLONG MftStartLcn;//MFT表的起始簇号LCN 
	ULONGLONG Mft2StartLcn;//MFT备份表的起始簇号LCN 
	ULONG			ClustersPerFileRecord;//每个MFT记录包含几个簇  记录的字节不一定为：ClustersPerFileRecord*SectorsPerCluster*BytesPerSector
	ULONG				ClustersPerIndexBlock;//每个索引块的簇数 
	ULONGLONG VolumeSerialNumber;//卷序列号 
	UCHAR				Code[0x1AE]; 
	USHORT				BootSignature; 
} BOOT_BLOCK, *PBOOT_BLOCK; 


typedef struct _RUNITEM_NODE{
	ULONGLONG		n64Len;
	ULONGLONG		n64Offset;
	_RUNITEM_NODE	*pNext;

}RUNITEM_NODE, *PRUNITEM_NODE;


//文件信息
typedef struct
{
	LONGLONG	dwMftParentDir;            // Seq-nr parent-dir MFT entry
	LONGLONG	n64Create;                  // Creation time
	LONGLONG	n64Modify;                  // Last Modify time
	LONGLONG	n64Modfil;                  // Last modify of record
	LONGLONG	n64Access;                  // Last Access time
	LONGLONG	n64Allocated;               // Allocated disk space
	LONGLONG	n64RealSize;                // Size of the file
	DWORD		dwFlags;					// attribute
	DWORD		dwEAsReparsTag;				// Used by EAs and Reparse
	BYTE		chFileNameLength;
	BYTE		chFileNameType;            // 8.3 / Unicode
	WCHAR		wFilename[512];             // Name (in Unicode ?)

}ATTR_FILENAME, *PATTR_FILENAME; 

typedef struct
{
	LONGLONG	n64Create;		// Creation time
	LONGLONG	n64Modify;		// Last Modify time
	LONGLONG	n64Modfil;		// Last modify of record
	LONGLONG	n64Access;		// Last Access time
	DWORD		dwFATAttributes;// As FAT + 0x800 = compressed
	DWORD		dwReserved1;	// unknown

} ATTR_STANDARD;   

typedef struct{
	ULONGLONG		n64RealSize;
	ULONGLONG		n64AllocSize;
	PRUNITEM_NODE	pRunList;

}ATTR_DATA, *PATTR_DATA, DATARUN, *PDATARUN;


//MFT  中每一个文件的文件记录
typedef struct{
	UCHAR		szSignature[4];//MFT标志，一定为"FILE"
	WORD		wFixupOffset;//更新序列号的偏移
	WORD		wFixupSize;
	LONGLONG	n64LogSeqNumber;//日志文件序列号
	WORD		wSequence;//序列号
	WORD		wHardLinks;//硬连接数
	WORD		wAttribOffset;//第一个属性偏移
	WORD		wFlags;//0x00表示文件被删除，0x01表示文件在使用中,0x02表示目录被删除,0x03表示目录在使用中
	DWORD		dwRecLength;//文件记录单实际长度
	DWORD		dwAllLength;//文件记录分配长度
	LONGLONG	n64BaseMftRec;//基本文件记录中的索引号
	WORD		wNextAttrID;//下一个属性ID
	WORD		wFixupPattern;// Current fixup pattern
	DWORD		dwMFTRecNumber;//文件记录参考号

}FILE_RECORD_HEADER, *PFILE_RECORD_HEADER;



typedef struct
{
	DWORD	dwType;
	DWORD	dwFullLength;
	BYTE	uchNonResFlag;
	BYTE	uchNameLength;
	WORD	wNameOffset;
	WORD	wFlags;
	WORD	wID;

	union ATTR
	{
		struct RESIDENT
		{
			DWORD	dwLength;
			WORD	wAttrOffset;
			BYTE	uchIndexedTag;
			BYTE	uchPadding;
		} Resident;

		struct NONRESIDENT
		{
			LONGLONG	n64StartVCN;
			LONGLONG	n64EndVCN;
			WORD		wDatarunOffset;
			WORD		wCompressionSize; // compression unit size
			BYTE		uchPadding[4];
			LONGLONG	n64AllocSize;
			LONGLONG	n64RealSize;
			LONGLONG	n64StreamSize;
			// data runs...
		}NonResident;
	}Attr;
} NTFS_ATTRIBUTE, *PNTFS_ATTRIBUTE;

#pragma pack(pop)


/************************************************************************/
/* FAT32 结构信息                                                                     */
/************************************************************************/



#define	ATTR_READWRITE			0x00
#define ATTR_READONLY           0x01
#define ATTR_HIDDEN				0x02
#define ATTR_SYSTEM				0x04
#define ATTR_UNKNOW				0x08
#define ATTR_DIR				0x10


typedef struct _DOS_BOOT_RECORD
{
	TCHAR	szJmp[3];
	TCHAR	szOem[8];
	WORD	wBytesPerSector;//每扇区字节
	BYTE	bySectorsPerCluster;//每簇扇区数
	WORD	wReservedSectors;//保留扇区
	BYTE	byNumberOfFats;//fat数量
	WORD	wUnsed1;
	WORD	wUnsed2;
	BYTE	byMediaDescriptor;//媒体描述
	WORD	wUnsed3;
	WORD	wSectorsPerTrack;//每磁道扇区
	WORD	wHead;//磁头数
	DWORD	dwSectorsOfHidden;//隐藏扇区
	DWORD	dwTotalSectors;//该分区扇区总数
	DWORD	dwSectorsPerFat;//每Fat扇区数
	WORD	wFlag;//标记
	WORD	wVersion;//没有使用，总是为零
	DWORD	dwRootDirFirstCluster;//根目录首簇
	WORD	wFileSytemInfoSector;//文件系统信息扇区号
	WORD	wDBRBackupSector;//DBR备份扇区
	TCHAR	szReserved[12];
	BYTE	byBlosDriveNum;//BLOS 驱动器号
	BYTE	byUnsed4;
	BYTE	byExtBootSignature;//扩展引导标记
	DWORD	dwVolumeSerialNumber;//卷序列号
	TCHAR	szVolumeLabel[11];//卷标
	TCHAR	szFileSystem[8];//文件系统
	TCHAR	szCode[420];
	WORD	wSignature[2];

}DOS_BOOT_RECORD, *PDOS_BOOT_RECORD;


typedef struct _LONGFILENAME
{
	BYTE	bySequenceNum;//序列号
	WCHAR	wszFileName_5[5];
	BYTE	byCharacteristics;//属性标志
	BYTE	byReserved;
	BYTE	byCheckSum;//短文名校验和
	WCHAR	wszFileName_6[6];
	WORD	wAlwayZero;
	WCHAR	wszFileName_2[2];

}LONGFILENAME, *PLONGFILENAME;

typedef struct _SHORTFILENAME
{
	TCHAR	szFileName[8];
	TCHAR	szExtension[3];
	BYTE	byAttributes;
	BYTE	byUnuse;
	BYTE	byCreateMilliSecond;
	WORD	wCreateTime;
	WORD	wCreateDate;
	WORD	wAccessDate;
	WORD	wStartClusterHi;
	WORD	wUpdateTime;
	WORD	wUpdateDate;
	WORD	wStartClusterLo;
	DWORD	dwFileSize;

}SHORTFILENAME, *PSHORTFILENAME;



typedef struct _DIRECTORYITEM
{
	union
	{
		struct{
			WORD	wCreateTime;
			WORD	wCreateDate;
		};
		WORD	wCreate;
	};
	union
	{
		struct{
			WORD	wStartClusterLo;
			WORD	wStartClusterHi;
		};
		DWORD	dwStartCluster;
	};
	DWORD	dwFileSize;
	union
	{
		TCHAR	szFileName[MAX_PATH];
	};

}DIRECTORYITEM, *PDIRECTORYITEM;


#endif