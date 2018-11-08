#pragma once

#include "Modules/ModuleManager.h"

class IAssetTypeActions;

class FNvFlowEditorModule : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

private:
	TSharedPtr<IAssetTypeActions>	FlowGridAssetTypeActions;
	TSharedPtr<IAssetTypeActions>	FlowMaterialTypeActions;
	TSharedPtr<IAssetTypeActions>	FlowRenderMaterialTypeActions;
};

//////////////////////////////////////////////////////////////////////////

