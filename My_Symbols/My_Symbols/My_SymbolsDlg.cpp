
// My_SymbolsDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "My_Symbols.h"
#include "My_SymbolsDlg.h"
#include "afxdialogex.h"
#include "Symbols.h"
#include "SymbolsDlg.h"
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif




COLUMNSTRUCT g_Column_Module[] = 
{
	{	L"����",		50	},
	{	L"ģ����",		200	},
	{	L"��ַ",		140	},
	{	L"��С",		100	},
	{	L"����",		50	},
	{	L"�����ļ�",	300	},
	{	L"ģ���ļ�",	300	}
};


int g_Column_Module_Count = 7;

CString SYMBOLSPATH ;
extern vector<MODULE_INFO>  g_all_module_info;

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


// CMy_SymbolsDlg �Ի���




CMy_SymbolsDlg::CMy_SymbolsDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMy_SymbolsDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMy_SymbolsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MODULE, m_module_list);
}

BEGIN_MESSAGE_MAP(CMy_SymbolsDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(ID_MENU_REFRESH, &CMy_SymbolsDlg::OnMenuRefresh)
	ON_MESSAGE(MY_GETALLMODULE, GetAllModuleFinished)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_MODULE, &CMy_SymbolsDlg::OnNMRClickListModule)
	ON_COMMAND(ID_MENU_ENUM_SYMBOLS, &CMy_SymbolsDlg::OnMenuEnumSymbols)
	ON_WM_CLOSE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_MODULE, &CMy_SymbolsDlg::OnLvnColumnclickListModule)
	ON_COMMAND(ID_MENU_LOCATE_MODULE_FILE, &CMy_SymbolsDlg::OnMenuLocateModuleFile)
	ON_COMMAND(ID_MENU_LOCATE_SYMBOLS_FILE, &CMy_SymbolsDlg::OnMenuLocateSymbolsFile)
	ON_COMMAND(ID_MENU_COPY_MODULE_PATH, &CMy_SymbolsDlg::OnMenuCopyModulePath)
	ON_COMMAND(ID_MENU_COPY_MODULE_NAME, &CMy_SymbolsDlg::OnMenuCopyModuleName)
	ON_COMMAND(ID_MENU_COPY_SYMBOLS_PATH, &CMy_SymbolsDlg::OnMenuCopySymbolsPath)
END_MESSAGE_MAP()


// CMy_SymbolsDlg ��Ϣ�������

BOOL CMy_SymbolsDlg::OnInitDialog()
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

	if (!InitSymbolsPath())
	{
		//���ű�·������ 
		return FALSE;
	}
	InitModuleList();
	CModuleList*  ptr_module_list = new CModuleList(this);
	ptr_module_list->GetAllModuleList();

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CMy_SymbolsDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMy_SymbolsDlg::OnPaint()
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
HCURSOR CMy_SymbolsDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



BOOL CMy_SymbolsDlg::InitSymbolsPath()
{	
	SYMBOLSPATH = ((CMy_SymbolsApp*)AfxGetApp())->m_IniFile.GetString(L"Settings", L"SymbolsPath"); //��ȡini�ļ��еķ��ű�·��
	if (SYMBOLSPATH == L"")
	{
		BROWSEINFO bi;
		WCHAR Buffer[MAX_PATH];
		//��ʼ����ڲ���bi��ʼ
		bi.hwndOwner = NULL;
		bi.pidlRoot =NULL;//��ʼ���ƶ���rootĿ¼�ܲ����ף�
		bi.pszDisplayName = Buffer;//�˲�����ΪNULL������ʾ�Ի���
		bi.lpszTitle = L"ѡ�񱾵ط��ű�·��";
		//bi.ulFlags = BIF_BROWSEINCLUDEFILES;//�����ļ�
		bi.ulFlags = BIF_EDITBOX;//�����ļ�
		bi.lpfn = NULL;
		bi.iImage=IDR_MAINFRAME;
		//��ʼ����ڲ���bi����
		LPITEMIDLIST pIDList = SHBrowseForFolder(&bi);//������ʾѡ��Ի���
		if(!pIDList)
		{
			return FALSE;
		}
		SHGetPathFromIDList(pIDList, Buffer);
		//ȡ���ļ���·����Buffer��
		SYMBOLSPATH = Buffer;//���ļ���·��������һ��CString������
		LPMALLOC lpMalloc;
		if(FAILED(SHGetMalloc(&lpMalloc)))
		{
			return   FALSE;
		}
		lpMalloc->Free(pIDList);
		lpMalloc->Release();
		((CMy_SymbolsApp *)AfxGetApp())->m_IniFile.SetString(L"Settings", L"SymbolsPath", SYMBOLSPATH);
	}

	InitSymHandler();
	return TRUE;
}


