#ifndef _COMMON_H
#define _COMMON_H

#include <Windows.h>


/*
	���� ->�ŵ� ->���� 
	
	�� �ɶ���������
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

//������
typedef struct _PARTITIONTABLE_
{
	UCHAR BootIndictor;//�Ƿ񼤻�
	UCHAR StartHead;//��ʼ��ͷ��

	struct
	{
		WORD StartSector:6;//��ʼ������
		WORD StartCylinder:10;//��ʼ�ŵ���
	};

	UCHAR PartitionTypeIndictor;//��������
	UCHAR EndHead;//������ͷ��

	struct
	{
		WORD EndSector:6;//����������
		WORD EndCylinder:10;//�����ŵ���
	};

	ULONG PrcedingSector;//֮ǰ��������
	ULONG TotalSector;//��������

}PARTITIONTABLE,*PPARTITIONTABLE;

#pragma pack(pop)



//��������¼
typedef struct _MASTERBOOTRECORD_
{
	UCHAR							BootCode[446];//��������
	PARTITIONTABLE			PartitionTable[4];//������
	WORD							EndFlag;//������־
}MASTER_BOOT_RECORD,*PMASTER_BOOT_RECORD;

typedef struct{
	TCHAR		szFileSize[20];
	TCHAR		szCreate[25];
	TCHAR		szFileName[MAX_PATH];
}NTFS_FILEINFO, *PNTFS_FILEINFO;



#pragma pack(push,1) 

//NTFS �ļ���Ϣ

// ���� �ṹ
typedef struct _DRIVEPACKET_
{
	UCHAR						uchBI;//�Ƿ񼤻�
	WORD						wSH;//��ʼ��ͷ��
	WORD						wSS;//��ʼ������
	WORD						wSC;//��ʼ�ŵ���
	UCHAR						uchPTI;//��������
	WORD						wEH;//������ͷ��
	WORD						wES;//����������
	WORD						wEC;//�����ŵ���
	ULONGLONG			ullPS;//֮ǰ��������
	ULONGLONG			ullTS;//��������
	ULONGLONG			ullEP;//��չ�����������
	ULONGLONG			ullRS;//�������
	ULONGLONG			ullMRS;//��չ�������������

}DRIVEPACKET, *PDRIVEPACKET;


//���� ��Ϣ
typedef struct { //512B 
	UCHAR				Jump[3];//����3���ֽ� 
	UCHAR				Format[8]; //��N��'T' 'F' 'S' 0x20 0x20 0x20 0x20 
	USHORT				BytesPerSector;//ÿ�����ж����ֽ� һ��Ϊ512B 0x200 
	UCHAR				SectorsPerCluster;//ÿ���ж��ٸ����� 
	USHORT				BootSectors;// 
	UCHAR				Mbz1;//����0 
	USHORT				Mbz2;//����0 
	USHORT				Reserved1;//����0 
	UCHAR				MediaType;//������������Ӳ��Ϊ0xf8 
	USHORT				Mbz3;//��Ϊ0 
	USHORT				SectorsPerTrack;//ÿ����������һ��Ϊ0x3f 
	USHORT				NumberOfHeads;//��ͷ�� 
	ULONG				PartitionOffset;//�÷�����ƫ�ƣ����÷���ǰ������������ һ��Ϊ�ŵ�������0x3f 63�� 
	ULONG				Reserved2[2]; 
	ULONGLONG TotalSectors;//�÷����������� 
	ULONGLONG MftStartLcn;//MFT�����ʼ�غ�LCN 
	ULONGLONG Mft2StartLcn;//MFT���ݱ����ʼ�غ�LCN 
	ULONG			ClustersPerFileRecord;//ÿ��MFT��¼����������  ��¼���ֽڲ�һ��Ϊ��ClustersPerFileRecord*SectorsPerCluster*BytesPerSector
	ULONG				ClustersPerIndexBlock;//ÿ��������Ĵ��� 
	ULONGLONG VolumeSerialNumber;//�����к� 
	UCHAR				Code[0x1AE]; 
	USHORT				BootSignature; 
} BOOT_BLOCK, *PBOOT_BLOCK; 


typedef struct _RUNITEM_NODE{
	ULONGLONG		n64Len;
	ULONGLONG		n64Offset;
	_RUNITEM_NODE	*pNext;

}RUNITEM_NODE, *PRUNITEM_NODE;


//�ļ���Ϣ
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


//MFT  ��ÿһ���ļ����ļ���¼
typedef struct{
	UCHAR		szSignature[4];//MFT��־��һ��Ϊ"FILE"
	WORD		wFixupOffset;//�������кŵ�ƫ��
	WORD		wFixupSize;
	LONGLONG	n64LogSeqNumber;//��־�ļ����к�
	WORD		wSequence;//���к�
	WORD		wHardLinks;//Ӳ������
	WORD		wAttribOffset;//��һ������ƫ��
	WORD		wFlags;//0x00��ʾ�ļ���ɾ����0x01��ʾ�ļ���ʹ����,0x02��ʾĿ¼��ɾ��,0x03��ʾĿ¼��ʹ����
	DWORD		dwRecLength;//�ļ���¼��ʵ�ʳ���
	DWORD		dwAllLength;//�ļ���¼���䳤��
	LONGLONG	n64BaseMftRec;//�����ļ���¼�е�������
	WORD		wNextAttrID;//��һ������ID
	WORD		wFixupPattern;// Current fixup pattern
	DWORD		dwMFTRecNumber;//�ļ���¼�ο���

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
/* FAT32 �ṹ��Ϣ                                                                     */
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
	WORD	wBytesPerSector;//ÿ�����ֽ�
	BYTE	bySectorsPerCluster;//ÿ��������
	WORD	wReservedSectors;//��������
	BYTE	byNumberOfFats;//fat����
	WORD	wUnsed1;
	WORD	wUnsed2;
	BYTE	byMediaDescriptor;//ý������
	WORD	wUnsed3;
	WORD	wSectorsPerTrack;//ÿ�ŵ�����
	WORD	wHead;//��ͷ��
	DWORD	dwSectorsOfHidden;//��������
	DWORD	dwTotalSectors;//�÷�����������
	DWORD	dwSectorsPerFat;//ÿFat������
	WORD	wFlag;//���
	WORD	wVersion;//û��ʹ�ã�����Ϊ��
	DWORD	dwRootDirFirstCluster;//��Ŀ¼�״�
	WORD	wFileSytemInfoSector;//�ļ�ϵͳ��Ϣ������
	WORD	wDBRBackupSector;//DBR��������
	TCHAR	szReserved[12];
	BYTE	byBlosDriveNum;//BLOS ��������
	BYTE	byUnsed4;
	BYTE	byExtBootSignature;//��չ�������
	DWORD	dwVolumeSerialNumber;//�����к�
	TCHAR	szVolumeLabel[11];//���
	TCHAR	szFileSystem[8];//�ļ�ϵͳ
	TCHAR	szCode[420];
	WORD	wSignature[2];

}DOS_BOOT_RECORD, *PDOS_BOOT_RECORD;


typedef struct _LONGFILENAME
{
	BYTE	bySequenceNum;//���к�
	WCHAR	wszFileName_5[5];
	BYTE	byCharacteristics;//���Ա�־
	BYTE	byReserved;
	BYTE	byCheckSum;//������У���
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