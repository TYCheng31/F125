// F125.cpp: 定義應用程式的類別表現方式。

#include "pch.h"
#include "framework.h"
#include "F125.h"
#include "F125Dlg.h"
#include <winsock2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CF125App

BEGIN_MESSAGE_MAP(CF125App, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

// CF125App 建構

CF125App::CF125App()
{
    // 支援重新啟動管理員
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}

// 唯一一個 CF125App 物件
CF125App theApp;

// CF125App 初始化

BOOL CF125App::InitInstance()
{
    // 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        AfxMessageBox(_T("WSAStartup failed!"));
        return FALSE;
    }

    // 初始化通用控制項
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();
    AfxEnableControlContainer();

    // 建立殼層管理員
    CShellManager* pShellManager = new CShellManager;

    // 啟動視覺化管理員
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    // 設定註冊表機碼
    SetRegistryKey(_T("F125 Dashboard"));

    CMainDlg dlg;
    m_pMainWnd = &dlg;
    INT_PTR nResponse = dlg.DoModal();

    // 清理
    if (pShellManager != nullptr)
    {
        delete pShellManager;
    }

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
    ControlBarCleanUp();
#endif

    // 清理 Winsock
    WSACleanup();

    return FALSE;
}

BOOL CF125App::ExitInstance()
{
    return CWinApp::ExitInstance();
}