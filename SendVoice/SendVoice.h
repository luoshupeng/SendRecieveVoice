
// SendVoice.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CSendVoiceApp:
// �йش����ʵ�֣������ SendVoice.cpp
//

class CSendVoiceApp : public CWinApp
{
public:
	CSendVoiceApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CSendVoiceApp theApp;