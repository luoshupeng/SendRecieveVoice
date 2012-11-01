
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
	, m_hWavein(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	::InitializeCriticalSection(&m_section);

	m_SforVoiceIn = INVALID_SOCKET;
	memset(&m_SendAddr,0,sizeof(SOCKADDR_IN));
	m_nSendAddrLen	= sizeof(SOCKADDR_IN);
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
	ON_MESSAGE(MM_WIM_DATA, &CRecieveVoiceDlg::OnRecordFull)
	ON_MESSAGE(MM_MIXM_CONTROL_CHANGE, &CRecieveVoiceDlg::OnVolumeChanged)
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
	if ( ! waveInGetNumDevs() )
	{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(_T("�Ҳ������������豸!"));
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

	//Receive Voice Socket for waveOut
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


	//Send Voice Socket for waveIn
	if ( (m_SforVoiceIn=socket(AF_INET,SOCK_DGRAM,0)) == INVALID_SOCKET )
	{
		MessageBox(_T("waveIn Socket Error!"));
		WSACleanup();
		return FALSE;
	}
	// Open wave input device
	wBits	= 16;
	wChannel= 1;
	dwSample= 22050;
	// format of voice: 16 bits,22.05KHz,Mono audio
	format.cbSize			= 0;
	format.wBitsPerSample	= wBits;		//��������
	format.nChannels		= wChannel;		//��������Mono/������
	format.nSamplesPerSec	= dwSample;	//������
	format.nBlockAlign		= format.nChannels * (format.wBitsPerSample/8);		//ÿ������ֽ���
	format.nAvgBytesPerSec	= format.nSamplesPerSec * format.nBlockAlign;	//������		
	format.wFormatTag		= WAVE_FORMAT_PCM;
	result = waveInOpen(&m_hWavein, WAVE_MAPPER, (LPWAVEFORMATEX)&format,(DWORD)hwnd,0,CALLBACK_WINDOW);
	if ( result != MMSYSERR_NOERROR )
	{
		TCHAR szError[100];
		if ( ! waveOutGetErrorText(result, szError, sizeof(szError)))
		{
			MessageBeep(MB_ICONEXCLAMATION);
			MessageBox(szError,_T("Wave In Error"), MB_ICONEXCLAMATION|MB_OK);
		} 
		else
		{
			MessageBeep(MB_ICONEXCLAMATION);
			MessageBox(_T("Unknown Error!"),_T("Wave In Error"), MB_ICONEXCLAMATION|MB_OK);
		}
		m_hWavein = NULL;
		return FALSE;
	}
	m_lpHdrIn1	= (LPWAVEHDR)GlobalAllocPtr(GHND|GMEM_SHARE,sizeof(WAVEHDR));
	m_lpHdrIn2	= (LPWAVEHDR)GlobalAllocPtr(GHND|GMEM_SHARE,sizeof(WAVEHDR));
	m_pBufIn1	= (char*)GlobalAllocPtr(GHND|GMEM_SHARE,PCMBUFFER_SIZE);
	m_pBufIn2	= (char*)GlobalAllocPtr(GHND|GMEM_SHARE,PCMBUFFER_SIZE);
	m_lpHdrIn1->dwBufferLength	= PCMBUFFER_SIZE;
	m_lpHdrIn1->dwBytesRecorded	= 0;
	m_lpHdrIn1->dwFlags			= 0;
	m_lpHdrIn1->dwLoops			= 0;
	m_lpHdrIn1->dwUser			= 0;
	m_lpHdrIn1->lpData			= (LPSTR)m_pBufIn1;
	m_lpHdrIn1->lpNext			= NULL;
	m_lpHdrIn1->reserved		= 0;
	waveInPrepareHeader(m_hWavein,m_lpHdrIn1,sizeof(WAVEHDR));
	waveInAddBuffer(m_hWavein,m_lpHdrIn1,sizeof(WAVEHDR));
	m_lpHdrIn2->dwBufferLength	= PCMBUFFER_SIZE;
	m_lpHdrIn2->dwBytesRecorded	= 0;
	m_lpHdrIn2->dwFlags			= 0;
	m_lpHdrIn2->dwLoops			= 0;
	m_lpHdrIn2->dwUser			= 0;
	m_lpHdrIn2->lpData			= (LPSTR)m_pBufIn2;
	m_lpHdrIn2->lpNext			= NULL;
	m_lpHdrIn2->reserved		= 0;
	waveInPrepareHeader(m_hWavein,m_lpHdrIn2,sizeof(WAVEHDR));
	waveInAddBuffer(m_hWavein,m_lpHdrIn2,sizeof(WAVEHDR));
	if ( (result=waveInStart(m_hWavein)) != MMSYSERR_NOERROR )
	{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(_T("WaveIn Start Failed!"));
		m_hWavein = NULL;
		return FALSE;
	}

	//����������Χ
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_WAVEOUT))->SetRange(0,0xffff);
	DWORD dwVolumn = 0;
	waveOutGetVolume(hWaveOut,&dwVolumn);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_WAVEOUT))->SetPos( (dwVolumn & 0xffff0000)>>16 );
	CString str;
	str.Format(_T("%0.0f%%"),(double)((dwVolumn & 0xffff0000)>>16) / (double)0xffff * 100.0);
	GetDlgItem(IDC_STATIC_OUT)->SetWindowTextW(str);
	//waveIn Volume
	DWORD dwMin = 0;
	DWORD dwMax	= 0;
	MixerInit(dwMin, dwMax);
	if (m_hMixer != NULL)
	{
		((CSliderCtrl*)GetDlgItem(IDC_SLIDER_WAVEIN))->SetRange(dwMin,dwMax);
		DWORD dwvolume	= GetVolume(m_hMixer,m_dwControlID);
		((CSliderCtrl*)GetDlgItem(IDC_SLIDER_WAVEIN))->SetPos(dwvolume);
		double nPer = (double)dwvolume / (double)dwMax * 100.0;
		CString str;
		str.Format(_T("%0.0f%%"),nPer);
		GetDlgItem(IDC_STATIC_VOLUME)->SetWindowText(str);
	}	

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
	int rLength, lenofAcceptAddr = sizeof(SOCKADDR);
	// ���ڲ��ŵ�һ�����������õڶ�������������
	if (m_bPlayFirst)
	{		
		EnterCriticalSection(&m_section);
		rLength = recvfrom(m_SforVoiceOut,pOutBuffer2,PCMBUFFER_SIZE,0,(SOCKADDR*)&m_AcceptAddr,&lenofAcceptAddr);
		LeaveCriticalSection(&m_section);
		if ( rLength == SOCKET_ERROR ) return 0;
		if ( rLength != PCMBUFFER_SIZE )
		{
			return 0;
		}
		m_SendAddr = m_AcceptAddr;
		m_SendAddr.sin_port = htons(6666);
		m_bPlayFirst = FALSE;
		::PostMessage(hwnd, WM_PLAY, 0, 0);
		GetDlgItem(IDC_STATIC_INFO)->SetWindowText(_T("�յ���Ϣ1"));
	}
	// ���ڲ��ŵڶ������������õ�һ������������
	else
	{
		EnterCriticalSection(&m_section);
		rLength = recvfrom(m_SforVoiceOut,pOutBuffer1,PCMBUFFER_SIZE,0,(SOCKADDR*)&m_AcceptAddr,&lenofAcceptAddr);
		LeaveCriticalSection(&m_section);
		if (rLength == SOCKET_ERROR ) return 0;
		if ( rLength != PCMBUFFER_SIZE )
		{
			return 0;
		}
		m_SendAddr = m_AcceptAddr;
		m_SendAddr.sin_port = htons(6666);
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
	if ( hWaveOut!=NULL && pScrollBar->m_hWnd == GetDlgItem(IDC_SLIDER_WAVEOUT)->m_hWnd )
	{
		pSlider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_WAVEOUT);
		nSliderPos = pSlider->GetPos();
		nSliderPos = (nSliderPos << 16);
		nSliderPos |= nSliderPos;
		waveOutSetVolume(hWaveOut, nSliderPos);
		CString str;
		str.Format(_T("%0.0f%%"),(double)((nSliderPos & 0xffff0000)>>16) / (double)0xffff * 100.0);
		GetDlgItem(IDC_STATIC_OUT)->SetWindowTextW(str);
	}
	if (m_hWavein!=NULL && pScrollBar->m_hWnd==GetDlgItem(IDC_SLIDER_WAVEIN)->m_hWnd)
	{
		if (m_hMixer != NULL)
		{
			CSliderCtrl* pSlider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_WAVEIN);
			DWORD nSliderPos = pSlider->GetPos();
			SetVolume(m_hMixer,m_dwControlID,nSliderPos);
			DWORD dwMax	= pSlider->GetRangeMax();
			double nPer = (double)nSliderPos / (double)dwMax * 100.0;
			CString str;
			str.Format(_T("%0.0f%%"),nPer);
			GetDlgItem(IDC_STATIC_VOLUME)->SetWindowText(str);
		}
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
	if (m_SforVoiceIn != INVALID_SOCKET)
	{
		closesocket(m_SforVoiceIn);
		m_SforVoiceIn	= INVALID_SOCKET;
	}
	if (m_hWavein != NULL)
	{
		waveInReset(m_hWavein);
		waveInClose(m_hWavein);
		m_hWavein	= NULL;
		GlobalFreePtr(m_lpHdrIn1);
		GlobalFreePtr(m_lpHdrIn2);
		GlobalFreePtr(m_pBufIn1);
		GlobalFreePtr(m_pBufIn2);
	}

	CDialogEx::OnDestroy();
}

