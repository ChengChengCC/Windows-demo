#include "StdAfx.h"
#include "Drive.h"
#include <Windows.h>
#include "FAT32Drive.h"
#include "NTFSDrive.h"



CDrive::CDrive(void)
{
}


CDrive::~CDrive(void)
{
}



//ͨ�� ��ͬ�ķ������� ������ͬ�������
CDrive *FactoryDrive(int nType)
{
	switch(nType)
	{	
	case PART_DOS32X:
	case PART_DOS32:
		return new CFAT32Drive;
	case PART_NTFS:
		return new CNTFSDrive;
	default:
		return NULL;
	}
}