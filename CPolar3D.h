#pragma once
#include "commontypes.h"

// ３Ｄ極座標系（左手座標系）
//   仰角　 : +Y 軸からの傾き（0=真上, 90°=水平, 180°=真下）
//   方位角 : Y 軸まわりの回転。0=+Z（画面奥）方向で、正方向は +Z → +X
//            ＝ CreateFromYawPitchRoll のヨー角と同じ向き
//   例）仰角 90°・方位角 0° → 位置は (0,0,r)、ToLookMatrix() は単位行列になる
//
//   ※ FreeCamera は仰角をマイナス側（-90°=水平）で使う。上ベクトルを
//      「仰角＋90°」で求めているため、その範囲でないと上下が反転する
class CPolor3D {
	float m_radius;				// 半径		
	float m_elevation;			// 仰角
	float m_azimuth;			// 方位角
public:
	CPolor3D() = delete;
	CPolor3D(float radius,
		float elevation,
		float azimuth) : m_radius(radius), m_elevation(elevation), m_azimuth(azimuth) {}
	~CPolor3D() {}

	Vector3 ToCartesian() const {
		Vector3 position;

		position.x = m_radius * sinf(m_elevation) * sinf(m_azimuth);
		position.y = m_radius * cosf(m_elevation);
		position.z = m_radius * sinf(m_elevation) * cosf(m_azimuth);	// 方位角0で +Z を向く

		return position;
	}

	// この極座標が示す方向を「前方(+Z)」として、原点に置いた回転行列を作る（左手座標系）
	Matrix4x4 ToLookMatrix() const {
		const float se = sinf(m_elevation);
		const float ce = cosf(m_elevation);
		const float sa = sinf(m_azimuth);
		const float ca = cosf(m_azimuth);

		// 前方：原点 → ToCartesian() の向き（半径を 1 にしたもの）
		const Vector3 forward(se * sa, ce, se * ca);

		// 上：仰角を 90°戻した方向（前方と直角になる）
		const Vector3 up(-ce * sa, se, -ce * ca);

		// 右：right × up == forward を満たす向き（左手系）
		const Vector3 right(ca, 0.0f, -sa);

		// 行優先：第1行＝右, 第2行＝上, 第3行＝前, 平行移動は原点なのでゼロ
		return Matrix4x4(right, up, forward);
	}
};