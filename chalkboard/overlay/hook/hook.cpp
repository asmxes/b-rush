#include "hook.hpp"
#include <d3dx11.h>
#pragma comment(lib, "d3d11.lib")
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#include "overlay/draw/draw.hpp"
#include "shared/logger/logger.hpp"
#include "utility/event/event.hpp"

#include <chrono>
#include <thread>

extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler (HWND hWnd, UINT msg, WPARAM wParam,
				LPARAM lParam);

namespace overlay {
namespace hook {

void
uninstall_impl ();

typedef HRESULT (__stdcall *present_fn) (IDXGISwapChain *, UINT, UINT);
typedef HRESULT (__stdcall *resize_fn) (IDXGISwapChain *, UINT, UINT, UINT,
					DXGI_FORMAT, UINT);

present_fn _o_present{};
resize_fn _o_resize{};

ptr _swapchain_vtable{};
ID3D11Device *_d3d_device{};
ID3D11DeviceContext *_d3d_context{};
ID3D11RenderTargetView *_render_target_view{};
HWND _hwnd{};
WNDPROC _o_wnd_proc{};
bool _is_hooked{};
bool _imgui_init{};
bool _quit_requested{};
bool _can_quit{};

void
hook_vtable (void **vtable, int index, void *hook_func, void **original_func)
{
  DWORD oldProtect;
  VirtualProtect (&vtable[index], sizeof (void *), PAGE_EXECUTE_READWRITE,
		  &oldProtect);
  if (original_func)
    *original_func = vtable[index];
  vtable[index] = hook_func;
  VirtualProtect (&vtable[index], sizeof (void *), oldProtect, &oldProtect);
}

void
create_render_target (IDXGISwapChain *swap_chain)
{
  ID3D11Texture2D *pBackBuffer = nullptr;
  swap_chain->GetBuffer (0, __uuidof (ID3D11Texture2D), (void **) &pBackBuffer);
  _d3d_device->CreateRenderTargetView (pBackBuffer, nullptr,
				       &_render_target_view);
  pBackBuffer->Release ();
  TRACE ("Created RenderTarget object");
}

void
cleanup_render_target ()
{
  if (_render_target_view)
    {
      _render_target_view->Release ();
      _render_target_view = nullptr;
      WARNING ("Cleaned RenderTarget object");
    }
}

LRESULT CALLBACK
wnd_proc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  if (msg == WM_KEYDOWN && wparam == VK_DELETE)
    {
      INFO ("Quit request by user");
      _quit_requested = true;
    }

  if (msg == WM_DESTROY || msg == WM_CLOSE)
    {
      INFO ("Window closing, quit requested");
      _quit_requested = true;
    }

  // TODO: respond to injector challenge

  PUBLISH (utility::event::id::wnd_proc, msg, wparam, lparam);

