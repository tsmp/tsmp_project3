#pragma once

#ifdef XRAPI_EXPORTS
#define XRAPI_API __declspec(dllexport)
#else
#define XRAPI_API __declspec(dllimport)
#endif

//class IRender_interface;
//extern XRAPI_API IRender_interface* Render;

class IRenderFactory;
extern XRAPI_API IRenderFactory* RenderFactory;

class CDUInterface;
extern XRAPI_API CDUInterface* DU;

//Return this definition when HW moves to rendering
//struct xr_token;
//extern XRAPI_API xr_token* vid_mode_token;

class IUIRender;
extern XRAPI_API IUIRender* UIRender;

#ifdef DEBUG
class IDebugRender;
extern XRAPI_API IDebugRender* DRender;
#endif // DEBUG