LRESULT CRecieveVoiceDlg::OnRecordFull(WPARAM wParam, LPARAM lParam)
{
	char* recordBuf	= (char*)((LPWAVEHDR)lParam)->lpData;
	int recordLen		= ((LPWAVEHDR)lParam)->dwBytesRecorded;
	if ( recordBuf == NULL )
	{
		waveInPrepareHeader(m_hWavein, (LPWAVEHDR)lParam, sizeof(WAVEHDR));
		waveInAddBuffer(m_hWavein,(LPWAVEHDR)lParam, sizeof(WAVEHDR));
		waveInReset(m_hWavein);
		return 0;
	}

	if (m_SendAddr.sin_port == 0)
	{
		waveInPrepareHeader(m_hWavein, (LPWAVEHDR)lParam, sizeof(WAVEHDR));
		waveInAddBuffer(m_hWavein,(LPWAVEHDR)lParam, sizeof(WAVEHDR));
		return 0;
	}

	sendto(m_SforVoiceIn, recordBuf, recordLen,0,(SOCKADDR*)&m_SendAddr,m_nSendAddrLen);

	waveInPrepareHeader(m_hWavein, (LPWAVEHDR)lParam, sizeof(WAVEHDR));
	waveInAddBuffer(m_hWavein,(LPWAVEHDR)lParam, sizeof(WAVEHDR));

	return 0;
}

