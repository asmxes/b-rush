#include "overlay.hpp"
// #include <d3d11.h>
#include <d3dx11.h>
#pragma comment(lib, "d3d11.lib")
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler (HWND hWnd, UINT msg, WPARAM wParam,
				LPARAM lParam);

void
hook ();
void
unhook ();

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
}

void
cleanup_render_target ()
{
  if (render_target_view)
    {
      render_target_view->Release ();
      render_target_view = nullptr;
    }
}

LRESULT CALLBACK
wnd_proc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  if (wparam == VK_DELETE)
    {
      quit_requested = true;
    }
  // TODO: respond to injector challenge

  // TODO: publish event for keypresses
  if (ImGui_ImplWin32_WndProcHandler (hwnd, msg, wparam, lparam))
    return true;
  return CallWindowProc (o_wnd_proc, hwnd, msg, wparam, lparam);
}

HRESULT __stdcall
hk_present (IDXGISwapChain *swap_chain, UINT sync_internal, UINT flags)
{
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

      // push_imgui_styles ();

      imgui_init = true;
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
      RY_PUBLISH ("render", nullptr);
      RY_PUBLISH ("render_menu", nullptr);
      ImGui::ShowDemoWindow ();

      ImGui::End ();
    }
    ImGui::Render ();
  }

  d3d_context->OMSetRenderTargets (1, &render_target_view, nullptr);
  ImGui_ImplDX11_RenderDrawData (ImGui::GetDrawData ());

  auto result = o_present (swap_chain, sync_internal, flags);

  if (quit_requested)
    {
      unhook ();
      can_quit = true;
    }

  return result;
}

HRESULT __stdcall
hk_resize (IDXGISwapChain *swap_chain, UINT buffer_count, UINT w, UINT h,
	   DXGI_FORMAT new_format, UINT flags)
{
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
hook ()
{
  if (is_hooked)
    return;

  DXGI_SWAP_CHAIN_DESC scd = {};
  scd.BufferCount = 1;
  scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  scd.OutputWindow = GetForegroundWindow (); // Replace if needed
  scd.SampleDesc.Count = 1;
  scd.Windowed = TRUE;
  scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  ID3D11Device *device = nullptr;
  ID3D11DeviceContext *context = nullptr;
  IDXGISwapChain *swap_chain = nullptr;

  D3D_FEATURE_LEVEL featureLevel;
  D3D11CreateDeviceAndSwapChain (nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
				 nullptr, 0, D3D11_SDK_VERSION, &scd,
				 &swap_chain, &device, &featureLevel, &context);

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
}

void
unhook ()
{
  if (!is_hooked)
    return;

  if (imgui_init)
    {
      ImGui_ImplDX11_Shutdown ();
      ImGui_ImplWin32_Shutdown ();
      ImGui::DestroyContext ();
      cleanup_render_target ();
      SetWindowLongPtr (hwnd, GWLP_WNDPROC, (LONG_PTR) o_wnd_proc);

      imgui_init = false;
    }

  hook_vtable (static_cast<void **> (swapchain_vtable), 8,
	       reinterpret_cast<void *> (o_present), nullptr);
  hook_vtable (static_cast<void **> (swapchain_vtable), 13,
	       reinterpret_cast<void *> (o_resize), nullptr);
  is_hooked = false;
}

std::string
overlay::get_name ()
{
  return "Chalkboard Overlay Module DX11";
}

void
overlay::start ()
{
  hook ();
};

void
overlay::update () {};

void
overlay::stop ()
{
  unhook ();
};

overlay *
overlay::get ()
{
  static overlay object{};
  return &object;
}