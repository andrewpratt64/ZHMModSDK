#include "DirectXTKRenderer.h"

#include <d3dcompiler.h>
#include <dxgi1_4.h>

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZApplicationEngineWin32.h>

#include <UI/Console.h>

#include "CommonStates.h"
#include "ResourceUploadBatch.h"
#include "DirectXHelpers.h"
#include "Functions.h"
#include "Globals.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZActor.h"
#include "Glacier/ZRender.h"
#include "Glacier/ZCameraEntity.h"
#include "Rendering/D3DUtils.h"
#include "Fonts.h"

using namespace Rendering::Renderers;

bool DirectXTKRenderer::m_RendererSetup = false;
UINT DirectXTKRenderer::m_BufferCount = 0;
ID3D12DescriptorHeap* DirectXTKRenderer::m_RtvDescriptorHeap = nullptr;
ID3D12CommandQueue* DirectXTKRenderer::m_CommandQueue = nullptr;
DirectXTKRenderer::FrameContext* DirectXTKRenderer::m_FrameContext = nullptr;
IDXGISwapChain3* DirectXTKRenderer::m_SwapChain = nullptr;
HWND DirectXTKRenderer::m_Hwnd = nullptr;
float DirectXTKRenderer::m_WindowWidth = 1.f;
float DirectXTKRenderer::m_WindowHeight = 1.f;
bool DirectXTKRenderer::m_Shutdown = false;

std::unique_ptr<DirectX::GraphicsMemory> DirectXTKRenderer::m_GraphicsMemory;
std::unique_ptr<DirectX::BasicEffect> DirectXTKRenderer::m_LineEffect;
std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> DirectXTKRenderer::m_LineBatch;
DirectX::SimpleMath::Matrix DirectXTKRenderer::m_World;
DirectX::SimpleMath::Matrix DirectXTKRenderer::m_View;
DirectX::SimpleMath::Matrix DirectXTKRenderer::m_Projection;
DirectX::SimpleMath::Matrix DirectXTKRenderer::m_ViewProjection;

std::unique_ptr<DirectX::DescriptorHeap> DirectXTKRenderer::m_ResourceDescriptors;
std::unique_ptr<DirectX::SpriteFont> DirectXTKRenderer::m_Font;
std::unique_ptr<DirectX::SpriteBatch> DirectXTKRenderer::m_SpriteBatch;

void DirectXTKRenderer::Init()
{
	Hooks::ZRenderGraphNodeCamera_Unknown01->AddDetour(nullptr, &DirectXTKRenderer::ZRenderGraphNodeCamera_Unknown01);
}

void DirectXTKRenderer::OnEngineInit()
{
}

void DirectXTKRenderer::Shutdown()
{
	m_Shutdown = true;

	Hooks::ZRenderGraphNodeCamera_Unknown01->RemoveDetour(&DirectXTKRenderer::ZRenderGraphNodeCamera_Unknown01);

	OnReset();
}

DECLARE_STATIC_DETOUR(DirectXTKRenderer, bool, ZRenderGraphNodeCamera_Unknown01, void* a1)
{
	m_View = *reinterpret_cast<DirectX::FXMMATRIX*>(&Globals::RenderManager->m_pRenderContext->m_mWorldToView);
	m_Projection = *reinterpret_cast<DirectX::FXMMATRIX*>(&Globals::RenderManager->m_pRenderContext->m_mViewToProjection);

	m_ViewProjection = m_View * m_Projection;

	m_LineEffect->SetView(m_View);
	m_LineEffect->SetProjection(m_Projection);

	return HookResult<bool>(HookAction::Continue());
}

