// MyList.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "MyList.h"


// CMyList

IMPLEMENT_DYNAMIC(CMyList, CListCtrl)

CMyList::CMyList()
{

}

CMyList::~CMyList()
{
}

CMap<DWORD , DWORD& , COLORREF , COLORREF&> MapItemColor;
int CMyList::InsertItem(int nItem,LPCTSTR lpText,COLORREF fontcolor)
{
	const int IDX = CListCtrl::InsertItem(nItem, lpText);
	//�ı���ɫ
	DWORD iItem=(DWORD)nItem;
	MapItemColor.SetAt(iItem, fontcolor);
	CListCtrl::RedrawItems(iItem,iItem);
	CListCtrl::Update(iItem);
	return IDX;
}



BEGIN_MESSAGE_MAP(CMyList, CListCtrl)
END_MESSAGE_MAP()



// CMyList ��Ϣ�������


