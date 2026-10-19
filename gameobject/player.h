#pragma once

#include	<cstdint>
#include	<string>
#include	"gameobject.h"
#include	"../system/IScene.h"

/**
 * @brief 球面上を走る車
 *
 * 姿勢をオイラー角ではなく m_up / m_forward の2本のベクトルで持つ。
 *   移動 : 球の中心を通る軸周りの回転（大円移動）
 *   旋回 : m_up 軸周りの回転
 *
 * 操作はカメラ基準
 *   W/S/A/D : 画面の 上/下/左/右 に見える方向へ移動（車はその方向へ補間しながら向く）
 *   ←/→    : 目標向きをその場で旋回
 */
class player : public gameobject {
	inline static float VALUE_MOVE_MODEL = 2.0f;				// 移動量
	inline static float VALUE_ROTATE_MODEL = PI * 0.02f;		// 回転量（1フレームあたり）
	inline static float RATE_ROTATE_MODEL = 0.4f;				// 回転割合（目標向きへの補間率）
	inline static float RATE_MOVE_MODEL = 0.2f;					// 移動減衰割合
	inline static float MAX_ROTATE_LEAD = PI * 0.5f;			// 目標向きが先行できる最大角度
public:

	explicit player(SceneBase* scene) : gameobject(scene) {}

	void update(uint64_t delta) override;
	void draw(uint64_t delta) override;
	void init() override;
	void dispose() override;

	void setmeshid(const std::string& id) { m_meshid = id; }

	/**
	 * @brief 球面上に配置する（addGameObject の後に呼ぶ）
	 * @param center 球の中心
	 * @param radius 球の半径
	 */
	void initSphere(const Vector3& center, float radius);

	/// @brief m_up / m_forward から作る姿勢＋位置の行列（スケールは含まない）
	Matrix4x4 getSphereWorldMatrix() const;

	const Vector3& getUp() const { return m_up; }
	const Vector3& getForward() const { return m_forward; }

private:
	Vector3 readCameraInput() const;	// WASD → 画面基準の接平面上の方向（入力なしならゼロ）
	void applyDriveInput();				// 入力 → 目標向きと加速
	void turnTowardDest();				// 目標向きへ補間して向く
	void moveOnSphere();				// 速度ぶん球面に沿って進む
	void snapToSphere();				// 誤差の蓄積を補正する

	std::string m_meshid{};

	Vector3 m_move{};							// 速度（接平面上のワールドベクトル）

	Vector3 m_up{ 0.0f, 1.0f, 0.0f };			// 上方向（球の中心から外向き）
	Vector3 m_forward{ 0.0f, 0.0f, -1.0f };		// 前方向（接平面上）
	Vector3 m_destforward{ 0.0f, 0.0f, -1.0f };	// 目標の前方向（接平面上）

	Vector3 m_planetcenter{ 0.0f, 0.0f, 0.0f };	// 球の中心
	float   m_planetradius{ 100.0f };			// 球の半径
};