void DirectXTKRenderer::Draw(FrameContext* p_Frame)
{
	ID3D12DescriptorHeap* s_Heaps[] = { m_ResourceDescriptors->Heap() };
	p_Frame->CommandList->SetDescriptorHeaps(static_cast<UINT>(std::size(s_Heaps)), s_Heaps);

	m_SpriteBatch->Begin(p_Frame->CommandList);

	m_LineEffect->Apply(p_Frame->CommandList);

	m_LineBatch->Begin(p_Frame->CommandList);

	if (Globals::ActorManager && Globals::NextActorId && *Globals::NextActorId > 0)
	{
		for (int i = 0; i < *Globals::NextActorId; ++i)
		{
			auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

			ZEntityRef s_ActorRef;
			s_Actor->GetID(&s_ActorRef);

			auto* s_SpatialEntity = s_ActorRef.QueryInterface<ZSpatialEntity>();
			auto* s_RepoEntity = s_ActorRef.QueryInterface<ZRepositoryItemEntity>();

			SMatrix s_Transform;
			Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

			DirectX::SimpleMath::Vector3 from(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z);
			DirectX::SimpleMath::Vector3 to(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2);

			DirectX::VertexPositionColor v1(from, DirectX::Colors::Red);
			DirectX::VertexPositionColor v2(to, DirectX::Colors::White);
			m_LineBatch->DrawLine(v1, v2);

			{
				DirectX::SimpleMath::Vector4 s_World(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.f, 1.f);
				const DirectX::SimpleMath::Vector4 s_Projected = DirectX::XMVector4Transform(s_World, m_ViewProjection);

				if (s_Projected.w > 0.000001f)
				{
					float s_InvertedZ = 1.f / s_Projected.w;

					DirectX::SimpleMath::Vector3 s_FinalProjected(s_Projected.x * s_InvertedZ, s_Projected.y * s_InvertedZ, s_Projected.z * s_InvertedZ);

					float s_ScreenX = (1.f + s_FinalProjected.x) * 0.5f * m_WindowWidth;
					float s_ScreenY = (1.f - s_FinalProjected.y) * 0.5f * m_WindowHeight;

					std::string s_Name(s_Actor->m_sActorName.c_str(), s_Actor->m_sActorName.size());

					DirectX::SimpleMath::Vector2 s_Origin = m_Font->MeasureString(s_Name.c_str());
					s_Origin /= 2.f;

					m_Font->DrawString(m_SpriteBatch.get(), s_Name.c_str(), DirectX::SimpleMath::Vector2(s_ScreenX, s_ScreenY), DirectX::Colors::Red, 0.f, s_Origin, 0.4f);
				}
			}

			{
				DirectX::SimpleMath::Vector4 s_World(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.1f, 1.f);
				const DirectX::SimpleMath::Vector4 s_Projected = DirectX::XMVector4Transform(s_World, m_ViewProjection);

				if (s_Projected.w > 0.000001f)
				{
					float s_InvertedZ = 1.f / s_Projected.w;

					DirectX::SimpleMath::Vector3 s_FinalProjected(s_Projected.x * s_InvertedZ, s_Projected.y * s_InvertedZ, s_Projected.z * s_InvertedZ);

					float s_ScreenX = (1.f + s_FinalProjected.x) * 0.5f * m_WindowWidth;
					float s_ScreenY = (1.f - s_FinalProjected.y) * 0.5f * m_WindowHeight;

					std::string s_Name = s_RepoEntity->m_sId.ToString();

					DirectX::SimpleMath::Vector2 s_Origin = m_Font->MeasureString(s_Name.c_str());
					s_Origin /= 2.f;

					m_Font->DrawString(m_SpriteBatch.get(), s_Name.c_str(), DirectX::SimpleMath::Vector2(s_ScreenX, s_ScreenY), DirectX::Colors::Red, 0.f, s_Origin, 0.4f);
				}
			}
		}
	}

	m_LineBatch->End();

	m_SpriteBatch->End();
}

