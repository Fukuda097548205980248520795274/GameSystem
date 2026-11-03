#include "AnimationStore.hpp"
#include "Func/AnimationFunc/AnimationFunc.hpp"

/// @brief 更新処理
/// @param maxBufferCount 
/// @param frameIndex 
void Detail::AnimationStore::Update(uint32_t maxBufferCount, uint32_t frameIndex)
{
	// 破棄を遅延処理する
	ProcessDeferredDeletions(maxBufferCount, frameIndex);
}

/// @brief 読み込み
/// @param directory 
/// @param fileName 
/// @param device 
/// @return 
Detail::AnimationHandle Detail::AnimationStore::Load(const std::string& filePath)
{
	// キャッシュチェック
	auto it = cache_.find(filePath);
	if (it != cache_.end())
	{
		// 既存のハンドルが有効か確認
		if (GetAnimationData(it->second) != nullptr)
		{
			return it->second;
		}
	}


	// アニメーションデータのロード
	AnimationData animationData = LoadAnimation(filePath);
	if (animationData.nodes.empty())
	{
		return AnimationHandle::Invalid();
	}

	// スロットの割り当て
	uint32_t slotIndex = 0;
	if (!freeSlots_.empty())
	{
		// 空いているスロットを再利用
		slotIndex = freeSlots_.front();
		freeSlots_.pop();
	}
	else
	{
		// 新規スロット作成
		slotIndex = static_cast<uint32_t>(slots_.size());
		slots_.emplace_back();
	}

	// スロットにアニメーションデータを格納
	Slot& slot = slots_[slotIndex];
	slot.data = std::make_unique<AnimationData>(std::move(animationData));
	slot.inUse = true;

	// キャッシュに登録
	AnimationHandle handle{ slotIndex, slot.generation };
	cache_[filePath] = handle;

	return handle;
}

/// @brief アニメーションの破棄
/// @param handle 
/// @param frameIndex 
void Detail::AnimationStore::Unload(AnimationHandle handle, uint32_t frameIndex)
{
	if (!handle.IsValid() || handle.index >= slots_.size()) return;

	Slot& slot = slots_[handle.index];

	if (slot.inUse && slot.generation == handle.generation)
	{
		if (slot.data)
		{
			cache_.erase(slot.data->filePath);

			// 即座に破棄せず、所有権を破棄キューに移動する
			deferredDeletionQueue_.push({ std::move(slot.data), frameIndex });
		}

		// スロットは空いたものとして即座に再利用可能にする
		slot.inUse = false;
		slot.generation++;
		freeSlots_.push(handle.index);
	}
}

/// @brief アニメーションデータを取得する
/// @param handle 
/// @return 
Detail::AnimationData* Detail::AnimationStore::GetAnimationData(AnimationHandle handle)
{
	// ハンドルが無効、またはインデックスが範囲外の場合はnullptrを返す
	if (!handle.IsValid() || handle.index >= slots_.size())
		return nullptr;

	// スロットを取得する
	Slot& slot = slots_[handle.index];

	// スロットが使用中でない、または世代番号が一致しない場合は無効なハンドルとみなす
	if (!slot.inUse || slot.generation != handle.generation)
		return nullptr;

	return slot.data.get();
}

/// @brief インデックスでハンドルを取得する
/// @param index 
/// @return 
Detail::AnimationHandle Detail::AnimationStore::GetHandleByIndex(uint32_t index) const
{
	// インデックスが範囲外の場合は無効なハンドルを返す
	if (index >= slots_.size())
		return AnimationHandle{};

	// スロットを取得する
	const Slot& slot = slots_[index];

	// スロットが使用中でない場合は無効なハンドルを返す
	if (!slot.inUse)
		return AnimationHandle{};

	return AnimationHandle{ index, slot.generation };
}

/// @brief 破棄を遅延処理する
/// @param maxBufferCount 
/// @param frameIndex 
void Detail::AnimationStore::ProcessDeferredDeletions(uint32_t maxBufferCount, uint32_t frameIndex)
{
	while (!deferredDeletionQueue_.empty())
	{
		// キュー先頭の破棄要求フレームを取得
		int32_t requestedFrame = deferredDeletionQueue_.front().requestedFrameIndex;

		// 破棄要求からGPUが十分に処理を進めたか判定
		if (frameIndex - requestedFrame >= maxBufferCount)
		{
			// キューからポップすることで、std::unique_ptrが破棄されリソースが安全に解放される
			deferredDeletionQueue_.pop();
		}
		else
		{
			// 先頭要素がまだ破棄できないなら、後続の要素もまだ破棄できないのでループを抜ける
			break;
		}
	}
}