  if (ImGui_ImplWin32_WndProcHandler (hwnd, msg, wparam, lparam))
    return true;
  return CallWindowProc (_o_wnd_proc, hwnd, msg, wparam, lparam);
}

void
push_imgui_styles ()
{
  ImGui::GetIO ().LogFilename = nullptr;
  ImGui::GetIO ().IniFilename = nullptr;
  ImGui::GetIO ().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
}

HRESULT __stdcall
hk_present (IDXGISwapChain *swap_chain, UINT sync_internal, UINT flags)
{
  if (!_imgui_init)
    {
      _swapchain_vtable = *reinterpret_cast<uintptr_t **> (swap_chain);
      swap_chain->GetDevice (__uuidof (ID3D11Device), (void **) &_d3d_device);
      _d3d_device->GetImmediateContext (&_d3d_context);
      DXGI_SWAP_CHAIN_DESC sd;
      swap_chain->GetDesc (&sd);
      _hwnd = sd.OutputWindow;
      _o_wnd_proc
	= (WNDPROC) SetWindowLongPtr (_hwnd, GWLP_WNDPROC, (LONG_PTR) wnd_proc);
      ImGui::CreateContext ();
      ImGui_ImplWin32_Init (_hwnd);
      ImGui_ImplDX11_Init (_d3d_device, _d3d_context);
      create_render_target (swap_chain);

      push_imgui_styles ();

      _imgui_init = true;
      INFO ("Created ImGui context");
    }

  {
    ImGui_ImplDX11_NewFrame ();
    ImGui_ImplWin32_NewFrame ();
    ImGui::NewFrame ();
    {
      ImGui::SetNextWindowPos (ImVec2 (0, 0), ImGuiCond_Always);
      ImGui::SetNextWindowSize (ImVec2 (ImGui::GetIO ().DisplaySize.x,
					ImGui::GetIO ().DisplaySize.y),
				ImGuiCond_Always);
      ImGui::Begin ("##overlay", nullptr,
		    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
		      | ImGuiWindowFlags_NoSavedSettings
		      | ImGuiWindowFlags_NoInputs);

      // draw::line ({0, 0}, {200, 200}, 40.f, color ("#ff23ffff"));

      PUBLISH (utility::event::id::render, sync_internal);
      PUBLISH (utility::event::id::render_menu, sync_internal);

      // ImGui::ShowDemoWindow ();

      ImGui::End ();
    }
    ImGui::Render ();
  }

  _d3d_context->OMSetRenderTargets (1, &_render_target_view, nullptr);
  ImGui_ImplDX11_RenderDrawData (ImGui::GetDrawData ());

  auto result = _o_present (swap_chain, sync_internal, flags);

  if (_quit_requested)
    {
      uninstall_impl ();
      _can_quit = true;
    }

  return result;
}

HRESULT __stdcall
hk_resize (IDXGISwapChain *swap_chain, UINT buffer_count, UINT w, UINT h,
	   DXGI_FORMAT new_format, UINT flags)
{
  TRACE ("DX11 Resize called, {}:{}, flags {} ", w, h, flags);

  // return o_resize (swap_chain, buffer_count, w, h, new_format, flags);

  if (_imgui_init)
    {
      cleanup_render_target ();
      auto result
	= _o_resize (swap_chain, buffer_count, w, h, new_format, flags);
      create_render_target (swap_chain);
      return result;
    }

  return _o_resize (swap_chain, buffer_count, w, h, new_format, flags);
}

HWND
get_hwnd ()
{
  return _hwnd;
}

bool
is_rendering ()
{
  TRACE ("Checking");
  return _imgui_init && _is_hooked && !_can_quit;
}

bool
is_installed ()
{
  TRACE ("Checking");
  return _is_hooked & !_can_quit;
}

bool
install ()
{
  INFO ("Starting overlay");
  if (_is_hooked)
    return true;

  DXGI_SWAP_CHAIN_DESC scd = {};
  scd.BufferCount = 1;
  scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  scd.OutputWindow = FindWindow ("VALORANTUnrealWindow",
				 "VALORANT  "); // Replace if needed
  scd.SampleDesc.Count = 1;
  scd.Windowed = TRUE;
  scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  ID3D11Device *device = nullptr;
  ID3D11DeviceContext *context = nullptr;
  IDXGISwapChain *swap_chain = nullptr;

  D3D_FEATURE_LEVEL featureLevel;
  for (auto i = 0; i < 5; i++)
    {
      HRESULT result
	= D3D11CreateDeviceAndSwapChain (nullptr, D3D_DRIVER_TYPE_HARDWARE,
					 nullptr, 0, nullptr, 0,
					 D3D11_SDK_VERSION, &scd, &swap_chain,
					 &device, &featureLevel, &context);

      TRACE ("D3D11CreateDeviceAndSwapChain returned: {}", result);
      if (SUCCEEDED (result))
	break;

      WARNING ("Failed to create dummy device and swapchain, returned: {}",
	       result);
      std::this_thread::sleep_for (std::chrono::seconds (1));
    }

  if (!swap_chain)
    return false;

  _swapchain_vtable = *reinterpret_cast<uintptr_t **> (swap_chain);
  TRACE ("Swapchain VTABLE: {}", _swapchain_vtable);

  hook_vtable (static_cast<void **> (_swapchain_vtable), 8,
	       reinterpret_cast<void *> (&hk_present),
	       reinterpret_cast<void **> (&_o_present));
  TRACE ("Hooked present");
  hook_vtable (static_cast<void **> (_swapchain_vtable), 13,
	       reinterpret_cast<void *> (&hk_resize),
	       reinterpret_cast<void **> (&_o_resize));
  TRACE ("Hooked resize");

  swap_chain->Release ();
  device->Release ();
  context->Release ();
  _is_hooked = true;
  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  return true;
}

void
uninstall_impl ()
{
  TRACE ("Called");
  if (!_is_hooked)
    return;

  hook_vtable (static_cast<void **> (_swapchain_vtable), 8,
	       reinterpret_cast<void *> (_o_present), nullptr);
  hook_vtable (static_cast<void **> (_swapchain_vtable), 13,
	       reinterpret_cast<void *> (_o_resize), nullptr);

  if (_imgui_init)
    {
      SetWindowLongPtr (_hwnd, GWLP_WNDPROC, (LONG_PTR) _o_wnd_proc);

      cleanup_render_target ();

      ImGui_ImplDX11_Shutdown ();
      ImGui_ImplWin32_Shutdown ();
      ImGui::DestroyContext ();

      if (_d3d_device)
	{
	  _d3d_device->Release ();
	  _d3d_device = nullptr;
	}
      if (_d3d_context)
	{
	  _d3d_context->ClearState ();
	  _d3d_context->Flush ();
	  _d3d_context->Release ();
	  _d3d_context = nullptr;
	}
      _imgui_init = false;
      INFO ("UnInitialized ImGui and DirectX objects");
    }

  _is_hooked = false;
  INFO ("UnHooked PresentScene and ResizeBuffers DX11 virtual functions");
}

void
uninstall ()
{
  TRACE ("Requested overlay unhook");

  if (!is_rendering ())
    {
      TRACE ("Called while overlay is not rendering");
      uninstall_impl ();
    }

  while (is_rendering ())
    {
      TRACE ("Overlay is rendering, waiting for automatic unhook...");
      _quit_requested = true;
      std::this_thread::sleep_for (std::chrono::milliseconds (100));
    }
  TRACE ("Completed overlay unhook");
}

} // namespace hook

} // namespace overlay
