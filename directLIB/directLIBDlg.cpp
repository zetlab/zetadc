
// directLIBDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "directLIB.h"
#include "directLIBDlg.h"
#include "afxdialogex.h"
#include <tchar.h>
#include <cfgmgr32.h> // Для CM_Get_Parent
#include "../ZETADC.cpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
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


// CdirectLIBDlg dialog



CdirectLIBDlg::CdirectLIBDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIRECTLIB_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CdirectLIBDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON1, m_init);
	DDX_Control(pDX, IDC_BUTTON2, m_start);
	DDX_Control(pDX, IDC_BUTTON3, m_save);
	DDX_Control(pDX, IDC_BUTTON4, m_stop);
	DDX_Control(pDX, IDC_EDIT1, m_info);
	DDX_Control(pDX, IDC_EDIT2, m_inf2);
	DDX_Control(pDX, IDC_EDIT3, m_data);
}

BEGIN_MESSAGE_MAP(CdirectLIBDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CdirectLIBDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CdirectLIBDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CdirectLIBDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CdirectLIBDlg::OnBnClickedButton4)
	ON_WM_TIMER()  // Автоматически связывает WM_TIMER с OnTimer()
END_MESSAGE_MAP()


// CdirectLIBDlg message handlers

BOOL CdirectLIBDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CdirectLIBDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CdirectLIBDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CdirectLIBDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CdirectLIBDlg::OnBnClickedButton1()
{
	int num = modules();
	if (num <= 0) return;
	numModule = num;
	int err = 0;
	for (int i = 0; i < num; i++) {
		err = init(i);
		if (err < 0) return;
	}
	TRACE(L"err initADC = %d\n", err);
	std::string forInfo;
	std::string forInf2;
	forInfo.append("NAMEMOD = ");
	forInfo.append(getString(0, 0, NAMEMOD));
	forInfo.append(", TYPEMOD = ");
	forInfo.append(getString(0, 0, TYPEMOD));
	forInfo.append(", SERIALMOD = ");
	forInfo.append(getString(0, 0, SERIALMOD));
	int number = getInt(0, 0, NUMBEROFCHANNELS);
	forInfo.append(",\n NUMBEROFCHANNELS = ");
	forInfo += std::to_string(number);
	for (int i = 0; i < number; i++) {
		if (i != 0) forInf2.append(", ");
		forInf2.append("IDCHANNEL = ");
		forInf2.append(getString(0, i, IDCHANNEL));
		forInf2.append(", NAMECHANNEL = ");
		forInf2.append(getString(0, i, NAMECHANNEL));
		forInf2.append(", COMMENT = ");
		forInf2.append(getString(0, i, COMMENT));
	}
	// Конвертация в wide string
	int len = MultiByteToWideChar(CP_UTF8, 0, forInfo.c_str(), -1, nullptr, 0);
	if (len > 0) {
		std::wstring wInfo(len, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, forInfo.c_str(), -1, &wInfo[0], len);
		m_info.SetWindowTextW(wInfo.c_str());			
	}
	len = MultiByteToWideChar(CP_UTF8, 0, forInf2.c_str(), -1, nullptr, 0);
	if (len > 0) {
		std::wstring wInf2(len, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, forInf2.c_str(), -1, &wInf2[0], len);
		m_inf2.SetWindowTextW(wInf2.c_str());
	}
	SetTimer(1, 1000, NULL);
	return;
}


void CdirectLIBDlg::OnBnClickedButton2()
{
	for (int i = 0; i < numModule; i++) {
		startADC(i);
	}
}


void CdirectLIBDlg::OnBnClickedButton3()
{
	int err = 0;
	err = putXML(0);
}


void CdirectLIBDlg::OnBnClickedButton4()
{
	for (int i = 0; i < numModule; i++) {
		stopADC(i);
	}
}

void CdirectLIBDlg::OnTimer(UINT_PTR nIDEvent) {
	if (nIDEvent == 1) {  // Проверяем ID таймера
		long long ukaz = getPointerADC(0);
		if (ukaz > 0) {
			int number = 100;
			std::string forData;
			int err;
			int numchan = getInt(0, 0, NUMBEROFCHANNELS);
			static float massiv[100];
			for (int ch = 0; ch < numchan; ch++) {
				err = getDataADC(0, massiv, number, ukaz, ch);
				float sum = 0;
				if (err == 0) {
					for (int i = 0; i < number; i++) {
						sum += massiv[i];
					}
					sum = sum / number;
				}
				else {
					sum = (float)err;
				}
				if (ch != 0) forData.append(", ");
				forData.append(getString(0, ch, IDCHANNEL));
				forData.append(" = ");
				forData += std::to_string(sum);
			}
			// Конвертация в wide string
			int len = MultiByteToWideChar(CP_UTF8, 0, forData.c_str(), -1, nullptr, 0);
			if (len > 0) {
				std::wstring wData(len, L'\0');
				MultiByteToWideChar(CP_UTF8, 0, forData.c_str(), -1, &wData[0], len);
				m_data.SetWindowTextW(wData.c_str());
			}

		}
	}
	CDialogEx::OnTimer(nIDEvent);  // Важно: вызываем базовый класс
}
