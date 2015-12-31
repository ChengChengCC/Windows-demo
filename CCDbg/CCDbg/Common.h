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


struct StuCommand
{
	char chCmd[FIELD_LEN];
	char chParam1[FIELD_LEN];
	char chParam2[FIELD_LEN];
	char chParam3[FIELD_LEN];
	char chParam4[FIELD_LEN];
	char chParam5[FIELD_LEN];
	char chParam6[FIELD_LEN];
};

//断点类型
enum PointType
{
	ORD_POINT = 1,          //一般断点
	HARD_POINT = 2,         //硬件断点
	MEM_POINT = 3           //内存断点
};

enum PointAccess
{
	ACCESS = 1,             //访问
	WRITE = 2,              //写入
	EXECUTE = 3             //执行
};

//内存断点对应内存分页结构体
struct stuPointPage
{
	int         nPtNum;                 //断点序号
	DWORD       dwPageAddr;             //内存分页首地址
};

typedef BOOL (__cdecl *pfnCmdProcessFun)(StuCommand* cmd);


//命令链表中的节点结构体
struct StuCmdNode
{
	char            chCmd[20];
	pfnCmdProcessFun   pFun;
};


//断点信息结构体
struct stuPointInfo
{
	PointType   ptType;                 //断点类型
	int         nPtNum;                 //断点序号
	LPVOID      lpPointAddr;            //断点地址
	PointAccess ptAccess;               //读、写、执行属性
	DWORD       dwPointLen;             //断点长度（针对硬件断点、内存断点）
	BOOL        isOnlyOne;              //是否一次性断点（针对INT3断点）
	int         nNewProtect;            //新的内存页属性(针对内存断点)
	union{
		char chOldByte;                 //原先的字节（针对INT3断点）
		int nOldProtect;                //原先的内存页属性(针对内存断点)
	}u;
};





LPVOID HexStringToHex(char* pHexString, BOOL isShowError);

void __stdcall GetSafeStr(char *p, int n);
BOOL ChangeStrToCmd(IN char* chUserInputString, OUT StuCommand* pUserCmd);
char* DelFrontSpace(char * pStr);
pfnCmdProcessFun GetFunFromAryCmd(StuCommand m_UserCmd );
BOOL ShowHelp(StuCommand* pCmd);