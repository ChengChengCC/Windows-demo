#include "Utility.h"
#include "Nt.h"

extern ULONG_PTR NtKernelBase;
extern ULONG_PTR NtKernelSSDT;


ULONG_PTR GetNtoskrnlBase()
{
	ULONG uRtnLength;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	PVOID pBuffer = NULL;
	char* ImageName = NULL;
	ULONG uModuleCounts = 0;
	PMODULES pKrlList = NULL;

	ZwQuerySystemInformation(SystemModuleInformation, NULL, 0, &uRtnLength);
	if (!uRtnLength)
	{
		DbgPrint("Get SystemModuleInfo Length error,%d,%p\n", uRtnLength, Status);
		return Status;
	}

	pBuffer = ExAllocatePool(NonPagedPool, uRtnLength);
	if (pBuffer == NULL)
	{
		DbgPrint("ExAllocatePool error\n");
		return Status;
	}

	Status = ZwQuerySystemInformation(SystemModuleInformation, pBuffer, uRtnLength, 0);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("ZwQuerySystemInformation error\n");
		return Status;
	}

	pKrlList = (PMODULES)pBuffer;

	uModuleCounts = pKrlList->ulCount;

	return pKrlList->smi[0].Base;

}



ULONG_PTR GetSSDTBase()  //SSDT�ṹ���ַ
{
	PUCHAR StartSearchAddress = (PUCHAR)__readmsr(0xC0000082);
	PUCHAR EndSearchAddress = StartSearchAddress + 0x500;
	PUCHAR i = NULL;
	UCHAR b1=0,b2=0,b3=0;
	ULONG_PTR Temp = 0;
	ULONG_PTR Address = 0;
	for(i=StartSearchAddress;i<EndSearchAddress;i++)
	{
		if( MmIsAddressValid(i) && MmIsAddressValid(i+1) && MmIsAddressValid(i+2) )
		{
			b1=*i;
			b2=*(i+1);
			b3=*(i+2);
			if( b1==0x4c && b2==0x8d && b3==0x15 ) //4c8d15
			{
				memcpy(&Temp,i+3,4);
				Address = (ULONG_PTR)Temp + (ULONG_PTR)i + 7;	//�����ַ�ĺ��Ĵ����� 4c8d15 ������� 4 ���ֽڣ���������				һ�� long���� �ϵ�ǰָ�����ʼ��ַ�ټ��� 7��ΪʲôҪ���� 7 �أ���Ϊ[lea r10,XXXXXXXX] ָ��ĳ����� 7 ���ֽ�
				return Address;
			}
		}
	}
	return 0;
}



