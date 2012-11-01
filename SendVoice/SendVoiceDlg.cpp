
// SendVoiceDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SendVoice.h"
#include "SendVoiceDlg.h"
#include "afxdialogex.h"
#include <WindowsX.h>

#pragma comment(lib, "winmm.lib")

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


// CSendVoiceDlg 对话框




CSendVoiceDlg::CSendVoiceDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSendVoiceDlg::IDD, pParent)
	, m_bSaveFile(FALSE)
	, m_sSaveFile(_T(""))
	, hWaveIn(NULL)
	, m_hMixer(NULL)
	, m_hWaveOut(NULL)
	, m_bPlayFirst(TRUE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_dataBuf	= NULL;
	m_dataLen	= 0;
	hWaveIn		= NULL;
	m_nAcceptAddrLen	= sizeof(SOCKADDR_IN);
}

void CSendVoiceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSendVoiceDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_BEGIN, &CSendVoiceDlg::OnSend)
	ON_MESSAGE(MM_WIM_DATA, &CSendVoiceDlg::OnRecordFull)
	ON_MESSAGE(MM_WIM_CLOSE, &CSendVoiceDlg::OnRecordClose)
	ON_BN_CLICKED(IDC_CHECK_ALL, &CSendVoiceDlg::OnCheckAll)
	ON_BN_CLICKED(IDC_CHECK_SAVEFILE, &CSendVoiceDlg::OnCheckSaveFile)
	ON_BN_CLICKED(IDC_BUTTON_BROWSER, &CSendVoiceDlg::OnButtonBrowser)
	ON_BN_CLICKED(IDC_CHECK_T1, &CSendVoiceDlg::OnCheckT1)
	ON_BN_CLICKED(IDC_CHECK_T2, &CSendVoiceDlg::OnCheckT2)
	ON_WM_HSCROLL()
	ON_MESSAGE(MM_MIXM_CONTROL_CHANGE, &CSendVoiceDlg::OnVolumeChanged)
	ON_MESSAGE(WM_DATACOME, &CSendVoiceDlg::OnDataCome)
	ON_MESSAGE(WM_PLAY, &CSendVoiceDlg::OnPlay)
END_MESSAGE_MAP()


// CSendVoiceDlg 消息处理程序

