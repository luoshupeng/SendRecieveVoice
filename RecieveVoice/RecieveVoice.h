
// RecieveVoice.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CRecieveVoiceApp:
// �йش����ʵ�֣������ RecieveVoice.cpp
//

class CRecieveVoiceApp : public CWinApp
{
public:
	CRecieveVoiceApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CRecieveVoiceApp theApp;