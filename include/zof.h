#ifndef zof_h
#define zof_h

#ifdef __cplusplus
extern "C" {
#endif

// TODO Change comments to be /*C-friendly*/?

// Core expandeds.
// TODO Ensure fixed bit lengths.
typedef enum {zof_false=0, zof_true=1} zof_bool;
typedef int zof_int;
typedef double zof_num;
typedef unsigned int zof_uint;

#define zof_pi 3.14159265358979323846

// Core opaques.

#define zof_ref_def(name) typedef struct name##_struct {}* name;

#ifdef __cplusplus
	// C++ vs. C distinction from gcc.
	// I provide zof_null to avoid need to import stdlib.h.
	#define zof_null 0
#else
	#define zof_null ((void*)0)
#endif

typedef void* zof_any;

/**
 * Opaquify?
 */
typedef char* zof_str;

zof_ref_def(zof_err);
zof_ref_def(zof_type);

zof_str zof_err_code(zof_err err);

// Thread local reference to most recent error.
zof_err zof_err_last(void);

void zof_err_last_replace(zof_err err);

zof_num zof_num_max(zof_num a, zof_num b);

zof_num zof_num_min(zof_num a, zof_num b);

void zof_ref_free(zof_any ref);

zof_type zof_ref_type(zof_any ref);

typedef void (*zof_ref_close)(zof_any ref);

typedef struct {
	zof_ref_close close;
	zof_str name;
} zof_type_info;

zof_str zof_type_name(zof_type type);

// info - shallow copy to new type
zof_type zof_type_new(zof_type_info* info);

zof_ref_close zof_type_ref_close(zof_type type);

// Math expandeds.
enum {zof_x, zof_y, zof_z, zof_w};

typedef struct {
	zof_num vals[4];
} zof_vec4;

// Math opaques.

zof_ref_def(zof_mat);

/**
 * Leave it compatible. The int is just a hint.
 */
typedef zof_mat zof_mat_int;


zof_vec4 zof_xyz(zof_num x, zof_num y, zof_num z);

zof_vec4 zof_xyzw(zof_num x, zof_num y, zof_num z, zof_num w);


// More domainish stuff.

/**
 * ARGB
 *
 * TODO Perhaps something more sophisticated later, but this
 * TODO will do for now and at least tag our references to color.
 */
typedef zof_uint zof_color;

typedef enum {
	zof_part_kind_error,
	zof_part_kind_box,
	zof_part_kind_capsule,
	zof_part_kind_cylinder,
	zof_part_kind_group,
	zof_part_kind_mesh,
} zof_part_kind;

zof_ref_def(zof_app);
zof_ref_def(zof_box);
zof_ref_def(zof_capsule);
zof_ref_def(zof_joint);
zof_ref_def(zof_material);
zof_ref_def(zof_mesh);
zof_ref_def(zof_mod);
zof_ref_def(zof_part);
zof_ref_def(zof_sim);


zof_vec4 zof_box_radii(zof_box box);

zof_vec4 zof_capsule_end_pos(zof_capsule capsule, zof_num radius_ratio);

zof_vec4 zof_capsule_end_pos_ex(
	zof_capsule capsule,
	zof_num radius_ratio,
	zof_vec4 axis,
	zof_num half_spread_ratio
);

zof_str zof_joint_name(zof_joint joint);
zof_joint zof_joint_new(zof_str name, zof_vec4 pos, zof_vec4 rot);
zof_joint zof_joint_other(zof_joint joint);
zof_part zof_joint_part(zof_joint joint);

zof_num zof_mat_get(zof_mat, zof_mat pos);
zof_int zof_mat_geti(zof_mat, zof_mat pos);
zof_num zof_mat_get1(zof_mat, zof_int i);
zof_int zof_mat_get1i(zof_mat, zof_int i);
zof_num zof_mat_get2(zof_mat, zof_int i, zof_int j);
zof_int zof_mat_get2i(zof_mat, zof_int i, zof_int j);
zof_int zof_mat_ncols(zof_mat mat);
zof_int zof_mat_nrows(zof_mat mat);
zof_mat zof_mat_shape(zof_mat mat);
zof_int zof_mat_size(zof_mat mat);

zof_material zof_material_new(zof_color color, zof_num density);

// Meshes.
zof_mesh zof_mesh_new(zof_part shape);
// TODO zof_mesh_new_empty?
// TODO zof_mesh_new_copy
// TODO zof_mesh_subdivide, other coolness, ...


// TODO Allow mods at both app and sim level?
zof_bool zof_mod_sim_init(zof_mod mod, zof_sim sim);

// Just supports relativish local files for now.
zof_mod zof_mod_new(zof_str uri);
zof_str zof_mod_uri(zof_mod mod);

// Parts.
zof_bool zof_part_attach(zof_part part, zof_part kid);
zof_box zof_part_box(zof_part part);
zof_capsule zof_part_capsule(zof_part part);
zof_vec4 zof_part_end_pos(zof_part part, zof_vec4 ratios);
zof_joint zof_part_joint(zof_part part, zof_str name);

/**
 * Returns any previous joint with this name or null.
 * TODO Allow leaving name null for auto-assignment?
 */
zof_joint zof_part_joint_put(zof_part part, zof_joint joint);

zof_part_kind zof_part_part_kind(zof_part part);
void zof_part_material_put(zof_part part, zof_material material);
zof_str zof_part_name(zof_part part);
zof_part zof_part_new_box(zof_str name, zof_vec4 radii);
zof_part zof_part_new_capsule(zof_str name, zof_num radius, zof_num half_spread);
zof_part zof_part_new_group(zof_str name, zof_part root);
void zof_part_pos_add(zof_part part, zof_vec4 pos);
void zof_part_pos_put(zof_part part, zof_vec4 pos);
void zof_part_rot_add(zof_part part, zof_vec4 rot);
void zof_part_rot_put(zof_part part, zof_vec4 rot);


void zof_sim_part_add(zof_sim sim, zof_part part);



#ifdef __cplusplus
}
#endif

#endif
