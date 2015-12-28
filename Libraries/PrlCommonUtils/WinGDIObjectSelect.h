/*
* Copyright 2005-2012. Parallels IP Holdings GmbH. All Rights Reserved.
*/

#pragma once

#include <windows.h>
#include <prlcommon/Std/noncopyable.h>

class WinGDIObjectSelect: private noncopyable
{
public:
	WinGDIObjectSelect(const HDC dc, const HGDIOBJ obj):
		m_dc(dc),
		m_old(SelectObject(m_dc, obj)) {}
	~WinGDIObjectSelect() { SelectObject(m_dc, m_old); }
private:
	const HDC m_dc;
	const HGDIOBJ m_old;
};
