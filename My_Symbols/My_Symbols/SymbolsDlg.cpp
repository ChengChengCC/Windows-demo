// SymbolsDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "My_Symbols.h"
#include "SymbolsDlg.h"
#include "afxdialogex.h"
#include "My_SymbolsDlg.h"
#include "Symbols.h"
#include <algorithm>
// CSymbolsDlg 对话框


COLUMNSTRUCT g_Column_Symbols[] = 
{
	{	L"索引",		50	},
	{	L"符号名",		200	},
	{	L"符号类型",	100	},
	{	L"符号基址",	150	},
	{	L"模块基址",	150	},
	{	L"符号偏移",	100	},

};

int g_Column_Symbol_Count = 6;
vector<MY_SYMBOL_INFO> g_symbol_all;

extern HANDLE hProcess;

IMPLEMENT_DYNAMIC(CSymbolsDlg, CDialog)

CSymbolsDlg::CSymbolsDlg(MODULE_INFO module_info,CWnd* pParent /*=NULL*/)
	: CDialog(CSymbolsDlg::IDD, pParent)
{
	m_info = module_info;
}

CSymbolsDlg::~CSymbolsDlg()
{
}

void CSymbolsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SYMBOLS, m_symbols_list);

	DDX_Text(pDX, IDC_EDIT1, m_cstr_input);
}


BEGIN_MESSAGE_MAP(CSymbolsDlg, CDialog)
	ON_MESSAGE(MY_SYMBOLGET,ShowSymbols)
	ON_BN_CLICKED(IDC_BUTTON_LOCATE, &CSymbolsDlg::OnBnClickedButtonLocate)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_SYMBOLS, &CSymbolsDlg::OnLvnColumnclickListSymbols)
END_MESSAGE_MAP()


// CSymbolsDlg 消息处理程序

BOOL CSymbolsDlg::InitSymbolsListUI()
{

	m_symbols_list.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_HEADERDRAGDROP);
	for (int i = 0;i<g_Column_Symbol_Count;i++)
	{
		m_symbols_list.InsertColumn(i, g_Column_Symbols[i].szTitle,LVCFMT_CENTER,g_Column_Symbols[i].nWidth);
	}
	return TRUE;
}