BOOL CRecieveVoiceDlg::MixerInit(DWORD& dwMin, DWORD& dwMax)
{
	MMRESULT rc;
	UINT mixerID;
	rc = mixerGetID((HMIXEROBJ)m_hWavein,&mixerID,MIXER_OBJECTF_HWAVEIN);
	rc = mixerOpen(&m_hMixer,mixerID,(DWORD)this->m_hWnd,0,CALLBACK_WINDOW);
	if (rc != MMSYSERR_NOERROR)
	{
		MessageBox(_T("mixerOpen Error!"));
		return FALSE;
	}
	// mixerGetLineInfo
	MIXERLINE mixerLine;
	ZeroMemory(&mixerLine,sizeof(MIXERLINE));
	mixerLine.cbStruct	= sizeof(MIXERLINE);
	mixerLine.dwComponentType	= MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
	rc = mixerGetLineInfo((HMIXEROBJ)m_hMixer, &mixerLine,MIXER_GETLINEINFOF_COMPONENTTYPE);
	if (rc != MMSYSERR_NOERROR)
	{
		MessageBox(_T("mixerGetLineInfo Error!"));
		return FALSE;
	}

	// ��ȡ��˷��LineID
	DWORD dwConnections = mixerLine.cConnections;
	DWORD dwLineID = -1;
	for (DWORD i=0; i<dwConnections; i++)
	{
		mixerLine.dwSource = i;
		rc = mixerGetLineInfo((HMIXEROBJ)m_hMixer,&mixerLine,MIXER_OBJECTF_HMIXER|MIXER_GETLINEINFOF_SOURCE);
		if ( rc == MMSYSERR_NOERROR )
		{
			if (mixerLine.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
			{
				dwLineID = mixerLine.dwLineID;
				break;
			}
		}
	}
	if (dwLineID == -1)
	{
		MessageBox(_T("have not found microphone!"));
		return FALSE;
	}
	// mixerGetLineControls
	MIXERCONTROL mxControl;
	MIXERLINECONTROLS mxLineControls;	
	ZeroMemory(&mxLineControls,sizeof(mxLineControls));
	mxLineControls.cbStruct		= sizeof(mxLineControls);
	mxLineControls.dwLineID		= dwLineID;
	mxLineControls.dwControlType= MIXERCONTROL_CONTROLTYPE_VOLUME;
	mxLineControls.cControls	= 1;
	mxLineControls.cbmxctrl		= sizeof(mxControl);
	mxLineControls.pamxctrl		= &mxControl;
	ZeroMemory(&mxControl,sizeof(mxControl));
	mxControl.cbStruct	= sizeof(mxControl);
	rc = mixerGetLineControls((HMIXEROBJ)m_hMixer,&mxLineControls,MIXER_GETLINECONTROLSF_ONEBYTYPE);
	if (rc != MMSYSERR_NOERROR)
	{
		MessageBox(_T("mixerGetLineControls Error!"));
		return FALSE;
	}
	m_dwControlID	= mxControl.dwControlID;
	dwMin	= mxControl.Bounds.lMinimum;
	dwMax	= mxControl.Bounds.lMaximum;

	return TRUE;
}

// ��ȡ����
DWORD CRecieveVoiceDlg::GetVolume(HMIXER hMixer, DWORD dwControlID)
{
	MIXERCONTROLDETAILS mxControlDetails;
	MIXERCONTROLDETAILS_SIGNED volStruct;
	ZeroMemory(&mxControlDetails,sizeof(mxControlDetails));
	mxControlDetails.cbStruct	= sizeof(mxControlDetails);
	mxControlDetails.dwControlID= dwControlID;
	mxControlDetails.cChannels	= 1;
	mxControlDetails.cbDetails	= sizeof(volStruct);
	mxControlDetails.paDetails	= &volStruct;
	MMRESULT rc = mixerGetControlDetails((HMIXEROBJ)hMixer,&mxControlDetails,MIXER_GETCONTROLDETAILSF_VALUE);
	if (rc != MMSYSERR_NOERROR)
	{
		MessageBox(_T("mixerGetControlDetails Error!"));
		return 0;
	}
	return (DWORD)volStruct.lValue;
}


// ��������
void CRecieveVoiceDlg::SetVolume(HMIXER hMixer, DWORD dwControlID, DWORD dwVolume)
{
	MIXERCONTROLDETAILS mxControlDetails;
	MIXERCONTROLDETAILS_SIGNED volStruct;
	ZeroMemory(&mxControlDetails,sizeof(mxControlDetails));
	mxControlDetails.cbStruct	= sizeof(mxControlDetails);
	mxControlDetails.dwControlID= dwControlID;
	mxControlDetails.cChannels	= 1;
	mxControlDetails.cbDetails	= sizeof(volStruct);
	mxControlDetails.paDetails	= &volStruct;
	volStruct.lValue			= dwVolume;
	MMRESULT rc = mixerSetControlDetails((HMIXEROBJ)hMixer,&mxControlDetails,MIXER_GETCONTROLDETAILSF_VALUE);
	if (rc != MMSYSERR_NOERROR)
	{
		MessageBox(_T("mixerGetControlDetails Error!"));
		return;
	}
}

LRESULT CRecieveVoiceDlg::OnVolumeChanged(WPARAM wParam, LPARAM lParam)
{
	CSliderCtrl* pSlider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_WAVEIN);
	HMIXER hMixer = (HMIXER)wParam;
	DWORD dwControlID = (DWORD)lParam;

	DWORD dwVolume	= GetVolume(hMixer,dwControlID);

	pSlider->SetPos(dwVolume);

	DWORD dwMax	= pSlider->GetRangeMax();
	double nPer = (double)dwVolume / (double)dwMax * 100.0;
	CString str;
	str.Format(_T("%0.0f%%"),nPer);
	GetDlgItem(IDC_STATIC_VOLUME)->SetWindowText(str);

	return 0;
}
