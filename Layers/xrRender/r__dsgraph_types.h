#pragma once

#include "../../xrCore/fixedmap.h"

#ifndef USE_MEMORY_MONITOR
#	define USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
#endif // USE_MEMORY_MONITOR

#define render_alloc				xalloc
typedef xr_allocator			render_allocator;


class dxRender_Visual;

// #define	USE_RESOURCE_DEBUGGER

namespace	R_dsgraph
{
	// Elementary types
	struct _NormalItem	{
		float				ssa;
		dxRender_Visual*		pVisual;
	};

	struct _MatrixItem	{
		float				ssa;
		IRenderable*		pObject;
		dxRender_Visual*		pVisual;
		Fmatrix				Matrix;				// matrix (copy)
	};

	struct _MatrixItemS	: public _MatrixItem
	{
		ShaderElement*		se;
	};

	struct _LodItem		{
		float				ssa;
		dxRender_Visual*		pVisual;
	};

#ifdef USE_RESOURCE_DEBUGGER
	typedef	ref_vs						vs_type;
	typedef	ref_ps						ps_type;
	#ifdef	USE_DX10
		typedef	ref_gs						gs_type;
	#endif	//	USE_DX10
#else
	#ifdef	USE_DX10	//	DX10 needs shader signature to propperly bind deometry to shader
		typedef	SVS*					vs_type;
		typedef	ID3DGeometryShader*		gs_type;
	#else	//	USE_DX10
		typedef	ID3DVertexShader*		vs_type;
	#endif	//	USE_DX10
		typedef	ID3DPixelShader*		ps_type;
#endif

	// NORMAL
	typedef xr_vector<_NormalItem,render_allocator::helper<_NormalItem>::result>			mapNormalDirect;
	struct	mapNormalItems		: public	mapNormalDirect										{	float	ssa;	};
	struct	mapNormalTextures	: public	FixedMAP<STextureList*,mapNormalItems,render_allocator>				{	float	ssa;	};
	struct	mapNormalStates		: public	FixedMAP<ID3DState*,mapNormalTextures,render_allocator>	{	float	ssa;	};
	struct	mapNormalCS			: public	FixedMAP<R_constant_table*,mapNormalStates,render_allocator>			{	float	ssa;	};
	struct	mapNormalPS			: public	FixedMAP<ps_type, mapNormalCS,render_allocator>						{	float	ssa;	};
#ifdef	USE_DX10
	struct	mapNormalGS			: public	FixedMAP<gs_type, mapNormalPS,render_allocator>						{	float	ssa;	};
	struct	mapNormalVS			: public	FixedMAP<vs_type, mapNormalGS,render_allocator>						{	};
#else	//	USE_DX10
	struct	mapNormalVS			: public	FixedMAP<vs_type, mapNormalPS,render_allocator>						{	};
#endif	//	USE_DX10
	typedef mapNormalVS			mapNormal_T;
	typedef mapNormal_T			mapNormalPasses_T[SHADER_PASSES_MAX];

	// MATRIX
	typedef xr_vector<_MatrixItem,render_allocator::helper<_MatrixItem>::result>	mapMatrixDirect;
	struct	mapMatrixItems		: public	mapMatrixDirect										{	float	ssa;	};
	struct	mapMatrixTextures	: public	FixedMAP<STextureList*,mapMatrixItems,render_allocator>				{	float	ssa;	};
	struct	mapMatrixStates		: public	FixedMAP<ID3DState*,mapMatrixTextures,render_allocator>	{	float	ssa;	};
	struct	mapMatrixCS			: public	FixedMAP<R_constant_table*,mapMatrixStates,render_allocator>			{	float	ssa;	};
	struct	mapMatrixPS			: public	FixedMAP<ps_type, mapMatrixCS,render_allocator>						{	float	ssa;	};
#ifdef	USE_DX10
	struct	mapMatrixGS			: public	FixedMAP<gs_type, mapMatrixPS,render_allocator>						{	float	ssa;	};
	struct	mapMatrixVS			: public	FixedMAP<vs_type, mapMatrixGS,render_allocator>						{	};
#else	//	USE_DX10
	struct	mapMatrixVS			: public	FixedMAP<vs_type, mapMatrixPS,render_allocator>						{	};
#endif	//	USE_DX10
	typedef mapMatrixVS			mapMatrix_T;
	typedef mapMatrix_T			mapMatrixPasses_T[SHADER_PASSES_MAX];

	// Top level
	typedef FixedMAP<float,_MatrixItemS,render_allocator>			mapSorted_T;
	typedef mapSorted_T::TNode						mapSorted_Node;

	typedef FixedMAP<float,_MatrixItemS,render_allocator>			mapHUD_T;
	typedef mapHUD_T::TNode							mapHUD_Node;

	typedef FixedMAP<float,_LodItem,render_allocator>				mapLOD_T;
	typedef mapLOD_T::TNode							mapLOD_Node;
};