BOOL CMy_SymbolsDlg::InitModuleList()
{
	m_module_list.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_HEADERDRAGDROP);

	INT i;
	for (i = 0;i<4;i++)
	{
		m_module_list.InsertColumn(i, g_Column_Module[i].szTitle,LVCFMT_CENTER,g_Column_Module[i].nWidth);
	}
	for (;i<g_Column_Module_Count;i++)
	{
		m_module_list.InsertColumn(i, g_Column_Module[i].szTitle,LVCFMT_LEFT,g_Column_Module[i].nWidth);
	}
	
	
	//��һ��������̶��еĿ��
	m_module_list.SetColumnWidth(0,LVSCW_AUTOSIZE_USEHEADER);/*����������*/
	return TRUE;
}


void CMy_SymbolsDlg::OnMenuRefresh()
{
	// TODO: �ڴ���������������
	m_module_list.DeleteAllItems();
	CModuleList*  ptr_module_list = new CModuleList(this);
	ptr_module_list->GetAllModuleList();
}


//�Զ�����Ϣ
LRESULT CMy_SymbolsDlg::GetAllModuleFinished(WPARAM wParam,LPARAM lParam)
{
	char symbols_file_name[MAX_PATH]={0};
	for (vector<MODULE_INFO>::iterator itor= g_all_module_info.begin();
		itor != g_all_module_info.end();itor++)
	{
		ZeroMemory(symbols_file_name,MAX_PATH);
		CString cstr_index,cstr_module_name,cstr_base_addr,cstr_size, cstr_symbols_path,cstr_full_path;
		cstr_index.Format(L"%d",itor->Index);
		cstr_module_name = itor->ImageName;
		cstr_base_addr.Format(L"0x%08p",itor->BaseAddr);
		cstr_size.Format(L"0x%x",itor->Size);
		cstr_full_path = itor->FullPath;
		int n = m_module_list.InsertItem(m_module_list.GetItemCount(),cstr_index);
		m_module_list.SetItemText(n, 1, cstr_module_name);
		m_module_list.SetItemText(n, 2, cstr_base_addr);
		m_module_list.SetItemText(n, 3, cstr_size);
		if (GetFileSymbolsA(itor->FullPath,symbols_file_name,MAX_PATH))
		{
			m_module_list.SetItemText(n, 4,L"��");
			cstr_symbols_path = symbols_file_name;
			itor->bSymbols = true;
			strcpy_s(itor->SymbolsPath,symbols_file_name);
		}
		else
		{
			itor->bSymbols = false;
			m_module_list.SetItemText(n, 4, L"      ��");
			cstr_symbols_path = L"";
		}
		//(*itor).SymbolsPath = cstr_symbols_path;
 		m_module_list.SetItemText(n, 5, cstr_symbols_path);
 		m_module_list.SetItemText(n, 6, cstr_full_path);
	}
	return 0;
}


void CMy_SymbolsDlg::OnNMRClickListModule(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CMenu	popup;
	popup.LoadMenu(IDR_MENU_MAIN);             //���ز˵���Դ
	CMenu*	pM = popup.GetSubMenu(0);                 //��ò˵�������
	CPoint	p;
	GetCursorPos(&p);
	int	count = pM->GetMenuItemCount();
	if (m_module_list.GetSelectedCount() == 0)       //���û��ѡ��
	{ 
		for (int i = 0;i<count;i++)
		{
			pM->EnableMenuItem(i, MF_BYPOSITION | MF_DISABLED | MF_GRAYED);          //�˵�ȫ�����
		}

	}
	pM->TrackPopupMenu(TPM_LEFTALIGN, p.x, p.y, this);

	*pResult = 0;
}



