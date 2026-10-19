#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "../system/commontypes.h"
#include "../system/IScene.h"
#include "../system/SceneClassFactory.h"
#include "../system/DirectWrite.h"
#include "../system/C3DShape.h"

/**
 * @brief 球面上を車が走るシーン
 */
class SphereCarScene : public SceneBase {
public:
	SphereCarScene(const SphereCarScene&) = delete;
	SphereCarScene& operator=(const SphereCarScene&) = delete;

	explicit SphereCarScene() = default;

	/**
	 * @brief 毎フレームの更新処理
	 * @param deltatime 前フレームからの経過時間（マイクロ秒）
	 */
	void update(uint64_t deltatime) override;

	/**
	 * @brief 毎フレームの描画処理
	 * @param deltatime 前フレームからの経過時間（マイクロ秒）
	 */
	void draw(uint64_t deltatime) override;

	/// @brief シーンの初期化処理
	void init() override;

	/// @brief シーンの終了処理
	void dispose() override;

private:
	void resourceLoader();			// リソースを読み込む
	void initDrawers();				// デバッグ形状描画の初期化
	void drawWorldAxis();			// ワールド3軸を描画
	void DebugFreeCamera();			// フリーカメラのデバッグUI

	std::unique_ptr<DirectWrite> m_directwrite;		// DirectWrite
	FontData m_fontdata;							// フォントデータ

	Vector3 m_planetcenter{ 0.0f, 0.0f, 0.0f };		// 大きな球の中心
	float   m_planetradius{ 100.0f };				// 大きな球の半径

	std::array<std::unique_ptr<Segment>, 3> m_segments;	// 3軸表示用線分
};

REGISTER_CLASS(SphereCarScene)
