#pragma once
namespace Anim {
    class IPoseElement;
}

namespace Audio {
    struct SAudioEventExtraData;
    struct SAudioEventExtraDataID;
}

namespace COFlags {
    enum TYPE;
}

namespace EBones {
    enum TYPE;
}

namespace EObjectToSimpleObjectsQueryResult {
    enum TYPE;
}

namespace LodDissolves {
    union SState;
}

namespace cbs {
    class CEntity;
    template <typename T>
    class CPointer;
}

struct AnimEventInfo;
class CHierarchyElement;
class CModelObject;
class CRTTI;
class IAnimBind;
class ICoSkeleton;
class IGSObject;
class IMpc;
class ISGChunk;
struct SCollision;
struct SMeshVisibilityParams;
struct SSurfParams;
struct TAnimId;
class aabb;
class extents;
struct uint4;

namespace ttl {
    namespace vector_allocators {
        template <typename T>
        class heap_allocator;
    }

    template <typename T>
    class string_base;

    template <typename T>
    class string_const;

    template <typename T1, typename T2, size_t T3>
    class vector;

    template <typename T1, typename T2, typename T3, typename T4>
    class map;

    template <typename T1>
    struct less;

    class allocator;
}

class vec3;
class vec4;
class mtx34;
class CGSObject;
class IControlObject;

