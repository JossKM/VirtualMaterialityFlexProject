// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
D3D12Device.cpp: D3D device RHI implementation.
=============================================================================*/
#include "D3D12RHIPrivate.h"


namespace D3D12RHI
{
	extern void EmptyD3DSamplerStateCache();
}
using namespace D3D12RHI;

FD3D12Device::FD3D12Device() :
	FD3D12Device(FRHIGPUMask::GPU0(), nullptr)
	{
	}

FD3D12Device::FD3D12Device(FRHIGPUMask InGPUMask, FD3D12Adapter* InAdapter) :
	FD3D12SingleNodeGPUObject(InGPUMask),
	FD3D12AdapterChild(InAdapter),
	CommandListManager(nullptr),
	CopyCommandListManager(nullptr),
	AsyncCommandListManager(nullptr),
	TextureStreamingCommandAllocatorManager(this, D3D12_COMMAND_LIST_TYPE_COPY),
	RTVAllocator(InGPUMask, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256),
	DSVAllocator(InGPUMask, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256),
	SRVAllocator(InGPUMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024),
	UAVAllocator(InGPUMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024),
#if USE_STATIC_ROOT_SIGNATURE
	CBVAllocator(InGPUMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2048),
#endif
	SamplerAllocator(InGPUMask, FD3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 128),
	GlobalSamplerHeap(this, InGPUMask),
	GlobalViewHeap(this, InGPUMask),
	OcclusionQueryHeap(this, D3D12_QUERY_HEAP_TYPE_OCCLUSION, 65536, 4 /*frames to keep results */ * 1 /*batches per frame*/),
	TimestampQueryHeap(this, D3D12_QUERY_HEAP_TYPE_TIMESTAMP, 8192, 4 /*frames to keep results */ * 5 /*batches per frame*/ ),
	DefaultBufferAllocator(this, InGPUMask), //Note: Cross node buffers are possible 
	SamplerID(0),
	DefaultFastAllocator(this, InGPUMask, D3D12_HEAP_TYPE_UPLOAD, 1024 * 1024 * 4),
	TextureAllocator(this, FRHIGPUMask::All())
{
	InitPlatformSpecific();
}

FD3D12Device::~FD3D12Device()
{
	// Cleanup the allocator near the end, as some resources may be returned to the allocator or references are shared by multiple GPUs
	DefaultBufferAllocator.FreeDefaultBufferPools();

	DefaultFastAllocator.Destroy<FD3D12ScopeLock>();

	TextureAllocator.CleanUpAllocations();
	TextureAllocator.Destroy();

	delete CommandListManager;
	delete CopyCommandListManager;
	delete AsyncCommandListManager;
}

ID3D12Device* FD3D12Device::GetDevice()
{
	return GetParentAdapter()->GetD3DDevice();
}

FD3D12DynamicRHI* FD3D12Device::GetOwningRHI()
{ 
	return GetParentAdapter()->GetOwningRHI();
}