BOOL CSendVoiceDlg::OnInitDialog()
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
	// 初始化Socket
	WORD	wVersionRequest;
	WSADATA wsadata;
	int		nRet;
	wVersionRequest	= MAKEWORD(1,1);
	nRet = WSAStartup(wVersionRequest, &wsadata);
	if ( nRet != 0 )
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
	if ( (m_SforVoiceIn=socket(AF_INET,SOCK_DGRAM,0)) == INVALID_SOCKET )
	{
		MessageBox(_T("Socket Error!"));
		WSACleanup();
		return FALSE;
	}

	hwnd = AfxGetMainWnd()->m_hWnd;

	// 检查设备
	if ( ! waveInGetNumDevs() )
	{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(_T("找不到语音输入设备!"));
		return FALSE;
	}
	if ( ! waveOutGetNumDevs() )
	{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(_T("找不到语音输出设备!"));
		return FALSE;
	}

	// 分配空间
	pWaveHdr1	= (LPWAVEHDR)GlobalAllocPtr(GHND|GMEM_SHARE,sizeof(WAVEHDR));
	pWaveHdr2	= (LPWAVEHDR)GlobalAllocPtr(GHND|GMEM_SHARE,sizeof(WAVEHDR));
	// 录音缓冲区
	pInBuffer1	= (char*)GlobalAllocPtr(GHND|GMEM_SHARE,PCMBUFFER_SIZE);
	pInBuffer2	= (char*)GlobalAllocPtr(GHND|GMEM_SHARE,PCMBUFFER_SIZE);

	((CIPAddressCtrl*)GetDlgItem(IDC_IPADDRESS_T1))->SetAddress(htonl(inet_addr("127.0.0.1")));
	((CIPAddressCtrl*)GetDlgItem(IDC_IPADDRESS_T2))->SetAddress(htonl(inet_addr("192.168.1.110")));
	GetDlgItem(IDC_IPADDRESS_T1)->EnableWindow(FALSE);
	GetDlgItem(IDC_IPADDRESS_T2)->EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT_BROWSER)->SetWindowText(_T("d:\\wav.wav"));

	// alloc memery of voice output
	m_lpHdrOut	= (LPWAVEHDR)GlobalAllocPtr(GHND|GMEM_SHARE,sizeof(WAVEHDR));
	m_pBufOut1	= (char*)GlobalAllocPtr(GHND|GMEM_SHARE,PCMBUFFER_SIZE);
	m_pBufOut2	= (char*)GlobalAllocPtr(GHND|GMEM_SHARE,PCMBUFFER_SIZE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CSendVoiceDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CSendVoiceDlg::OnPaint()
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
HCURSOR CSendVoiceDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CSendVoiceDlg::OnSend()
{
	if ( hWaveIn == NULL)
	{
		// 确定发送地址
		m_sendAddrVec.clear();
		int nCheck;
		nCheck = ((CButton*)GetDlgItem(IDC_CHECK_T1))->GetCheck();
		if (nCheck == BST_CHECKED)
		{
			DWORD address = 0;
			((CIPAddressCtrl*)GetDlgItem(IDC_IPADDRESS_T1))->GetAddress(address);

			SOCKADDR_IN sock_in;
			sock_in.sin_family	= AF_INET;
			sock_in.sin_port	= htons(6666);
			sock_in.sin_addr.S_un.S_addr	= htonl(address);
			m_sendAddrVec.push_back(sock_in);
		}
		nCheck = ((CButton*)GetDlgItem(IDC_CHECK_T2))->GetCheck();
		if (nCheck == BST_CHECKED)
		{
			DWORD address = 0;
			((CIPAddressCtrl*)GetDlgItem(IDC_IPADDRESS_T2))->GetAddress(address);

			SOCKADDR_IN sock_in;
			sock_in.sin_family	= AF_INET;
			sock_in.sin_port	= htons(6666);
			sock_in.sin_addr.S_un.S_addr	= htonl(address);
			m_sendAddrVec.push_back(sock_in);
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

		// 打开语音输入设备
		if ( (result=waveInOpen(&hWaveIn, WAVE_MAPPER,&format,(DWORD)hwnd, 0, CALLBACK_WINDOW)) != MMSYSERR_NOERROR )
		{
			TCHAR szErrorText[100] = {0};
			if (waveInGetErrorText(result,szErrorText,sizeof(szErrorText)) == MMSYSERR_NOERROR)
			{
				MessageBeep(MB_ICONEXCLAMATION);
				MessageBox(szErrorText,_T("WaveIn Open Error!"),MB_ICONEXCLAMATION|MB_OK);
			} 
			else
			{
				MessageBeep(MB_ICONEXCLAMATION);
				MessageBox(_T("Unknown Error"),_T("WaveIn Open Error!"),MB_ICONEXCLAMATION|MB_OK);
			}
			hWaveIn = NULL;
			return;
		}
		//打开音量控制
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

		pWaveHdr1->dwBufferLength		= PCMBUFFER_SIZE;
		pWaveHdr1->dwBytesRecorded		= 0;
		pWaveHdr1->dwFlags				= 0;
		pWaveHdr1->dwLoops				= 0;
		pWaveHdr1->dwUser				= 0;
		pWaveHdr1->lpData				= (LPSTR)pInBuffer1;
		pWaveHdr1->lpNext				= NULL;
		pWaveHdr1->reserved				= 0;
		waveInPrepareHeader(hWaveIn,pWaveHdr1,sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn,pWaveHdr1,sizeof(WAVEHDR));
		pWaveHdr2->dwBufferLength		= PCMBUFFER_SIZE;
		pWaveHdr2->dwBytesRecorded		= 0;
		pWaveHdr2->dwFlags				= 0;
		pWaveHdr2->dwLoops				= 0;
		pWaveHdr2->dwUser				= 0;
		pWaveHdr2->lpData				= (LPSTR)pInBuffer2;
		pWaveHdr2->lpNext				= NULL;
		pWaveHdr2->reserved				= 0;
		waveInPrepareHeader(hWaveIn,pWaveHdr2,sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn,pWaveHdr2,sizeof(WAVEHDR));

		result = waveInStart(hWaveIn);
		if (result != MMSYSERR_NOERROR)
		{
			MessageBeep(MB_ICONEXCLAMATION);
			MessageBox(_T("WaveIn Start Failed!"));
			hWaveIn = NULL;
			return;
		}

		//语音输出
		if ( (m_SforVoiceOut=socket(AF_INET,SOCK_DGRAM,0)) == INVALID_SOCKET )
		{
			MessageBox(_T("Socket For Wave Out Error!"));
			WSACleanup();
			return;
		}
		m_AcceptAddr.sin_family	= AF_INET;
		m_AcceptAddr.sin_port	= htons(6666);
		m_AcceptAddr.sin_addr.s_addr	= htonl(INADDR_ANY);
		if ( bind(m_SforVoiceOut,(SOCKADDR*)&m_AcceptAddr,sizeof(SOCKADDR)) == SOCKET_ERROR )
		{
			MessageBox(_T("Voice out socket bind error!"));  
			WSACleanup();
			return;  
		}
		//设置Socket为非阻塞模式
		if ( WSAAsyncSelect(m_SforVoiceOut,hwnd,WM_DATACOME,FD_READ) == SOCKET_ERROR )
		{
			MessageBox(_T("Voice out socket select error!")); 
			WSACleanup();
			return;  
		}
		//open voice output device
		wBits	= 16;
		wChannel= 1;
		dwSample= 22050;
		// format of voice: 16 bits,22.05KHz,Mono audio
		format.cbSize			= 0;
		format.wBitsPerSample	= wBits;		//采样精度
		format.nChannels		= wChannel;		//单声道，Mono/立体声
		format.nSamplesPerSec	= dwSample;	//采样率
		format.nBlockAlign		= format.nChannels * (format.wBitsPerSample/8);		//每样点的字节数
		format.nAvgBytesPerSec	= format.nSamplesPerSec * format.nBlockAlign;	//数据率		
		format.wFormatTag		= WAVE_FORMAT_PCM;
		result = waveOutOpen(&m_hWaveOut,WAVE_MAPPER,(LPWAVEFORMATEX)&format,(DWORD)hwnd,0,CALLBACK_WINDOW);
		if ( result != MMSYSERR_NOERROR)
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
			m_hWaveOut = NULL;
			return;
		}
		m_bPlayFirst	= TRUE;
		//waveOut Volume
		((CSliderCtrl*)GetDlgItem(IDC_SLIDER_WAVEOUT))->SetRange(0,0xffff);
		DWORD dwVolumn = 0;
		waveOutGetVolume(m_hWaveOut,&dwVolumn);
		((CSliderCtrl*)GetDlgItem(IDC_SLIDER_WAVEOUT))->SetPos( (dwVolumn & 0xffff0000)>>16 );
		CString str;
		str.Format(_T("%0.0f%%"),(double)((dwVolumn & 0xffff0000)>>16) / (double)0xffff * 100.0);
		GetDlgItem(IDC_STATIC_OUT)->SetWindowTextW(str);

		GetDlgItem(IDC_BUTTON_BEGIN)->SetWindowText(_T("停止讲话"));
	} 
	else
	{
		waveInReset(hWaveIn);
		waveInClose(hWaveIn);

		hWaveIn = NULL;
		if (m_SforVoiceOut != INVALID_SOCKET)
		{
			closesocket(m_SforVoiceOut);
			m_SforVoiceOut	= INVALID_SOCKET;
		}
		GetDlgItem(IDC_BUTTON_BEGIN)->SetWindowText(_T("开始讲话"));
	}
	
}

LRESULT CSendVoiceDlg::OnRecordFull(WPARAM wParam, LPARAM lParam)
{	// MM_WIM_DATA消息， 录音缓冲区满
	char* recordBuf	= (char*)((LPWAVEHDR)lParam)->lpData;
	int recordLen		= ((LPWAVEHDR)lParam)->dwBytesRecorded;
	if ( recordBuf == NULL )
	{
		waveInPrepareHeader(hWaveIn, (LPWAVEHDR)lParam, sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn,(LPWAVEHDR)lParam, sizeof(WAVEHDR));
		waveInReset(hWaveIn);
		return 0;
	}

	// 保存数据缓冲区
// 	if ( m_dataBuf == NULL )
// 	{
// 		m_dataBuf = (char*)malloc(recordLen+1);
// 		memcpy(m_dataBuf,recordBuf,recordLen);
// 		m_dataBuf[recordLen] = 0;
// 		m_dataLen += recordLen;
// 	} 
// 	else
// 	{
// 		char* m_newBuf;
// 		m_newBuf = (char*)realloc(m_dataBuf,m_dataLen+recordLen);
// 		if ( m_newBuf == NULL )
// 		{
// 			waveInReset(hWaveIn);
// 			return 0;
// 		}
// 		m_dataBuf = m_newBuf;
// 		CopyMemory(m_dataBuf+m_dataLen,recordBuf,recordLen);
// 		m_dataLen += recordLen;
// 	}

	//发送数据
	//sendto(m_SforVoiceIn,recordBuf,recordLen,0,(SOCKADDR*)&m_sendAddr,sizeof(SOCKADDR));
	int nSendSize = m_sendAddrVec.size();
	for (int i=0;i<nSendSize;i++)
	{
		sendto(m_SforVoiceIn,recordBuf,recordLen,0,(SOCKADDR*)&m_sendAddrVec.at(i),sizeof(SOCKADDR));
	}

	//重新添加语音输入缓冲区
	waveInPrepareHeader(hWaveIn, (LPWAVEHDR)lParam, sizeof(WAVEHDR));
	waveInAddBuffer(hWaveIn,(LPWAVEHDR)lParam, sizeof(WAVEHDR));

	// 保存文件
	//SaveWavFile();
	if ( m_bSaveFile )
		SaveWavFile(recordBuf,recordLen);

	return 0;
}

LRESULT CSendVoiceDlg::OnRecordClose(WPARAM wParam, LPARAM lParam)
{
	//SaveWavFile();

	return 0;
}


void CSendVoiceDlg::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class
	if (hWaveIn != NULL)
	{
		waveInReset(hWaveIn);
		waveInUnprepareHeader(hWaveIn, pWaveHdr1, sizeof(WAVEHDR));
		waveInUnprepareHeader(hWaveIn, pWaveHdr2, sizeof(WAVEHDR));
		waveInClose(hWaveIn);
	}

	GlobalFreePtr(pInBuffer1);
	GlobalFreePtr(pInBuffer2);
	GlobalFreePtr(pWaveHdr1);
	GlobalFreePtr(pWaveHdr2);
	if ( m_SforVoiceIn != INVALID_SOCKET)
		closesocket(m_SforVoiceIn);

	if ( m_SforVoiceOut)
		closesocket(m_SforVoiceOut);
	if ( m_hWaveOut != NULL )
	{
		waveOutReset(m_hWaveOut);
		waveOutUnprepareHeader(m_hWaveOut,m_lpHdrOut,sizeof(WAVEHDR));
		waveOutClose(m_hWaveOut);
	}
	GlobalFreePtr(m_lpHdrOut);
	GlobalFreePtr(m_pBufOut1);
	GlobalFreePtr(m_pBufOut2);

	CDialogEx::OnCancel();
}


