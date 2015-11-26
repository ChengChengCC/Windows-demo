
// FileRecoverDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FileRecover.h"
#include "FileRecoverDlg.h"
#include "afxdialogex.h"

#include "MyList.h"

#include "Drive.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



typedef struct _COLUMNSTRUCT
{
	WCHAR*		szTitle;
	UINT				nWidth;
}COLUMNSTRUCT;


UINT g_Column_Drive_Count  = 10;	  //进程列表列数

COLUMNSTRUCT g_Column_Drive[] = 
{
	{	L"序号",			60	},
	{	L"文件系统",			120	},
	{	L"标识",			75	},
	{	L"起始柱面",			85	},
	{	L"磁头",		65	},
	{	L"扇区",		65	},
	{	L"终止柱面",			85	}, 
	{	L"磁头",			65},
	{L"扇区",				65},
	{L"容量",					130}
};


UINT g_Column_File_Count  = 4;	  //进程列表列数

COLUMNSTRUCT g_Column_File[] = 
{
	{	L"序号",			100	},
	{	L"文件名",			150	},
	{	L"创建时间",			220	},
	{	L"大小",			100	}
};



int			dpix = 0;
int			dpiy = 0;

int							g_DriveCount = 0;
int							g_FileCount = 0;

BOOL					g_bStopScanThread = TRUE;

CDrive					*g_pDrive;
HWND				g_hDlg;

CMyList*				g_ListDrive = NULL;
CMyList*				g_ListFile = NULL;


DRIVEPACKET			g_DrivePacket[64] = {0};
ULONG				g_FileIndex[0x10000] = {0};


LONG					sort_column; //记录点击的列
BOOL					method; //记录比较方法

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CFileRecoverDlg 对话框




CFileRecoverDlg::CFileRecoverDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFileRecoverDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_pDrivePacket = NULL;
}

void CFileRecoverDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DRIVE, m_ListDrive);
	DDX_Control(pDX, IDC_LIST_FILE, m_ListFile);
}

BEGIN_MESSAGE_MAP(CFileRecoverDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_SCAN, &CFileRecoverDlg::OnBnClickedButtonScan)
	ON_BN_CLICKED(IDC_BUTTON_SAVE, &CFileRecoverDlg::OnBnClickedButtonSave)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_FILE, &CFileRecoverDlg::OnLvnColumnclickListFile)
END_MESSAGE_MAP()


// CFileRecoverDlg 消息处理程序

BOOL CFileRecoverDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	g_hDlg = AfxGetMainWnd()->m_hWnd;

	g_ListFile = (CMyList*)&m_ListFile;
	g_ListDrive = (CMyList*)&m_ListDrive;

	CPaintDC dc(this);
	dpix = GetDeviceCaps(dc.m_hDC,LOGPIXELSX);
	dpiy = GetDeviceCaps(dc.m_hDC,LOGPIXELSY);

	InitList();

	//
	InitListDriveItem();


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CFileRecoverDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CFileRecoverDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CFileRecoverDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


VOID CFileRecoverDlg::InitList()
{

	m_ListDrive.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_HEADERDRAGDROP);
	m_ListFile.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_HEADERDRAGDROP);

	UINT i;


	for (i = 0;i<g_Column_Drive_Count;i++)
	{
		m_ListDrive.InsertColumn(i, g_Column_Drive[i].szTitle,LVCFMT_CENTER,(int)(g_Column_Drive[i].nWidth*(dpix/96.0)));
	}

	for (i = 0;i<g_Column_File_Count;i++)
	{
		m_ListFile.InsertColumn(i, g_Column_File[i].szTitle,LVCFMT_CENTER,(int)(g_Column_File[i].nWidth*(dpix/96.0)));
	}

}



