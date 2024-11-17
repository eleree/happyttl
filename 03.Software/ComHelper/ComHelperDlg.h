
// ComHelperDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"

typedef struct ComMessage {
	DWORD mType;
	CString mString;
	CString mComString;
	TCHAR mMessage[128];
}ComMessage;

// CComHelperDlg �Ի���
class CComHelperDlg : public CDialogEx
{
// ����
public:
	CComHelperDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_COMHELPER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��
	int mTrayId;

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

	int iNeedTimerRefresh = 0;
	afx_msg void OnBnClickedHide();
	afx_msg void OnLvnItemchangedPortevent(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnDeviceChange(UINT nEventType,DWORD dwData);
	CListCtrl mPortEventList;
	CTreeCtrl mPortTreeView;
	CImageList mImageList;
	CButton mStartup; 
	NOTIFYICONDATA mNid;

	afx_msg void OnBnClickedRefresh();
	afx_msg void OnBnClickedDeviceManage();
	void OnRefreshPort(void);
	void OnTimer(UINT_PTR nIDEvent);
	void ShowTrayMenu();
	void CloseNotification(HWND hwnd, UINT uID);
	afx_msg LRESULT OnUserEvent(WPARAM, LPARAM);
	afx_msg LRESULT OnTrayNotification(WPARAM wParam, LPARAM lParam);
	static void PostUserMessage(HWND, ComMessage * );
	afx_msg LRESULT OnTrayIcon(WPARAM wParam, LPARAM lParam);

	afx_msg void OnBnClickedStartup();
};
