#include "StdAfx.h"
#include "..\TSMP3_Build_Config.h"

#ifdef PUBLIC_BUILD
void RegisterCarDbgCommands() {};
#else

#include "Console.h"
#include "console_commands.h"
#include "Actor.h"
#include "Car.h"

CCar* GetMyCar()
{
	if (!Actor())
	{
		Msg("! cant get actor");
		return nullptr;
	}
		
	if (CCar* car = smart_cast<CCar*>(Actor()->Holder()))
		return car;

	Msg("! cant get current car");
	return nullptr;
}

float ToFloat(const char* str)
{
	return static_cast<float>(atof(str));
}

class CCC_CarEnginePower : public IConsole_Command
{
public:
	CCC_CarEnginePower(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
		CCar* car = GetMyCar();

		if (!car)
			return;
		
		if (!xr_strlen(args))
		{
			Msg("- car_engine_power %f", car->GetEnginePower());
			return;
		}

		car->ChangeEnginePower(ToFloat(args));
	}
};

class CCC_CarReferenceRadius : public IConsole_Command
{
public:
	CCC_CarReferenceRadius(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
		CCar* car = GetMyCar();

		if (!car)
			return;

		if (!xr_strlen(args))
		{
			Msg("- car_reference_radius %f", car->GetReferenceRadius());
			return;
		}

		car->ChangeReferenceRadius(ToFloat(args));
	}
};

class CCC_CarSteeringSpeed : public IConsole_Command
{
public:
	CCC_CarSteeringSpeed(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
		CCar* car = GetMyCar();

		if (!car)
			return;

		if (!xr_strlen(args))
		{
			Msg("- car_steering_speed %f", car->GetSteeringSpeed());
			return;
		}

		car->ChangeSteeringSpeed(ToFloat(args));
	}
};

class CCC_CarFrictionFactor : public IConsole_Command
{
public:
	CCC_CarFrictionFactor(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
		CCar* car = GetMyCar();

		if (!car)
			return;

		if (!xr_strlen(args))
		{
			Msg("- car_friction_factor %f", car->GetFrictionFactor());
			return;
		}

		car->ChangeFrictionFactor(ToFloat(args));
	}
};

class CCC_CarHandBreakTorque : public IConsole_Command
{
public:
	CCC_CarHandBreakTorque(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
		CCar* car = GetMyCar();

		if (!car)
			return;

		if (!xr_strlen(args))
		{
			Msg("- car_hand_break_torque %f", car->GetHandBreakTorque());
			return;
		}

		car->ChangeHandBreakTorque(ToFloat(args));
	}
};

class CCC_CarMaxPowerRpm : public IConsole_Command
{
public:
	CCC_CarMaxPowerRpm(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
		CCar* car = GetMyCar();

		if (!car)
			return;

		if (!xr_strlen(args))
		{
			Msg("- car_max_power_rpm %f", car->GetMaxPowerRpm());
			return;
		}

		car->ChangeMaxPowerRpm(ToFloat(args));
	}
};

class CCC_CarMaxTorqueRpm : public IConsole_Command
{
public:
	CCC_CarMaxTorqueRpm(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
		CCar* car = GetMyCar();

		if (!car)
			return;

		if (!xr_strlen(args))
		{
			Msg("- car_max_torque_rpm %f", car->GetMaxTorqueRpm());
			return;
		}

		car->ChangeMaxTorqueRpm(ToFloat(args));
	}
};

void RegisterCarDbgCommands()
{
	CMD1(CCC_CarEnginePower, "car_engine_power");
	CMD1(CCC_CarReferenceRadius, "car_reference_radius");
	CMD1(CCC_CarSteeringSpeed, "car_steering_speed");
	CMD1(CCC_CarFrictionFactor, "car_friction_factor");
	CMD1(CCC_CarHandBreakTorque, "car_hand_break_torque");
	CMD1(CCC_CarMaxPowerRpm, "car_max_power_rpm");
	CMD1(CCC_CarMaxTorqueRpm, "car_max_torque_rpm");
}

#endif
