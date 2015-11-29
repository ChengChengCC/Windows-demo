#pragma once
#include "afxcmn.h"
#include "ModuleList.h"
#include <DbgHelp.h>

// CSymbolsDlg 对话框

#define MY_SYMBOLGET  WM_USER+101

enum SORT_TYPE
{
	sort_index,
	sort_name,
	sort_type,
	sort_addr,
	sort_mod_base,
	sort_offset
};

class CSymbolsDlg : public CDialog
{
	DECLARE_DYNAMIC(CSymbolsDlg)

public:
	CSymbolsDlg(MODULE_INFO module_info,CWnd* pParent = NULL);   // 标准构造函数
	static unsigned _stdcall thread_enmu_symbols(void*);
	virtual ~CSymbolsDlg();
	BOOL InitSymbolsListUI();
	BOOL Sort(SORT_TYPE sort);
	void update_symbols_list();
	static BOOL __stdcall EnumSymCallBack(PSYMBOL_INFO ptr_symbol_info,ULONG symbol_size,PVOID user_context);
// 对话框数据
	enum { IDD = IDD_DIALOG_SYMBOLS };
	MODULE_INFO m_info;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_symbols_list;
	virtual BOOL OnInitDialog();
	afx_msg LRESULT ShowSymbols(WPARAM wParam , LPARAM lParam);
	afx_msg void OnBnClickedButtonLocate();
	afx_msg void OnLvnColumnclickListSymbols(NMHDR *pNMHDR, LRESULT *pResult);
	
	CString m_cstr_input;
};


