#include "SphereCarScene.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "../system/C3DShape.h"
#include "../system/camera.h"
#include "../system/commontypes.h"
#include "../system/CShader.h"
#include "../system/CStaticMesh.h"
#include "../system/CStaticMeshRenderer.h"
#include "../system/DebugUI.h"
#include "../system/DirectWrite.h"
#include "../system/meshmanager.h"
#include "../system/renderer.h"

#include "../gameobject/player.h"

namespace {

	// 読み込むモデル
	struct Load3DInfo {
		std::string_view filename;			// モデルファイル
		std::string_view texdirectory;		// テクスチャのディレクトリ
	};

	constexpr std::array<Load3DInfo, 3> g_loadmodels = { {
		{ "assets/model/car000.x", "assets/model/" },
		{ "assets/model/car001.x", "assets/model/" },
		{ "assets/model/car002.x", "assets/model/" },
	} };

	// プレイヤーに使うメッシュ
	constexpr const char* PLAYER_MESHID = "car000.x";

	// ワールド3軸の長さと色（X:赤 Y:緑 Z:シアン）
	constexpr float AXIS_LENGTH = 100.0f;
	const std::array<Color, 3> AXIS_COLORS = {
		Color(1, 0, 0, 1),
		Color(0, 1, 0, 1),
		Color(0, 1, 1, 1),
	};

	// 大きな球の表示色（半透明）
	const Color PLANET_COLOR = Color(1, 1, 1, 0.5f);

	// パスからファイル名だけを取り出す（メッシュIDに使う）
	std::string getfilename(std::string_view filestring) {
		auto u8name = std::filesystem::path(filestring).filename().u8string();
		return { reinterpret_cast<const char*>(u8name.data()), u8name.size() };
	}

	// 指定位置に球を描画
	void SphereDraw(const Vector3& center, float radius, const Color& col)
	{
		Matrix4x4 mtx = Matrix4x4::CreateScale(radius);
		mtx._41 = center.x;
		mtx._42 = center.y;
		mtx._43 = center.z;

		SphereDrawerDraw(mtx, col);
	}
}

// ============================================================================
// 更新
// ============================================================================

void SphereCarScene::update(uint64_t deltatime)
{
	// 全オブジェクトを更新
	for (auto& objects : m_gameobjects)
	{
		for (auto& obj : objects)
		{
			if (obj && !obj->isDestroyed())
			{
				obj->update(deltatime);
			}
		}
	}

	// 削除
	for (auto& objects : m_gameobjects)
	{
		std::erase_if(objects, [](const std::unique_ptr<gameobject>& obj) {
			return obj == nullptr || obj->isDestroyed();
			});
	}

	// 追加
	for (int layer = 0; layer < LAYERMAX; ++layer)
	{
		auto& objects = m_gameobjects[layer];
		auto& pending = m_pendingAddObjects[layer];

		for (auto& obj : pending)
		{
			objects.push_back(std::move(obj));
		}
		pending.clear();
	}
}

// ============================================================================
// 描画
// ============================================================================

void SphereCarScene::draw(uint64_t deltatime)
{
	m_camera->Draw();

	drawWorldAxis();

	for (auto& objects : m_gameobjects)
	{
		for (auto& obj : objects)
		{
			if (obj && !obj->isDestroyed())
			{
				obj->draw(deltatime);
			}
		}
	}

	// 半透明なので最後に描く
	SphereDraw(m_planetcenter, m_planetradius, PLANET_COLOR);
}

void SphereCarScene::drawWorldAxis()
{
	for (size_t i = 0; i < m_segments.size(); ++i)
	{
		m_segments[i]->SetWidth(1.0f);
		m_segments[i]->Draw(Matrix4x4::Identity, AXIS_COLORS[i]);
	}
}

// ============================================================================
// 初期化・終了
// ============================================================================