// 保存WAV文件
void CSendVoiceDlg::SaveWavFile(void)
{
	CFile m_file;  
    CFileException fileException;  
    CString m_csFileName= _T("D:\\test1.wav");  
    m_file.Open(m_csFileName,CFile::modeCreate|CFile::modeReadWrite, &fileException);  
    /*if (m_file.Open(m_csFileName,CFile::modeCreate|CFile::modeReadWrite, &fileException)) 
    {MessageBox("open file"); 
}*/  
    DWORD m_WaveHeaderSize = 38;  
    DWORD m_WaveFormatSize = 18;  
    m_WaveFormatSize = sizeof(WAVEFORMATEX);  
    m_file.SeekToBegin();  
  
    // riff block  
    m_file.Write("RIFF",4);  
    //unsigned int Sec=(sizeof  + m_WaveHeaderSize);  
    unsigned int Sec=(sizeof m_dataBuf + m_WaveHeaderSize);  // 整个wav文件大小 - 8 （RIFF+SIZE）  
    m_file.Write(&Sec,sizeof(Sec));  
    m_file.Write("WAVE",4);  
  
    // fmt block  
    m_file.Write("fmt ",4);  
    m_file.Write(&m_WaveFormatSize, sizeof(m_WaveFormatSize));  
      
//  m_file.Write(&format.wFormatTag, sizeof(format.wFormatTag));  
//  m_file.Write(&format.nChannels, sizeof(format.nChannels));  
//  m_file.Write(&format.nSamplesPerSec, sizeof(format.nSamplesPerSec));  
//  m_file.Write(&format.nAvgBytesPerSec, sizeof(format.nAvgBytesPerSec));  
//  m_file.Write(&format.nBlockAlign, sizeof(format.nBlockAlign));  
//  m_file.Write(&format.wBitsPerSample, sizeof(format.wBitsPerSample));  
//  m_file.Write(&format.cbSize, sizeof(format.cbSize));  
    m_file.Write(&format, sizeof(WAVEFORMATEX));  
  
    // data block  
    m_file.Write("data",4);  
    m_file.Write(&m_dataLen,sizeof(DWORD));  
    m_file.Write(m_dataBuf,m_dataLen);  
  
    m_file.Seek(m_dataLen,CFile::begin);  
    m_file.Close(); 
}


