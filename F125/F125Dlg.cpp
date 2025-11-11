#include "pch.h"
#include "F125Dlg.h"
#include "resource.h"
#include <string>
#include <ws2tcpip.h>
#include <afxwin.h>
#include <afxdlgs.h> // 用於文件對話框
#include <iostream>
using namespace std;



#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE    65536 
#define UDP_PORT    20777
#define UDP_IP      "0.0.0.0"

#define CAR_DAMAGE_PACKET_LEN       1041    // 29 + 46*22
#define CAR_STATUS_PACKET_LEN       1239    // 29 + 55*22
#define PARTICIPANTS_PACKET_LEN     1284    // 29 + 1 + 57*22

#define LB_TYRES_DAMAGE_INDEX  16
#define RB_TYRES_DAMAGE_INDEX  17
#define LF_TYRES_DAMAGE_INDEX  18
#define RF_TYRES_DAMAGE_INDEX  19

#define FL_WING_DAMAGE_INDEX  28
#define FR_WING_DAMAGE_INDEX  29

#define TYRES_TYPE_INDEX       26
#define TYRES_AGE_LAPS_INDEX   27

#define NAME_INDEX 7


#define TIMER_ID 1001

BEGIN_MESSAGE_MAP(CMainDlg, CDialog)
    ON_WM_TIMER()
//    ON_STN_ENABLE(IDC_STATIC_IMAGE, &CMainDlg::OnStnEnableStaticImage)
END_MESSAGE_MAP()

CMainDlg::CMainDlg(CWnd* pParent)
    : CDialog(IDD, pParent)
    , m_sockfd(INVALID_SOCKET)
    , m_myIndex(0)
    , m_buffer(BUF_SIZE)
{
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_STATIC_NAME, m_staticName);

    DDX_Control(pDX, IDC_STATIC_FLW, m_lblTyreFLW);
    DDX_Control(pDX, IDC_STATIC_FRW, m_lblTyreFRW);

    DDX_Control(pDX, IDC_STATIC_FL, m_lblTyreFL);
    DDX_Control(pDX, IDC_STATIC_FR, m_lblTyreFR);
    DDX_Control(pDX, IDC_STATIC_RL, m_lblTyreRL);
    DDX_Control(pDX, IDC_STATIC_RR, m_lblTyreRR);
    DDX_Control(pDX, IDC_STATIC_AGE, m_lblTyreAge);
    DDX_Control(pDX, IDC_STATIC_TYRE_TYPE, m_lblTyreType);


    DDX_Control(pDX, IDC_STATIC_IMAGE, m_staticImage);
}

BOOL CMainDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    CStatic* pImageCtrl = (CStatic*)GetDlgItem(IDC_STATIC_IMAGE);
    if (pImageCtrl)
    {
        // 從資源載入圖片
        HBITMAP hBitmap = (HBITMAP)LoadImage(
            AfxGetInstanceHandle(),
            MAKEINTRESOURCE(IDB_BITMAP_CAR),
            IMAGE_BITMAP,
            0, 0,
            LR_SHARED
        );

        if (hBitmap)
        {
            // 獲取圖片尺寸
            BITMAP bm;
            GetObject(hBitmap, sizeof(BITMAP), &bm);

            // 設置控制項大小以符合圖片
            CRect rect;
            pImageCtrl->GetWindowRect(&rect);
            ScreenToClient(&rect);
            pImageCtrl->SetWindowPos(
                NULL,
                rect.left,
                rect.top,
                bm.bmWidth,
                bm.bmHeight,
                SWP_NOZORDER
            );

            // 設置圖片
            pImageCtrl->ModifyStyle(0, SS_BITMAP | SS_REALSIZEIMAGE);
            pImageCtrl->SetBitmap(hBitmap);

            // 強制重繪
            pImageCtrl->Invalidate();
            pImageCtrl->UpdateWindow();
        }
        else
        {
            CString errMsg;
            DWORD error = GetLastError();
            errMsg.Format(_T("無法載入圖片！錯誤碼: %d"), error);
            AfxMessageBox(errMsg);
        }
    }

    // 其餘初始化程式碼...
    InitializeSocket();
    SetTimer(TIMER_ID, 10, nullptr);

    return TRUE;
}

