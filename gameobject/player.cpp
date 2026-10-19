#include	<algorithm>
#include	<cstdint>
#include	"player.h"
#include	"../scene/spheremath.h"
#include	"../system/commontypes.h"
#include	"../system/Inputmanager.h"
#include	"../system/transform.h"
#include	"../system/meshmanager.h"
#include	"../system/camera.h"

using namespace spheremath;

// ============================================================================
// 初期化・終了
// ============================================================================

void player::init()
{
	m_srt.pos = Vector3(0.0f, 0.0f, 0.0f);
	m_srt.scale = Vector3(1.0f, 1.0f, 1.0f);
	m_srt.rot = Vector3(0.0f, 0.0f, 0.0f);	// 球面版では未使用
}

void player::initSphere(const Vector3& center, float radius)
{
	m_planetcenter = center;
	m_planetradius = radius;

	m_up = Vector3(0.0f, 1.0f, 0.0f);	// 北極に置く
	m_forward = Vector3(0.0f, 0.0f, -1.0f);
	m_destforward = m_forward;
	m_move = Vector3(0.0f, 0.0f, 0.0f);

	m_srt.pos = m_planetcenter + m_up * m_planetradius;
}

void player::dispose()
{}

// ============================================================================
// 更新
// ============================================================================

void player::update(uint64_t /*dt*/)
{
	// 現在地の上方向 = 球の中心から外向き
	m_up = SafeNormalize(m_srt.pos - m_planetcenter, m_up);

	applyDriveInput();
	turnTowardDest();
	moveOnSphere();
	snapToSphere();

	// 移動量に慣性をかける（減速率）
	m_move += -m_move * RATE_MOVE_MODEL;
}

/**
 * @brief WASD を「画面上でその方向に見える」接平面上のベクトルに変換する
 *
 * 画面の上に見える接ベクトル t は
 *   ・接平面上にある           → t ⊥ m_up
 *   ・画面の左右成分を持たない → t ⊥ カメラの右
 * を満たすので、t = カメラの右 × m_up で求まる。
 * 画面の右は、接平面上で t と直交する方向 m_up × t。
 *
 * ※ 固定カメラなので、カメラの右と m_up が平行になる点（画面の左右の輪郭）では
 *   方向が決まらない。その場合は車の現在の向きを代わりに使う。
 */
Vector3 player::readCameraInput() const
{
	auto* camera = dynamic_cast<FreeCamera*>(m_ownerscene->getcamera());
	if (camera == nullptr) { return Vector3(0.0f, 0.0f, 0.0f); }

	Matrix4x4 viewmtx = camera->GetViewMatrix();
	Vector3 camright(viewmtx._11, viewmtx._21, viewmtx._31);

	Vector3 screenup = SafeNormalize(camright.Cross(m_up), m_forward);
	Vector3 screenright = m_up.Cross(screenup);

	auto& input = CInputManager::GetInstance();

	Vector3 dir(0.0f, 0.0f, 0.0f);
	if (input.IsKeyPressed(DIK_W)) { dir = dir + screenup; }		// 画面の上
	if (input.IsKeyPressed(DIK_S)) { dir = dir - screenup; }		// 画面の下
	if (input.IsKeyPressed(DIK_A)) { dir = dir - screenright; }		// 画面の左
	if (input.IsKeyPressed(DIK_D)) { dir = dir + screenright; }		// 画面の右

	return SafeNormalize(dir, Vector3(0.0f, 0.0f, 0.0f));
}

/**
 * @brief 入力を目標向きと速度に反映する
 *   WASD  : 入力方向を目標向きにし、その方向へ加速（平面版と同じ）
 *   ←/→ : 目標向きをその場で回す
 */
void player::applyDriveInput()
{
	// カメラ基準の移動
	Vector3 inputdir = readCameraInput();
	if (Len(inputdir) > EPSILON)
	{
		m_move = m_move + inputdir * VALUE_MOVE_MODEL;
		m_destforward = inputdir;
	}

	// 矢印キーでの旋回（左手系では正の角度で右旋回）
	auto& input = CInputManager::GetInstance();

	float turn = 0.0f;
	if (input.IsKeyPressed(DIK_RIGHT)) { turn += VALUE_ROTATE_MODEL; }
	if (input.IsKeyPressed(DIK_LEFT)) { turn -= VALUE_ROTATE_MODEL; }

	if (turn != 0.0f)
	{
		m_destforward = RotateAroundAxis(m_destforward, m_up, turn);

		// 目標向きが現在の向きより先行しすぎないよう制限する
		float lead = SignedAngle(m_forward, m_destforward, m_up);
		float clamped = std::clamp(lead, -MAX_ROTATE_LEAD, MAX_ROTATE_LEAD);
		if (clamped != lead)
		{
			m_destforward = RotateAroundAxis(m_forward, m_up, clamped);
		}
	}
}

