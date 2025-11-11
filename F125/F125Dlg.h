#pragma once
#include <afxwin.h>
#include <winsock2.h>
#include "resource.h"
#include<vector>

class CMainDlg : public CDialog
{
public:
    CMainDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_F125_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    DECLARE_MESSAGE_MAP()

private:
    std::vector<char> m_buffer;
    SOCKET m_sockfd;
    int m_myIndex;
    int tyre_age = 0;
    char tyre_type;
    bool index_lock = 1;
    char user_name[32];

	CStatic m_staticName;

	CStatic m_lblTyreFLW;
	CStatic m_lblTyreFRW;

    CStatic m_lblTyreFL;
    CStatic m_lblTyreFR;
    CStatic m_lblTyreRL;
    CStatic m_lblTyreRR;
    CStatic m_lblTyreAge;
	CStatic m_lblTyreType;
    CStatic m_staticImage;

    void InitializeSocket();
    void CloseSocket();
    void ProcessUDPData();
public:
//    afx_msg void OnStnEnableStaticImage();
};