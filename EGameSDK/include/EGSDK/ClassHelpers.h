#pragma once
#include <typeinfo>
#include <type_traits>
#include <EGSDK\Utils\Memory.h>
#include <EGSDK\Utils\Values.h>
#include <EGSDK\Exports.h>
#include <EGSDK\Offsets.h>
#include <fixed_string\fixed_string.hpp>

namespace EGSDK {
    namespace ClassHelpers {
#pragma pack(1)
        template<size_t size, typename T>
        class StaticBuffer {
            char buffer[size];
        public:
            T data;

            operator T() {
                return data;
            }
            T operator->() {
                return data;
            }

            uint64_t operator&(const uint64_t other) const {
                return reinterpret_cast<uint64_t>(data) & other;
            }
            uint64_t operator>>(const int shift) const {
                return reinterpret_cast<uint64_t>(data) >> shift;
            }
            uint64_t operator<<(const int shift) const {
                return reinterpret_cast<uint64_t>(data) << shift;
            }

            T& operator=(const T& other) {
                data = other;
                return data;
            }
            T& operator*=(const T& other) {
                data *= other;
                return data;
            }
            T operator*(const T& other) const {
                return data * other;
            }
            T& operator/=(const T& other) {
                data /= other;
                return data;
            }
            T operator/(const T& other) const {
                return data / other;
            }
            T& operator+=(const T& other) {
                data += other;
                return data;
            }
            T operator+(const T& other) const {
                return data + other;
            }
            T& operator-=(const T& other) {
                data -= other;
                return data;
            }
            T operator-(const T& other) const {
                return data - other;
            }
        };
#pragma pack()

        extern EGameSDK_API std::string GetFieldNameFromRTTI(std::string fullName);
        template <typename ParentT, typename T>
        static std::string GetOffsetNameFromClassMember(T ParentT::* member) {
            return Utils::Values::GetSimpleRTTITypeName(typeid(ParentT).name()) + "::" + GetFieldNameFromRTTI(typeid(T).name());
        }

#define DynamicField(parentType, type, field) ClassHelpers::DynamicBuffer<parentType, type, #field> field
#pragma pack(1)
        template<typename ParentT, typename T, fixstr::fixed_string name>
        class DynamicBuffer {
        public:
            T* getPointer() const {
                static std::string bufferName = Utils::Values::GetSimpleRTTITypeName(typeid(ParentT).name()) + "::" + std::string(name);
                static uint64_t bufferOffset = OffsetManager::GetOffset(bufferName);
                return reinterpret_cast<T*>(reinterpret_cast<uint64_t>(this) + bufferOffset);
            }

            operator T() const {
                return *getPointer();
            }
            // When T is U*, getPointer() is the address of the stored pointer (type U* in memory); return U* for -> chaining.
            template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
            std::remove_pointer_t<U>* operator->() const {
                return *getPointer();
            }
            template<typename U = T, typename = std::enable_if_t<!std::is_pointer_v<U>>, typename = void>
            U* operator->() const {
                return getPointer();
            }

            uint64_t operator&(const uint64_t other) const {
                return reinterpret_cast<uint64_t>(*getPointer()) & other;
            }
            uint64_t operator>>(const int shift) const {
                return reinterpret_cast<uint64_t>(*getPointer()) >> shift;
            }
            uint64_t operator<<(const int shift) const {
                return reinterpret_cast<uint64_t>(*getPointer()) << shift;
            }

            T& operator=(const T& other) {
                *getPointer() = other;
                return *getPointer();
            }
            T& operator*=(const T& other) {
                *getPointer() *= other;
                return *getPointer();
            }
            T operator*(const T& other) const {
                return *getPointer() * other;
            }
            T& operator/=(const T& other) {
                *getPointer() /= other;
                return *getPointer();
            }
            T operator/(const T& other) const {
                return *getPointer() / other;
            }
            T& operator+=(const T& other) {
                *getPointer() += other;
                return *getPointer();
            }
            T operator+(const T& other) const {
                return *getPointer() + other;
            }
            T& operator-=(const T& other) {
                *getPointer() -= other;
                return *getPointer();
            }
            T operator-(const T& other) const {
                return *getPointer() - other;
            }
        };
#pragma pack()

        extern EGameSDK_API bool IsVftableScanningDisabled();
        extern EGameSDK_API void SetIsVftableScanningDisabled(bool value);

        template <typename T, typename GetOffsetFunc, typename... Args>
        __forceinline T* _SafeGetImpl(GetOffsetFunc getOffset, bool isDoublePtr, bool checkForVTable, Args... args) {
            auto offset = getOffset(args...);
            if (!offset)
                return nullptr;

            T* ptr = isDoublePtr ? *reinterpret_cast<T**>(offset) : reinterpret_cast<T*>(offset);
            if (Utils::Memory::IsBadReadPtr(ptr))
                return nullptr;

            if (!checkForVTable || IsVftableScanningDisabled())
                return ptr;

            static const std::string expectedVtableName = Utils::Values::GetSimpleRTTITypeName(typeid(T).name());
            if (Utils::RTTI::GetVTableNameFromVTPtr(*reinterpret_cast<T**>(ptr)) != expectedVtableName)
                return nullptr;

            return ptr;
        }

        template <typename T, typename GetOffsetFunc, typename... Args>
        __forceinline T* SafeGetter(GetOffsetFunc getOffset, bool isDoublePtr = true, bool checkForVTable = true, Args... args) {
            __try {
				return _SafeGetImpl<T>(getOffset, isDoublePtr, checkForVTable, args...);
            } __except (Utils::Memory::SafeExecution::fail(GetExceptionCode(), GetExceptionInformation())) {
                return nullptr;
            }
        }
    }
}