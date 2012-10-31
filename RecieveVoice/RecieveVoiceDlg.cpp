
// RecieveVoiceDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RecieveVoice.h"
#include "RecieveVoiceDlg.h"
#include "afxdialogex.h"
#include <WindowsX.h>

#pragma comment(lib,"winmm.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRecieveVoiceDlg 对话框




CRecieveVoiceDlg::CRecieveVoiceDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRecieveVoiceDlg::IDD, pParent)
	, hWaveOut(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	::InitializeCriticalSection(&m_section);
}

void CRecieveVoiceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CRecieveVoiceDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
//	ON_BN_CLICKED(IDC_BUTTON_RECIEVE, &CRecieveVoiceDlg::OnRecieve)
	ON_MESSAGE(WM_DATACOME, &CRecieveVoiceDlg::OnDataCome)
	ON_MESSAGE(WM_PLAY, &CRecieveVoiceDlg::OnPlay)
	ON_MESSAGE(MM_WOM_DONE, &CRecieveVoiceDlg::OnWomDone)
	ON_WM_HSCROLL()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CRecieveVoiceDlg 消息处理程序

BOOL CRecieveVoiceDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	hwnd = AfxGetMainWnd()->m_hWnd;
	//初始化Socket
	WORD wVersionRequest;
	WSADATA wsadata;
	int nRet;
	wVersionRequest = MAKEWORD(1,1);
	if ( (nRet=WSAStartup(wVersionRequest,&wsadata)) != 0 )
	{
		MessageBox(_T("Cannot found a usable Winsocket DLL!"));
		WSACleanup();
		return FALSE;
	}
	if ( LOBYTE(wVersionRequest)!=1 || HIBYTE(wVersionRequest)!=1 )
	{
		MessageBox(_T("You don't have Winsock or your Winsock version is too low!"));
		WSACleanup();
		return FALSE;
	}
	// 检查设备
	if ( ! waveOutGetNumDevs() )
	{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(_T("找不到语音输出设备!"));
		return FALSE;
	}
	WORD	wBits	= 16;
	WORD	wChannel= 1;
	DWORD	dwSample= 22050;
	// format of voice: 16 bits,22.05KHz,Mono audio
	format.cbSize			= 0;
	format.wBitsPerSample	= wBits;		//采样精度
	format.nChannels		= wChannel;		//单声道，Mono/立体声
	format.nSamplesPerSec	= dwSample;	//采样率
	format.nBlockAlign		= format.nChannels * (format.wBitsPerSample/8);		//每样点的字节数
	format.nAvgBytesPerSec	= format.nSamplesPerSec * format.nBlockAlign;	//数据率		
	format.wFormatTag		= WAVE_FORMAT_PCM;
	//分配空间
	pWaveOut	= (PWAVEHDR)GlobalAllocPtr(GHND|GMEM_SHARE,sizeof(WAVEHDR));
	pOutBuffer1	= (char*)GlobalAllocPtr(GHND|GMEM_SHARE,PCMBUFFER_SIZE);
	pOutBuffer2	= (char*)GlobalAllocPtr(GHND|GMEM_SHARE,PCMBUFFER_SIZE);

	m_bPlayFirst	= TRUE;

	if ( (m_SforVoiceOut=socket(AF_INET,SOCK_DGRAM,0)) == INVALID_SOCKET )
	{
		MessageBox(_T("Socket Error!"));
		WSACleanup();
		return FALSE;
	}
	m_AcceptAddr.sin_family	= AF_INET;
	m_AcceptAddr.sin_port	= htons(6666);
	m_AcceptAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if ( bind(m_SforVoiceOut,(SOCKADDR*)&m_AcceptAddr,sizeof(SOCKADDR)) == SOCKET_ERROR )
	{
		MessageBox(_T("Voice out socket bind error!"));  
		WSACleanup();
		return FALSE;  
	}

	//设置Socket为非阻塞模式
	if ( WSAAsyncSelect(m_SforVoiceOut,hwnd,WM_DATACOME,FD_READ) == SOCKET_ERROR )
	{
		MessageBox(_T("Voice out socket select error!")); 
		WSACleanup();
		return FALSE;  
	}
	//打开放音设备
	if ( (result=waveOutOpen(&hWaveOut,WAVE_MAPPER,(LPWAVEFORMATEX)&format,(DWORD)hwnd,0,CALLBACK_WINDOW)) != MMSYSERR_NOERROR )
	{
		TCHAR szError[100];
		if ( ! waveOutGetErrorText(result, szError, sizeof(szError)))
		{
			MessageBeep(MB_ICONEXCLAMATION);
			MessageBox(szError,_T("Wave Out Error"), MB_ICONEXCLAMATION|MB_OK);
		} 
		else
		{
			MessageBeep(MB_ICONEXCLAMATION);
			MessageBox(_T("Unknown Error!"),_T("Wave Out Error"), MB_ICONEXCLAMATION|MB_OK);
		}
		hWaveOut = NULL;
		return FALSE;
	}

	//设置竟是范围
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_LEFT))->SetRange(0,0xffff);
	DWORD dwVolumn = 0;
	waveOutGetVolume(hWaveOut,&dwVolumn);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_LEFT))->SetPos( (dwVolumn & 0xffff0000)>>16 );

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRecieveVoiceDlg::OnSysCommand(UINT nID, LPARAM lParam)
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
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRecieveVoiceDlg::OnPaint()
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
HCURSOR CRecieveVoiceDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


