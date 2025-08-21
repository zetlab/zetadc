
// exampleLIBDlg.h : header file
//

#pragma once


// CexampleLIBDlg dialog
class CexampleLIBDlg : public CDialogEx
{
// Construction
public:
	CexampleLIBDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EXAMPLELIB_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT_PTR nIDEvent);  // ќбъ€вление обработчика
	DECLARE_MESSAGE_MAP()
public:
	int running = 0;	// поток сбора данных в циклический буфер
	int numModule = 0;

	CEdit m_info;
	CEdit m_inf2;
	CEdit m_data;
	CButton m_init;
	CButton m_start;
	CButton m_save;
	CButton m_stop;
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton3();
};