BOOL CSymbolsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	InitSymbolsListUI();
	
	_beginthreadex(NULL,0,thread_enmu_symbols,this,0,NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


unsigned CSymbolsDlg::thread_enmu_symbols(void* arg)
{
	CSymbolsDlg* ptr_this = (CSymbolsDlg*)arg;
	MODULE_INFO module_info = ptr_this->m_info;
	LONG_PTR load_handle = NULL;
	if (module_info.bSymbols==false)
	{
		//exit(0);
		return 0;
	}
	load_handle = LoadSymModuleA(module_info.FullPath,(ULONG_PTR)module_info.BaseAddr,module_info.Size);
	if (NULL == load_handle)
	{
		return 0;
	}
	SymEnumSymbols(hProcess,load_handle,NULL,EnumSymCallBack,ptr_this);
	UnLoadSymModule(load_handle);
	return 0;
}



//每枚举一个符号都会调用,pSymInfo有详细信息
BOOL CSymbolsDlg:: EnumSymCallBack(PSYMBOL_INFO ptr_symbol_info,ULONG symbol_size,PVOID user_context)
{
	if (NULL == ptr_symbol_info)
	{ 
		SymCleanup(hProcess);
		return FALSE; 
	}	
	CSymbolsDlg* ptr_this = (CSymbolsDlg*)user_context;
	MY_SYMBOL_INFO symbol_info = {0};
	BOOL    InitTypeError = FALSE;
	wchar_t type_name[32] = {0};
	//typedef struct _SYMBOL_INFOW {
	//	ULONG       SizeOfStruct;	  //结构的大小
	//	ULONG       TypeIndex;        //符号类型指标
	if (NULL == ptr_symbol_info->TypeIndex || FALSE == GetTypeName(ptr_symbol_info->ModBase, ptr_symbol_info->TypeIndex, type_name))
	{
		InitTypeError = TRUE;
	}

	//	ULONG64     Reserved[2];	  //此成员是保留给系统使用。
	//	ULONG       Index;			  //对于符号的独特价值。与符号相关联的值是不能保证每次运行的过程是相同的。

	symbol_info.index = ptr_symbol_info->Index;

	//	ULONG       Size;			  //符号的大小，以字节为单位。这个值是有意义的只有模块的符号从PDB文件；否则，该值通常为零，应该是可以忽略的。
	//	ULONG64     ModBase;          //这个符号模块的基地址

	//	ULONG       Flags;			  //包含符号的模块的基地址。
	//	ULONG64     Value;            //符号值，valuepresent应该是1
	//	ULONG64     Address;          //符号包括模块的基地址

	symbol_info.symbol_addr = ptr_symbol_info->Address;
	symbol_info.module_addr = ptr_symbol_info->ModBase;
	//	ULONG       Register;         //寄存器保持值或指针值 
	//	ULONG       Scope;            //符号的范围
	//	ULONG       Tag;              //pdb 分类 
	//	ULONG       NameLen;          //名字的长度,不包括空终止字符。 
	//	ULONG       MaxNameLen;		  //名称的字符缓冲区的大小。如果这个成员是0名成员，不使用。
	//	WCHAR       Name[1];          //名称符号
	//} SYMBOL_INFOW, *PSYMBOL_INFOW;
	CopyMemory(symbol_info.name,ptr_symbol_info->Name,ptr_symbol_info->NameLen);

	if (TRUE == InitTypeError){
		if (NULL != strstr(symbol_info.name, "::")){
			
			wcscpy_s(symbol_info.type,L"Class Function");
		}
		else{
			
			wcscpy_s(symbol_info.type,L"Function");
		}
	}
	

	//更新最后的偏移
	symbol_info.offset = symbol_info.symbol_addr-symbol_info.module_addr;
	g_symbol_all.push_back(symbol_info);
	ptr_this->SendMessage(MY_SYMBOLGET,(WPARAM)&symbol_info,NULL);

	return TRUE;
}


//展示符号名称和 符号类型 
LRESULT CSymbolsDlg::ShowSymbols(WPARAM wParam , LPARAM lParam)
{
	
	PMY_SYMBOL_INFO ptr_symbol_info = (PMY_SYMBOL_INFO)wParam;
	CString cstr_index,cstr_name,cstr_type,cstr_addr,cstr_mod_base,cstr_offset;
	if (NULL==ptr_symbol_info)
	{
		return 0;
	}
	cstr_index.Format(L"%d",ptr_symbol_info->index);
	cstr_name = ptr_symbol_info->name;
	cstr_type = ptr_symbol_info->type; 
	cstr_addr.Format(L"0x%08p",ptr_symbol_info->symbol_addr);
	cstr_mod_base.Format(L"0x%08p",ptr_symbol_info->module_addr);
	cstr_offset.Format(L"0x%x",ptr_symbol_info->offset);

	int n = m_symbols_list.InsertItem(m_symbols_list.GetItemCount(),cstr_index);
	m_symbols_list.SetItemText(n, 1, cstr_name);
	m_symbols_list.SetItemText(n, 2, cstr_type);
	m_symbols_list.SetItemText(n, 3, cstr_addr);
	m_symbols_list.SetItemText(n, 4, cstr_mod_base);
	m_symbols_list.SetItemText(n,5,cstr_offset);
	return 0;
}

void CSymbolsDlg::OnBnClickedButtonLocate()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	CString cstr_tmp;
	int index  = 0;
	for (vector<MY_SYMBOL_INFO>::iterator itor = g_symbol_all.begin();
		itor!=g_symbol_all.end();itor++)
	{
		cstr_tmp = itor->name;
		if (!m_cstr_input.CompareNoCase(cstr_tmp))
		{
			m_symbols_list.SetItemState(index,LVIS_SELECTED,LVIS_SELECTED);
			break;
		}	
		index++;
	}
	m_symbols_list.SetFocus(); //很重要，不然显示不了，网上找的都缺这句代码
	m_symbols_list.SetItemState(index,  LVNI_FOCUSED | LVNI_SELECTED, LVNI_SELECTED | LVNI_FOCUSED);
	m_symbols_list.EnsureVisible(index, FALSE);
}


