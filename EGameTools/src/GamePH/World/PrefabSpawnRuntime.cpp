#include <cmath>
#include <cstdint>
#include <string>
#include <ImGui\imguiex.h>
#include <EGSDK\Engine\CBulletPhysicsCharacter.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\GamePH\cbs.h>
#include <EGSDK\mtx34.h>
#include <EGSDK\vec3.h>
#include <EGSDK\vec4.h>
#include <EGT\GamePH\World\PrefabSpawnRuntime.h>
#include <spdlog/spdlog.h>

namespace EGT::GamePH::World::PrefabSpawnRuntime {
	static void SetMtx34Translation(mtx34* matrix, const vec3& position) {
		if (!matrix)
			return;
		matrix->Row1.W = position.X;
		matrix->Row2.W = position.Y;
		matrix->Row3.W = position.Z;
	}
	static void Multiply3x3RowMajor(const float left[3][3], const float right[3][3], float product[3][3]) {
		for (int row = 0; row < 3; ++row) {
			for (int col = 0; col < 3; ++col) {
				product[row][col] =
					left[row][0] * right[0][col] + left[row][1] * right[1][col] + left[row][2] * right[2][col];
			}
		}
	}
	static void SetMtx34TranslateEulerIntrinsicZyxDegrees(mtx34* outMatrix, const vec3& translateWorld, const vec3& rotateDegrees) {
		if (!outMatrix)
			return;
		const float degToRad = 3.14159265f / 180.0f;
		const float ax = rotateDegrees.X * degToRad;
		const float ay = rotateDegrees.Y * degToRad;
		const float az = rotateDegrees.Z * degToRad;
		const float cosX = std::cos(ax);
		const float sinX = std::sin(ax);
		const float cosY = std::cos(ay);
		const float sinY = std::sin(ay);
		const float cosZ = std::cos(az);
		const float sinZ = std::sin(az);
		const float rotX[3][3] = {
			{ 1.0f, 0.0f, 0.0f },
			{ 0.0f, cosX, -sinX },
			{ 0.0f, sinX, cosX },
		};
		const float rotY[3][3] = {
			{ cosY, 0.0f, sinY },
			{ 0.0f, 1.0f, 0.0f },
			{ -sinY, 0.0f, cosY },
		};
		const float rotZ[3][3] = {
			{ cosZ, -sinZ, 0.0f },
			{ sinZ, cosZ, 0.0f },
			{ 0.0f, 0.0f, 1.0f },
		};
		float rotYTimesRotX[3][3]{};
		float rotation[3][3]{};
		Multiply3x3RowMajor(rotY, rotX, rotYTimesRotX);
		Multiply3x3RowMajor(rotZ, rotYTimesRotX, rotation);
		outMatrix->Row1 = vec4(rotation[0][0], rotation[0][1], rotation[0][2], translateWorld.X);
		outMatrix->Row2 = vec4(rotation[1][0], rotation[1][1], rotation[1][2], translateWorld.Y);
		outMatrix->Row3 = vec4(rotation[2][0], rotation[2][1], rotation[2][2], translateWorld.Z);
	}
	static vec3 ComputeSpawnWorldPosition(float forwardMeters, float verticalBump) {
		EGSDK::Engine::CBulletPhysicsCharacter* const playerCharacter = EGSDK::Engine::CBulletPhysicsCharacter::Get();
		if (!playerCharacter)
			return vec3(0.f, 0.f, 0.f);
		vec3 position = playerCharacter->playerPos;
		EGSDK::GamePH::LevelDI* const levelDi = EGSDK::GamePH::LevelDI::Get();
		if (levelDi) {
			EGSDK::Engine::IBaseCamera* const viewCamera = levelDi->GetViewCamera();
			if (viewCamera) {
				vec3 forward{};
				if (viewCamera->GetForwardVector(&forward) && !forward.isDefault())
					position = position + forward.normalize() * -forwardMeters;
			}
		}
		position.Y += verticalBump;
		return position;
	}
	static void BuildPrefabRootTransform(mtx34* outMatrix, bool spawnAtPlayer, float forwardMeters, float verticalBump, const vec3& worldPosition, const vec3& eulerDegrees) {
		if (!outMatrix)
			return;
		if (spawnAtPlayer) {
			*outMatrix = mtx34{};
			SetMtx34Translation(outMatrix, ComputeSpawnWorldPosition(forwardMeters, verticalBump));
			return;
		}
		SetMtx34TranslateEulerIntrinsicZyxDegrees(outMatrix, worldPosition, eulerDegrees);
	}
	static void RunNamedPresetSpawn(const std::string& prefabPath, const std::string& presetName, bool spawnAtPlayer, const vec3& worldPosition, const vec3& eulerDegrees, float forwardMeters, float verticalBump) {
		constexpr std::uint32_t defaultWorldIndex = 0;
		void* const level = EGSDK::GamePH::cbs::GetILevel(defaultWorldIndex);
		if (!level) {
			SPDLOG_ERROR("GetILevel({}) returned null (missing export or no level) [prefab spawn].", defaultWorldIndex);
			return;
		}
		mtx34 root{};
		BuildPrefabRootTransform(&root, spawnAtPlayer, forwardMeters, verticalBump, worldPosition, eulerDegrees);
		const vec3 spawnPosition = root.GetPosition();
		alignas(8) std::uint64_t entityPointerBitsOut{};
		const bool succeeded = EGSDK::GamePH::cbs::CreateEntityFromPrefab(
			prefabPath.c_str(),
			level,
			&root,
			presetName.c_str(),
			&entityPointerBitsOut
		);
		if (!succeeded)
			SPDLOG_ERROR("CreateEntityFromPrefab (named preset) failed (SEH, missing export, or game error) [prefab spawn].");
		else {
			const vec3 eulerLogged = spawnAtPlayer ? vec3(0.f, 0.f, 0.f) : eulerDegrees;
			SPDLOG_INFO(
				"CreateEntityFromPrefab (named preset) [prefab spawn]: CPointer=0x{:016X} pos=({:.2f},{:.2f},{:.2f}) eulerDeg=({:.2f},{:.2f},{:.2f}) spawnAtPlayer={}",
				static_cast<unsigned long long>(entityPointerBitsOut),
				static_cast<double>(spawnPosition.X),
				static_cast<double>(spawnPosition.Y),
				static_cast<double>(spawnPosition.Z),
				static_cast<double>(eulerLogged.X),
				static_cast<double>(eulerLogged.Y),
				static_cast<double>(eulerLogged.Z),
				spawnAtPlayer ? 1 : 0
			);
		}
	}

