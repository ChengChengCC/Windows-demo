
// FileRecoverDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "Common.h"
#include "Sector.h"



LPCSTR GetFileSystemString(int nType);
DWORD WINAPI ScanFilesThread(LPVOID		lpParam);
WCHAR* DoFileOpenSave(HWND hwnd, BOOL bSave, LPWSTR lpszFilter, LPWSTR lpszDefExt);

// CFileRecoverDlg 对话框
class CFileRecoverDlg : public CDialogEx
{
// 构造
public:
	CFileRecoverDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_FILERECOVER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


public:
	VOID  CFileRecoverDlg::InitListDriveItem();


	VOID CFileRecoverDlg::InitList();

	void CFileRecoverDlg::InsertListItem_Drive(DRIVEPACKET &stDP);
	void CFileRecoverDlg::CreateScanThread(BOOL bIsCreate, LPVOID lpThreadParameter = FALSE);
	void CFileRecoverDlg::InsertListItem_File(DWORD nFileSeq, int nItem, NTFS_FILEINFO &stfInfo);

	PDRIVEPACKET				m_pDrivePacket;



// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_ListDrive;
	CListCtrl m_ListFile;
	afx_msg void OnBnClickedButtonScan();
	afx_msg void OnBnClickedButtonSave();
	afx_msg void OnLvnColumnclickListFile(NMHDR *pNMHDR, LRESULT *pResult);
};


static int CALLBACK HsCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);