void CMainDlg::InitializeSocket()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        MessageBox(_T("WSAStartup failed!"), _T("Error"), MB_ICONERROR);
        return;
    }

    m_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sockfd == INVALID_SOCKET) {
        MessageBox(_T("Error creating socket!"), _T("Error"), MB_ICONERROR);
        WSACleanup();
        return;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    // 設置為廣播地址
    if (InetPtonA(AF_INET, UDP_IP, &server_addr.sin_addr) != 1) {
        MessageBox(_T("IP address conversion failed!"), _T("Error"), MB_ICONERROR);
        CloseSocket();
        return;
    }

    server_addr.sin_port = htons(UDP_PORT);

    if (bind(m_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        int error_code = WSAGetLastError();
        CString error_msg;
        error_msg.Format(_T("Bind failed! Error code: %d"), error_code);
        MessageBox(error_msg, _T("Error"), MB_ICONERROR);
        CloseSocket();
        return;
    }

    // 設置為廣播模式
    int enable_broadcast = 1;
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_BROADCAST, (char*)&enable_broadcast, sizeof(enable_broadcast)) == SOCKET_ERROR) {
        MessageBox(_T("Error enabling broadcast!"), _T("Error"), MB_ICONERROR);
        CloseSocket();
        return;
    }

    // 設置非阻塞模式
    u_long mode = 1;
    ioctlsocket(m_sockfd, FIONBIO, &mode);
}

void CMainDlg::CloseSocket()
{
    if (m_sockfd != INVALID_SOCKET) {
        closesocket(m_sockfd);
        m_sockfd = INVALID_SOCKET;
    }
    WSACleanup();
}

void CMainDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_ID) {
        ProcessUDPData();
    }
    CDialog::OnTimer(nIDEvent);
}

void CMainDlg::ProcessUDPData()
{
    char buffer[BUF_SIZE];
    sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    

    int packet_len = recvfrom(m_sockfd, buffer, BUF_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);

    if (packet_len == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            // 處理錯誤
        }
        return;
    }

    if (packet_len == CAR_DAMAGE_PACKET_LEN) {
        int tyre_index = 29 + 46 * m_myIndex + LB_TYRES_DAMAGE_INDEX;
		int front_wing_index = 29 + 46 * m_myIndex + FL_WING_DAMAGE_INDEX;

        CString strName, strFLW, strFRW, strFL, strFR, strRL, strRR, strAge, strTyreType;

        strName.Format(_T("USER NAME: %s"), user_name);

        strFLW.Format(_T("LW: %d%%"), buffer[front_wing_index]);
        strFRW.Format(_T("RW: %d%%"), buffer[front_wing_index + 1]);

        strAge.Format(_T("Tyre Age: %d laps"), tyre_age);
        strTyreType.Format(_T("Tyre Type: %c"), tyre_type);

        strFL.Format(_T("%d%%"), buffer[tyre_index + 2]);
        strFR.Format(_T("%d%%"), buffer[tyre_index + 3]);
        strRL.Format(_T("%d%%"), buffer[tyre_index]);
        strRR.Format(_T("%d%%"), buffer[tyre_index + 1]);

        m_staticName.SetWindowText(strName);

        m_lblTyreFLW.SetWindowText(strFLW);
        m_lblTyreFRW.SetWindowText(strFRW);

        m_lblTyreFL.SetWindowText(strFL);
        m_lblTyreFR.SetWindowText(strFR);
        m_lblTyreRL.SetWindowText(strRL);
        m_lblTyreRR.SetWindowText(strRR);

        m_lblTyreAge.SetWindowText(strAge);
        m_lblTyreType.SetWindowText(strTyreType);
    }
    else if (packet_len == CAR_STATUS_PACKET_LEN) {
        int tyre_age_index = 29 + 55 * m_myIndex + TYRES_AGE_LAPS_INDEX;
        int tyre_type_index = 29 + 55 * m_myIndex + TYRES_TYPE_INDEX;
        tyre_age = buffer[tyre_age_index];
        //tyre_type = buffer[tyre_type_index];
        
        if (buffer[tyre_type_index] == 16) {
            tyre_type = 'S';
        }
        else if (buffer[tyre_type_index] == 17) {
            tyre_type = 'M';
        }
        else if (buffer[tyre_type_index] == 18) {
            tyre_type = 'H';
        }
        else if (buffer[tyre_type_index] == 7) {
            tyre_type = 'I';
        }
        else if (buffer[tyre_type_index] == 8) {
            tyre_type = 'W';
        }
		else
		tyre_type = buffer[tyre_type_index];
        
    }
    else if (packet_len == PARTICIPANTS_PACKET_LEN) {
        m_myIndex = buffer[29] - 1;
        int name_index = 29 + 1 + 57 * m_myIndex + NAME_INDEX;
        for (int i = 0; i < 32; ++i) {
            user_name[i] = buffer[name_index + i];
        }
        //index_lock = 0;
    }
}

