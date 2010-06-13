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

// Core opaques.

#define zof_ref_def(name) typedef struct name##_ {}* name;

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

typedef zof_uint zof_color;

typedef enum {
	zof_shape_kind_error,
	zof_shape_kind_box,
	zof_shape_kind_capsule,
	zof_shape_kind_cylinder,
	zof_shape_kind_mesh,
} zof_shape_kind;

zof_ref_def(zof_app);
zof_ref_def(zof_box);
zof_ref_def(zof_joint);
zof_ref_def(zof_mesh);
zof_ref_def(zof_mod);
zof_ref_def(zof_part);
zof_ref_def(zof_shape);
zof_ref_def(zof_sim);


zof_vec4 zof_box_radii(zof_box box);

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

// Meshes.
zof_mesh zof_mesh_new(zof_shape shape);
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
zof_joint zof_part_joint(zof_part part, zof_str name);

/**
 * Returns any previous joint with this name or null.
 * TODO Allow leaving name null for auto-assignment?
 */
zof_joint zof_part_joint_put(zof_part part, zof_joint joint);

zof_vec4 zof_part_end_pos(zof_part part, zof_vec4 ratios);
zof_shape_kind zof_part_shape_kind(zof_part part);
zof_str zof_part_name(zof_part part);
zof_part zof_part_new(zof_str name, zof_shape shape);
zof_part zof_part_new_box(zof_str name, zof_vec4 radii);
zof_part zof_part_new_group(zof_str name);
void zof_part_pos_put(zof_part part, zof_vec4 pos);

// TODO Remove shape entirely? Add zof_box, zof_capsule, ..., for casting from parts?
zof_shape zof_shape_new_box(zof_vec4 radii);
zof_shape zof_shape_new_capsule(zof_num rad_xy, zof_num half_spread);
zof_shape zof_shape_new_cylinder(zof_vec4 radii);
zof_shape zof_shape_new_mesh(zof_mesh mesh);
// zof_shape_new_copy
// zof_shape_scale


void zof_sim_part_add(zof_sim sim, zof_part part);



#ifdef __cplusplus
}
#endif

#endif
