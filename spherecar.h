#pragma once

#include <cmath>

// 簡易的な3Dベクトル構造体（実際はGLMやエンジンの独自型を使用してください）
struct Vec3 {
    float x, y, z;

    // ベクトルの足し算・引き算・掛け算などのオペレーターは定義されている前提
    Vec3 operator+(const Vec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
    Vec3 operator-(const Vec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
    Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
};


class SphereCar {
public:
    Vec3 position;      // 車の現在位置
    Vec3 forward;       // 車の前方向ベクトル
    float sphereRadius; // 球の半径

    float moveSpeed = 10.0f; // 前後移動の速度
    float turnSpeed = 2.0f;  // 旋回の速度（ラジアン/秒）

    // 毎フレーム呼ばれる更新関数
    void Update(bool keyW, bool keyA, bool keyS, bool keyD, float deltaTime);
};