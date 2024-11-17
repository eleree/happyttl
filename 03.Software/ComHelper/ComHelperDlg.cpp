
// ComHelperDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ComHelper.h"
#include "ComHelperDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "dbt.h"
#include <winuser.h>
#include <afx.h>
#include <shellapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CComHelperDlg 对话框



CComHelperDlg::CComHelperDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_COMHELPER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_USB);
}

void CComHelperDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PORTEVENT, mPortEventList);
	DDX_Control(pDX, IDC_PORTTREE, mPortTreeView);
}

BEGIN_MESSAGE_MAP(CComHelperDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_HIDE, &CComHelperDlg::OnBnClickedHide)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_PORTEVENT, &CComHelperDlg::OnLvnItemchangedPortevent)
	ON_BN_CLICKED(IDC_REFRESH, &CComHelperDlg::OnBnClickedRefresh)
	ON_BN_CLICKED(IDC_DEVICE_MANAGE, &CComHelperDlg::OnBnClickedDeviceManage)
	ON_WM_DEVICECHANGE()
	ON_WM_TIMER()
	ON_MESSAGE(WM_USEREVENT, &CComHelperDlg::OnUserEvent)
	ON_MESSAGE(MYWM_NOTIFYICON, OnTrayNotification)
	ON_MESSAGE(WM_SHOWTASK, OnTrayIcon)
	ON_BN_CLICKED(IDC_STARTUP, &CComHelperDlg::OnBnClickedStartup)
END_MESSAGE_MAP()


// CComHelperDlg 消息处理程序
int iRefreshComPort(HWND hWin);
int iRefreshComPortHid(HWND hWin);
DWORD iGetComPortNumber(void);
TCHAR * iGetComPortName(DWORD dwIndex);
time_t iGetComPortTimestamp(DWORD dwIndex);

CString GetFileNameWithoutExtension(const CString& filePath);
bool IsAppInStartup(const CString& appName);

BOOL CComHelperDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

#ifdef _DEBUG
	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
#endif
	TCHAR exePath[MAX_PATH];

	mStartup.SubclassDlgItem(IDC_STARTUP, this);
	if (GetModuleFileName(NULL, exePath, MAX_PATH) != 0) {
		// Extract executable file name without extension as appName
		CString appName = GetFileNameWithoutExtension(exePath);
		if (IsAppInStartup(appName))		
			mStartup.SetCheck(BST_CHECKED);
		else
			mStartup.SetCheck(BST_UNCHECKED);
	}
	else {
		AfxMessageBox(_T("Failed to retrieve application path."));
	}
	//iReadComContainerID((TCHAR *)"COM18");
	iRefreshComPort(this->GetSafeHwnd());
	// 将“关于...”菜单项添加到系统菜单中。
	mPortEventList.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_AUTOSIZECOLUMNS);
	//mPortEventList.SetExtendedStyle(mPortEventList.GetExtendedStyle() | LVS_EX_AUTOSIZECOLUMNS);

	CString str[4] = {TEXT("空"), TEXT("时间"),TEXT("端口") ,TEXT("事件") };
	for (int i = 0; i < 4; i++)
	{
		//设置表头 参数：索引、内容、对其方式、列宽
		mPortEventList.InsertColumn(i, str[i], LVCFMT_CENTER, LVSCW_AUTOSIZE);
	}
	mPortEventList.DeleteColumn(0);

	mPortEventList.SetColumnWidth(0, 160);
	mPortEventList.SetColumnWidth(1, 80);
	mPortEventList.SetColumnWidth(2, 200);

	HICON hIcon[3];      // 图标句柄数组
	HTREEITEM hRoot;     // 树的根节点的句柄
	HTREEITEM hCataItem; // 可表示任一分类节点的句柄
	HTREEITEM hArtItem;  // 可表示任一文章节点的句柄

	hIcon[0] = theApp.LoadIcon(IDI_PORT);
	hIcon[1] = theApp.LoadIcon(IDI_PORT);
	hIcon[2] = theApp.LoadIcon(IDI_PORT);
	mImageList.Create(16, 16, ILC_COLOR32, 3, 3);
	for (int i = 0; i < 3; i++)
	{
		mImageList.Add(hIcon[i]);
	}
	mPortTreeView.SetImageList(&mImageList, TVSIL_NORMAL);
	//TCHAR * hello = _T("你好");
	for (DWORD i = 0; i < iGetComPortNumber(); i++)
	{
		TCHAR * hello = (TCHAR *)(iGetComPortName(i));
		hRoot = mPortTreeView.InsertItem(hello, 0, 0);
	}
	//hRoot = mPortTreeView.InsertItem(_T("COM13"), 0, 0);

	//hRoot = mPortTreeView.InsertItem(_T("COM12"), 0, 0);
	//hRoot = mPortTreeView.InsertItem(_T("COM11"), 0, 0);
	//hCataItem = mPortTreeView.InsertItem(_T("USB HID"), 1, 1, hRoot, TVI_LAST);
	//hArtItem = mPortTreeView.InsertItem(_T("V/P ID"), 2, 2, hCataItem, TVI_LAST);

	OnRefreshPort();
	ComMessage * comMessage = new ComMessage();
	comMessage->mString.Format(TEXT("%s"), _T("程序启动"));
	comMessage->mType = 0;
	::PostMessage(GetSafeHwnd(), WM_USEREVENT, 0, (LPARAM)comMessage);
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	ShowWindow(SW_MINIMIZE);
	SetTimer(1, 10000, NULL);
	// TODO: 在此添加额外的初始化代码


	ModifyStyle(WS_THICKFRAME, 0);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CComHelperDlg::OnSysCommand(UINT nID, LPARAM lParam)
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
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CComHelperDlg::OnPaint()
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
HCURSOR CComHelperDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CComHelperDlg::OnBnClickedHide()
{
	// TODO: 在此添加控件通知处理程序代码
	memset(&mNid, 0, sizeof(mNid));
    mNid.cbSize = sizeof(NOTIFYICONDATA);
    mNid.hWnd = this->m_hWnd;
    mNid.uID = IDR_MAINFRAME; // 托盘图标的 ID
    mNid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    mNid.uCallbackMessage = MYWM_NOTIFYICON;
    mNid.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    wcscpy_s(mNid.szTip, _T("Your application tip"));

    Shell_NotifyIcon(NIM_ADD, &mNid);

    // 隐藏窗口
    ShowWindow(SW_HIDE);
}


