#include <cmath>
#include "spherecar.h"

namespace {
    // ベクトルの外積
    Vec3 Cross(const Vec3& a, const Vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    // ベクトルの正規化（長さを1にする）
    Vec3 Normalize(const Vec3& v) {
        float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (length == 0.0f) return { 0, 0, 0 };
        return { v.x / length, v.y / length, v.z / length };
    }

    // 任意の軸周りの回転（ロドリゲスの回転公式）
    Vec3 RotateAroundAxis(const Vec3& v, const Vec3& axis, float angle) {
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        Vec3 part1 = v * cosA;
        Vec3 part2 = Cross(axis, v) * sinA;

        // 内積
        float dot = (axis.x * v.x + axis.y * v.y + axis.z * v.z);
        Vec3 part3 = axis * (dot * (1.0f - cosA));

        return part1 + part2 + part3;
    }
}

void SphereCar::Update(
	bool keyW,
	bool keyA,
	bool keyS,
	bool keyD,
	float deltaTime)
{

	// 1. 現在の「上方向」ベクトル（球の中心から車への向き）
	Vec3 up = Normalize(position);

	// 2. 旋回 (A / D キー)
	if (keyA) {
		forward = RotateAroundAxis(forward, up, -turnSpeed * deltaTime);
	}
	if (keyD) {
		forward = RotateAroundAxis(forward, up, turnSpeed * deltaTime);
	}
	forward = Normalize(forward);

	// 3. 前後移動 (W / S キー)
	if (keyW) {
		position = position + forward * moveSpeed * deltaTime;
	}
	if (keyS) {
		position = position - forward * moveSpeed * deltaTime;
	}

	// 4. 球面に押し戻す（現在位置を正規化し、半径を掛ける）
	position = Normalize(position) * sphereRadius;

	// 5. 新しい位置に合わせて姿勢ベクトルを再計算
	up = Normalize(position);                         // 新しい上方向
	Vec3 right = Normalize(Cross(forward, up));       // 新しい右方向
	forward = Normalize(Cross(up, right));            // 新しい前方向（直交化）
}