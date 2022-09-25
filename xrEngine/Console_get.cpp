#include "pch_xrengine.h"
#include "Console.h"
#include "Console_commands.h"

bool CConsole::GetBool(LPCSTR cmd, bool& val)
{
	vecCMD_IT I = Commands.find(cmd);

	if (I != Commands.end())
	{
		IConsole_Command* C = I->second;
		CCC_Mask* cf = dynamic_cast<CCC_Mask*>(C);

		if (cf)
			val = !!cf->GetValue();
		else
		{
			CCC_Integer* ci = dynamic_cast<CCC_Integer*>(C);
			val = !!ci->GetValue();
		}
	}

	return val;
}

float CConsole::GetFloat(LPCSTR cmd, float& val, float& min, float& max)
{
	vecCMD_IT I = Commands.find(cmd);

	if (I != Commands.end())
	{
		IConsole_Command* C = I->second;
		CCC_Float* cf = dynamic_cast<CCC_Float*>(C);
		val = cf->GetValue();
		min = cf->GetMin();
		max = cf->GetMax();
		return val;
	}

	return val;
}

int CConsole::GetInteger(LPCSTR cmd, int& val, int& min, int& max)
{
	vecCMD_IT I = Commands.find(cmd);

	if (I != Commands.end())
	{
		IConsole_Command* C = I->second;
		CCC_Integer* cf = dynamic_cast<CCC_Integer*>(C);

		if (cf)
		{
			val = cf->GetValue();
			min = cf->GetMin();
			max = cf->GetMax();
		}
		else
		{
			CCC_Mask* cm = dynamic_cast<CCC_Mask*>(C);
			R_ASSERT(cm);
			val = (0 != cm->GetValue()) ? 1 : 0;
			min = 0;
			max = 1;
		}

		return val;
	}

	return val;
}

char* CConsole::GetString(LPCSTR cmd)
{
	static IConsole_Command::TStatus stat;
	vecCMD_IT I = Commands.find(cmd);

	if (I != Commands.end())
	{
		IConsole_Command* C = I->second;
		C->Status(stat);
		return stat;
	}

	return nullptr;
}

char* CConsole::GetToken(LPCSTR cmd)
{
	return GetString(cmd);
}

xr_token* CConsole::GetXRToken(LPCSTR cmd)
{
	vecCMD_IT I = Commands.find(cmd);

	if (I != Commands.end())
	{
		IConsole_Command* C = I->second;
		CCC_Token* cf = dynamic_cast<CCC_Token*>(C);
		return cf->GetToken();
	}

	return nullptr;
}