void CComHelperDlg::OnLvnItemchangedPortevent(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


void CComHelperDlg::OnBnClickedRefresh()
{
	ComMessage * comMessage = new ComMessage();
	comMessage->mString.Format(TEXT("%s"), _T("手动刷新"));
	comMessage->mType = 0;
	::PostMessage(GetSafeHwnd(), WM_USEREVENT, 0, (LPARAM)comMessage);
	OnRefreshPort();
}


void CComHelperDlg::OnBnClickedDeviceManage()
{
	// TODO: 在此添加控件通知处理程序代码
	ComMessage * comMessage = new ComMessage();
	comMessage->mString.Format(TEXT("%s"), _T("打开设备管理器"));
	comMessage->mType = 0;
	::PostMessage(GetSafeHwnd(), WM_USEREVENT, 0, (LPARAM)comMessage);
	OnRefreshPort();
	system("devmgmt.msc");
}

void CComHelperDlg::OnRefreshPort(void)
{
	// TODO: 在此添加控件通知处理程序代码
	iRefreshComPort(this->GetSafeHwnd());
	iRefreshComPortHid(this->GetSafeHwnd());
	mPortTreeView.DeleteAllItems();


	//HTREEITEM hRoot;     // 树的根节点的句柄
	//HTREEITEM hCataItem; // 可表示任一分类节点的句柄
	//HTREEITEM hArtItem;  // 可表示任一文章节点的句柄
	CTime tm = CTime::GetCurrentTime();
	time_t currentTime = tm.GetTime();

	for (DWORD i = 0; i < iGetComPortNumber(); i++)
	{
		TCHAR * pcPortName = (TCHAR *)(iGetComPortName(i));
		time_t comTime = iGetComPortTimestamp(i);
		_tprintf(_T("current %I64d, ts %I64d  \r\n"), currentTime, comTime);
		if (currentTime - comTime < 10 )
		{
			CString lable;
			lable.Format(TEXT("[NEW] %s"), pcPortName);
			mPortTreeView.InsertItem(lable, 0, 0);

		}else
			mPortTreeView.InsertItem(pcPortName, 0, 0);
	}

}

BOOL CComHelperDlg::OnDeviceChange(UINT nEventType, DWORD dwData)
{

	ComMessage * comMessage = new ComMessage();
	comMessage->mString.Format(TEXT("事件 %d"), nEventType);
	comMessage->mType = 0;
	::PostMessage(GetSafeHwnd(), WM_USEREVENT, 0, (LPARAM)comMessage);

	switch (nEventType)
	{
	case DBT_DEVNODES_CHANGED:
		//RefreshTree();
		iNeedTimerRefresh = 1;
		OnRefreshPort();
		break;
	}
	return 0;
}

void CComHelperDlg::OnTimer(UINT_PTR nIDEvent)
{

	CTime tm = CTime::GetCurrentTime();
	time_t currentTime = tm.GetTime();
	for (DWORD i = 0; i < iGetComPortNumber(); i++)
	{
		TCHAR * pcPortName = (TCHAR *)(iGetComPortName(i));
		time_t comTime = iGetComPortTimestamp(i);
		//_tprintf(_T("timer refresh current %I64d, ts %I64d  \r\n"), currentTime, comTime);
		if (currentTime - comTime > 10 && currentTime - comTime <= 25)
		{
			OnRefreshPort();
			iNeedTimerRefresh = 0;
			break;
		}
	}

}

LRESULT CComHelperDlg::OnTrayIcon(WPARAM wParam, LPARAM lParam) {
	UINT uID = (UINT)wParam;
	switch (lParam) {
		case WM_LBUTTONDOWN:
			// 处理右键单击事件
			CloseNotification(this->m_hWnd, uID);
		break;
	}
	UINT uMouseMsg = LOWORD(lParam);
	return 0;
}

#define MAX_ICONS 10
NOTIFYICONDATA g_notifyIconData[MAX_ICONS];

void CComHelperDlg::CloseNotification(HWND hwnd, UINT uID) {
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = uID;

	// 使用 NIM_DELETE 参数来删除通知
	Shell_NotifyIcon(NIM_DELETE, &nid);
}

#define TIMER_ID 100
time_t pTimerTimestamp = 0;
void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	_tprintf(_T("Notification timer check\r\n"));
	CTime tm = CTime::GetCurrentTime();
	time_t milliSeconds = tm.GetTime();

	if ((milliSeconds - pTimerTimestamp) <= 20)
	{
		_tprintf(_T("Notification not timeout %lld->%lld \r\n"), milliSeconds, pTimerTimestamp);
		return;
	}
	_tprintf(_T("Notification timer timeout, clear all notifications\r\n"));
	KillTimer(hwnd, TIMER_ID);
	for (int i = 0;i < MAX_ICONS;i++)
	{
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = hwnd;
		nid.uID =  i;
		
		Shell_NotifyIcon(NIM_DELETE, &nid);

	}
}