//这里是 枚举每个分区的信息   分区信息在MBR中存有
VOID  CFileRecoverDlg::InitListDriveItem()
{
	MASTER_BOOT_RECORD		stMBR = {0};			//主引导记录		里边儿分区表地址
	PARTITIONTABLE						stPT = {0};				//分区表
	DRIVEPACKET							stDP = {0};				//磁盘的每个分区

	CSector										Sct;							//定义个扇区类对象

	//初始化  扇区类	读到扇区、磁道、柱面等基本信息
	if(!Sct.Initial())
		return;

	//读取MBR扇区		在第0扇区		读到MBR结构体 
	if(!Sct.Read(0, (LPWSTR)&stMBR))		//开始读取的扇区号 缓冲区  要读的扇区数(默认为1)
		return;

	for(int i=0; i<4; i++)	//主引导记录存有 分区表		一块硬盘最多4个主分区，或者3个主分区+1个逻辑分区， 逻辑分区不能直接使用，需要划分成逻辑分区 . 主分区(活动的)一般为系统分区，扩展分区
	{

		//分区表
		memcpy(&stPT, &stMBR.PartitionTable[i], sizeof(PARTITIONTABLE));			//从 MBR中取出每个表 然后得到每个分区的信息

		//扩展分区
		if(stPT.PartitionTypeIndictor == PART_EXTENDED || stPT.PartitionTypeIndictor == PART_DOSX13X)
		{
			stDP.uchPTI = stPT.PartitionTypeIndictor;
			stDP.ullMRS = stDP.ullRS = stPT.PrcedingSector;

			break;
		}

		//没有分区了
		if(stPT.PartitionTypeIndictor == 0)
			break;

		stDP.ullPS = stPT.PrcedingSector;		//之前的扇区数
		stDP.ullTS = stPT.TotalSector;				//总扇区数
		stDP.uchBI = stPT.BootIndictor;			//是否激活
		stDP.wSH = stPT.StartHead;				//开始磁道号
		stDP.wSS = stPT.StartSector;				//开始扇区号

		//起始柱面 = 之前总扇区数 / 每柱面扇区数
		stDP.wSC = (stPT.StartCylinder == 0x3FF)?
			(WORD)(stDP.ullPS  / Sct.m_dwSPC):(stPT.StartCylinder);

		stDP.uchPTI = stPT.PartitionTypeIndictor;			//分区类型
		stDP.wEH= stPT.EndHead;									//结束磁道头
		stDP.wES = stPT.EndSector;								//结束扇区号

		//结束柱面 = (之前总扇区数 + 总扇区数 - 1) / 每柱面扇区数
		stDP.wEC = (stPT.EndCylinder == 0x3FF)?
			(WORD)((stDP.ullPS + stDP.ullTS - 1)  / Sct.m_dwSPC):(stPT.EndCylinder);

		//插入条目
		InsertListItem_Drive(stDP);


		//没有扩展分区
		if(i == 3) return;
	}



	for(int j=0; j<50; j++)
	{
		//如果 最后一个分区不是 扩展分区 直接退出
		if((stDP.uchPTI != PART_EXTENDED) && (stDP.uchPTI != PART_DOSX13X))
			return;

		//读 扩展分区的信息    扩展分区也可以理解为逻辑分区的老大 “主分区”
		if(!Sct.Read(stDP.ullRS, (LPWSTR)&stMBR))
			return;

		for(int i=0; i<4; i++)
		{
			memcpy(&stPT, &stMBR.PartitionTable[i], sizeof(PARTITIONTABLE));

			//扩展分区
			if(stPT.PartitionTypeIndictor == PART_EXTENDED || stPT.PartitionTypeIndictor == PART_DOSX13X)
			{
				stDP.uchPTI = stPT.PartitionTypeIndictor;
				stDP.ullRS = stDP.ullMRS + stPT.PrcedingSector;
				break;
			}

			//没有分区了
			if(stPT.PartitionTypeIndictor == 0)
			{
				stDP.uchPTI = stPT.PartitionTypeIndictor;
				stDP.ullRS = stDP.ullMRS = 0;
				break;
			}

			stDP.ullPS = stPT.PrcedingSector;
			stDP.ullTS = stPT.TotalSector;
			stDP.uchBI = stPT.BootIndictor;
			stDP.wSH = stPT.StartHead;
			stDP.wSS = stPT.StartSector;
			//起始柱面 = (相对扇区数 + 之前总扇区数) / 每柱面扇区数
			stDP.wSC = (stPT.StartCylinder == 0x3FF)?
				(WORD)((stDP.ullRS + stDP.ullPS) / Sct.m_dwSPC):(stPT.StartCylinder);
			stDP.uchPTI = stPT.PartitionTypeIndictor;
			stDP.wEH= stPT.EndHead;
			stDP.wES = stPT.EndSector;
			//结束柱面 = (扩展分区相对扇区数 + 之前总扇区数 + 总扇区数 - 1) / 每柱面扇区数
			stDP.wEC = (stPT.EndCylinder == 0x3FF)?
				(WORD)((stDP.ullRS + stDP.ullPS + stDP.ullTS - 1) / Sct.m_dwSPC):(stPT.EndCylinder);

			//插入条目
			InsertListItem_Drive(stDP);

			if(i == 3) return;
		}
	}
}


