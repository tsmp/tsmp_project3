#pragma once

#define LT_DIRECT		0
#define LT_POINT		1
#define LT_SECONDARY	2

//#define USE_PORTED_XRLC

struct R_Light
{
	u16				type;				// Type of light source		
	u16				level;				// GI level
	Fvector         diffuse;			// Diffuse color of light	
	Fvector         position;			// Position in world space	
	Fvector         direction;			// Direction in world space	
	float		    range;				// Cutoff range
	float			range2;				// ^2
#ifdef USE_PORTED_XRLC
	float			falloff;			// precalc to make light aqal to zero at light range
#else
	float			falloff				() const { return 1.0f/(range * (attenuation0 + attenuation1 * range + attenuation2 * range2)); }
#endif
	float	        attenuation0;		// Constant attenuation		
	float	        attenuation1;		// Linear attenuation		
	float	        attenuation2;		// Quadratic attenuation	
	float			energy;				// For radiosity ONLY

	Fvector			tri[3];

	R_Light()		{
		tri[0].set	(0,0,0);
		tri[1].set	(0,0,EPS_S);
		tri[2].set	(EPS_S,0,0);
	}
};
