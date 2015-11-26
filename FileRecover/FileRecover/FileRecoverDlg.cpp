
// FileRecoverDlg.cpp : ʵ���ļ�
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


UINT g_Column_Drive_Count  = 10;	  //�����б�����

COLUMNSTRUCT g_Column_Drive[] = 
{
	{	L"���",			60	},
	{	L"�ļ�ϵͳ",			120	},
	{	L"��ʶ",			75	},
	{	L"��ʼ����",			85	},
	{	L"��ͷ",		65	},
	{	L"����",		65	},
	{	L"��ֹ����",			85	}, 
	{	L"��ͷ",			65},
	{L"����",				65},
	{L"����",					130}
};


UINT g_Column_File_Count  = 4;	  //�����б�����

COLUMNSTRUCT g_Column_File[] = 
{
	{	L"���",			100	},
	{	L"�ļ���",			150	},
	{	L"����ʱ��",			220	},
	{	L"��С",			100	}
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


LONG					sort_column; //��¼�������
BOOL					method; //��¼�ȽϷ���

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CFileRecoverDlg �Ի���




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


// CFileRecoverDlg ��Ϣ�������

BOOL CFileRecoverDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	g_hDlg = AfxGetMainWnd()->m_hWnd;

	g_ListFile = (CMyList*)&m_ListFile;
	g_ListDrive = (CMyList*)&m_ListDrive;

	CPaintDC dc(this);
	dpix = GetDeviceCaps(dc.m_hDC,LOGPIXELSX);
	dpiy = GetDeviceCaps(dc.m_hDC,LOGPIXELSY);

	InitList();

	//
	InitListDriveItem();


	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CFileRecoverDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
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



//������ ö��ÿ����������Ϣ   ������Ϣ��MBR�д���
VOID  CFileRecoverDlg::InitListDriveItem()
{
	MASTER_BOOT_RECORD		stMBR = {0};			//��������¼		��߶��������ַ
	PARTITIONTABLE						stPT = {0};				//������
	DRIVEPACKET							stDP = {0};				//���̵�ÿ������

	CSector										Sct;							//��������������

	//��ʼ��  ������	�����������ŵ�������Ȼ�����Ϣ
	if(!Sct.Initial())
		return;

	//��ȡMBR����		�ڵ�0����		����MBR�ṹ�� 
	if(!Sct.Read(0, (LPWSTR)&stMBR))		//��ʼ��ȡ�������� ������  Ҫ����������(Ĭ��Ϊ1)
		return;

	for(int i=0; i<4; i++)	//��������¼���� ������		һ��Ӳ�����4��������������3��������+1���߼������� �߼���������ֱ��ʹ�ã���Ҫ���ֳ��߼����� . ������(���)һ��Ϊϵͳ��������չ����
	{

		//������
		memcpy(&stPT, &stMBR.PartitionTable[i], sizeof(PARTITIONTABLE));			//�� MBR��ȡ��ÿ���� Ȼ��õ�ÿ����������Ϣ

		//��չ����
		if(stPT.PartitionTypeIndictor == PART_EXTENDED || stPT.PartitionTypeIndictor == PART_DOSX13X)
		{
			stDP.uchPTI = stPT.PartitionTypeIndictor;
			stDP.ullMRS = stDP.ullRS = stPT.PrcedingSector;

			break;
		}

		//û�з�����
		if(stPT.PartitionTypeIndictor == 0)
			break;

		stDP.ullPS = stPT.PrcedingSector;		//֮ǰ��������
		stDP.ullTS = stPT.TotalSector;				//��������
		stDP.uchBI = stPT.BootIndictor;			//�Ƿ񼤻�
		stDP.wSH = stPT.StartHead;				//��ʼ�ŵ���
		stDP.wSS = stPT.StartSector;				//��ʼ������

		//��ʼ���� = ֮ǰ�������� / ÿ����������
		stDP.wSC = (stPT.StartCylinder == 0x3FF)?
			(WORD)(stDP.ullPS  / Sct.m_dwSPC):(stPT.StartCylinder);

		stDP.uchPTI = stPT.PartitionTypeIndictor;			//��������
		stDP.wEH= stPT.EndHead;									//�����ŵ�ͷ
		stDP.wES = stPT.EndSector;								//����������

		//�������� = (֮ǰ�������� + �������� - 1) / ÿ����������
		stDP.wEC = (stPT.EndCylinder == 0x3FF)?
			(WORD)((stDP.ullPS + stDP.ullTS - 1)  / Sct.m_dwSPC):(stPT.EndCylinder);

		//������Ŀ
		InsertListItem_Drive(stDP);


		//û����չ����
		if(i == 3) return;
	}



	for(int j=0; j<50; j++)
	{
		//��� ���һ���������� ��չ���� ֱ���˳�
		if((stDP.uchPTI != PART_EXTENDED) && (stDP.uchPTI != PART_DOSX13X))
			return;

		//�� ��չ��������Ϣ    ��չ����Ҳ�������Ϊ�߼��������ϴ� ����������
		if(!Sct.Read(stDP.ullRS, (LPWSTR)&stMBR))
			return;

		for(int i=0; i<4; i++)
		{
			memcpy(&stPT, &stMBR.PartitionTable[i], sizeof(PARTITIONTABLE));

			//��չ����
			if(stPT.PartitionTypeIndictor == PART_EXTENDED || stPT.PartitionTypeIndictor == PART_DOSX13X)
			{
				stDP.uchPTI = stPT.PartitionTypeIndictor;
				stDP.ullRS = stDP.ullMRS + stPT.PrcedingSector;
				break;
			}

			//û�з�����
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
			//��ʼ���� = (��������� + ֮ǰ��������) / ÿ����������
			stDP.wSC = (stPT.StartCylinder == 0x3FF)?
				(WORD)((stDP.ullRS + stDP.ullPS) / Sct.m_dwSPC):(stPT.StartCylinder);
			stDP.uchPTI = stPT.PartitionTypeIndictor;
			stDP.wEH= stPT.EndHead;
			stDP.wES = stPT.EndSector;
			//�������� = (��չ������������� + ֮ǰ�������� + �������� - 1) / ÿ����������
			stDP.wEC = (stPT.EndCylinder == 0x3FF)?
				(WORD)((stDP.ullRS + stDP.ullPS + stDP.ullTS - 1) / Sct.m_dwSPC):(stPT.EndCylinder);

			//������Ŀ
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
	CString					strSC;		//��ʼ����
	CString					strSH;		//��ʼ��ͷ
	CString					strSS;		//��ʼ����
	CString					strEC;	
	CString					strEH;
	CString					strES;
	CString					strMount;			//����



	strID.Format(L"%d",g_DriveCount);


	strFileSystem.Format(L"%S%s",GetFileSystemString(stDP.uchPTI), (stDP.uchBI == ACTIVE_PART)?L"(�)":L"");
		

	strIdentify.Format(L"%02X",stDP.uchPTI);

	strSC.Format(L"%d",stDP.wSC);
	strSH.Format(L"%d",stDP.wSH);
	strSS.Format(L"%d",stDP.wSS);
	strEC.Format(L"%d",stDP.wEC);
	strEH.Format(L"%d",stDP.wEH);
	strES.Format(L"%d",stDP.wES);

	strMount.Format(L"%.1fGB",(signed __int64)((stDP.ullTS * 512)/1024/1024)/(float)1024);					//ÿ�������Ĵ�С�ǹ̶��� 512�ֽ� 


	i = MyListDrive->InsertItem(n,strID,n); //Ĭ��Ϊ0��  �������в�������ж���������

	
	//����������� �õ��ļ�ϵͳ����

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
	// TODO: �ڴ���ӿؼ�֪ͨ����������



	DRIVEPACKET*			DriverPacket = NULL;
	WCHAR						wzBuffer[64] = {0};

	int									iSelect = m_ListDrive.GetSelectionMark();

	DriverPacket = &g_DrivePacket[iSelect];



	GetDlgItemText(IDC_BUTTON_SCAN, wzBuffer, 20);
	
	if(lstrcmpi(wzBuffer, L"ɨ��") == 0)
	{
		g_FileCount = 0;					//�ļ����� ��ʼ��
		CreateScanThread(TRUE, (LPVOID)DriverPacket);				//����Ĵ���
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

			SetDlgItemText(IDC_BUTTON_SCAN, L"ֹͣ");
		}
	}
	else
	{
		if(hThread)
		{
			g_bStopScanThread = TRUE;

			CloseHandle(hThread);
			hThread = NULL;

			SetDlgItemText(IDC_BUTTON_SCAN, L"ɨ��");
		}
	}
}




void CFileRecoverDlg::InsertListItem_File(DWORD nFileSeq, int nItem, NTFS_FILEINFO &stfInfo)
{
	int						i = 0;
	CString				strNum;
	

	int						n = m_ListFile.GetItemCount();

	strNum.Format(L"%d",g_FileCount);
	


	i = g_ListFile->InsertItem(n,strNum,n); //Ĭ��Ϊ0��  �������в�������ж���������

	//��g_ListFile ����Ϊ�����Լ��ķ�װ�� MyList��ȽϺ���
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
		MessageBox(0, L"��֧�ֵķ���", L"", 0);
		return 0;
	}

	//�����ʼ�� ʧ�� ���ر��߳�
	if(!g_pDrive->Initial(pDR))
	{
		FileDlg->CreateScanThread(FALSE);
		return 0;
	}

	//���
	SetDlgItemInt(g_hDlg, IDC_STATIC_SCANED, 0, 0);
	SetDlgItemInt(g_hDlg, IDC_STATIC_DELTED, 0, 0);

	////ɾ��������Ŀ
	g_ListFile->DeleteAllItems();
	
	//��ʼѭ�� ��ѯ
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

	if(!g_bStopScanThread)//��������
		FileDlg->CreateScanThread(FALSE);

	return 1;
}




void CFileRecoverDlg::OnBnClickedButtonSave()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	
	LPWSTR				lpszFileName = 0, lpszData = 0;
	DWORD				cchData = 0;

	int					iSelect = m_ListFile.GetSelectionMark();

	int					nFileSeq = g_FileIndex[iSelect];

	

	
	lpszFileName = DoFileOpenSave(g_hDlg, TRUE, L"�ı��ļ� (*.txt)\0*.txt\0�����ļ� (*.*)\0*.*\0\0", L"txt");
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

//�򿪶Ի���
WCHAR* DoFileOpenSave(HWND hwnd, BOOL bSave, LPWSTR lpszFilter, LPWSTR lpszDefExt)
{
	OPENFILENAME					ofn = {0};				//���ļ� ����Ϣ
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������


	sort_column=pNMLV->iSubItem;//�������

	int count=m_ListFile.GetItemCount();

	for(int i=0;i<count;i++)
	{

		m_ListFile.SetItemData(i,i);//ÿ�еıȽϹؼ��֣��˴�Ϊ����ţ�������кţ�����������Ϊ���� �ȽϺ����ĵ�һ��������

	}
	m_ListFile.SortItems(HsCompareProc,(DWORD_PTR)&m_ListFile);//���� �ڶ��������ǱȽϺ����ĵ���������

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




//ʵ������
static int CALLBACK HsCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{

	//�Ӳ�������ȡ����Ƚ�lc����������

	int row1=(int) lParam1;
	int row2=(int) lParam2;

	CListCtrl*lc=(CListCtrl*)lParamSort;

	CString lp1=lc->GetItemText(row1,sort_column);
	CString lp2=lc->GetItemText(row2,sort_column);


	//�Ƚϣ��Բ�ͬ���У���ͬ�Ƚϣ�ע���¼ǰһ����������һ��Ҫ�෴����

	if (sort_column == 0 ||
		sort_column == 2)
	{
		// int�ͱȽ�
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
		// �����ͱȽ�
		if(method)
			return lp1.CompareNoCase(lp2);
		else
			return lp2.CompareNoCase(lp1);
	}

	return 0;
}
