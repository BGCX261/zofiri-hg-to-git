#ifndef zof_h
#define zof_h

#ifdef _WIN32
	#ifdef zof_EXPORTS
		#define zofExport __declspec(dllexport)
	#else
		#define zofExport __declspec(dllimport)
	#endif
	// zofExport becomes export or import.
	// zofModExport is always export and should be used for functions
	//   exported by mods.
	// TODO A system of exports from mods with structs of function pointers.
	// TODO That guarantees keeping the namespace clean.
	// TODO Then each mod could just have a single dllexport function that
	// TODO provides all setup and other exports in the struct.
	#define zofModExport __declspec(dllexport)
#else
	#define zofExport
	#define zofModExport
#endif

#ifdef __cplusplus
extern "C" {
#endif

// TODO Change comments to be /*C-friendly*/?

// Core expandeds.
// TODO Ensure fixed bit lengths.
typedef enum {zofFalse=0, zofTrue=1} zofBool;
typedef int zofInt;
typedef double zofNum;
typedef unsigned int zofUint;

#define zofPi 3.14159265358979323846

// Core opaques.

#define zofRefDef(name) typedef struct name##_struct {int unused;}* name;

#ifdef __cplusplus
	// C++ vs. C distinction from gcc.
	// I provide zofNull to avoid need to import stdlib.h.
	#define zofNull 0
#else
	#define zofNull ((void*)0)
#endif

typedef void* zofAny;

/**
 * Opaquify?
 */
typedef char* zofString;

zofRefDef(zofErr);
zofRefDef(zofType);

zofExport zofString zofErrCode(zofErr err);

// Thread local reference to most recent error.
zofExport zofErr zofErrLast(void);

zofExport void zofErrLastReplace(zofErr err);

zofExport zofNum zofNumMax(zofNum a, zofNum b);

zofExport zofNum zofNumMin(zofNum a, zofNum b);

zofExport void zofRefFree(zofAny ref);

zofExport zofType zofRefType(zofAny ref);

typedef void (*zofRefClose)(zofAny ref);

typedef struct {
	zofRefClose close;
	zofString name;
} zofTypeInfo;

zofExport zofString zofTypeName(zofType type);

// info - shallow copy to new type
zofExport zofType zofTypeNew(zofTypeInfo* info);

zofExport zofRefClose zofTypeRefClose(zofType type);

// Math expandeds.
enum {zofX, zofY, zofZ, zofW};

/**
 * Basic expanded vector type.
 */
typedef struct {
	zofNum vals[4];
} zofVec4;

typedef zofVec4 zofVec3;

/**
 * Pairs of vectors for min/max ranges.
 */
typedef struct {
	zofVec4 min;
	zofVec4 max;
} zofExtents4;

typedef zofExtents4 zofExtents3;

/**
 * Meters.
 */
typedef zofNum zofM;

/**
 * For positions or sizes (meters cubed).
 */
typedef zofExtents3 zofExtentsM3;
typedef zofVec3 zofM3;

/**
 * Here "rat" means "ratio of pi" or in other words, radians/pi.
 * So, say '0.5' instead of '0.5*zofPi', for example.
 */
typedef zofVec3 zofRat3;

/**
 * For axis-angle orientations.
 */
typedef zofVec4 zofM3Rat;

// Math opaques.

zofRefDef(zofMat);

/**
 * Leave it compatible. The int is just a hint.
 */
typedef zofMat zofMatInt;

zofExport zofNum zofNan();

zofExport zofNum zofInf();

zofExport zofVec4 zofV3(zofNum x, zofNum y, zofNum z);

zofExport zofVec4 zofV4(zofNum x, zofNum y, zofNum z, zofNum w);


// More domainish stuff.

/**
 * ARGB
 *
 * TODO Perhaps something more sophisticated later, but this
 * TODO will do for now and at least tag our references to color.
 */
typedef zofUint zofColor;

typedef enum {
	zofPartKindError,
	zofPartKindBox,
	zofPartKindCapsule,
	zofPartKindCylinder,
	zofPartKindGroup,
	zofPartKindMesh,
} zofPartKind;

zofRefDef(zofApp);
zofRefDef(zofBox);
zofRefDef(zofCapsule);
zofRefDef(zofJoint);
zofRefDef(zofMaterial);
zofRefDef(zofMesh);
zofRefDef(zofMod);
zofRefDef(zofPart);
zofRefDef(zofSim);


zofExport zofVec4 zofCapsuleEndPos(zofCapsule capsule, zofNum radiusRatio);

zofExport zofVec4 zofCapsuleEndPosEx(
	zofCapsule capsule,
	zofNum radiusRatio,
	zofVec4 axis,
	zofNum halfSpreadRatio
);

zofExport zofNum zofCapsuleRadius(zofCapsule capsule);

/**
 * For attaching joints directly rather than matching by name as with zof_part_attach.
 */
zofExport void zofJointAttach(zofJoint joint, zofJoint kid);

zofExport void zofJointLimitsRotPut(zofJoint joint, zofRat3 min, zofRat3 max);

zofExport zofString zofJointName(zofJoint joint);

/**
 * Assumes no rotation for the joint transform.
 */
zofExport zofJoint zofJointNew(zofString name, zofM3 pos);

zofExport zofJoint zofJointNewEx(zofString name, zofM3 pos, zofM3Rat rot);

zofExport zofJoint zofJointOther(zofJoint joint);

zofExport zofPart zofJointPart(zofJoint joint);

zofExport void zofJointPosPut(zofJoint joint, zofNum pos);

/**
 * If there is only one degree of freedom on this joint, set its target
 * velocity to vel.
 */
zofExport void zofJointVelPut(zofJoint joint, zofNum vel);

/**
 * TODO Do I really want any of this?
 */
zofExport zofNum zofMatGet(zofMat, zofMat pos);
zofExport zofInt zofMatGetInt(zofMat, zofMat pos);
zofExport zofNum zofMatGet1D(zofMat, zofInt i);
zofExport zofInt zofMatGetInt1D(zofMat, zofInt i);
zofExport zofNum zofMatGet2D(zofMat, zofInt i, zofInt j);
zofExport zofInt zofMatGetInt2D(zofMat, zofInt i, zofInt j);
zofExport zofInt zofMatNCols(zofMat mat);
zofExport zofInt zofMatNRows(zofMat mat);
zofExport zofMat zofMatShape(zofMat mat);
zofExport zofInt zofMatSize(zofMat mat);

zofExport zofMaterial zofMaterialNew(zofColor color, zofNum density);

// Meshes.
zofExport zofMesh zofMeshNew(zofPart shape);
// TODO zof_mesh_new_empty?
// TODO zof_mesh_new_copy
// TODO zof_mesh_subdivide, other coolness, ...


// TODO Allow mods at both app and sim level?
// TODO Should this be in zof.h or just a mod.h for internal use? Maybe good to let mods load others?
zofExport zofBool zofModSimInit(zofMod mod, zofSim sim);

// Just supports relativish local files for now.
zofExport zofMod zofModNew(zofString uri);

// This one definitely stays in zof.h.
zofExport zofString zofModUri(zofMod mod);

// Parts.

/**
 * Attach parts based on joint names matching part names.
 */
zofExport zofBool zofPartAttach(zofPart part, zofPart kid);

zofExport zofBox zofPartBox(zofPart part);

zofExport zofCapsule zofPartCapsule(zofPart part);

zofExport zofPart zofPartCopyTo(zofPart part, zofM3 pos, zofString oldSub, zofString newSub);

zofExport zofVec4 zofPartEndPos(zofPart part, zofVec4 ratios);

zofExport zofExtentsM3 zofPartExtents(zofPart part);

zofExport zofJoint zofPartJoint(zofPart part, zofString name);

/**
 * Returns any previous joint with this name or null.
 * TODO Allow leaving name null for auto-assignment?
 */
zofExport zofJoint zofPartJointPut(zofPart part, zofJoint joint);

zofExport zofMaterial zofPartMaterial(zofPart part);

zofExport void zofPartMaterialPut(zofPart part, zofMaterial material);

/**
 * Convenience for building symmetries.
 *
 * Creates a mirror image across X of the group or single basic part.
 * The part in question must be attached to at most one other part.
 * If the part is attached to a another, the mirror part will also be
 * attached to the same part at the mirror X location.
 */
zofExport zofPart zofPartMirror(zofPart part);

zofExport zofString zofPartName(zofPart part);

zofExport void zofPartNamePut(zofPart part, zofString name);

zofExport zofPart zofPartNewBox(zofString name, zofVec4 radii);

zofExport zofPart zofPartNewCapsule(zofString name, zofNum radius, zofNum halfSpread);

zofExport zofPart zofPartNewCylinder(zofString name, zofVec4 radii);

zofExport zofPart zofPartNewGroup(zofString name, zofPart root);

zofExport zofPartKind zofPartPartKind(zofPart part);

zofExport zofVec4 zofPartPos(zofPart part);

zofExport void zofPartPosAdd(zofPart part, zofM3 pos);

zofExport void zofPartPosPut(zofPart part, zofM3 pos);

zofExport zofVec4 zofPartRadii(zofPart part);

zofExport void zofPartRotAdd(zofPart part, zofM3Rat rot);

zofExport void zofPartRotPut(zofPart part, zofM3Rat rot);


zofExport void zofSimPartAdd(zofSim sim, zofPart part);

zofExport void zofSimUpdaterAdd(zofSim sim, void (*updater)(zofSim,zofAny), zofAny data);

zofExport zofString zofStringNewCopy(zofString contents);


#ifdef __cplusplus
}
#endif

#endif