void CSymbolsDlg::OnLvnColumnclickListSymbols(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
    //进行排序
	int sort  = pNMLV->iSubItem;
	Sort((SORT_TYPE)sort);
	*pResult = 0;
}


bool cmp_name(MY_SYMBOL_INFO & sym_info1,MY_SYMBOL_INFO & sym_info2)
{
	return *(char*)sym_info1.name < *(char*)sym_info2.name;
}

bool cmp_index(MY_SYMBOL_INFO & sym_info1,MY_SYMBOL_INFO & sym_info2)
{

	return sym_info1.index < sym_info2.index;
}

bool cmp_type(MY_SYMBOL_INFO & sym_info1,MY_SYMBOL_INFO & sym_info2)
{

	return *(wchar_t*)sym_info1.type < *(wchar_t*)sym_info2.type;
}


bool cmp_addr(MY_SYMBOL_INFO & sym_info1,MY_SYMBOL_INFO & sym_info2)
{

	return sym_info1.symbol_addr < sym_info2.symbol_addr;
}


bool cmp_mod_base(MY_SYMBOL_INFO & sym_info1,MY_SYMBOL_INFO & sym_info2)
{

	return sym_info1.module_addr < sym_info2.module_addr;
}


bool cmp_offset(MY_SYMBOL_INFO & sym_info1,MY_SYMBOL_INFO & sym_info2)
{

	return sym_info1.offset < sym_info2.offset;
}

BOOL CSymbolsDlg::Sort(SORT_TYPE type)
{
	switch(type)
	{
	case sort_index:
		std::sort(g_symbol_all.begin(),g_symbol_all.end(),cmp_index);
		break;
	case sort_name:
		std::sort(g_symbol_all.begin(),g_symbol_all.end(),cmp_name);		
		break;
	case sort_type:
		std::sort(g_symbol_all.begin(),g_symbol_all.end(),cmp_type);
		break;
	case sort_addr:
		std::sort(g_symbol_all.begin(),g_symbol_all.end(),cmp_addr);
		break;
	case sort_mod_base:
		std::sort(g_symbol_all.begin(),g_symbol_all.end(),cmp_mod_base);
		break;
	case sort_offset:
		std::sort(g_symbol_all.begin(),g_symbol_all.end(),cmp_offset);
		break;
	}

	update_symbols_list();
	return TRUE;

}



void CSymbolsDlg::update_symbols_list()
{
	m_symbols_list.DeleteAllItems();
	CString cstr_index,cstr_name,cstr_type,cstr_addr,cstr_mod_base,cstr_offset;
	for (vector<MY_SYMBOL_INFO>::iterator itor = g_symbol_all.begin();
		itor!=g_symbol_all.end();itor++)
	{
		
		cstr_index.Format(L"%d",itor->index);
		cstr_name = itor->name;
		cstr_type = itor->type;
#ifdef _WIN64
		cstr_addr.Format(L"0x%08p",itor->symbol_addr);
		cstr_mod_base.Format(L"0x%08p",itor->module_addr);
#else
		//x86
#endif
		cstr_offset.Format(L"0x%x",itor->offset);

		int n = m_symbols_list.InsertItem(m_symbols_list.GetItemCount(),cstr_index);
		m_symbols_list.SetItemText(n, 1, cstr_name);
		m_symbols_list.SetItemText(n, 2, cstr_type);
		m_symbols_list.SetItemText(n, 3, cstr_addr);
		m_symbols_list.SetItemText(n, 4, cstr_mod_base);
		m_symbols_list.SetItemText(n,5,cstr_offset);
	}

}