class IModelObject {
public:
    __declspec(dllimport) __cdecl IModelObject(class IModelObject const&);
    __declspec(dllimport) void AdjustExtentsToAllElements(bool, bool);
    __declspec(dllimport) class Anim::IPoseElement* AnimGetMeshPoseElement(void);
    __declspec(dllimport) class Anim::IPoseElement const* AnimGetMeshPoseElement(void) const;
    __declspec(dllimport) class Anim::IPoseElement* AnimGetModelObjectMorphPoseElement(void);
    __declspec(dllimport) class Anim::IPoseElement const* AnimGetModelObjectMorphPoseElement(void) const;
    __declspec(dllimport) class Anim::IPoseElement* AnimGetModelObjectPoseElement(void);
    __declspec(dllimport) class Anim::IPoseElement const* AnimGetModelObjectPoseElement(void) const;
    __declspec(dllimport) bool AnimReInit(class ttl::string_base<char> const&);
    __declspec(dllimport) static void CollectMeshSkins(class ttl::string_base<char> const&, bool, class ttl::vector<class ttl::string_base<char>, class ttl::vector_allocators::heap_allocator<class ttl::string_base<char>>, 1>&);
    __declspec(dllimport) void CollectUsedTextures(class ttl::vector<class ttl::string_base<char>, class ttl::vector_allocators::heap_allocator<class ttl::string_base<char>>, 1>&);
    __declspec(dllimport) void CopyElementsPosition(class IModelObject*);
    __declspec(dllimport) void DissolveObject(bool);
    __declspec(dllimport) void DumpAnims(void);
    __declspec(dllimport) bool EnableElementPhysics(int, bool, bool);
    __declspec(dllimport) void EnableHierarchySerialization(bool);
    __declspec(dllimport) void EnableRenderingRayTracing(bool);
    __declspec(dllimport) void EnableRenderingScene(bool);
    __declspec(dllimport) void EnableRenderingShadows(bool);
    __declspec(dllimport) void EnableUpdateExtents(bool);
    __declspec(dllimport) class aabb const& GetAABBExtents(void) const;
    __declspec(dllimport) void GetAnimationNames(class ttl::map<class ttl::string_base<char>, struct TAnimId, struct ttl::less<class ttl::string_base<char>>, class ttl::allocator>&);
    __declspec(dllimport) int GetCurrentLOD(void) const;
    __declspec(dllimport) union LodDissolves::SState GetCurrentLodState(void) const;
    __declspec(dllimport) static enum COFlags::TYPE GetDefaultCOFlags(void);
    __declspec(dllimport) char GetForcedAnimLod(void) const;
    __declspec(dllimport) float GetLodDissolveStep(void) const;
    __declspec(dllimport) void GetMeshElementsMatrices(class ttl::vector<class mtx34, class ttl::vector_allocators::heap_allocator<class mtx34>, 0>&) const;
    __declspec(dllimport) unsigned int GetMeshElementsMatricesCount(void) const;
    __declspec(dllimport) int GetMeshElementsState(class ttl::vector<unsigned char, class ttl::vector_allocators::heap_allocator<unsigned char>, 8>&, class ttl::vector<int, class ttl::vector_allocators::heap_allocator<int>, 2> const&) const;
    __declspec(dllimport) int GetMeshElementsStateSize(class ttl::vector<int, class ttl::vector_allocators::heap_allocator<int>, 2> const&) const;
    __declspec(dllimport) float GetMeshLodDistance(int) const;
    __declspec(dllimport) struct SMeshVisibilityParams const* GetMeshVisibilityParams(void) const;
    __declspec(dllimport) float GetMeshVisibilityRange(void);
    __declspec(dllimport) static class IModelObject* GetModelObject(class IGSObject*);
    __declspec(dllimport) static class IModelObject* GetModelObject(class cbs::CEntity const*);
    __declspec(dllimport) static class IModelObject const* GetModelObject(class IGSObject const*);
    __declspec(dllimport) static class CRTTI const* GetNativeClass(void);
    __declspec(dllimport) unsigned int GetNumCollisionHullFaces(void) const;
    __declspec(dllimport) unsigned int GetNumCollisionHullPrimitives(void) const;
    __declspec(dllimport) unsigned int GetNumCollisionHullVertices(void) const;
    __declspec(dllimport) unsigned int GetNumSurfaceParams(void) const;
    __declspec(dllimport) unsigned int GetNumTraceHullFaces(void) const;
    __declspec(dllimport) unsigned int GetNumTraceHullPrimitives(void) const;
    __declspec(dllimport) unsigned int GetNumTraceHullVertices(void) const;
    __declspec(dllimport) class ttl::string_const<char> GetSkin(void) const;
    __declspec(dllimport) static __int64 GetSkinTagsFromStr(char const*);
    __declspec(dllimport) struct SSurfParams* GetSurfaceParams(void) const;
    __declspec(dllimport) int GetTraceCollType(void) const;
    __declspec(dllimport) bool GetValidSkinsEditor(class ttl::vector<class ttl::string_base<char>, class ttl::vector_allocators::heap_allocator<class ttl::string_base<char>>, 1>&, class ttl::vector<class ttl::string_base<char>, class ttl::vector_allocators::heap_allocator<class ttl::string_base<char>>, 1>&) const;
    __declspec(dllimport) bool IsDefaultMeshLoaded(void);
    __declspec(dllimport) bool IsElementIDValid(int) const;
    __declspec(dllimport) bool IsElementPhysicsEnabled(int);
    __declspec(dllimport) bool IsObjectDissolved(void) const;
    __declspec(dllimport) void LoadMesh(bool);
    __declspec(dllimport) void LoadMeshElements(class ISGChunk*);
    __declspec(dllimport) int MT_GetCount(void);
    __declspec(dllimport) char const* MT_GetName(unsigned int);
    __declspec(dllimport) float MT_WeightGet(unsigned int);
    __declspec(dllimport) static bool MeshAndSkinExists(class ttl::string_base<char> const&, class ttl::string_base<char> const&);
    __declspec(dllimport) static bool MeshExist(class ttl::string_base<char> const&);
    __declspec(dllimport) void MeshUseDefaultVisibilityParameters(void);
    __declspec(dllimport) void MoveElementBoxSide(int, int, float);
    __declspec(dllimport) void MoveElementBoxSides(int, class vec3 const&, class vec3 const&);
    __declspec(dllimport) void MoveElementBoxSides(int, float, float, float, float, float, float);
    __declspec(dllimport) bool RaytestMe(class vec3 const&, class vec3&, unsigned short, bool, unsigned short);
    __declspec(dllimport) bool RaytraceMe(struct SCollision*, class vec3 const&, class vec3&, unsigned short, bool, unsigned short);
    __declspec(dllimport) bool ReplaceMaterial(class ttl::string_base<char> const&, class ttl::string_base<char> const&);
    __declspec(dllimport) void ResetBoneAndDescendantsToReferenceFrame(int);
    __declspec(dllimport) void ResetBonesExtentsToReferenceFrame(void);
    __declspec(dllimport) void ResetBonesToReferenceFrame(bool);
    __declspec(dllimport) void ResetElementsDescendantsToReferenceFrame(int);
    __declspec(dllimport) void SaveMeshElements(class ISGChunk*);
    __declspec(dllimport) void SetBestGeomLods(void);
    __declspec(dllimport) void SetDontApplyAnim(bool);
    __declspec(dllimport) void SetEngineObject(class CGSObject*);
    __declspec(dllimport) void SetExtentsLocal(class extents const&);
    __declspec(dllimport) void SetForcedAnimLod(char);
    __declspec(dllimport) void SetLodDissolveStep(float);
    __declspec(dllimport) void SetLodStateForSpawnedObjects(union LodDissolves::SState);
    __declspec(dllimport) void SetMeshCullSizeEnable(bool);
    __declspec(dllimport) bool SetMeshElementsMatrices(class ttl::vector<class mtx34, class ttl::vector_allocators::heap_allocator<class mtx34>, 0> const&);
    __declspec(dllimport) int SetMeshElementsState(class ttl::vector<unsigned char, class ttl::vector_allocators::heap_allocator<unsigned char>, 8> const&, class ttl::vector<int, class ttl::vector_allocators::heap_allocator<int>, 2> const&, bool, bool);
    __declspec(dllimport) void SetMeshLodDistance(int, float);
    __declspec(dllimport) void SetMeshVisibilityRange(float);
    __declspec(dllimport) bool SetSkin(void);
    __declspec(dllimport) bool SetSkinNoCharacterPreset(void);
    __declspec(dllimport) void SetTraceCollType(int);
    __declspec(dllimport) bool ShouldApplyAnim(void) const;
    __declspec(dllimport) void ShowCollisionHull(bool);
    __declspec(dllimport) void ShowElementBox(class ttl::string_base<char> const&);
    __declspec(dllimport) void ShowElementBoxes(bool);
    __declspec(dllimport) void ShowElementBoxesFrom(class ttl::string_base<char> const&);
    __declspec(dllimport) void ShowElementBoxesFromTo(class ttl::string_base<char> const&, class ttl::string_base<char> const&);
    __declspec(dllimport) void ShowElementBoxesTo(class ttl::string_base<char> const&);
    __declspec(dllimport) void ShowExtents(bool);
    __declspec(dllimport) bool SkinExists(class ttl::string_base<char> const&);
    __declspec(dllimport) class CModelObject* ToCModelObject(void);
    __declspec(dllimport) class CModelObject const* ToCModelObject(void) const;
};