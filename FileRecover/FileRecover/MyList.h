#pragma once


// CMyList

class CMyList : public CListCtrl
{
	DECLARE_DYNAMIC(CMyList)

public:
	CMyList();
	virtual ~CMyList();

	//�������ݣ�������������ɫ
	int InsertItem(int nItem,LPCTSTR lpText,COLORREF fontcolor=RGB(0,0,0));

protected:
	DECLARE_MESSAGE_MAP()
};