void CFileRecoverDlg::InsertListItem_Drive(DRIVEPACKET &stDP)
{
	static						int nItem;
	CMyList*				MyListDrive = (CMyList*)&m_ListDrive;

	memcpy(&g_DrivePacket[g_DriveCount],&stDP,sizeof(DRIVEPACKET));

	int							i = 0;
	int							n = m_ListDrive.GetItemCount();

	CString					strID;
	CString					strFileSystem;
	CString					strIdentify;
	CString					strSC;		//开始柱面
	CString					strSH;		//开始磁头
	CString					strSS;		//开始扇区
	CString					strEC;	
	CString					strEH;
	CString					strES;
	CString					strMount;			//容量



	strID.Format(L"%d",g_DriveCount);


	strFileSystem.Format(L"%S%s",GetFileSystemString(stDP.uchPTI), (stDP.uchBI == ACTIVE_PART)?L"(活动)":L"");
		

	strIdentify.Format(L"%02X",stDP.uchPTI);

	strSC.Format(L"%d",stDP.wSC);
	strSH.Format(L"%d",stDP.wSH);
	strSS.Format(L"%d",stDP.wSS);
	strEC.Format(L"%d",stDP.wEC);
	strEH.Format(L"%d",stDP.wEH);
	strES.Format(L"%d",stDP.wES);

	strMount.Format(L"%.1fGB",(signed __int64)((stDP.ullTS * 512)/1024/1024)/(float)1024);					//每个扇区的大小是固定的 512字节 


	i = MyListDrive->InsertItem(n,strID,n); //默认为0行  这样所有插入的新列都在最上面

	
	//传入分区类型 得到文件系统名字

	MyListDrive->SetItemText(i,1,strFileSystem);

	MyListDrive->SetItemText(i,2,strIdentify);
	MyListDrive->SetItemText(i,3,strSC);
	MyListDrive->SetItemText(i,4,strSH);
	MyListDrive->SetItemText(i,5,strSS);
	MyListDrive->SetItemText(i,6,strEC);
	MyListDrive->SetItemText(i,7,strEH);
	MyListDrive->SetItemText(i,8,strES);
	MyListDrive->SetItemText(i,9,strMount);

	g_DriveCount++;
}


LPCSTR GetFileSystemString(int nType)
{
	switch(nType)
	{
	case PART_DOS2_FAT:
		return "FAT12";
	case PART_DOS3_FAT:
	case PART_DOS4_FAT:
	case PART_DOSX13:
		return "FAT16";
	case PART_EXTENDED:
	case PART_DOSX13X:
		return "EXTENDED";
	case PART_DOS32X:
	case PART_DOS32:
		return "FAT32";
	case PART_NTFS:
		return "NTFS";
	default:
		return "UNKNOWN";
	}
}