// 增量保存WAV文件
void CSendVoiceDlg::SaveWavFile(char* data, UINT datalen)
{
	CFile wavFile;
	CString sFileName = m_sSaveFile;
	if ( GetFileAttributes(sFileName) == INVALID_FILE_ATTRIBUTES )	//文件不存在
	{
		wavFile.Open(sFileName,CFile::modeCreate|CFile::modeWrite);

		DWORD m_WaveHeaderSize = 38;  
		DWORD m_WaveFormatSize = 18;  
		m_WaveFormatSize = sizeof(WAVEFORMATEX);  
		wavFile.SeekToBegin();  

		// riff block  
		wavFile.Write("RIFF",4);  
		unsigned int Sec=m_WaveHeaderSize;  // 整个wav文件大小 - 8 （RIFF+SIZE）  
		wavFile.Write(&Sec,4);  
		wavFile.Write("WAVE",4); 

		// fmt block  
		wavFile.Write("fmt ",4);  
		wavFile.Write(&m_WaveFormatSize, sizeof(m_WaveFormatSize));
		wavFile.Write(&format, sizeof(WAVEFORMATEX)); 

		// data block  
		wavFile.Write("data",4);  
		DWORD tmp = 0;
		wavFile.Write(&tmp,4); 

		wavFile.Close();
	}

	wavFile.Open(sFileName,CFile::modeNoTruncate|CFile::modeReadWrite);
	wavFile.SeekToBegin();
	wavFile.Seek(4,CFile::begin);
	// 获取wav文件长度
	DWORD filesize = 0;
	wavFile.Read(&filesize,4);
	filesize += datalen;
	// 写入wav文件长度
	wavFile.Seek(4,CFile::begin);
	wavFile.Write(&filesize,4);
	// 获取wav声音数据大小
	wavFile.Seek(4*6+sizeof(WAVEFORMATEX),CFile::begin);
	DWORD datasize = 0;
	wavFile.Read(&datasize,4);
	// 写入wav声音数据大小
	wavFile.Seek(4*6+sizeof(WAVEFORMATEX),CFile::begin);
	datasize += datalen;
	wavFile.Write(&datasize,4);
	// 写入wav声音数据
	wavFile.SeekToEnd();
	wavFile.Write(data,datalen);
	wavFile.Close();
}