/**
 * @brief 目標向きへ補間しながら向く
 *   平面版の「diffrot = destrot - rot（±PI補正）; rot += diffrot * RATE」と同じ処理を、
 *   m_up 軸周りの符号付き角度で行う（-PI～PI に収まるので ±2PI の補正は不要）
 */
void player::turnTowardDest()
{
	float diffrot = SignedAngle(m_forward, m_destforward, m_up);
	m_forward = RotateAroundAxis(m_forward, m_up, diffrot * RATE_ROTATE_MODEL);
}

/**
 * @brief 速度ぶん球面に沿って進む（大円移動）
 *   回転軸   = 上方向 × 移動方向（球の中心を通る）
 *   回転角度 = 移動距離 / 半径  （弧長 = 半径 × 角度）
 */
void player::moveOnSphere()
{
	m_move = ProjectOnPlane(m_move, m_up);

	float dist = Len(m_move);
	if (dist < EPSILON) { return; }

	Vector3 movedir = m_move * (1.0f / dist);
	Vector3 axis = SafeNormalize(m_up.Cross(movedir), AnyPerpendicular(m_up));
	float   angle = dist / m_planetradius;

	// 位置と一緒に、向き・目標向き・速度も同じ回転で運ぶ（平行移動）
	Vector3 offset = RotateAroundAxis(m_srt.pos - m_planetcenter, axis, angle);
	m_forward = RotateAroundAxis(m_forward, axis, angle);
	m_destforward = RotateAroundAxis(m_destforward, axis, angle);
	m_move = RotateAroundAxis(m_move, axis, angle);

	m_up = SafeNormalize(offset, m_up);
}

/**
 * @brief 誤差の蓄積対策
 *   ・位置を半径ぴったりに戻す
 *   ・向き・目標向きを接平面に戻して正規化する
 */
void player::snapToSphere()
{
	m_srt.pos = m_planetcenter + m_up * m_planetradius;

	m_forward = SafeNormalize(ProjectOnPlane(m_forward, m_up), AnyPerpendicular(m_up));
	m_destforward = SafeNormalize(ProjectOnPlane(m_destforward, m_up), m_forward);
}

// ============================================================================
// 描画
// ============================================================================

void player::draw(uint64_t /*dt*/)
{
	// 行ベクトル形式なので スケール → 姿勢＋位置 の順に掛ける
	Matrix4x4 mtx = Matrix4x4::CreateScale(m_srt.scale) * getSphereWorldMatrix();

	ShaderManager::getShader<CShader>("Shader3D")->SetGPU();
	Renderer::SetWorldMatrix(&mtx);
	MeshManager::getRenderer<CMeshRenderer>(m_meshid)->Draw();
}

/**
 * @brief m_up / m_forward から姿勢＋位置の行列を組み立てる
 *   ※ モデルは -Z 方向が正面の前提
 *   ※ 行ベクトル形式（DirectX流: v * M）
 */
Matrix4x4 player::getSphereWorldMatrix() const
{
	Vector3 right = m_up.Cross(m_forward);

	// モデルの -Z を m_forward に向けるため、X軸と Z軸を反転する
	Vector3 ax = right * -1.0f;
	Vector3 ay = m_up;
	Vector3 az = m_forward * -1.0f;

	Matrix4x4 m = Matrix4x4::Identity;
	m._11 = ax.x;        m._12 = ax.y;        m._13 = ax.z;
	m._21 = ay.x;        m._22 = ay.y;        m._23 = ay.z;
	m._31 = az.x;        m._32 = az.y;        m._33 = az.z;
	m._41 = m_srt.pos.x; m._42 = m_srt.pos.y; m._43 = m_srt.pos.z;
	return m;
}