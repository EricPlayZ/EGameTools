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

class __declspec(dllimport) IModelObject {
public:
    __cdecl IModelObject(class IModelObject const&);
    void AdjustExtentsToAllElements(bool, bool);
    class Anim::IPoseElement* AnimGetMeshPoseElement(void);
    class Anim::IPoseElement const* AnimGetMeshPoseElement(void) const;
    class Anim::IPoseElement* AnimGetModelObjectMorphPoseElement(void);
    class Anim::IPoseElement const* AnimGetModelObjectMorphPoseElement(void) const;
    class Anim::IPoseElement* AnimGetModelObjectPoseElement(void);
    class Anim::IPoseElement const* AnimGetModelObjectPoseElement(void) const;
    bool AnimReInit(class ttl::string_base<char> const&);
    static void CollectMeshSkins(class ttl::string_base<char> const&, bool, class ttl::vector<class ttl::string_base<char>, class ttl::vector_allocators::heap_allocator<class ttl::string_base<char>>, 1>&);
    void CollectUsedTextures(class ttl::vector<class ttl::string_base<char>, class ttl::vector_allocators::heap_allocator<class ttl::string_base<char>>, 1>&);
    void CopyElementsPosition(class IModelObject*);
    void DissolveObject(bool);
    void DumpAnims(void);
    bool EnableElementPhysics(int, bool, bool);
    void EnableHierarchySerialization(bool);
    void EnableRenderingRayTracing(bool);
    void EnableRenderingScene(bool);
    void EnableRenderingShadows(bool);
    void EnableUpdateExtents(bool);
    class aabb const& GetAABBExtents(void) const;
    void GetAnimationNames(class ttl::map<class ttl::string_base<char>, struct TAnimId, struct ttl::less<class ttl::string_base<char>>, class ttl::allocator>&);
    int GetCurrentLOD(void) const;
    union LodDissolves::SState GetCurrentLodState(void) const;
    static enum COFlags::TYPE GetDefaultCOFlags(void);
    char GetForcedAnimLod(void) const;
    float GetLodDissolveStep(void) const;
    void GetMeshElementsMatrices(class ttl::vector<class mtx34, class ttl::vector_allocators::heap_allocator<class mtx34>, 0>&) const;
    unsigned int GetMeshElementsMatricesCount(void) const;
    int GetMeshElementsState(class ttl::vector<unsigned char, class ttl::vector_allocators::heap_allocator<unsigned char>, 8>&, class ttl::vector<int, class ttl::vector_allocators::heap_allocator<int>, 2> const&) const;
    int GetMeshElementsStateSize(class ttl::vector<int, class ttl::vector_allocators::heap_allocator<int>, 2> const&) const;
    float GetMeshLodDistance(int) const;
    struct SMeshVisibilityParams const* GetMeshVisibilityParams(void) const;
    float GetMeshVisibilityRange(void);
    static class IModelObject* GetModelObject(class IGSObject*);
    static class IModelObject* GetModelObject(class cbs::CEntity const*);
    static class IModelObject const* GetModelObject(class IGSObject const*);
    static class CRTTI const* GetNativeClass(void);
    unsigned int GetNumCollisionHullFaces(void) const;
    unsigned int GetNumCollisionHullPrimitives(void) const;
    unsigned int GetNumCollisionHullVertices(void) const;
    unsigned int GetNumSurfaceParams(void) const;
    unsigned int GetNumTraceHullFaces(void) const;
    unsigned int GetNumTraceHullPrimitives(void) const;
    unsigned int GetNumTraceHullVertices(void) const;
    class ttl::string_const<char> GetSkin(void) const;
    static __int64 GetSkinTagsFromStr(char const*);
    struct SSurfParams* GetSurfaceParams(void) const;
    int GetTraceCollType(void) const;
    bool GetValidSkinsEditor(class ttl::vector<class ttl::string_base<char>, class ttl::vector_allocators::heap_allocator<class ttl::string_base<char>>, 1>&, class ttl::vector<class ttl::string_base<char>, class ttl::vector_allocators::heap_allocator<class ttl::string_base<char>>, 1>&) const;
    bool IsDefaultMeshLoaded(void);
    bool IsElementIDValid(int) const;
    bool IsElementPhysicsEnabled(int);
    bool IsObjectDissolved(void) const;
    void LoadMesh(bool);
    void LoadMeshElements(class ISGChunk*);
    int MT_GetCount(void);
    char const* MT_GetName(unsigned int);
    float MT_WeightGet(unsigned int);
    static bool MeshAndSkinExists(class ttl::string_base<char> const&, class ttl::string_base<char> const&);
    static bool MeshExist(class ttl::string_base<char> const&);
    void MeshUseDefaultVisibilityParameters(void);
    void MoveElementBoxSide(int, int, float);
    void MoveElementBoxSides(int, class vec3 const&, class vec3 const&);
    void MoveElementBoxSides(int, float, float, float, float, float, float);
    bool RaytestMe(class vec3 const&, class vec3&, unsigned short, bool, unsigned short);
    bool RaytraceMe(struct SCollision*, class vec3 const&, class vec3&, unsigned short, bool, unsigned short);
    bool ReplaceMaterial(class ttl::string_base<char> const&, class ttl::string_base<char> const&);
    void ResetBoneAndDescendantsToReferenceFrame(int);
    void ResetBonesExtentsToReferenceFrame(void);
    void ResetBonesToReferenceFrame(bool);
    void ResetElementsDescendantsToReferenceFrame(int);
    void SaveMeshElements(class ISGChunk*);
    void SetBestGeomLods(void);
    void SetDontApplyAnim(bool);
    void SetEngineObject(class CGSObject*);
    void SetExtentsLocal(class extents const&);
    void SetForcedAnimLod(char);
    void SetLodDissolveStep(float);
    void SetLodStateForSpawnedObjects(union LodDissolves::SState);
    void SetMeshCullSizeEnable(bool);
    bool SetMeshElementsMatrices(class ttl::vector<class mtx34, class ttl::vector_allocators::heap_allocator<class mtx34>, 0> const&);
    int SetMeshElementsState(class ttl::vector<unsigned char, class ttl::vector_allocators::heap_allocator<unsigned char>, 8> const&, class ttl::vector<int, class ttl::vector_allocators::heap_allocator<int>, 2> const&, bool, bool);
    void SetMeshLodDistance(int, float);
    void SetMeshVisibilityRange(float);
    bool SetSkin(void);
    bool SetSkinNoCharacterPreset(void);
    void SetTraceCollType(int);
    bool ShouldApplyAnim(void) const;
    void ShowCollisionHull(bool);
    void ShowElementBox(class ttl::string_base<char> const&);
    void ShowElementBoxes(bool);
    void ShowElementBoxesFrom(class ttl::string_base<char> const&);
    void ShowElementBoxesFromTo(class ttl::string_base<char> const&, class ttl::string_base<char> const&);
    void ShowElementBoxesTo(class ttl::string_base<char> const&);
    void ShowExtents(bool);
    bool SkinExists(class ttl::string_base<char> const&);
    class CModelObject* ToCModelObject(void);
    class CModelObject const* ToCModelObject(void) const;
};