void CSendVoiceDlg::OnCheckAll()
{
	int nCheck = ((CButton*)GetDlgItem(IDC_CHECK_ALL))->GetCheck();
	if (nCheck == BST_CHECKED)
	{
		((CButton*)GetDlgItem(IDC_CHECK_T1))->SetCheck(nCheck);
		((CButton*)GetDlgItem(IDC_CHECK_T2))->SetCheck(nCheck);
		GetDlgItem(IDC_IPADDRESS_T1)->EnableWindow(TRUE);
		GetDlgItem(IDC_IPADDRESS_T2)->EnableWindow(TRUE);
	} 
	else
	{
		((CButton*)GetDlgItem(IDC_CHECK_T1))->SetCheck(nCheck);
		((CButton*)GetDlgItem(IDC_CHECK_T2))->SetCheck(nCheck);
		GetDlgItem(IDC_IPADDRESS_T1)->EnableWindow(FALSE);
		GetDlgItem(IDC_IPADDRESS_T2)->EnableWindow(FALSE);
	}
}


void CSendVoiceDlg::OnCheckSaveFile()
{
	int nCheck = ((CButton*)GetDlgItem(IDC_CHECK_SAVEFILE))->GetCheck();
	if (nCheck == BST_CHECKED)
	{
		GetDlgItem(IDC_EDIT_BROWSER)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_BROWSER)->EnableWindow(TRUE);
		m_bSaveFile	= TRUE;
		GetDlgItem(IDC_EDIT_BROWSER)->GetWindowText(m_sSaveFile);
	} 
	else
	{
		GetDlgItem(IDC_EDIT_BROWSER)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_BROWSER)->EnableWindow(FALSE);
		m_bSaveFile	= FALSE;
	}
}


void CSendVoiceDlg::OnButtonBrowser()
{
	CFileDialog openDlg(FALSE,_T("wav"),NULL,OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY,_T("声音文件(*.wav)|*.wav"),NULL);
	if (openDlg.DoModal() == IDOK)
	{
		CString sName = openDlg.GetPathName();
		GetDlgItem(IDC_EDIT_BROWSER)->SetWindowText(sName);
		m_sSaveFile	= sName;
	}
}


void CSendVoiceDlg::OnCheckT1()
{
	int nCheck = ((CButton*)GetDlgItem(IDC_CHECK_T1))->GetCheck();
	if (nCheck == BST_CHECKED)
	{
		GetDlgItem(IDC_IPADDRESS_T1)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_IPADDRESS_T1)->EnableWindow(FALSE);
	}
}


void CSendVoiceDlg::OnCheckT2()
{
	int nCheck = ((CButton*)GetDlgItem(IDC_CHECK_T2))->GetCheck();
	if (nCheck == BST_CHECKED)
	{
		GetDlgItem(IDC_IPADDRESS_T2)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_IPADDRESS_T2)->EnableWindow(FALSE);
	}
}


void CSendVoiceDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	if (hWaveIn!=NULL && pScrollBar->m_hWnd==GetDlgItem(IDC_SLIDER_WAVEIN)->m_hWnd)
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
	
	if ( m_hWaveOut!=NULL && pScrollBar->m_hWnd == GetDlgItem(IDC_SLIDER_WAVEOUT)->m_hWnd )
	{
		CSliderCtrl* pSlider;
		DWORD nSliderPos;
		pSlider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_WAVEOUT);
		nSliderPos = pSlider->GetPos();
		nSliderPos = (nSliderPos << 16);
		nSliderPos |= nSliderPos;
		waveOutSetVolume(m_hWaveOut, nSliderPos);
		CString str;
		str.Format(_T("%0.0f%%"),(double)((nSliderPos & 0xffff0000)>>16) / (double)0xffff * 100.0);
		GetDlgItem(IDC_STATIC_OUT)->SetWindowTextW(str);
	}

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}


