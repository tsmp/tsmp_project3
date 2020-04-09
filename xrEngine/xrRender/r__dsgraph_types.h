#pragma once
#include "fixedmap.h"

#define render_alloc xalloc

using render_allocator = xr_allocator;

// #define	USE_RESOURCE_DEBUGGER

namespace R_dsgraph // Elementary types
{	
	struct _NormalItem	
	{
		float ssa;
		IRender_Visual *pVisual;
	};

	struct _MatrixItem	
	{
		float ssa;
		IRenderable *pObject;
		IRender_Visual *pVisual;
		Fmatrix Matrix;				// matrix (copy)
	};

	struct _MatrixItemS	: public _MatrixItem
	{
		ShaderElement *se;
	};

	struct _LodItem		
	{
		float ssa;
		IRender_Visual *pVisual;
	};

#ifdef USE_RESOURCE_DEBUGGER
	using vs_type = ref_vs;
	using ps_type = ref_ps;
#else
	using vs_type = IDirect3DVertexShader9 * ;
	using ps_type = IDirect3DPixelShader9 * ;
#endif

	// NORMAL
	using mapNormalDirect = xr_vector<_NormalItem, render_allocator::helper<_NormalItem>::result>;

	struct mapNormalItems: public mapNormalDirect { float ssa; };
	struct mapNormalTextures: public FixedMAP<STextureList*,mapNormalItems,render_allocator> { float ssa; };
	struct mapNormalStates: public FixedMAP<IDirect3DStateBlock9*,mapNormalTextures,render_allocator> { float ssa; };
	struct mapNormalCS: public FixedMAP<R_constant_table*,mapNormalStates,render_allocator> { float ssa; };
	struct mapNormalPS: public FixedMAP<ps_type, mapNormalCS,render_allocator> { float	ssa; };
	struct mapNormalVS: public FixedMAP<vs_type, mapNormalPS,render_allocator> {};

	// MATRIX
	using mapMatrixDirect = xr_vector<_MatrixItem, render_allocator::helper<_MatrixItem>::result>;

	struct mapMatrixItems: public mapMatrixDirect { float ssa; };
	struct mapMatrixTextures: public FixedMAP<STextureList*,mapMatrixItems,render_allocator> { float ssa; };
	struct mapMatrixStates: public FixedMAP<IDirect3DStateBlock9*,mapMatrixTextures,render_allocator> { float ssa; };
	struct mapMatrixCS: public FixedMAP<R_constant_table*,mapMatrixStates,render_allocator> { float ssa; };
	struct mapMatrixPS: public FixedMAP<ps_type, mapMatrixCS,render_allocator> { float ssa; };
	struct mapMatrixVS: public FixedMAP<vs_type, mapMatrixPS,render_allocator> {};
	
	// Top level
	using mapSorted_T = FixedMAP<float, _MatrixItemS, render_allocator>;
	using mapSorted_Node = mapSorted_T::TNode;

	using mapHUD_T = FixedMAP<float, _MatrixItemS, render_allocator>;
	using mapHUD_Node = mapHUD_T::TNode;

	using mapLOD_T = FixedMAP<float, _LodItem, render_allocator>;
	using mapLOD_Node = mapLOD_T::TNode;
};
