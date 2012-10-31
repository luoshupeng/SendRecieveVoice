
// RecieveVoiceDlg.cpp : ʵ���ļ�
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


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CRecieveVoiceDlg �Ի���




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


// CRecieveVoiceDlg ��Ϣ�������

BOOL CRecieveVoiceDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	hwnd = AfxGetMainWnd()->m_hWnd;
	//��ʼ��Socket
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
	// ����豸
	if ( ! waveOutGetNumDevs() )
	{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(_T("�Ҳ�����������豸!"));
		return FALSE;
	}
	WORD	wBits	= 16;
	WORD	wChannel= 1;
	DWORD	dwSample= 22050;
	// format of voice: 16 bits,22.05KHz,Mono audio
	format.cbSize			= 0;
	format.wBitsPerSample	= wBits;		//��������
	format.nChannels		= wChannel;		//��������Mono/������
	format.nSamplesPerSec	= dwSample;	//������
	format.nBlockAlign		= format.nChannels * (format.wBitsPerSample/8);		//ÿ������ֽ���
	format.nAvgBytesPerSec	= format.nSamplesPerSec * format.nBlockAlign;	//������		
	format.wFormatTag		= WAVE_FORMAT_PCM;
	//����ռ�
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

	//����SocketΪ������ģʽ
	if ( WSAAsyncSelect(m_SforVoiceOut,hwnd,WM_DATACOME,FD_READ) == SOCKET_ERROR )
	{
		MessageBox(_T("Voice out socket select error!")); 
		WSACleanup();
		return FALSE;  
	}
	//�򿪷����豸
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

	//���þ��Ƿ�Χ
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_LEFT))->SetRange(0,0xffff);
	DWORD dwVolumn = 0;
	waveOutGetVolume(hWaveOut,&dwVolumn);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_LEFT))->SetPos( (dwVolumn & 0xffff0000)>>16 );

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CRecieveVoiceDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
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
	// ���ڲ��ŵ�һ�����������õڶ�������������
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
		GetDlgItem(IDC_STATIC_INFO)->SetWindowText(_T("�յ���Ϣ1"));
	}
	// ���ڲ��ŵڶ������������õ�һ������������
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
		GetDlgItem(IDC_STATIC_INFO)->SetWindowText(_T("�յ���Ϣ2"));
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