//void CSendVoiceDlg::OnBnClickedButton1()
//{
// 	WAVEFORMATEX wfx;
// 	memset( &wfx, 0, sizeof(WAVEFORMATEX) );
// 	wfx.wFormatTag = WAVE_FORMAT_PCM;
// 	wfx.nChannels = 1;
// 	wfx.nSamplesPerSec = 8000;
// 	wfx.nAvgBytesPerSec = 1 * 8000 * 16 / 8;
// 	wfx.nBlockAlign = 16 * 1 / 8;
// 	wfx.wBitsPerSample = 16;
// 	wfx.cbSize = 0;
// 
// 	HWAVEIN hwaveIn;
// 	MMRESULT m_mmr = waveInOpen( &hwaveIn, WAVE_MAPPER, &wfx, 0L, 0L, CALLBACK_NULL );
// 
// 	if ( m_mmr != MMSYSERR_NOERROR )
// 	{
// 		return;
// 	}
// 	UINT m_uiMixerId = 0;
// 	m_mmr = mixerGetID((HMIXEROBJ)hwaveIn, &m_uiMixerId, MIXER_OBJECTF_HWAVEIN );
// 	waveInClose( hwaveIn );
// 	if ( m_mmr != MMSYSERR_NOERROR )
// 	{
// 		return;
// 	}
// 	HMIXER m_hMixer = NULL;
// 	m_mmr = mixerOpen(&m_hMixer, m_uiMixerId, (DWORD)m_hWnd, 0L, CALLBACK_WINDOW );
// 	if (m_mmr != MMSYSERR_NOERROR )
// 	{
// 		return;
// 	}
// 
// 	DWORD dwUserValue = 0;
// 	MIXERLINE MixerLine;
// 	memset( &MixerLine, 0, sizeof(MIXERLINE) );
// 	MixerLine.cbStruct = sizeof(MIXERLINE);
// 	MixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN ;
// 
// 	m_mmr = mixerGetLineInfo( (HMIXEROBJ)m_hMixer, &MixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE );
// 	if (m_mmr != MMSYSERR_NOERROR )
// 	{
// 		return;
// 	}
// 
// 	MIXERLINE Line;
// 	for (UINT uLineIndex = 0; uLineIndex < MixerLine.cConnections; uLineIndex++ )
// 	{
// 		memset( &Line, 0, sizeof(MIXERLINE));
// 		Line.cbStruct = sizeof(MIXERLINE);
// 		Line.dwDestination = MixerLine.dwDestination;
// 		Line.dwSource = uLineIndex;
// 		m_mmr = mixerGetLineInfo((HMIXEROBJ)m_hMixer, &Line, MIXER_GETLINEINFOF_SOURCE );
// 		if (m_mmr != MMSYSERR_NOERROR )
// 		{
// 			return;
// 		}
// 
// 		if ( Line.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE )
// 		{
// 			dwUserValue = uLineIndex;
// 			break;
// 		}
// 	}
// 
// 
// 	//MIXERLINE MixerLine;
// 	memset( &MixerLine, 0, sizeof(MIXERLINE) );
// 	MixerLine.cbStruct = sizeof(MIXERLINE);
// 	MixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
// 	MixerLine.dwSource = 1;
// 
// 	m_mmr = mixerGetLineInfo( (HMIXEROBJ)m_hMixer, &MixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE );
// 	if ( m_mmr != MMSYSERR_NOERROR )
// 	{
// 		return;
// 	}
// 	MIXERCONTROL Control;
// 	memset( &Control, 0, sizeof(MIXERCONTROL));
// 	Control.cbStruct = sizeof(MIXERCONTROL);
// 
// 	MIXERLINECONTROLS LineControls;
// 	memset( &LineControls, 0, sizeof(MIXERLINECONTROLS));
// 	LineControls.cbStruct = sizeof(MIXERLINECONTROLS);
// 
// 	//MIXERLINE Line;
// 	memset( &Line, 0, sizeof(MIXERLINE) );
// 	Line.cbStruct = sizeof(MIXERLINE);
// 
// 	if ((dwUserValue < MixerLine.cConnections))
// 	{
// 		Line.dwDestination = MixerLine.dwDestination;
// 		Line.dwSource = dwUserValue;
// 		m_mmr = mixerGetLineInfo((HMIXEROBJ)m_hMixer, &Line, MIXER_GETLINEINFOF_SOURCE );
// 		if ( m_mmr != MMSYSERR_NOERROR )
// 		{
// 			return ;
// 		}
// 
// 		LineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
// 		LineControls.dwLineID = Line.dwLineID;
// 		LineControls.cControls = 1;
// 		LineControls.cbmxctrl = sizeof(MIXERCONTROL);
// 		LineControls.pamxctrl = &Control;
// 		m_mmr = mixerGetLineControls( (HMIXEROBJ)m_hMixer, &LineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE );
// 		if ( m_mmr == MMSYSERR_NOERROR )
// 		{
// 			if ((Control.fdwControl & MIXERCONTROL_CONTROLF_DISABLED) )
// 			{
// 				return;
// 			}
// 		}
// 		else
// 		{
// 			return;
// 		}		
// 	} 
// 	else
// 	{
// 		return ;
// 	}
// 	DWORD m_dwMinimalVolume = Control.Bounds.dwMinimum;
// 	DWORD m_dwMaximalVolume = Control.Bounds.dwMaximum;
//	///////////////////////////////////////
//	// 打开mixer
// 	HMIXER hmx;
// 	MMRESULT rc;
// 	rc = mixerOpen(&hmx,0,0,0,0);
// 	if (rc != MMSYSERR_NOERROR)
// 	{
// 		MessageBox(_T("mixerOpen Error!"));
// 		return;
// 	}
// 	//mixerGetLineInfo
// 	MIXERLINE mxLine;
// 	ZeroMemory(&mxLine,sizeof(mxLine));
// 	mxLine.cbStruct	= sizeof(mxLine);
// 	mxLine.dwComponentType	= MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE;
// 	rc = mixerGetLineInfo((HMIXEROBJ)hmx, &mxLine, MIXER_GETLINEINFOF_COMPONENTTYPE);
// 	if (rc != MMSYSERR_NOERROR)
// 	{
// 		MessageBox(_T("mixerGetLineInfo Error!"));
// 		return;
// 	}
// 	// get the control
// 	MIXERCONTROL mxControl;
// 	MIXERLINECONTROLS mxLineControls;
// 	ZeroMemory(&mxLineControls, sizeof(mxLineControls));
// 	mxLineControls.cbStruct	= sizeof(mxLineControls);
// 	mxLineControls.dwLineID	= mxLine.dwLineID;
// 	mxLineControls.dwControlType	= MIXERCONTROL_CONTROLTYPE_VOLUME;
// 	mxLineControls.cControls		= 1;
// 	mxLineControls.cbmxctrl			= sizeof(mxControl);
// 	mxLineControls.pamxctrl			= &mxControl;
// 	ZeroMemory(&mxControl,sizeof(mxControl));
// 	mxControl.cbStruct	= sizeof(mxControl);
// 	rc = mixerGetLineControls((HMIXEROBJ)hmx, &mxLineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
// 	if (rc != MMSYSERR_NOERROR)
// 	{
// 		MessageBox(_T("mixerGetLineControls Error!"));
// 		return;
// 	}
// 	// get the value
// 	MIXERCONTROLDETAILS mxControlDetails;
// 	MIXERCONTROLDETAILS_UNSIGNED volStruct;
// 	ZeroMemory(&mxControlDetails,sizeof(mxControlDetails));
// 	mxControlDetails.cbStruct	= sizeof(mxControlDetails);
// 	mxControlDetails.cChannels	= 1;
// 	mxControlDetails.dwControlID= mxControl.dwControlID;
// 	mxControlDetails.cbDetails	= sizeof(volStruct);
// 	mxControlDetails.paDetails	= &volStruct;
// 	rc = mixerGetControlDetails((HMIXEROBJ)hmx, &mxControlDetails, MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_VALUE);
// 	if (rc != MMSYSERR_NOERROR)
// 	{
// 		MessageBox(_T("mixerGetLineControls Error!"));
// 		return;
// 	}
// 	DWORD volume = volStruct.dwValue;
// 	CString str;
// 	str.Format(_T("%d"),volume);
// 	MessageBox(str);
//	//******
//	// 打开mixer
//	MMRESULT rc;
//	HMIXER hMixer;
//	UINT mixerID;
//	rc = mixerGetID((HMIXEROBJ)hWaveIn,&mixerID,MIXER_OBJECTF_HWAVEIN);
//	rc = mixerOpen(&hMixer,mixerID,(DWORD)this->m_hWnd,0,CALLBACK_WINDOW);
//	if (rc != MMSYSERR_NOERROR)
//	{
//		MessageBox(_T("mixerOpen Error!"));
//		return;
//	}
//	// mixerGetLineInfo
//	MIXERLINE mixerLine;
//	ZeroMemory(&mixerLine,sizeof(MIXERLINE));
//	mixerLine.cbStruct	= sizeof(MIXERLINE);
//	mixerLine.dwComponentType	= MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
//	rc = mixerGetLineInfo((HMIXEROBJ)hMixer, &mixerLine,MIXER_GETLINEINFOF_COMPONENTTYPE);
//	if (rc != MMSYSERR_NOERROR)
//	{
//		MessageBox(_T("mixerGetLineInfo Error!"));
//		return;
//	}
//
//	// 获取麦克风的LineID
//	DWORD dwConnections = mixerLine.cConnections;
//	DWORD dwLineID = -1;
//	for (DWORD i=0; i<dwConnections; i++)
//	{
//		mixerLine.dwSource = i;
//		rc = mixerGetLineInfo((HMIXEROBJ)hMixer,&mixerLine,MIXER_OBJECTF_HMIXER|MIXER_GETLINEINFOF_SOURCE);
//		if ( rc == MMSYSERR_NOERROR )
//		{
//			if (mixerLine.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
//			{
//				dwLineID = mixerLine.dwLineID;
//				break;
//			}
//		}
//	}
//	if (dwLineID == -1)
//	{
//		return;
//	}
//	// mixerGetLineControls
//	MIXERCONTROL mxControl;
//	MIXERLINECONTROLS mxLineControls;	
//	ZeroMemory(&mxLineControls,sizeof(mxLineControls));
//	mxLineControls.cbStruct		= sizeof(mxLineControls);
//	mxLineControls.dwLineID		= dwLineID;
//	mxLineControls.dwControlType= MIXERCONTROL_CONTROLTYPE_VOLUME;
//	mxLineControls.cControls	= 1;
//	mxLineControls.cbmxctrl		= sizeof(mxControl);
//	mxLineControls.pamxctrl		= &mxControl;
//	ZeroMemory(&mxControl,sizeof(mxControl));
//	mxControl.cbStruct	= sizeof(mxControl);
//	rc = mixerGetLineControls((HMIXEROBJ)hMixer,&mxLineControls,MIXER_GETLINECONTROLSF_ONEBYTYPE);
//	if (rc != MMSYSERR_NOERROR)
//	{
//		MessageBox(_T("mixerGetLineControls Error!"));
//		return;
//	}
//	// get volume
//	MIXERCONTROLDETAILS mxControlDetails;
//	MIXERCONTROLDETAILS_SIGNED volStruct;
//	ZeroMemory(&mxControlDetails,sizeof(mxControlDetails));
//	mxControlDetails.cbStruct	= sizeof(mxControlDetails);
//	mxControlDetails.dwControlID= mxControl.dwControlID;
//	mxControlDetails.cChannels	= 1;
//	mxControlDetails.cbDetails	= sizeof(volStruct);
//	mxControlDetails.paDetails	= &volStruct;
//	rc = mixerGetControlDetails((HMIXEROBJ)hMixer,&mxControlDetails,MIXER_GETCONTROLDETAILSF_VALUE);
//	if (rc != MMSYSERR_NOERROR)
//	{
//		MessageBox(_T("mixerGetControlDetails Error!"));
//		return;
//	}
//	long step = (mxControl.Bounds.lMaximum - mxControl.Bounds.lMinimum) / 100;
//	int index = (volStruct.lValue - mxControl.Bounds.lMinimum) / step;
//	//set volume
//	volStruct.lValue = 65535;
//	ZeroMemory(&mxControlDetails,sizeof(mxControlDetails));
//	mxControlDetails.cbStruct	= sizeof(mxControlDetails);
//	mxControlDetails.dwControlID= mxControl.dwControlID;
//	mxControlDetails.cChannels	= 1;
//	mxControlDetails.cbDetails	= sizeof(volStruct);
//	mxControlDetails.paDetails	= &volStruct;
//	rc = mixerSetControlDetails((HMIXEROBJ)hMixer,&mxControlDetails,MIXER_GETCONTROLDETAILSF_VALUE);
//	if (rc != MMSYSERR_NOERROR)
//	{
//		MessageBox(_T("mixerGetControlDetails Error!"));
//		return;
//	}
//	/*****/
//}