// NVCHANGE_BEGIN: Add HBAO+
#if WITH_GFSDK_SSAO
void FD3D12DynamicRHI::CreateHBAOContext(FD3D12Device* InParent, FD3D12SubAllocatedOnlineHeap::SubAllocationDesc& SubHeapDesc)
{
	//Last sub-divided portion of GlobalHeap is used for HBAO context only
	//TODO: size(num of descriptors) of sub-divided heap is big for HBAO only, 
	ID3D12Device* device = this->GetRHIDevice()->GetDevice();
	FD3D12DynamicRHI* OwningRHI = InParent->GetOwningRHI();
	if (GMaxRHIFeatureLevel >= ERHIFeatureLevel::SM5)
	{
		FString HBAOBinariesPath = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/GameWorks/GFSDK_SSAO/");
#if PLATFORM_64BITS
		OwningRHI->HBAOModuleHandle = LoadLibraryW(*(HBAOBinariesPath + "GFSDK_SSAO_D3D12.win64.dll"));
#else
		OwningRHI->HBAOModuleHandle = LoadLibraryW(*(HBAOBinariesPath + "GFSDK_SSAO_D3D12.win32.dll"));
#endif
		check(OwningRHI->HBAOModuleHandle);

		//Create RTV Heap
		D3D12_DESCRIPTOR_HEAP_DESC D3D12DescritorHeapDesc = {};
		ID3D12DescriptorHeap* D3D12DescriptorHeap_RTV;
		D3D12DescritorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		D3D12DescritorHeapDesc.NumDescriptors = GFSDK_SSAO_NUM_DESCRIPTORS_RTV_HEAP_D3D12;
		D3D12DescritorHeapDesc.NodeMask = 1;
		D3D12DescritorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		VERIFYD3D12RESULT(device->CreateDescriptorHeap(&D3D12DescritorHeapDesc, IID_PPV_ARGS(&D3D12DescriptorHeap_RTV)));

		//Save Heap info for setting inputData for RenderAO in D3D12CommandContext
		OwningRHI->HBAODescriptorHeaps = new(GFSDK_SSAO_DescriptorHeaps_D3D12);
		GFSDK_SSAO_DescriptorHeaps_D3D12* DescriptorHeaps = OwningRHI->HBAODescriptorHeaps;
		DescriptorHeaps->CBV_SRV_UAV.pDescHeap = SubHeapDesc.ParentHeap->GetHeap();
		DescriptorHeaps->CBV_SRV_UAV.BaseIndex = SubHeapDesc.BaseSlot + 3; // +3 for dual depth(for dualLayerAO) and normal SRV
		DescriptorHeaps->CBV_SRV_UAV.NumDescriptors = GFSDK_SSAO_NUM_DESCRIPTORS_CBV_SRV_UAV_HEAP_D3D12;

		DescriptorHeaps->RTV.pDescHeap = D3D12DescriptorHeap_RTV;
		DescriptorHeaps->RTV.BaseIndex = 0;
		DescriptorHeaps->RTV.NumDescriptors = GFSDK_SSAO_NUM_DESCRIPTORS_RTV_HEAP_D3D12;

		GFSDK_SSAO_Status status;
		status = GFSDK_SSAO_CreateContext_D3D12(device, 1, *DescriptorHeaps, &(OwningRHI->HBAOContext));
		check(status == GFSDK_SSAO_OK);
	}
}
#endif
	// NVCHANGE_END: Add HBAO+

