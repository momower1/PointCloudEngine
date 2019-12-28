#ifndef IGUIELEMENT_H
#define IGUIELEMENT_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
	class IGUIElement
	{
	public:
		virtual void Update() {}
		virtual void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {}
		virtual void SetPosition(XMUINT2 position) = 0;
		virtual void Show(int SW_COMMAND) = 0;
	};
}
#endif