LRESULT CSendVoiceDlg::OnVolumeChanged(WPARAM wParam, LPARAM lParam)
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

BOOL CSendVoiceDlg::MixerInit(DWORD& dwMin, DWORD& dwMax)
{
	MMRESULT rc;
	UINT mixerID;
	rc = mixerGetID((HMIXEROBJ)hWaveIn,&mixerID,MIXER_OBJECTF_HWAVEIN);
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

	// 获取麦克风的LineID
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


// 获取音量
DWORD CSendVoiceDlg::GetVolume(HMIXER hMixer, DWORD dwControlID)
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


// 设备音量
void CSendVoiceDlg::SetVolume(HMIXER hMixer, DWORD dwControlID, DWORD dwVolume)
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


// receive voice
LRESULT CSendVoiceDlg::OnDataCome(WPARAM wParam, LPARAM lParam)
{
	if (WSAGETSELECTERROR(lParam))
	{
		return 0;
	}
	if ( WSAGETSELECTEVENT(lParam) != FD_READ)
	{
		return 0;
	}
	int nRecLen;
	if (m_bPlayFirst)
	{
		nRecLen = recvfrom(m_SforVoiceOut,m_pBufOut1,PCMBUFFER_SIZE,0,(SOCKADDR*)&m_AcceptAddr,&m_nAcceptAddrLen);
		if (nRecLen == SOCKET_ERROR )	return 0;
		if (nRecLen != PCMBUFFER_SIZE )		return 0;
		m_bPlayFirst = FALSE;
		::PostMessage(hwnd,WM_PLAY,0,0);
	} 
	else
	{
		nRecLen = recvfrom(m_SforVoiceOut,m_pBufOut2,PCMBUFFER_SIZE,0,(SOCKADDR*)&m_AcceptAddr,&m_nAcceptAddrLen);
		if (nRecLen == SOCKET_ERROR )	return 0;
		if (nRecLen != PCMBUFFER_SIZE )		return 0;
		m_bPlayFirst = TRUE;
		::PostMessage(hwnd,WM_PLAY,0,0);
	}
	return 0;
}

// play sound
LRESULT CSendVoiceDlg::OnPlay(WPARAM wParam, LPARAM lParam)
{
	if ( m_bPlayFirst )
		m_lpHdrOut->lpData = (LPSTR)m_pBufOut1;
	else
		m_lpHdrOut->lpData = (LPSTR)m_pBufOut2;
	m_lpHdrOut->dwBufferLength	= PCMBUFFER_SIZE;
	m_lpHdrOut->dwBytesRecorded	= 0;
	m_lpHdrOut->dwFlags			= WHDR_BEGINLOOP;
	m_lpHdrOut->dwLoops			= 1;
	m_lpHdrOut->dwUser			= 0;
	m_lpHdrOut->lpNext			= NULL;
	m_lpHdrOut->reserved		= 0;
	waveOutPrepareHeader(m_hWaveOut,m_lpHdrOut,sizeof(WAVEHDR));
	waveOutWrite(m_hWaveOut,m_lpHdrOut,sizeof(WAVEHDR));

	return 0;
}