void SphereCarScene::init()
{
	// カメラ(3D)の初期化
	m_camera = std::make_unique<FreeCamera>(
		Vector3(0, 30, -100),		// カメラ位置
		Vector3(0, 0, 0),			// 注視点
		Vector3(0, 1, 0));			// 上向きベクトル

	// DirectWrite生成
	m_directwrite = std::make_unique<DirectWrite>(&m_fontdata);
	m_directwrite->Init(Renderer::GetSwapChain());

	// 軸表示用線分
	m_segments[0] = std::make_unique<Segment>(Vector3(0, 0, 0), Vector3(AXIS_LENGTH, 0, 0));
	m_segments[1] = std::make_unique<Segment>(Vector3(0, 0, 0), Vector3(0, AXIS_LENGTH, 0));
	m_segments[2] = std::make_unique<Segment>(Vector3(0, 0, 0), Vector3(0, 0, AXIS_LENGTH));

	initDrawers();
	resourceLoader();

	// プレイヤー（車）を球の上に配置
	player* pobj = addGameObject<player>(0);
	pobj->setmeshid(PLAYER_MESHID);
	pobj->initSphere(m_planetcenter, m_planetradius);

	// デバッグUIにフリーカメラのパラメータを表示する関数を登録
	DebugUI::RedistDebugFunction([this]() {
		DebugFreeCamera();
		});
}

void SphereCarScene::initDrawers()
{
	SphereDrawerInit();
	BoxDrawerInit();
	CylinderDrawerInit();
	ConeDrawerInit(true);
	LineDrawerInit();
	CapsuleDrawerInit();
	PlaneDrawerInit();
}

void SphereCarScene::dispose()
{}

/**
 * @brief シェーダとメッシュを読み込む
 */
void SphereCarScene::resourceLoader()
{
	// シェーダ
	{
		auto shader = std::make_unique<CShader>();
		shader->Create(
			"shader/vertexLightingVS.hlsl",		// 頂点シェーダー
			"shader/vertexLightingPS.hlsl");	// ピクセルシェーダー
		ShaderManager::RegisterShader<CShader>("Shader3D", std::move(shader));
	}

	// メッシュとメッシュレンダラ（IDはファイル名）
	for (const auto& info : g_loadmodels)
	{
		const std::string filename(info.filename);
		const std::string texdir(info.texdirectory);
		const std::string meshid = getfilename(filename);

		auto mesh = std::make_unique<CStaticMesh>();
		mesh->Load(filename, texdir);

		auto meshrenderer = std::make_unique<CStaticMeshRenderer>();
		meshrenderer->Init(*mesh);

		MeshManager::RegisterMesh<CStaticMesh>(meshid, std::move(mesh));
		MeshManager::RegisterMeshRenderer<CStaticMeshRenderer>(meshid, std::move(meshrenderer));
	}
}

// ============================================================================
// デバッグUI
// ============================================================================

/**
 * @brief フリーカメラのデバッグUI
 *
 * 半径300, 方位角0°, 仰角-90°
 *  → カメラ位置(0,0,-300) 上ベクトル(0,1,0) 注視点(0,0,0)
 *  → 画面上で +X が右、+Y が上、+Z が奥になる
 * ※ FreeCamera は上ベクトルを「仰角＋90°」で求めるので仰角はマイナス側で指定する
 */
void SphereCarScene::DebugFreeCamera()
{
	struct CameraState {
		float radius;
		float azimuth;
		float elevation;
	};
	static CameraState state = { 300.0f, 0.0f, -PI / 2.0f };

	auto* freecam = dynamic_cast<FreeCamera*>(m_camera.get());
	if (freecam == nullptr) { return; }

	ImGui::Begin("Debug Free Camera Parameter");

	ImGui::SliderFloat("Radius", &state.radius, 10.0f, 500.0f);
	ImGui::SliderFloat("Azimuth", &state.azimuth, 0.0f, 2.0f * PI);
	ImGui::SliderFloat("Elevation", &state.elevation, -PI, PI);

	ImGui::End();

	freecam->SetRadius(state.radius);
	freecam->SetAzimuth(state.azimuth);
	freecam->SetElevation(state.elevation);
	freecam->Update();
}
