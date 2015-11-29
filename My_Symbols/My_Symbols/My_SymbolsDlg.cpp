
// My_SymbolsDlg.cpp : 实现文件
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
	{	L"索引",		50	},
	{	L"模块名",		200	},
	{	L"基址",		140	},
	{	L"大小",		100	},
	{	L"符号",		50	},
	{	L"符号文件",	300	},
	{	L"模块文件",	300	}
};


int g_Column_Module_Count = 7;

CString SYMBOLSPATH ;
extern vector<MODULE_INFO>  g_all_module_info;

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


// CMy_SymbolsDlg 对话框




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


// CMy_SymbolsDlg 消息处理程序

BOOL CMy_SymbolsDlg::OnInitDialog()
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

	if (!InitSymbolsPath())
	{
		//符号表路径错误 
		return FALSE;
	}
	InitModuleList();
	CModuleList*  ptr_module_list = new CModuleList(this);
	ptr_module_list->GetAllModuleList();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMy_SymbolsDlg::OnPaint()
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
HCURSOR CMy_SymbolsDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



BOOL CMy_SymbolsDlg::InitSymbolsPath()
{	
	SYMBOLSPATH = ((CMy_SymbolsApp*)AfxGetApp())->m_IniFile.GetString(L"Settings", L"SymbolsPath"); //读取ini文件中的符号表路径
	if (SYMBOLSPATH == L"")
	{
		BROWSEINFO bi;
		WCHAR Buffer[MAX_PATH];
		//初始化入口参数bi开始
		bi.hwndOwner = NULL;
		bi.pidlRoot =NULL;//初始化制定的root目录很不容易，
		bi.pszDisplayName = Buffer;//此参数如为NULL则不能显示对话框
		bi.lpszTitle = L"选择本地符号表路径";
		//bi.ulFlags = BIF_BROWSEINCLUDEFILES;//包括文件
		bi.ulFlags = BIF_EDITBOX;//包括文件
		bi.lpfn = NULL;
		bi.iImage=IDR_MAINFRAME;
		//初始化入口参数bi结束
		LPITEMIDLIST pIDList = SHBrowseForFolder(&bi);//调用显示选择对话框
		if(!pIDList)
		{
			return FALSE;
		}
		SHGetPathFromIDList(pIDList, Buffer);
		//取得文件夹路径到Buffer里
		SYMBOLSPATH = Buffer;//将文件夹路径保存在一个CString对象里
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
	
	
	//第一个参数会固定列的宽度
	m_module_list.SetColumnWidth(0,LVSCW_AUTOSIZE_USEHEADER);/*产生滚动条*/
	return TRUE;
}


void CMy_SymbolsDlg::OnMenuRefresh()
{
	// TODO: 在此添加命令处理程序代码
	m_module_list.DeleteAllItems();
	CModuleList*  ptr_module_list = new CModuleList(this);
	ptr_module_list->GetAllModuleList();
}


//自定义消息
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
			m_module_list.SetItemText(n, 4,L"√");
			cstr_symbols_path = symbols_file_name;
			itor->bSymbols = true;
			strcpy_s(itor->SymbolsPath,symbols_file_name);
		}
		else
		{
			itor->bSymbols = false;
			m_module_list.SetItemText(n, 4, L"      ×");
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
	// TODO: 在此添加控件通知处理程序代码
	CMenu	popup;
	popup.LoadMenu(IDR_MENU_MAIN);             //加载菜单资源
	CMenu*	pM = popup.GetSubMenu(0);                 //获得菜单的子项
	CPoint	p;
	GetCursorPos(&p);
	int	count = pM->GetMenuItemCount();
	if (m_module_list.GetSelectedCount() == 0)       //如果没有选中
	{ 
		for (int i = 0;i<count;i++)
		{
			pM->EnableMenuItem(i, MF_BYPOSITION | MF_DISABLED | MF_GRAYED);          //菜单全部变灰
		}

	}
	pM->TrackPopupMenu(TPM_LEFTALIGN, p.x, p.y, this);

	*pResult = 0;
}



//枚举符号
void CMy_SymbolsDlg::OnMenuEnumSymbols()
{
	// TODO: 在此添加命令处理程序代码
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
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	UnSymHandler();
	CDialogEx::OnClose();
}


void CMy_SymbolsDlg::OnLvnColumnclickListModule(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
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
			m_module_list.SetItemText(n, 4,L"√");
			cstr_symbols_path = itor->SymbolsPath;
		}
		else
		{
		
			m_module_list.SetItemText(n, 4, L"      ×");
			cstr_symbols_path = L"";
		}
		m_module_list.SetItemText(n, 5, cstr_symbols_path);
		m_module_list.SetItemText(n, 6, cstr_full_path);
	}

}

void CMy_SymbolsDlg::OnMenuLocateModuleFile()
{
	// TODO: 在此添加命令处理程序代码

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
	// TODO: 在此添加命令处理程序代码
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



//剪切板    
bool ToShearPlate(char * Text)
{
	bool Ret = false;
	if (NULL == Text){ return Ret; }
	int Size = (int)strlen(Text);
	if (NULL >= Size){ return Ret; }

	HGLOBAL hGlobalMemory = NULL;//记录一个内存 句柄 (全局)，如果说释放，那么将出错 导致无法正常的写入数据

	if (FALSE == OpenClipboard(NULL)){//打开剪切板
		return Ret;
	}
	hGlobalMemory = GlobalAlloc(GHND, Size + 1);//申请一块 全局的 堆栈内存
	if (NULL == (int)hGlobalMemory)
	{
		goto GoToFree;
	}
	void * lpGlobalMemory = GlobalLock(hGlobalMemory);//锁定一块全局内存
	if (NULL == (int)lpGlobalMemory)
	{
		goto GoToFree;
	}
	if (FALSE == EmptyClipboard())
	{//清除剪切板内容
		goto GoToFree;
	}
	memcpy(lpGlobalMemory, Text, Size + 1);//将内容拷贝到 全局堆栈中
	if (NULL == SetClipboardData(CF_TEXT, hGlobalMemory))
	{//将指定内容放到剪切板中，按照指定的格式
		goto GoToFree;
	}
	Ret = true;
GoToFree:
	CloseClipboard();//关闭剪切板
	if (NULL != hGlobalMemory)
	{
		//释放掉 之前申请的内存
		GlobalFree(hGlobalMemory);
	}
	return Ret;
}



void CMy_SymbolsDlg::OnMenuCopyModulePath()
{
	// TODO: 在此添加命令处理程序代码
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
	// TODO: 在此添加命令处理程序代码
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
	// TODO: 在此添加命令处理程序代码

	int index = m_module_list.GetSelectionMark();
	wchar_t buf[MAX_PATH] = {0};
	wchar_t com_line[MAX_PATH] = {0};
	m_module_list.GetItemText(index,type_symbol_path,buf,MAX_PATH);

	char tmp_com_line[MAX_PATH] = {0};
	wcstombs_s(NULL,tmp_com_line,com_line,MAX_PATH);
	ToShearPlate(tmp_com_line);
}



