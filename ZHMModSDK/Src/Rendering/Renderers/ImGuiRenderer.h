#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>

#include "Hooks.h"

struct ImFont;

namespace Rendering::Renderers
{
	class ImGuiRenderer
	{
	private:
		struct FrameContext
		{
			size_t Index = 0;

			ID3D12CommandAllocator* CommandAllocator = nullptr;
			ID3D12GraphicsCommandList* CommandList = nullptr;

			ID3D12Resource* BackBuffer = nullptr;
			D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle = { 0 };

			ID3D12Fence* Fence = nullptr;
			HANDLE FenceEvent = nullptr;
			volatile uint64_t FenceValue = 0;

			volatile bool Recording = false;
		};

	public:
		static void Init();
		static void OnEngineInit();
		static void Shutdown();
		
	public:
		static void OnPresent(IDXGISwapChain3* p_SwapChain);
		static void SetCommandQueue(ID3D12CommandQueue* p_CommandQueue);
		static void OnReset();

	public:
		static ImFont* GetFontLight() { return m_FontLight; }
		static ImFont* GetFontRegular() { return m_FontRegular; }
		static ImFont* GetFontMedium() { return m_FontMedium; }
		static ImFont* GetFontBold() { return m_FontBold; }
		static ImFont* GetFontBlack() { return m_FontBlack; }

		static void SetFocus(bool p_Focused) { m_ImguiHasFocus = p_Focused; }
		
	private:
		static bool SetupRenderer(IDXGISwapChain3* p_SwapChain);
		static void WaitForGpu(FrameContext* p_Frame);
		static void ExecuteCmdList(FrameContext* p_Frame);
		static void Draw();
		static void SetupStyles();

	private:
		DEFINE_STATIC_DETOUR(LRESULT, WndProc, ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM);
		DEFINE_STATIC_DETOUR(void, ZKeyboardWindows_Update, ZKeyboardWindows*, bool);

	private:
		static bool m_RendererSetup;
		static bool m_ImguiInitialized;

		static UINT m_BufferCount;
		static ID3D12DescriptorHeap* m_RtvDescriptorHeap;
		static ID3D12DescriptorHeap* m_SrvDescriptorHeap;
		//static ID3D12CommandQueue* m_CommandQueue;
		static FrameContext* m_FrameContext;
		static IDXGISwapChain3* m_SwapChain;
		static HWND m_Hwnd;
		static bool m_Shutdown;

		static int64_t m_Time;
		static int64_t m_TicksPerSecond;

		static ImFont* m_FontLight;
		static ImFont* m_FontRegular;
		static ImFont* m_FontMedium;
		static ImFont* m_FontBold;
		static ImFont* m_FontBlack;

		static volatile bool m_ImguiHasFocus;

		static SRWLOCK m_Lock;
	};
}