void FD3D12Device::CreateCommandContexts()
{
	check(CommandContextArray.Num() == 0);
	check(AsyncComputeContextArray.Num() == 0);

	const uint32 NumContexts = FTaskGraphInterface::Get().GetNumWorkerThreads() + 1;
	const uint32 NumAsyncComputeContexts = GEnableAsyncCompute ? 1 : 0;

	const uint32 TotalContexts = NumContexts + NumAsyncComputeContexts;


	// We never make the default context free for allocation by the context containers
	CommandContextArray.Reserve(NumContexts);
	FreeCommandContexts.Reserve(NumContexts - 1);
	AsyncComputeContextArray.Reserve(NumAsyncComputeContexts);

	// NVCHANGE_BEGIN: Add HBAO+
#if WITH_GFSDK_SSAO
	//Last portion of global view heap will be used for HBAO context
	const uint32 DescriptorSuballocationPerContext = (GlobalViewHeap.GetTotalSize() - GFSDK_SSAO_NUM_DESCRIPTORS_CBV_SRV_UAV_HEAP_D3D12)  / TotalContexts;
#else
	const uint32 DescriptorSuballocationPerContext = GlobalViewHeap.GetTotalSize() / TotalContexts;
#endif
	// NVCHANGE_END: Add HBAO+
	uint32 CurrentGlobalHeapOffset = 0;

	for (uint32 i = 0; i < NumContexts; ++i)
	{
		FD3D12SubAllocatedOnlineHeap::SubAllocationDesc SubHeapDesc(&GlobalViewHeap, CurrentGlobalHeapOffset, DescriptorSuballocationPerContext);

		const bool bIsDefaultContext = (i == 0);
		FD3D12CommandContext* NewCmdContext = GetOwningRHI()->CreateCommandContext(this, SubHeapDesc, bIsDefaultContext);
		CurrentGlobalHeapOffset += DescriptorSuballocationPerContext;

		// without that the first RHIClear would get a scissor rect of (0,0)-(0,0) which means we get a draw call clear 
		NewCmdContext->RHISetScissorRect(false, 0, 0, 0, 0);

		CommandContextArray.Add(NewCmdContext);

		// Make available all but the first command context for parallel threads
		if (!bIsDefaultContext)
		{
			FreeCommandContexts.Add(CommandContextArray[i]);
		}
	}

	for (uint32 i = 0; i < NumAsyncComputeContexts; ++i)
	{
		FD3D12SubAllocatedOnlineHeap::SubAllocationDesc SubHeapDesc(&GlobalViewHeap, CurrentGlobalHeapOffset, DescriptorSuballocationPerContext);

		const bool bIsDefaultContext = (i == 0); //-V547
		const bool bIsAsyncComputeContext = true;
		FD3D12CommandContext* NewCmdContext = GetOwningRHI()->CreateCommandContext(this, SubHeapDesc, bIsDefaultContext, bIsAsyncComputeContext);
		CurrentGlobalHeapOffset += DescriptorSuballocationPerContext;

		AsyncComputeContextArray.Add(NewCmdContext);
	}

// NVCHANGE_BEGIN: Add HBAO+
#if WITH_GFSDK_SSAO
	//HBAO context creation
	{
		FD3D12SubAllocatedOnlineHeap::SubAllocationDesc SubHeapDesc(&GlobalViewHeap, CurrentGlobalHeapOffset, DescriptorSuballocationPerContext);
		GetOwningRHI()->CreateHBAOContext(this, SubHeapDesc);
	}
#endif
// NVCHANGE_END: Add HBAO+

	CommandContextArray[0]->OpenCommandList();
	if (GEnableAsyncCompute)
	{
		AsyncComputeContextArray[0]->OpenCommandList();
	}
}

bool FD3D12Device::IsGPUIdle()
{
	FD3D12Fence& Fence = CommandListManager->GetFence();
	return Fence.IsFenceComplete(Fence.GetLastSignaledFence());
}

#if PLATFORM_WINDOWS
typedef HRESULT(WINAPI *FDXGIGetDebugInterface1)(UINT, REFIID, void **);
#endif


