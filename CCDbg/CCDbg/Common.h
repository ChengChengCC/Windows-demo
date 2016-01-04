#pragma once
#include <iostream>
#include <Windows.h>
#include <list>
#include <TlHelp32.h>
#include "disasm.h"
using namespace std;

#define FIELD_LEN 20
#define TF 0x100
#define ADD_COMMAND(str, memberFxn) \
{str, memberFxn},


struct CCCommand
{
	char chCmd[FIELD_LEN];
	char chParam1[FIELD_LEN];
	char chParam2[FIELD_LEN];
	char chParam3[FIELD_LEN];
	char chParam4[FIELD_LEN];
	char chParam5[FIELD_LEN];
	char chParam6[FIELD_LEN];
};

//�ϵ�����
enum PointType
{
	ORD_POINT = 1,          //һ��ϵ�
	HARD_POINT = 2,         //Ӳ���ϵ�
	MEM_POINT = 3           //�ڴ�ϵ�
};

enum PointAccess
{
	ACCESS = 1,             //����
	WRITE = 2,              //д��
	EXECUTE = 3             //ִ��
};

//�ڴ�ϵ��Ӧ�ڴ��ҳ�ṹ��
struct CCPointPage
{
	int         nPtNum;                 //�ϵ����
	DWORD       dwPageAddr;             //�ڴ��ҳ�׵�ַ
};

typedef BOOL (__cdecl *pfnCmdProcessFun)(CCCommand* cmd);


//���������еĽڵ�ṹ��
struct CCCmdNode
{
	char            chCmd[20];
	pfnCmdProcessFun   pFun;
};


//�ϵ���Ϣ�ṹ��
struct CCPointInfo
{
	PointType   ptType;                 //�ϵ�����
	int         nPtNum;                 //�ϵ����
	LPVOID      lpPointAddr;            //�ϵ��ַ
	PointAccess ptAccess;               //����д��ִ������
	DWORD       dwPointLen;             //�ϵ㳤�ȣ����Ӳ���ϵ㡢�ڴ�ϵ㣩
	BOOL        isOnlyOne;              //�Ƿ�һ���Զϵ㣨���INT3�ϵ㣩
	int         nNewProtect;            //�µ��ڴ�ҳ����(����ڴ�ϵ�)
	union{
		char chOldByte;                 //ԭ�ȵ��ֽڣ����INT3�ϵ㣩
		int nOldProtect;                //ԭ�ȵ��ڴ�ҳ����(����ڴ�ϵ�)
	}u;
};


//��ҳ��Ϣ�ṹ��
struct CCPageInfo
{
	DWORD       dwPageAddr;             //�ڴ��ҳ�׵�ַ
	DWORD       dwOldProtect;           //��ҳԭ������
};


//��Ҫ�ָ������裩���ڴ�ϵ�ṹ��
struct CCResetMemBp
{
	DWORD dwAddr;
	int nID;
};


LPVOID HexStringToHex(char* pHexString, BOOL isShowError);

void __stdcall GetSafeStr(char *p, int n);
BOOL ChangeStrToCmd(IN char* chUserInputString, OUT CCCommand* pUserCmd);
char* DelFrontSpace(char * pStr);
pfnCmdProcessFun GetFunFromAryCmd(CCCommand m_UserCmd );
BOOL ShowHelp(CCCommand* pCmd);