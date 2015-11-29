
// My_SymbolsDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"

#include "resource.h"
#include <iostream>
#include <vector>
#include "ModuleList.h"
using namespace std;
#define  MY_GETALLMODULE   WM_USER+100


enum TYPE
{
	type_index,
	type_name,
	type_addr,
	type_size,
	type_bsymbol,
	type_symbol_path,
	type_module_path
};

// CMy_SymbolsDlg �Ի���
class CMy_SymbolsDlg : public CDialogEx
{
// ����
public:
	CMy_SymbolsDlg(CWnd* pParent = NULL);	// ��׼���캯��

	
	BOOL InitSymbolsPath();
	BOOL InitModuleList();
	BOOL Sort(TYPE);
	void update_module_list();
// �Ի�������
	enum { IDD = IDD_MY_SYMBOLS_DIALOG };
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_module_list;
	afx_msg void OnMenuRefresh();
	afx_msg	LRESULT	GetAllModuleFinished(WPARAM, LPARAM);
	afx_msg void OnNMRClickListModule(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMenuEnumSymbols();
	afx_msg void OnClose();
	afx_msg void OnLvnColumnclickListModule(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMenuLocateModuleFile();
	afx_msg void OnMenuLocateSymbolsFile();
	afx_msg void OnMenuCopyModulePath();
	afx_msg void OnMenuCopyModuleName();
	afx_msg void OnMenuCopySymbolsPath();
private:
	
};


typedef struct _COLUMNSTRUCT
{
	WCHAR*	szTitle;
	UINT    nWidth;
}COLUMNSTRUCT;