LRESULT CRecieveVoiceDlg::OnDataCome(WPARAM wParam, LPARAM lParam)
{
	SOCKET socket = (SOCKET)wParam;
	if (WSAGETSELECTERROR(lParam))
	{
		return 0;
	}
	if ( WSAGETSELECTEVENT(lParam) != FD_READ)
	{
		return 0;
	}
	// 正在播放第一个缓冲区，用第二个缓冲区接收
	if (m_bPlayFirst)
	{
		int rLength, lenofAcceptAddr = sizeof(SOCKADDR);
		EnterCriticalSection(&m_section);
		rLength = recvfrom(m_SforVoiceOut,pOutBuffer2,PCMBUFFER_SIZE,0,(SOCKADDR*)&m_AcceptAddr,&lenofAcceptAddr);
		LeaveCriticalSection(&m_section);
		if ( rLength == SOCKET_ERROR ) return 0;
		if ( rLength != PCMBUFFER_SIZE )
		{
			return 0;
		}
		m_bPlayFirst = FALSE;
		::PostMessage(hwnd, WM_PLAY, 0, 0);
		GetDlgItem(IDC_STATIC_INFO)->SetWindowText(_T("收到信息1"));
	}
	// 正在播放第二个缓冲区，用第一个缓冲区接收
	else
	{
		int rLength, lenofAcceptAddr = sizeof(SOCKADDR);
		EnterCriticalSection(&m_section);
		rLength = recvfrom(m_SforVoiceOut,pOutBuffer1,PCMBUFFER_SIZE,0,(SOCKADDR*)&m_AcceptAddr,&lenofAcceptAddr);
		LeaveCriticalSection(&m_section);
		if (rLength == SOCKET_ERROR ) return 0;
		if ( rLength != PCMBUFFER_SIZE )
		{
			return 0;
		}
		m_bPlayFirst = TRUE;
		::PostMessage(hwnd, WM_PLAY, 0, 0);
		GetDlgItem(IDC_STATIC_INFO)->SetWindowText(_T("收到信息2"));
	}
	return 0;
}

LRESULT CRecieveVoiceDlg::OnPlay(WPARAM wParam, LPARAM lParam)
{
	if (m_bPlayFirst)
		pWaveOut->lpData = (LPSTR)pOutBuffer1;
	else
		pWaveOut->lpData = (LPSTR)pOutBuffer2;
	pWaveOut->dwBufferLength	= PCMBUFFER_SIZE;
	pWaveOut->dwBytesRecorded	= 0;
	pWaveOut->dwFlags			= WHDR_BEGINLOOP;
	pWaveOut->dwLoops			= 1;
	pWaveOut->dwUser			= 0;
	pWaveOut->lpNext			= 0;
	pWaveOut->reserved			= 0;

	waveOutPrepareHeader(hWaveOut,pWaveOut,sizeof(WAVEHDR));
	waveOutWrite(hWaveOut,pWaveOut,sizeof(WAVEHDR));

	return 0;
}

LRESULT CRecieveVoiceDlg::OnWomDone(WPARAM wParam, LPARAM lParam)
{
	EnterCriticalSection(&m_section);
	waveOutPrepareHeader(hWaveOut, (LPWAVEHDR)lParam, sizeof(WAVEHDR));
	LeaveCriticalSection(&m_section);

	return 0;
}

void CRecieveVoiceDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	CSliderCtrl* pSlider;
	DWORD nSliderPos;
	if ( hWaveOut!=NULL && pScrollBar->m_hWnd == GetDlgItem(IDC_SLIDER_LEFT)->m_hWnd )
	{
		pSlider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_LEFT);
		nSliderPos = pSlider->GetPos();
		nSliderPos = (nSliderPos << 16);
		nSliderPos |= nSliderPos;
		waveOutSetVolume(hWaveOut, nSliderPos);
	}

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CRecieveVoiceDlg::OnDestroy()
{
	if (m_SforVoiceOut != INVALID_SOCKET)
	{
		closesocket(m_SforVoiceOut);
		m_SforVoiceOut	= INVALID_SOCKET;
	}
	if (hWaveOut != NULL)
	{
		waveOutReset(hWaveOut);
		waveOutClose(hWaveOut);
		hWaveOut	= NULL;
		GlobalFreePtr(pOutBuffer1);
		GlobalFreePtr(pOutBuffer2);
		GlobalFreePtr(pWaveOut);
	}

	CDialogEx::OnDestroy();
}