LRESULT CComHelperDlg::OnUserEvent(WPARAM wParam, LPARAM lParam)
{

	ComMessage * comMessage = (ComMessage *)lParam;

	CTime stTime = CTime::GetCurrentTime();
	CString stTimeString = stTime.Format(_T("%Y-%m-%d %H:%M:%S"));
	mPortEventList.InsertItem(0, stTimeString);
	mPortEventList.SetItemText(0, 1, comMessage->mComString);
	mPortEventList.SetItemText(0, 2, comMessage->mString);

	//for (int i = 0; i < MAX_ICONS; ++i) {
	if (_tcsstr(comMessage->mString, _T("串口插入")) != NULL || _tcsstr(comMessage->mString, _T("串口拔出")) != NULL) {
		int i = mTrayId;
		CloseNotification(this->m_hWnd, mTrayId);
		_tprintf(_T("################### OnUserEvent Notification ID %d %s \r\n"), mTrayId, static_cast<const TCHAR*>(comMessage->mString));

		ZeroMemory(&g_notifyIconData[i], sizeof(NOTIFYICONDATA));
		g_notifyIconData[i].cbSize = sizeof(NOTIFYICONDATA);
		g_notifyIconData[i].hWnd = this->m_hWnd;
		g_notifyIconData[i].uID =  mTrayId;
		g_notifyIconData[i].uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO | NIF_SHOWTIP;
		g_notifyIconData[i].uCallbackMessage = WM_SHOWTASK;
		g_notifyIconData[i].hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME)); // 使用应用程序图标
		wcscpy_s(g_notifyIconData[i].szTip, stTimeString);
		wcscpy_s(g_notifyIconData[i].szInfo, comMessage->mComString);
		wcscpy_s(g_notifyIconData[i].szInfoTitle, comMessage->mString);
		g_notifyIconData[i].dwInfoFlags = NIIF_INFO;
		g_notifyIconData[i].uTimeout = 1000;
		if (Shell_NotifyIcon(NIM_ADD, &g_notifyIconData[i]) == false)
		{
			_tprintf(_T("OnUserEvent Notification ID %d Failed\r\n"), mTrayId);
		}
		else {
			_tprintf(_T("OnUserEvent Notification ID %d Success\r\n"), mTrayId);

		}

		CTime tm = CTime::GetCurrentTime();
		time_t milliSeconds = tm.GetTime();
		pTimerTimestamp = milliSeconds;
		KillTimer(TIMER_ID);
		SetTimer(TIMER_ID, 5000, TimerProc);

	
		mTrayId++;
		mTrayId = mTrayId % MAX_ICONS;
	}
	//}

	free(comMessage);
	return 0;
}

void CComHelperDlg::ShowTrayMenu(void)
{
	CMenu    menu;
	menu.CreatePopupMenu();//声明一个弹出式菜单   

	menu.AppendMenu(MF_STRING, WM_DESTROY, _T("关闭"));

	CPoint pt;
	GetCursorPos(&pt);
	menu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, this);

	HMENU    hmenu = menu.Detach();
	menu.DestroyMenu();

}