//ö�ٷ���
void CMy_SymbolsDlg::OnMenuEnumSymbols()
{
	// TODO: �ڴ���������������
	int Index = m_module_list.GetSelectionMark();
	CString module_name = m_module_list.GetItemText(Index,1);
	for (vector<MODULE_INFO>::iterator itor = g_all_module_info.begin();
		itor != g_all_module_info.end();itor++)
	{
		CString tmp_name;
		tmp_name = itor->ImageName;
		if (!module_name.CompareNoCase(tmp_name))
		{

			CSymbolsDlg *dlg = new CSymbolsDlg(*itor);

			dlg->Create(IDD_DIALOG_SYMBOLS,GetDesktopWindow());
			dlg->ShowWindow(SW_SHOW);	
		}	
		
	}


}


void CMy_SymbolsDlg::OnClose()
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	UnSymHandler();
	CDialogEx::OnClose();
}


void CMy_SymbolsDlg::OnLvnColumnclickListModule(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	Sort((TYPE)pNMLV->iSubItem);

	*pResult = 0;
}


bool cmp_name(MODULE_INFO & info1,MODULE_INFO & info2)
{
	return *(char*)info1.ImageName < *(char*)info2.ImageName;

}

bool cmp_index(MODULE_INFO & info1,MODULE_INFO & info2)
{

	return info1.Index < info2.Index;
}


bool cmp_addr(MODULE_INFO & info1,MODULE_INFO & info2)
{

	return info1.BaseAddr < info2.BaseAddr;
}


bool cmp_size(MODULE_INFO & info1,MODULE_INFO & info2)
{

	return info1.Size < info2.Size;
}


BOOL CMy_SymbolsDlg::Sort(TYPE sort)
{
	switch(sort)
	{
	case type_index:
		std::sort(g_all_module_info.begin(),g_all_module_info.end(),cmp_index);
		break;
	case type_name:
		std::sort(g_all_module_info.begin(),g_all_module_info.end(),cmp_name);
		break;
	case type_addr:
		std::sort(g_all_module_info.begin(),g_all_module_info.end(),cmp_addr);
		break;
	case type_size:
		std::sort(g_all_module_info.begin(),g_all_module_info.end(),cmp_size);
		break;
	}

	update_module_list();

	return TRUE;
}


void CMy_SymbolsDlg::update_module_list()
{
	m_module_list.DeleteAllItems();
	CString cstr_index,cstr_module_name,cstr_base_addr,cstr_size, cstr_symbols_path,cstr_full_path;
	for (vector<MODULE_INFO>::iterator itor= g_all_module_info.begin();
		itor != g_all_module_info.end();itor++)
	{
		cstr_index.Format(L"%d",itor->Index);
		cstr_module_name = itor->ImageName;
		cstr_base_addr.Format(L"0x%08p",itor->BaseAddr);
		cstr_size.Format(L"0x%x",itor->Size);
		cstr_full_path = itor->FullPath;
		int n = m_module_list.InsertItem(m_module_list.GetItemCount(),cstr_index);
		m_module_list.SetItemText(n, 1, cstr_module_name);
		m_module_list.SetItemText(n, 2, cstr_base_addr);
		m_module_list.SetItemText(n, 3, cstr_size);
		if (itor->bSymbols)
		{
			m_module_list.SetItemText(n, 4,L"��");
			cstr_symbols_path = itor->SymbolsPath;
		}
		else
		{
		
			m_module_list.SetItemText(n, 4, L"      ��");
			cstr_symbols_path = L"";
		}
		m_module_list.SetItemText(n, 5, cstr_symbols_path);
		m_module_list.SetItemText(n, 6, cstr_full_path);
	}

}

void CMy_SymbolsDlg::OnMenuLocateModuleFile()
{
	// TODO: �ڴ���������������

	int index = m_module_list.GetSelectionMark();
	wchar_t buf[MAX_PATH] = {0};
	wchar_t com_line[MAX_PATH] = {0};
	m_module_list.GetItemText(index,type_module_path,buf,MAX_PATH);

	wsprintf(com_line,L"explorer.exe /select , %s",buf);
	char tmp_com_line[MAX_PATH] = {0};
	wcstombs_s(NULL,tmp_com_line,com_line,MAX_PATH);
	if (31<WinExec(tmp_com_line,SW_SHOWMAXIMIZED))
	{
		return ;
	}
}


