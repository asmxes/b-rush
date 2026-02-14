#include "hook.hpp"
#include <d3dx11.h>
#pragma comment(lib, "d3d11.lib")
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#include "overlay/draw/draw.hpp"
#include "utility/logger/logger.hpp"
#include "utility/event/event.hpp"

#include <chrono>
#include <thread>

extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler (HWND hWnd, UINT msg, WPARAM wParam,
				LPARAM lParam);

namespace overlay {

void
hook_impl ();
void
unhook_impl ();

typedef HRESULT (__stdcall *present_fn) (IDXGISwapChain *, UINT, UINT);
typedef HRESULT (__stdcall *resize_fn) (IDXGISwapChain *, UINT, UINT, UINT,
					DXGI_FORMAT, UINT);

present_fn o_present = nullptr;
resize_fn o_resize = nullptr;

ptr swapchain_vtable = nullptr;
ID3D11Device *d3d_device = nullptr;
ID3D11DeviceContext *d3d_context = nullptr;
ID3D11RenderTargetView *render_target_view = nullptr;
HWND hwnd = nullptr;
WNDPROC o_wnd_proc = nullptr;
bool imgui_init = false;
bool quit_requested = false;
bool can_quit = false;
bool is_hooked = false;

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
  d3d_device->CreateRenderTargetView (pBackBuffer, nullptr,
				      &render_target_view);
  pBackBuffer->Release ();
  INFO ("Created RenderTarget object");
}

void
cleanup_render_target ()
{
  if (render_target_view)
    {
      render_target_view->Release ();
      render_target_view = nullptr;
      WARNING ("Cleaned RenderTarget object");
    }
}

LRESULT CALLBACK
wnd_proc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  // TRACE ("DX11 WNDPROC called, msg: " + std::to_string (msg) + ", wparam: "
  // + std::to_string (wparam) + ", lparam: " + std::to_string (lparam));

  if (wparam == VK_SHIFT)
    {
      quit_requested = true;
    }
  // TODO: respond to injector challenge

  PUBLISH (utility::event::id::wnd_proc, msg, wparam, lparam);

  // if (inst->wants_input ())
  //   return true;

  if (ImGui_ImplWin32_WndProcHandler (hwnd, msg, wparam, lparam))
    return true;
  return CallWindowProc (o_wnd_proc, hwnd, msg, wparam, lparam);
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
  // return o_present (swap_chain, sync_internal, flags);

  if (!imgui_init)
    {
      swapchain_vtable = *reinterpret_cast<uintptr_t **> (swap_chain);
      swap_chain->GetDevice (__uuidof (ID3D11Device), (void **) &d3d_device);
      d3d_device->GetImmediateContext (&d3d_context);

      DXGI_SWAP_CHAIN_DESC sd;
      swap_chain->GetDesc (&sd);
      hwnd = sd.OutputWindow;
      o_wnd_proc
	= (WNDPROC) SetWindowLongPtr (hwnd, GWLP_WNDPROC, (LONG_PTR) wnd_proc);

      ImGui::CreateContext ();
      ImGui_ImplWin32_Init (hwnd);
      ImGui_ImplDX11_Init (d3d_device, d3d_context);
      create_render_target (swap_chain);

      push_imgui_styles ();

      imgui_init = true;
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

  d3d_context->OMSetRenderTargets (1, &render_target_view, nullptr);
  ImGui_ImplDX11_RenderDrawData (ImGui::GetDrawData ());

  auto result = o_present (swap_chain, sync_internal, flags);

  if (quit_requested)
    {
      unhook_impl ();
      can_quit = true;
    }

  return result;
}

HRESULT __stdcall
hk_resize (IDXGISwapChain *swap_chain, UINT buffer_count, UINT w, UINT h,
	   DXGI_FORMAT new_format, UINT flags)
{
  TRACE ("DX11 Resize called, {}:{}, flags {} ", w, h, flags);

  // return o_resize (swap_chain, buffer_count, w, h, new_format, flags);

  if (imgui_init)
    {
      cleanup_render_target ();
      auto result
	= o_resize (swap_chain, buffer_count, w, h, new_format, flags);
      create_render_target (swap_chain);
      return result;
    }

  return o_resize (swap_chain, buffer_count, w, h, new_format, flags);
}

void
hook_impl ()
{
  if (is_hooked)
    return;

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

      if (SUCCEEDED (result))
	break;

      WARNING ("Failed to create dummy device and swapchain, returned: {}",
	       result);
      std::this_thread::sleep_for (std::chrono::seconds (1));
    }
  if (!swap_chain)
    return;

  swapchain_vtable = *reinterpret_cast<uintptr_t **> (swap_chain);
  hook_vtable (static_cast<void **> (swapchain_vtable), 8,
	       reinterpret_cast<void *> (&hk_present),
	       reinterpret_cast<void **> (&o_present));
  hook_vtable (static_cast<void **> (swapchain_vtable), 13,
	       reinterpret_cast<void *> (&hk_resize),
	       reinterpret_cast<void **> (&o_resize));

  swap_chain->Release ();
  device->Release ();
  context->Release ();
  is_hooked = true;

  INFO ("Hooked PresentScene and ResizeBuffers DX11 virtual functions");
}

void
unhook_impl ()
{
  if (!is_hooked)
    return;

  hook_vtable (static_cast<void **> (swapchain_vtable), 8,
	       reinterpret_cast<void *> (o_present), nullptr);
  hook_vtable (static_cast<void **> (swapchain_vtable), 13,
	       reinterpret_cast<void *> (o_resize), nullptr);

  if (imgui_init)
    {
      SetWindowLongPtr (hwnd, GWLP_WNDPROC, (LONG_PTR) o_wnd_proc);

      cleanup_render_target ();

      ImGui_ImplDX11_Shutdown ();
      ImGui_ImplWin32_Shutdown ();
      ImGui::DestroyContext ();

      if (d3d_device)
	{
	  d3d_device->Release ();
	  d3d_device = nullptr;
	}
      if (d3d_context)
	{
	  d3d_context->ClearState ();
	  d3d_context->Flush ();
	  d3d_context->Release ();
	  d3d_context = nullptr;
	}
      imgui_init = false;
      INFO ("UnInitialized ImGui and DirectX objects");
    }

  is_hooked = false;
  INFO ("UnHooked PresentScene and ResizeBuffers DX11 virtual functions");
}

HWND
hook::get_hwnd ()
{
  return hwnd;
}

bool
hook::is_rendering ()
{
  return !can_quit;
}

hook::hook ()
{
  INFO ("Starting overlay");
  hook_impl ();
}

hook::~hook ()
{
  INFO ("Unhooking and destroying overlay");
  unhook_impl ();
}

hook *
hook::get ()
{
  static hook instance;
  return &instance;
}

} // namespace overlay
