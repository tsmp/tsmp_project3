#pragma once
#include "UIGameCustom.h"

class UIVoteStatusWnd;

class CUIGameBaseMP : public CUIGameCustom
{
public:
	CUIGameBaseMP();
	virtual ~CUIGameBaseMP();

	virtual void OnFrame() override;
	virtual void Render() override;

	void SetVoteTimeResultMsg(const char* str);
	void SetVoteMessage(const char* str);

protected:
	UIVoteStatusWnd* m_voteStatusWnd;

private:
	using inherited = CUIGameCustom;
};