void CFileRecoverDlg::OnBnClickedButtonScan()
{
	// TODO: 在此添加控件通知处理程序代码



	DRIVEPACKET*			DriverPacket = NULL;
	WCHAR						wzBuffer[64] = {0};

	int									iSelect = m_ListDrive.GetSelectionMark();

	DriverPacket = &g_DrivePacket[iSelect];



	GetDlgItemText(IDC_BUTTON_SCAN, wzBuffer, 20);
	
	if(lstrcmpi(wzBuffer, L"扫描") == 0)
	{
		g_FileCount = 0;					//文件个数 初始化
		CreateScanThread(TRUE, (LPVOID)DriverPacket);				//这里的传参
	}
	else
	{
		CreateScanThread(FALSE);
	}

}


void CFileRecoverDlg::CreateScanThread(BOOL bIsCreate, LPVOID lpThreadParameter)
{
	m_pDrivePacket = (PDRIVEPACKET)lpThreadParameter;
	static HANDLE			hThread;


	if(bIsCreate)
	{
		if(hThread == FALSE)
		{
			g_bStopScanThread = FALSE;
			hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ScanFilesThread, this, 0, 0);

			SetDlgItemText(IDC_BUTTON_SCAN, L"停止");
		}
	}
	else
	{
		if(hThread)
		{
			g_bStopScanThread = TRUE;

			CloseHandle(hThread);
			hThread = NULL;

			SetDlgItemText(IDC_BUTTON_SCAN, L"扫描");
		}
	}
}




void CFileRecoverDlg::InsertListItem_File(DWORD nFileSeq, int nItem, NTFS_FILEINFO &stfInfo)
{
	int						i = 0;
	CString				strNum;
	

	int						n = m_ListFile.GetItemCount();

	strNum.Format(L"%d",g_FileCount);
	


	i = g_ListFile->InsertItem(n,strNum,n); //默认为0行  这样所有插入的新列都在最上面

	//用g_ListFile 是因为我们自己的封装的 MyList类比较好用
	g_ListFile->SetItemText(i,1,stfInfo.szFileName);
	g_ListFile->SetItemText(i,2,stfInfo.szCreate);
	g_ListFile->SetItemText(i,3,stfInfo.szFileSize);


	g_FileIndex[g_FileCount] = nFileSeq;

	g_FileCount++;
}




DWORD WINAPI ScanFilesThread(LPVOID lpParam)
{
	NTFS_FILEINFO		stFileInfo;
	CFileRecoverDlg*		FileDlg = (CFileRecoverDlg*)lpParam;

	DRIVEPACKET			*pDR = (PDRIVEPACKET)FileDlg->m_pDrivePacket;



	if(g_pDrive)
		delete g_pDrive;

	g_pDrive = FactoryDrive(pDR->uchPTI);

	if(!g_pDrive)
	{
		MessageBox(0, L"不支持的分区", L"", 0);
		return 0;
	}

	//如果初始化 失败 ，关闭线程
	if(!g_pDrive->Initial(pDR))
	{
		FileDlg->CreateScanThread(FALSE);
		return 0;
	}

	//清空
	SetDlgItemInt(g_hDlg, IDC_STATIC_SCANED, 0, 0);
	SetDlgItemInt(g_hDlg, IDC_STATIC_DELTED, 0, 0);

	////删除所有条目
	g_ListFile->DeleteAllItems();
	
	//开始循环 查询
	for(int i=0, j=0; !g_bStopScanThread; i++)
	{
		SetDlgItemInt(g_hDlg, IDC_STATIC_SCANED, i, 0);

		if(!g_pDrive->IsHavingFile(i))
			break;

		if(!g_pDrive->GetFileInfoBySeq(i, stFileInfo))
			continue;

		FileDlg->InsertListItem_File(i, j, stFileInfo);

		j++;

		SetDlgItemInt(g_hDlg, IDC_STATIC_DELTED, j, 0);
	}

	if(!g_bStopScanThread)//正常结束
		FileDlg->CreateScanThread(FALSE);

	return 1;
}