	void Render() {
		ImGui::SeparatorTextSection("Prefab spawn##World", false);
		static char prefabNameBuf[512]{};
		static char presetNameBuf[256]{};
		static bool spawnAtPlayer = true;
		static constexpr float spawnForwardMeters = 3.0f;
		static constexpr float spawnVerticalBumpMeters = 0.0f;
		static vec3 manualWorldPosition{ 207.4772949f, 0.0000027f, -39.5408935f };
		static vec3 manualEulerDegrees{ -179.9999084f, 60.0671577f, 179.9999084f };
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.72f);
		ImGui::InputTextWithHint("Prefab name##WorldPrefabPath", "", prefabNameBuf, sizeof(prefabNameBuf));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.72f);
		ImGui::InputTextWithHint("Preset name##WorldPrefabPreset", "e.g. Character;Survivor", presetNameBuf, sizeof(presetNameBuf));
		ImGui::Checkbox("Spawn at player##WorldPrefabAtPlr", &spawnAtPlayer, "Uses player position plus a short forward offset from the view camera.");
		ImGui::BeginDisabled(spawnAtPlayer);
		ImGui::DragFloat3("World XYZ##WorldPrefabPos", &manualWorldPosition.X, 0.05f, 0.0f, 0.0f, "%.4f");
		ImGui::DragFloat3("Rotation (deg X,Y,Z)##WorldPrefabEuler", &manualEulerDegrees.X, 0.25f, 0.0f, 0.0f, "%.3f");
		ImGui::EndDisabled();
		if (ImGui::ButtonSmooth("Spawn##WorldPrefabSpawn"))
			RunNamedPresetSpawn(std::string(prefabNameBuf), std::string(presetNameBuf), spawnAtPlayer, manualWorldPosition, manualEulerDegrees, spawnForwardMeters, spawnVerticalBumpMeters);
	}
} // namespace EGT::GamePH::World::PrefabSpawnRuntime
