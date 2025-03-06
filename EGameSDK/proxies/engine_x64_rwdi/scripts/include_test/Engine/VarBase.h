#pragma once
#include <unordered_map>
#include <string_view>
#include <mutex>
#include <shared_mutex>
#include <variant>
#include <type_traits>
#include <EGSDK\Exports.h>
#include <EGSDK\vec3.h>
#include <EGSDK\vec4.h>

namespace EGSDK::Engine {
    EGameSDK_API enum class VarType {
        NONE = 0,
        String,
        Float,
        Int,
        Vec3,
        Vec4,
        Bool
    };

    using VarValueType = std::variant<std::string, float, int, vec3, vec4, bool>;
    template <typename T>
    concept AllowedVarTypes = std::is_same_v<T, std::string> || std::is_same_v<T, float> || std::is_same_v<T, int> || std::is_same_v<T, vec3> || std::is_same_v<T, vec4> || std::is_same_v<T, bool>;

    class EGameSDK_API VarBase {
    public:
        VarBase(std::string_view name, VarType type = VarType::NONE);
        ~VarBase();

        const char* GetName() const;
        void SetName(std::string_view name);

        VarType GetType() const;
        void SetType(VarType type);
    protected:
        static std::mutex writeMutex;
        static std::shared_mutex readMutex;
    private:
        static std::unordered_map<const VarBase*, std::string> varNames;
        static std::unordered_map<const VarBase*, VarType> varTypes;
    };
}