void CMy_SymbolsDlg::OnMenuLocateSymbolsFile()
{
	// TODO: �ڴ���������������
	int index = m_module_list.GetSelectionMark();
	wchar_t buf[MAX_PATH] = {0};
	wchar_t com_line[MAX_PATH] = {0};
	m_module_list.GetItemText(index,type_symbol_path,buf,MAX_PATH);

	wsprintf(com_line,L"explorer.exe /select , %s",buf);
	char tmp_com_line[MAX_PATH] = {0};
	wcstombs_s(NULL,tmp_com_line,com_line,MAX_PATH);
	if (31<WinExec(tmp_com_line,SW_SHOWMAXIMIZED))
	{
		return ;
	}
}



//���а�    
bool ToShearPlate(char * Text)
{
	bool Ret = false;
	if (NULL == Text){ return Ret; }
	int Size = (int)strlen(Text);
	if (NULL >= Size){ return Ret; }

	HGLOBAL hGlobalMemory = NULL;//��¼һ���ڴ� ��� (ȫ��)�����˵�ͷţ���ô������ �����޷�������д������

	if (FALSE == OpenClipboard(NULL)){//�򿪼��а�
		return Ret;
	}
	hGlobalMemory = GlobalAlloc(GHND, Size + 1);//����һ�� ȫ�ֵ� ��ջ�ڴ�
	if (NULL == (int)hGlobalMemory)
	{
		goto GoToFree;
	}
	void * lpGlobalMemory = GlobalLock(hGlobalMemory);//����һ��ȫ���ڴ�
	if (NULL == (int)lpGlobalMemory)
	{
		goto GoToFree;
	}
	if (FALSE == EmptyClipboard())
	{//������а�����
		goto GoToFree;
	}
	memcpy(lpGlobalMemory, Text, Size + 1);//�����ݿ����� ȫ�ֶ�ջ��
	if (NULL == SetClipboardData(CF_TEXT, hGlobalMemory))
	{//��ָ�����ݷŵ����а��У�����ָ���ĸ�ʽ
		goto GoToFree;
	}
	Ret = true;
GoToFree:
	CloseClipboard();//�رռ��а�
	if (NULL != hGlobalMemory)
	{
		//�ͷŵ� ֮ǰ������ڴ�
		GlobalFree(hGlobalMemory);
	}
	return Ret;
}



void CMy_SymbolsDlg::OnMenuCopyModulePath()
{
	// TODO: �ڴ���������������
	int index = m_module_list.GetSelectionMark();
	wchar_t buf[MAX_PATH] = {0};
	wchar_t com_line[MAX_PATH] = {0};
	m_module_list.GetItemText(index,type_module_path,buf,MAX_PATH);
	char tmp_com_line[MAX_PATH] = {0};
	wcstombs_s(NULL,tmp_com_line,com_line,MAX_PATH);
	ToShearPlate(tmp_com_line);
}


void CMy_SymbolsDlg::OnMenuCopyModuleName()
{
	// TODO: �ڴ���������������
	int index = m_module_list.GetSelectionMark();
	wchar_t buf[MAX_PATH] = {0};
	wchar_t com_line[MAX_PATH] = {0};
	m_module_list.GetItemText(index,type_name,buf,MAX_PATH);

	char tmp_com_line[MAX_PATH] = {0};
	wcstombs_s(NULL,tmp_com_line,com_line,MAX_PATH);
	ToShearPlate(tmp_com_line);
}


void CMy_SymbolsDlg::OnMenuCopySymbolsPath()
{
	// TODO: �ڴ���������������

	int index = m_module_list.GetSelectionMark();
	wchar_t buf[MAX_PATH] = {0};
	wchar_t com_line[MAX_PATH] = {0};
	m_module_list.GetItemText(index,type_symbol_path,buf,MAX_PATH);

	char tmp_com_line[MAX_PATH] = {0};
	wcstombs_s(NULL,tmp_com_line,com_line,MAX_PATH);
	ToShearPlate(tmp_com_line);
}