LRESULT CComHelperDlg::OnTrayNotification(WPARAM wParam, LPARAM lParam)
{
	UINT uID;
	UINT uMsg;

	uID = (UINT)wParam;
	uMsg = (UINT)lParam;

	if (uID != mNid.uID)
		return 0;

	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		// 单击托盘图标时的处理逻辑
		Shell_NotifyIcon(NIM_DELETE, &mNid); // 从托盘中移除图标
		ShowWindow(SW_RESTORE); // 恢复窗口
		break;

	case WM_RBUTTONUP:
		ShowTrayMenu();
		break;
		// 其他消息处理...

	default:
		break;
	}

	return 0;
}


void CComHelperDlg::PostUserMessage(HWND hWin, ComMessage * mesage)
{
	::PostMessage(hWin, WM_USEREVENT, 0, (LPARAM)mesage);
}

CString GetFileNameWithoutExtension(const CString& filePath) {
	int lastIndex = filePath.ReverseFind('\\');
	if (lastIndex != -1) {
		CString fileName = filePath.Mid(lastIndex + 1);
		int dotIndex = fileName.ReverseFind('.');
		if (dotIndex != -1) {
			return fileName.Left(dotIndex);
		}
		else {
			return fileName;
		}
	}
	return filePath;
}


void AddToStartupWithAdminRights(const CString& appName, const CString& exePath) {
	SHELLEXECUTEINFO shExInfo = { 0 };
	shExInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.lpVerb = _T("runas"); // Request admin rights
	shExInfo.lpFile = _T("cmd.exe"); // Example executable (could be your app)
	CString parameters;
	parameters.Format(_T("/c REG ADD HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run /v \"%s\" /t REG_SZ /d \"%s\" /f"), appName, exePath);
	shExInfo.lpParameters = parameters;
	shExInfo.nShow = SW_SHOWNORMAL;

	if (!ShellExecuteEx(&shExInfo)) {
		DWORD error = GetLastError();
		if (error == ERROR_CANCELLED) {
			// User denied UAC prompt
			AfxMessageBox(_T("Administrator rights required to add to startup. Operation cancelled."));
		}
		else {
			// Handle other errors
			CString errorMessage;
			errorMessage.Format(_T("Failed to add to startup. Error code: %d"), error);
			AfxMessageBox(errorMessage);
		}
	}
}

void RemoveFromStartupWithAdminRights(const CString& appName) {
	SHELLEXECUTEINFO shExInfo = { 0 };
	shExInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.lpVerb = _T("runas"); // Request admin rights
	shExInfo.lpFile = _T("cmd.exe"); // Example executable (could be your app)
	CString parameters;
	parameters.Format(_T("/c REG DELETE HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run /v \"%s\" /f"), appName);
	shExInfo.lpParameters = parameters;
	shExInfo.nShow = SW_SHOWNORMAL;

	if (!ShellExecuteEx(&shExInfo)) {
		DWORD error = GetLastError();
		if (error == ERROR_CANCELLED) {
			// User denied UAC prompt
			AfxMessageBox(_T("Administrator rights required to remove from startup. Operation cancelled."));
		}
		else {
			// Handle other errors
			CString errorMessage;
			errorMessage.Format(_T("Failed to remove from startup. Error code: %d"), error);
			AfxMessageBox(errorMessage);
		}
	}
}

bool IsAppInStartup(const CString& appName) {
	bool isFound = false;

	HKEY hKey;
	// 尝试打开当前用户的启动项注册表键
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		TCHAR szValue[MAX_PATH];
		DWORD dwSize = sizeof(szValue);
		DWORD dwType = 0;

		// 查询是否存在指定的启动项
		if (RegQueryValueEx(hKey, appName, NULL, &dwType, reinterpret_cast<LPBYTE>(szValue), &dwSize) == ERROR_SUCCESS) {
			isFound = true;
		}

		RegCloseKey(hKey);
	}

	return isFound;
}

void CComHelperDlg::OnBnClickedStartup()
{
	TCHAR exePath[MAX_PATH];

	if (mStartup.GetCheck())
	{
		if (GetModuleFileName(NULL, exePath, MAX_PATH) != 0) {
			// Extract executable file name without extension as appName
			CString appName = GetFileNameWithoutExtension(exePath);
			AddToStartupWithAdminRights(appName, exePath);
		}
		else {
			AfxMessageBox(_T("Failed to retrieve application path."));
		}
	}
	else {
		if (GetModuleFileName(NULL, exePath, MAX_PATH) != 0) {
			// Extract executable file name without extension as appName
			CString appName = GetFileNameWithoutExtension(exePath);
			RemoveFromStartupWithAdminRights(appName);
		}
		else {
			AfxMessageBox(_T("Failed to retrieve application path."));
		}
	}
}