void CFileRecoverDlg::OnBnClickedButtonSave()
{
	// TODO: 在此添加控件通知处理程序代码

	
	LPWSTR				lpszFileName = 0, lpszData = 0;
	DWORD				cchData = 0;

	int					iSelect = m_ListFile.GetSelectionMark();

	int					nFileSeq = g_FileIndex[iSelect];

	

	
	lpszFileName = DoFileOpenSave(g_hDlg, TRUE, L"文本文件 (*.txt)\0*.txt\0所有文件 (*.*)\0*.*\0\0", L"txt");
	if(!lpszFileName)
		return;

	if(!g_pDrive)
		return;
	if(!g_pDrive->GetData(nFileSeq, lpszData, cchData))
		return;

	FILE *fp = 0;

	fp = _wfopen(lpszFileName, L"ab+");
	if(fp != NULL)
	{
		fwrite(lpszData, cchData, 1, fp);
		fclose(fp);
	}

	if(lpszFileName)
		delete []lpszFileName;
	if(lpszData)
		delete []lpszData;


}

//打开对话框
WCHAR* DoFileOpenSave(HWND hwnd, BOOL bSave, LPWSTR lpszFilter, LPWSTR lpszDefExt)
{
	OPENFILENAME					ofn = {0};				//打开文件 的信息
	WCHAR *									szFileName=new WCHAR [MAX_PATH];

	szFileName[0] = 0;

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = lpszFilter;
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = lpszDefExt;

	if(bSave)
	{
		ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
			OFN_OVERWRITEPROMPT;

		if(GetSaveFileName(&ofn))
		{
			return szFileName;
		}
	}
	else
	{
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		if(GetOpenFileName(&ofn))
		{
			return szFileName;
		}
	}

	delete []szFileName;
	return 0;
}


void CFileRecoverDlg::OnLvnColumnclickListFile(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码


	sort_column=pNMLV->iSubItem;//点击的列

	int count=m_ListFile.GetItemCount();

	for(int i=0;i<count;i++)
	{

		m_ListFile.SetItemData(i,i);//每行的比较关键字，此处为列序号（点击的列号），可以设置为其他 比较函数的第一二个参数

	}
	m_ListFile.SortItems(HsCompareProc,(DWORD_PTR)&m_ListFile);//排序 第二个参数是比较函数的第三个参数

	if (method == TRUE)
	{
		method = FALSE;
	}
	else
	{
		method = TRUE;
	}




	*pResult = 0;
}




//实现排序
static int CALLBACK HsCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{

	//从参数中提取所需比较lc的两行数据

	int row1=(int) lParam1;
	int row2=(int) lParam2;

	CListCtrl*lc=(CListCtrl*)lParamSort;

	CString lp1=lc->GetItemText(row1,sort_column);
	CString lp2=lc->GetItemText(row2,sort_column);


	//比较，对不同的列，不同比较，注意记录前一次排序方向，下一次要相反排序

	if (sort_column == 0 ||
		sort_column == 2)
	{
		// int型比较
		if (method)
			return _ttoi(lp1)-_ttoi(lp2);
		else
			return _ttoi(lp2)-_ttoi(lp1);
	}
	else if (sort_column == 3)
	{
		if (method)
		{
			ULONG_PTR nlp1 = 0, nlp2 = 0;

			lp1 = lp1.GetBuffer()+2;
			lp2 = lp2.GetBuffer()+2;

			swscanf_s(lp1.GetBuffer(),L"%P",&nlp1);
			swscanf_s(lp2.GetBuffer(),L"%P",&nlp2);
			return nlp1 - nlp2;
		}

		else
		{
			int nlp1 = 0, nlp2 = 0;
			lp1 = lp1.GetBuffer()+2;
			lp2 = lp2.GetBuffer()+2;
			swscanf_s(lp1.GetBuffer(),L"%X",&nlp1);
			swscanf_s(lp2.GetBuffer(),L"%X",&nlp2);
			return nlp2 - nlp1;
		}
	}
	else
	{
		// 文字型比较
		if(method)
			return lp1.CompareNoCase(lp2);
		else
			return lp2.CompareNoCase(lp1);
	}

	return 0;
}
