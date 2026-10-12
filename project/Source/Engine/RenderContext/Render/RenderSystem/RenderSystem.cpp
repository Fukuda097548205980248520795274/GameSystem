#include "RenderSystem.h"
#include "Engine.h"

#include "ECS/ComponentECS/DataForGPU/DataForGPU.h"

/// @brief 初期化
void Detail::RenderSystem::Initialize()
{
	// Engineのインスタンスを取得する
	auto  engine = Engine::GetInstance();
	assert(engine);

	// ECSレジストリを取得する
	auto registry = engine->GetRegistryECS();
	assert(registry);

	// PSOエディタを取得する
	auto psoEditor = engine->GetPSOEditor();
	assert(psoEditor);

	// メンバ変数に代入する
	registry_ = registry;
	psoEditor_ = psoEditor;


	/// レンダーリソースストアを生成する
	renderResourceStore_ = std::make_unique<RenderResourceStore>();
}

/// @brief エンティティを作成する
/// @return 
Entity Detail::RenderSystem::CreateRender3DEntity()
{
	// エンティティを作成する
	Entity entity = registry_->CreateEntity();

	// RenderComponentを追加する
	RenderComponent render3DComp;
	registry_->AddComponent(entity, render3DComp);

	// TransformComponentを追加する
	TransformComponent transformComp;
	registry_->AddComponent(entity, transformComp);

	// MaterialComponentを追加する
	MaterialComponent materialComp;
	registry_->AddComponent(entity, materialComp);

	auto engine = Engine::GetInstance();
	assert(engine);

	auto renderContext = engine->GetRenderContext();
	assert(renderContext);

	auto device = renderContext->GetCore()->GetDevice();
	assert(device);

	int32_t maxBufferCount = engine->GetMaxBufferCount();
	assert(maxBufferCount > 0);

	// TransformatDataForGPU用のConstantBufferComponentを追加する
	TransformConstantBufferComponent constantBufferComp(renderResourceStore_->CreateConstantBuffer<Detail::TransformDataForGPU>(device, maxBufferCount));
	registry_->AddComponent(entity, constantBufferComp);

	// MaterialComponentを追加する
	MaterialConstantBufferComponent materialConstantBufferComp(renderResourceStore_->CreateConstantBuffer<Detail::UnLightingMaterialDataForGPU>(device, maxBufferCount));
	registry_->AddComponent(entity, materialConstantBufferComp);

	return entity;
}

/// @brief すべての3D描画を実行する
/// @param commandList 
/// @param priority 
void Detail::RenderSystem::ExecuteAll(ID3D12GraphicsCommandList* commandList, int priority)
{
	// priority が負の値の場合は何もしない
	if (priority < 0)return;

	auto engine = Engine::GetInstance();
	if (!engine) return;

	auto textureStore = engine->GetRenderContext()->GetTextureStore();
	if (!textureStore) return;

	auto proceduralMeshStore = engine->GetRenderContext()->GetProceduralMeshStore();
	if (!proceduralMeshStore) return;

	// RenderComponentの配列を取得する
	auto* passArray = registry_->GetComponentArray<RenderComponent>();
	const auto& entities = passArray->GetEntities();

	// 今のフレームインデックスを取得する
	int32_t frameIndex = engine->GetFrameIndex();

	// priority に基づいてソートするためのペアのベクターを作成
	std::vector<std::pair<Entity, int>> sortedPasses;
	for (Entity entity : entities)
	{
		auto& pass = registry_->GetComponent<RenderComponent>(entity);
		if (pass.isEnabled && pass.priority == priority)
		{
			sortedPasses.push_back({ entity, pass.priority });
		}
	}

	// ソートされたレンダーパスが空の場合は何もしない
	if (sortedPasses.empty()) return;

	// priority に基づいて昇順でソート（数値が小さいほど先に描画）
	std::sort(sortedPasses.begin(), sortedPasses.end(), [](const auto& a, const auto& b) {return a.second < b.second;});

	// ソートされたレンダーパスを実行する
	for (const auto& [entity, priority] : sortedPasses)
	{
		// 必要なコンポーネントを取得
		auto& transform = registry_->GetComponent<TransformComponent>(entity);
		auto& material = registry_->GetComponent<MaterialComponent>(entity);

		auto& transformCB = registry_->GetComponent<TransformConstantBufferComponent>(entity);
		auto& materialCB = registry_->GetComponent<MaterialConstantBufferComponent>(entity);

		// PSOエディタ（または管理クラス）からPSOとルートシグネチャを取得
		auto psoItem = psoEditor_->GetPSOItem(material.psoName);
		if (!psoItem || !psoItem->pso) continue;


		// PSOとルートシグネチャをコマンドリストに設定
		commandList->SetPipelineState(psoItem->pso.Get());
		commandList->SetGraphicsRootSignature(psoItem->rootSig.Get());

		// ProceduralMeshStoreからキューブメッシュを登録する
		proceduralMeshStore->Register(ProceduralMeshType::Cube, commandList);

		// PSOの要求に従って動的にパラメータをバインドする
		for (UINT i = 0;i < psoItem->desc.rootParameters.size(); ++i)
		{
			const auto& param = psoItem->desc.rootParameters[i];

			// RenderBindingに基づいて適切なリソースをバインドする
			switch (param.binding)
			{
			case RenderBinding::Transform:
			{
				// Transform用のリソースをバインド
				auto cbResource = renderResourceStore_->GetConstantBuffer(transformCB.handle);
				if (cbResource)cbResource->RegisterGraphics(commandList, i, frameIndex);
				break;
			}
			case RenderBinding::Material:
			{
				// Material用のリソースをバインド
				auto cbResource = renderResourceStore_->GetConstantBuffer(materialCB.handle);
				if (cbResource)cbResource->RegisterGraphics(commandList, i, frameIndex);
				break;
			}
			case RenderBinding::Texture:
			{
				// Texture用のリソースをバインド
				commandList->SetGraphicsRootDescriptorTable(i, textureStore->GetSrvHandle(0).gpuHandle);
				break;
			}

			}
		}

		// トポロジを設定する
		if (psoItem->desc.primitiveTopology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE) commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		else if (psoItem->desc.primitiveTopology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE) commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		else if (psoItem->desc.primitiveTopology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT) commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

		// ドローコール
		commandList->DrawIndexedInstanced(proceduralMeshStore->GetNumIndex(ProceduralMeshType::Cube), 1, 0, 0, 0);
	}
}