void FD3D12Device::SetupAfterDeviceCreation()
{
	ID3D12Device* Direct3DDevice = GetParentAdapter()->GetD3DDevice();

#if PLATFORM_WINDOWS
	// Check if we're running under GPU capture
	bool bUnderGPUCapture = false;

	// RenderDoc
	{
		IID RenderDocID;
		if (SUCCEEDED(IIDFromString(L"{A7AA6116-9C8D-4BBA-9083-B4D816B71B78}", &RenderDocID)))
		{
			TRefCountPtr<IUnknown> RenderDoc;
			if (SUCCEEDED(Direct3DDevice->QueryInterface(RenderDocID, (void**)RenderDoc.GetInitReference())))
			{
				// Running under RenderDoc, so enable capturing mode
				bUnderGPUCapture = true;
			}
		}
	}

	// AMD RGP profiler
	if (GEmitRgpFrameMarkers && GetOwningRHI()->GetAmdAgsContext())
	{
		// Running on AMD with RGP profiling enabled, so enable capturing mode
		bUnderGPUCapture = true;
	}
#if USE_PIX
	// PIX (note that DXGIGetDebugInterface1 requires Windows 8.1 and up)
	if (FWindowsPlatformMisc::VerifyWindowsVersion(6, 3))
	{
		FDXGIGetDebugInterface1 DXGIGetDebugInterface1FnPtr = nullptr;

		// CreateDXGIFactory2 is only available on Win8.1+, find it if it exists
		HMODULE DxgiDLL = LoadLibraryA("dxgi.dll");
		if (DxgiDLL)
		{
#pragma warning(push)
#pragma warning(disable: 4191) // disable the "unsafe conversion from 'FARPROC' to 'blah'" warning
			DXGIGetDebugInterface1FnPtr = (FDXGIGetDebugInterface1)(GetProcAddress(DxgiDLL, "DXGIGetDebugInterface1"));
#pragma warning(pop)
			FreeLibrary(DxgiDLL);
		}
		
		if (DXGIGetDebugInterface1FnPtr)
		{
			IID GraphicsAnalysisID;
			if (SUCCEEDED(IIDFromString(L"{9F251514-9D4D-4902-9D60-18988AB7D4B5}", &GraphicsAnalysisID)))
			{
				TRefCountPtr<IUnknown> GraphicsAnalysis;
				if (SUCCEEDED(DXGIGetDebugInterface1FnPtr(0, GraphicsAnalysisID, (void**)GraphicsAnalysis.GetInitReference())))
				{
					// Running under PIX, so enable capturing mode
					bUnderGPUCapture = true;
				}
			}
		}
	}
#endif // USE_PIX

	if(bUnderGPUCapture)
	{
		GDynamicRHI->EnableIdealGPUCaptureOptions(true);
	}
#endif

	// Init offline descriptor allocators
	RTVAllocator.Init(Direct3DDevice);
	DSVAllocator.Init(Direct3DDevice);
	SRVAllocator.Init(Direct3DDevice);
	UAVAllocator.Init(Direct3DDevice);
#if USE_STATIC_ROOT_SIGNATURE
	CBVAllocator.Init(Direct3DDevice);
#endif
	SamplerAllocator.Init(Direct3DDevice);

	GlobalSamplerHeap.Init(NUM_SAMPLER_DESCRIPTORS, FD3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	// This value can be tuned on a per app basis. I.e. most apps will never run into descriptor heap pressure so
	// can make this global heap smaller
	uint32 NumGlobalViewDesc = GGlobalViewHeapSize;

	const D3D12_RESOURCE_BINDING_TIER Tier = GetParentAdapter()->GetResourceBindingTier();
	uint32 MaximumSupportedHeapSize = NUM_VIEW_DESCRIPTORS_TIER_1;

	switch (Tier)
	{
	case D3D12_RESOURCE_BINDING_TIER_1:
		MaximumSupportedHeapSize = NUM_VIEW_DESCRIPTORS_TIER_1;
		break;
	case D3D12_RESOURCE_BINDING_TIER_2:
		MaximumSupportedHeapSize = NUM_VIEW_DESCRIPTORS_TIER_2;
		break;
	case D3D12_RESOURCE_BINDING_TIER_3:
	default:
		MaximumSupportedHeapSize = NUM_VIEW_DESCRIPTORS_TIER_3;
		break;
	}
	check(NumGlobalViewDesc <= MaximumSupportedHeapSize);
		
	GlobalViewHeap.Init(NumGlobalViewDesc, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Init the occlusion and timestamp query heaps
	OcclusionQueryHeap.Init();
	TimestampQueryHeap.Init();

	CommandListManager->Create(L"3D Queue");
	CopyCommandListManager->Create(L"Copy Queue");
	AsyncCommandListManager->Create(L"Async Compute Queue", 0, AsyncComputePriority_Default);

	// Needs to be called before creating command contexts
	UpdateConstantBufferPageProperties();

	CreateCommandContexts();

	UpdateMSAASettings();

}

void FD3D12Device::UpdateConstantBufferPageProperties()
{
	//In genera, constant buffers should use write-combine memory (i.e. upload heaps) for optimal performance
	bool bForceWriteBackConstantBuffers = false;

	if (bForceWriteBackConstantBuffers)
	{
		ConstantBufferPageProperties = GetDevice()->GetCustomHeapProperties(0, D3D12_HEAP_TYPE_UPLOAD);
		ConstantBufferPageProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	}
	else
	{
		ConstantBufferPageProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	}
}

void FD3D12Device::UpdateMSAASettings()
{
	check(DX_MAX_MSAA_COUNT == 8);

	// quality levels are only needed for CSAA which we cannot use with custom resolves

	// 0xffffffff means not available
	AvailableMSAAQualities[0] = 0xffffffff;
	AvailableMSAAQualities[1] = 0xffffffff;
	AvailableMSAAQualities[2] = 0;
	AvailableMSAAQualities[3] = 0xffffffff;
	AvailableMSAAQualities[4] = 0;
	AvailableMSAAQualities[5] = 0xffffffff;
	AvailableMSAAQualities[6] = 0xffffffff;
	AvailableMSAAQualities[7] = 0xffffffff;
	AvailableMSAAQualities[8] = 0;
}

void FD3D12Device::Cleanup()
{
	// Wait for the command queues to flush
	CommandListManager->WaitForCommandQueueFlush();
	CopyCommandListManager->WaitForCommandQueueFlush();
	AsyncCommandListManager->WaitForCommandQueueFlush();

	check(!GIsCriticalError);

	SamplerMap.Empty();

	ReleasePooledUniformBuffers();

	// Delete array index 0 (the default context) last
	for (int32 i = CommandContextArray.Num() - 1; i >= 0; i--)
	{
		delete CommandContextArray[i];
		CommandContextArray[i] = nullptr;
	}

	// Flush all pending deletes before destroying the device.
	FRHIResource::FlushPendingDeletes();

	/*
	// Cleanup thread resources
	for (int32 index; (index = FPlatformAtomics::InterlockedDecrement(&NumThreadDynamicHeapAllocators)) != -1;)
	{
		FD3D12DynamicHeapAllocator* pHeapAllocator = ThreadDynamicHeapAllocatorArray[index];
		pHeapAllocator->ReleaseAllResources();
		delete(pHeapAllocator);
	}
	*/

	CommandListManager->Destroy();
	CopyCommandListManager->Destroy();
	AsyncCommandListManager->Destroy();

	OcclusionQueryHeap.Destroy();
	TimestampQueryHeap.Destroy();

	D3DX12Residency::DestroyResidencyManager(ResidencyManager);
	// NVCHANGE_BEGIN: Add HBAO+
#if WITH_GFSDK_SSAO
	if (GetOwningRHI()->HBAOContext)
	{
		GetOwningRHI()->HBAOContext->Release();
		GetOwningRHI()->HBAOContext = nullptr;
	}
	if (GetOwningRHI()->HBAOModuleHandle)
	{
		FreeLibrary(GetOwningRHI()->HBAOModuleHandle);
		GetOwningRHI()->HBAOModuleHandle = nullptr;
	}
#endif
	// NVCHANGE_END: Add HBAO+
}

ID3D12CommandQueue* FD3D12Device::GetD3DCommandQueue(ED3D12CommandQueueType InQueueType) const
{
	switch (InQueueType)
	{
	case ED3D12CommandQueueType::Default:
		check(CommandListManager->GetQueueType() == InQueueType);
		return CommandListManager->GetD3DCommandQueue();
	case ED3D12CommandQueueType::Async:
		check(AsyncCommandListManager->GetQueueType() == InQueueType);
		return AsyncCommandListManager->GetD3DCommandQueue();
	case ED3D12CommandQueueType::Copy:
		check(CopyCommandListManager->GetQueueType() == InQueueType);
		return CopyCommandListManager->GetD3DCommandQueue();
	default:
		check(false);
		return nullptr;
	}
}

void FD3D12Device::RegisterGPUWork(uint32 NumPrimitives, uint32 NumVertices)
{
	GetParentAdapter()->GetGPUProfiler().RegisterGPUWork(NumPrimitives, NumVertices);
}

void FD3D12Device::PushGPUEvent(const TCHAR* Name, FColor Color)
{
	GetParentAdapter()->GetGPUProfiler().PushEvent(Name, Color);
}

void FD3D12Device::PopGPUEvent()
{
	GetParentAdapter()->GetGPUProfiler().PopEvent();
}

void FD3D12Device::BlockUntilIdle()
{
	GetDefaultCommandContext().FlushCommands();

	if (GEnableAsyncCompute)
	{
		GetDefaultAsyncComputeContext().FlushCommands();
	}

	GetCommandListManager().WaitForCommandQueueFlush();
	GetCopyCommandListManager().WaitForCommandQueueFlush();
	GetAsyncCommandListManager().WaitForCommandQueueFlush();
}