void DirectXTKRenderer::OnPresent(IDXGISwapChain3* p_SwapChain)
{
	if (m_Shutdown)
		return;

	if (!SetupRenderer(p_SwapChain))
	{
		Logger::Error("Failed to set up DirectXTK renderer.");
		OnReset();
		return;
	}

	if (m_SwapChain != p_SwapChain)
		return;

	if (!m_CommandQueue)
		return;

	const auto s_BackBufferIndex = p_SwapChain->GetCurrentBackBufferIndex();
	auto& s_Frame = m_FrameContext[s_BackBufferIndex];

	if (s_Frame.Fence->GetCompletedValue() < s_Frame.FenceValue)
	{
		if (SUCCEEDED(s_Frame.Fence->SetEventOnCompletion(s_Frame.FenceValue, s_Frame.FenceEvent)))
		{
			WaitForSingleObject(s_Frame.FenceEvent, INFINITE);
		}
	}

	if (!s_Frame.Recording)
	{
		s_Frame.CommandAllocator->Reset();

		s_Frame.Recording = SUCCEEDED(s_Frame.CommandList->Reset(s_Frame.CommandAllocator, nullptr));

		if (s_Frame.Recording)
			s_Frame.CommandList->SetPredication(nullptr, 0, D3D12_PREDICATION_OP_EQUAL_ZERO);

		if (!s_Frame.Recording)
		{
			Logger::Warn("Could not reset command list of frame {}.", s_BackBufferIndex);
			return;
		}
	}

	{
		D3D12_RESOURCE_BARRIER s_Barrier = {};
		s_Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		s_Barrier.Transition.pResource = s_Frame.BackBuffer;
		s_Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		s_Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		s_Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		s_Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		s_Frame.CommandList->ResourceBarrier(1, &s_Barrier);
	}

	D3D12_VIEWPORT s_Viewport = { 0.0f, 0.0f, m_WindowWidth, m_WindowHeight, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	s_Frame.CommandList->RSSetViewports(1, &s_Viewport);

	D3D12_RECT s_ScissorRect = { 0, 0, static_cast<LONG>(m_WindowWidth), static_cast<LONG>(m_WindowHeight) };
	s_Frame.CommandList->RSSetScissorRects(1, &s_ScissorRect);

	s_Frame.CommandList->OMSetRenderTargets(1, &s_Frame.DescriptorHandle, false, nullptr);

	Draw(&s_Frame);

	{
		D3D12_RESOURCE_BARRIER s_Barrier = {};
		s_Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		s_Barrier.Transition.pResource = s_Frame.BackBuffer;
		s_Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		s_Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		s_Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		s_Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		s_Frame.CommandList->ResourceBarrier(1, &s_Barrier);
	}

	ExecuteCmdList(&s_Frame);

	const auto s_SyncValue = s_Frame.FenceValue + 1;

	if (SUCCEEDED(m_CommandQueue->Signal(s_Frame.Fence, s_SyncValue)))
		s_Frame.FenceValue = s_SyncValue;
}

void DirectXTKRenderer::PostPresent(IDXGISwapChain3* p_SwapChain)
{
	if (m_Shutdown)
		return;

	if (!m_CommandQueue || !m_SwapChain)
		return;

	m_GraphicsMemory->Commit(m_CommandQueue);
}

bool DirectXTKRenderer::SetupRenderer(IDXGISwapChain3* p_SwapChain)
{
	if (m_RendererSetup)
		return true;

	Logger::Debug("Setting up DirectXTK renderer.");

	m_SwapChain = p_SwapChain;

	ScopedD3DRef<ID3D12Device> s_Device;

	if (p_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK)
		return false;

	DXGI_SWAP_CHAIN_DESC1 s_SwapChainDesc;

	if (p_SwapChain->GetDesc1(&s_SwapChainDesc) != S_OK)
		return false;

	m_BufferCount = s_SwapChainDesc.BufferCount;
	m_FrameContext = new FrameContext[m_BufferCount];

	{
		D3D12_DESCRIPTOR_HEAP_DESC s_Desc = {};
		s_Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		s_Desc.NumDescriptors = m_BufferCount;
		s_Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		s_Desc.NodeMask = 0;

		if (s_Device->CreateDescriptorHeap(&s_Desc, IID_PPV_ARGS(&m_RtvDescriptorHeap)) != S_OK)
			return false;

		D3D_SET_OBJECT_NAME_A(m_RtvDescriptorHeap, "ZHMModSDK DirectXTK Rtv Descriptor Heap");
	}

	const auto s_RtvDescriptorSize = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for (size_t i = 0; i < m_BufferCount; ++i)
	{
		auto& s_Frame = m_FrameContext[i];

		s_Frame.Index = i;

		if (s_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&s_Frame.Fence)) != S_OK)
			return false;

		char s_FenceDebugName[128];
		sprintf_s(s_FenceDebugName, sizeof(s_FenceDebugName), "ZHMModSDK DirectXTK Fence #%llu", i);
		D3D_SET_OBJECT_NAME_A(s_Frame.Fence, s_FenceDebugName);

		if (s_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&s_Frame.CommandAllocator)) != S_OK)
			return false;

		char s_CmdAllocDebugName[128];
		sprintf_s(s_CmdAllocDebugName, sizeof(s_CmdAllocDebugName), "ZHMModSDK DirectXTK Command Allocator #%llu", i);
		D3D_SET_OBJECT_NAME_A(s_Frame.CommandAllocator, s_CmdAllocDebugName);

		if (s_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, s_Frame.CommandAllocator, nullptr, IID_PPV_ARGS(&s_Frame.CommandList)) != S_OK ||
			s_Frame.CommandList->Close() != S_OK)
			return false;

		char s_CmdListDebugName[128];
		sprintf_s(s_CmdListDebugName, sizeof(s_CmdListDebugName), "ZHMModSDK DirectXTK Command List #%llu", i);
		D3D_SET_OBJECT_NAME_A(s_Frame.CommandList, s_CmdListDebugName);

		if (p_SwapChain->GetBuffer(i, IID_PPV_ARGS(&s_Frame.BackBuffer)) != S_OK)
			return false;

		s_Frame.DescriptorHandle.ptr = s_RtvHandle.ptr + (i * s_RtvDescriptorSize);

		s_Device->CreateRenderTargetView(s_Frame.BackBuffer, nullptr, s_Frame.DescriptorHandle);

		s_Frame.FenceValue = 0;
		s_Frame.FenceEvent = CreateEventA(nullptr, false, false, nullptr);

		s_Frame.Recording = false;
	}

	if (p_SwapChain->GetHwnd(&m_Hwnd) != S_OK)
		return false;

	RECT s_Rect = { 0, 0, 0, 0 };
	GetClientRect(m_Hwnd, &s_Rect);

	m_WindowWidth = static_cast<float>(s_Rect.right - s_Rect.left);
	m_WindowHeight = static_cast<float>(s_Rect.bottom - s_Rect.top);

	m_GraphicsMemory = std::make_unique<DirectX::GraphicsMemory>(s_Device);

	m_LineBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(s_Device);

	{
		const DirectX::RenderTargetState s_RtState(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT);

		DirectX::EffectPipelineStateDescription s_Desc(
			&DirectX::VertexPositionColor::InputLayout,
			DirectX::CommonStates::Opaque,
			DirectX::CommonStates::DepthDefault,
			DirectX::CommonStates::CullNone,
			s_RtState,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

		m_LineEffect = std::make_unique<DirectX::BasicEffect>(s_Device, DirectX::EffectFlags::VertexColor, s_Desc);
	}

	{
		m_ResourceDescriptors = std::make_unique<DirectX::DescriptorHeap>(s_Device, static_cast<int>(Descriptors::Count));

		DirectX::ResourceUploadBatch resourceUpload(s_Device);

		resourceUpload.Begin();

		m_Font = std::make_unique<DirectX::SpriteFont>(
			s_Device,
			resourceUpload,
			RobotoRegularSpritefont_data,
			RobotoRegularSpritefont_size,
			m_ResourceDescriptors->GetCpuHandle(static_cast<int>(Descriptors::FontRegular)),
			m_ResourceDescriptors->GetGpuHandle(static_cast<int>(Descriptors::FontRegular))
			);

		DirectX::RenderTargetState rtState(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT);

		DirectX::SpriteBatchPipelineStateDescription pd(rtState);
		m_SpriteBatch = std::make_unique<DirectX::SpriteBatch>(s_Device, resourceUpload, pd);

		auto uploadResourcesFinished = resourceUpload.End(m_CommandQueue);

		uploadResourcesFinished.wait();

		D3D12_VIEWPORT viewport = { 0.0f, 0.0f, m_WindowWidth, m_WindowHeight, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
		m_SpriteBatch->SetViewport(viewport);
	}

	m_World = m_View = m_Projection = DirectX::SimpleMath::Matrix::Identity;

	m_LineEffect->SetWorld(m_World);
	m_LineEffect->SetView(m_View);
	m_LineEffect->SetProjection(m_Projection);

	m_RendererSetup = true;

	Logger::Debug("DirectXTK renderer successfully set up.");

	return true;
}

void DirectXTKRenderer::OnReset()
{
	Logger::Debug("Resetting DirectXTK renderer.");

	m_RendererSetup = false;

	m_SpriteBatch.reset();
	m_Font.reset();
	m_ResourceDescriptors.reset();

	m_LineEffect.reset();
	m_LineBatch.reset();

	m_GraphicsMemory.reset();

	for (UINT i = 0; i < m_BufferCount; i++)
	{
		auto& s_Frame = m_FrameContext[i];

		if (m_SwapChain && m_SwapChain->GetCurrentBackBufferIndex() != s_Frame.Index)
			s_Frame.Recording = false;

		WaitForGpu(&s_Frame);

		if (s_Frame.CommandList)
		{
			s_Frame.CommandList->Release();
			s_Frame.CommandList = nullptr;
		}

		if (s_Frame.CommandAllocator)
		{
			s_Frame.CommandAllocator->Release();
			s_Frame.CommandAllocator = nullptr;
		}

		if (s_Frame.BackBuffer)
		{
			s_Frame.BackBuffer->Release();
			s_Frame.BackBuffer = nullptr;
		}

		if (s_Frame.Fence)
		{
			s_Frame.Fence->Release();
			s_Frame.Fence = nullptr;
		}

		if (s_Frame.FenceEvent)
		{
			CloseHandle(s_Frame.FenceEvent);
			s_Frame.FenceEvent = nullptr;
		}

		s_Frame.FenceValue = 0;
		s_Frame.Recording = false;
		s_Frame.DescriptorHandle = { 0 };
	}

	delete[] m_FrameContext;
	m_FrameContext = nullptr;

	m_CommandQueue = nullptr;

	if (m_RtvDescriptorHeap)
	{
		m_RtvDescriptorHeap->Release();
		m_RtvDescriptorHeap = nullptr;
	}

	m_SwapChain = nullptr;
	m_BufferCount = 0;
	m_Hwnd = nullptr;
}

void DirectXTKRenderer::SetCommandQueue(ID3D12CommandQueue* p_CommandQueue)
{
	if (m_Shutdown)
		return;

	if (m_CommandQueue)
		return;

	m_CommandQueue = p_CommandQueue;
}

void DirectXTKRenderer::WaitForGpu(FrameContext* p_Frame)
{
	if (p_Frame->Recording)
		ExecuteCmdList(p_Frame);

	if (!p_Frame->FenceEvent || !m_CommandQueue)
		return;

	const auto s_SyncValue = p_Frame->FenceValue + 1;

	if (FAILED(m_CommandQueue->Signal(p_Frame->Fence, s_SyncValue)))
		return;

	if (SUCCEEDED(p_Frame->Fence->SetEventOnCompletion(s_SyncValue, p_Frame->FenceEvent)))
		WaitForSingleObject(p_Frame->FenceEvent, INFINITE);

	p_Frame->FenceValue = s_SyncValue;
}

void DirectXTKRenderer::ExecuteCmdList(FrameContext* p_Frame)
{
	if (FAILED(p_Frame->CommandList->Close()))
		return;

	p_Frame->Recording = false;

	ID3D12CommandList* const s_CommandLists[] = {
		p_Frame->CommandList,
	};

	m_CommandQueue->ExecuteCommandLists(1, s_CommandLists);
}