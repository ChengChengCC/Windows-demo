
// FileRecoverDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"
#include "Common.h"
#include "Sector.h"



LPCSTR GetFileSystemString(int nType);
DWORD WINAPI ScanFilesThread(LPVOID		lpParam);
WCHAR* DoFileOpenSave(HWND hwnd, BOOL bSave, LPWSTR lpszFilter, LPWSTR lpszDefExt);

// CFileRecoverDlg �Ի���
class CFileRecoverDlg : public CDialogEx
{
// ����
public:
	CFileRecoverDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_FILERECOVER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


public:
	VOID  CFileRecoverDlg::InitListDriveItem();


	VOID CFileRecoverDlg::InitList();

	void CFileRecoverDlg::InsertListItem_Drive(DRIVEPACKET &stDP);
	void CFileRecoverDlg::CreateScanThread(BOOL bIsCreate, LPVOID lpThreadParameter = FALSE);
	void CFileRecoverDlg::InsertListItem_File(DWORD nFileSeq, int nItem, NTFS_FILEINFO &stfInfo);

	PDRIVEPACKET				m_pDrivePacket;



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
	CListCtrl m_ListDrive;
	CListCtrl m_ListFile;
	afx_msg void OnBnClickedButtonScan();
	afx_msg void OnBnClickedButtonSave();
	afx_msg void OnLvnColumnclickListFile(NMHDR *pNMHDR, LRESULT *pResult);
};


static int CALLBACK HsCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);