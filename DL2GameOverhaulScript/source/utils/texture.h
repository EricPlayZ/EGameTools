#pragma once
#include <imgui.h>

namespace Utils {
	namespace Texture {
		extern ImTextureID LoadImGuiTexture(const unsigned char* rawData, const int rawSize);
	}
}