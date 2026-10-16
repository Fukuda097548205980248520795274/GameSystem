#pragma once
#include "ECS/EntityECS/EntityECS.h"
#include "Matrix/Matrix4x4/Matrix4x4.h"

namespace Detail
{
	/// @brief ECSレジストリ
	class RegistryECS;

	/// @brief カメラシステム
	class CameraSystem
	{
	public:

		/// @brief コンストラクタ
		CameraSystem() { Initialize(); }

		/// @brief 更新処理
		void Update();

		/// @brief エンティティを作成する
		/// @return 
		Entity CreateCameraEntity();

		/// @brief 現在のカメラEntityを設定する
		/// @return 
		Matrix4x4 GetCameraMatrix() const { return cameraMatrix_; }

		/// @brief ビュー行列を取得する
		/// @return 
		Matrix4x4 GetViewMatrix() const { return viewMatrix_; }

		/// @brief 射影行列を取得する
		/// @return 
		Matrix4x4 GetProjectionMatrix() const { return projectionMatrix_; }


	private:

		/// @brief 初期化
		void Initialize();

		/// @brief ECSレジストリ
		RegistryECS* registry_ = nullptr;


	private:

		// 現在のカメラEntity（kNullEntityの場合はカメラが設定されていない）
		Entity currentEntity_ = kNullEntity;

		// カメラのビュー行列
		Matrix4x4 cameraMatrix_;

		// ビュー行列
		Matrix4x4 viewMatrix_;

		// 射影行列
		Matrix4x4 projectionMatrix_;
	};
}