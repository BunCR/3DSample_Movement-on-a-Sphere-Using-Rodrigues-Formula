#pragma once

#include <cmath>
#include "../system/commontypes.h"

/**
 * @brief 球面移動で使うベクトル演算ヘルパー
 */
namespace spheremath {

	constexpr float EPSILON = 0.0001f;

	/// @brief 内積
	inline float Dot(const Vector3& a, const Vector3& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	/// @brief 長さ
	inline float Len(const Vector3& v)
	{
		return std::sqrt(Dot(v, v));
	}

	/// @brief 正規化（長さがほぼ0なら fallback を返す）
	inline Vector3 SafeNormalize(const Vector3& v, const Vector3& fallback)
	{
		float len = Len(v);
		return (len > EPSILON) ? v * (1.0f / len) : fallback;
	}

	/// @brief v から法線 n 方向の成分を取り除く（接平面へ投影）※ n は単位ベクトル
	inline Vector3 ProjectOnPlane(const Vector3& v, const Vector3& n)
	{
		return v - n * Dot(v, n);
	}

	/// @brief n に垂直な単位ベクトルを1本返す（n は単位ベクトル）
	inline Vector3 AnyPerpendicular(const Vector3& n)
	{
		// n と最も平行でない軸を選んで外積をとる
		Vector3 ref = (std::fabs(n.x) < 0.9f) ? Vector3(1.0f, 0.0f, 0.0f) : Vector3(0.0f, 0.0f, 1.0f);
		return SafeNormalize(n.Cross(ref), Vector3(0.0f, 0.0f, 1.0f));
	}

	/// @brief axis 周りで from → to への符号付き角度（-PI ～ PI）
	inline float SignedAngle(const Vector3& from, const Vector3& to, const Vector3& axis)
	{
		return std::atan2(Dot(axis, from.Cross(to)), Dot(from, to));
	}

	/**
	 * @brief 任意の軸周りの回転（ロドリゲスの回転公式）
	 * @param v     回転させるベクトル
	 * @param axis  回転軸（単位ベクトル）
	 * @param angle 回転角（ラジアン）
	 */
	inline Vector3 RotateAroundAxis(const Vector3& v, const Vector3& axis, float angle)
	{
		float cosA = std::cos(angle);
		float sinA = std::sin(angle);

		return v * cosA
			+ axis.Cross(v) * sinA
			+ axis * (Dot(axis, v) * (1.0f - cosA));		
	}
}
