/**************************************************************************/
/*  wedge_shape_3d.h                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2026 Godot Engine contributors.                          */
/* SPDX-License-Identifier: MIT                                           */
/**************************************************************************/

#pragma once

#include "scene/resources/3d/convex_polygon_shape_3d.h"

// A vertical sector of a cylinder, pointing along local +Z. The two radial
// faces meet at the Y axis. Its angular extent is symmetric about local +Z.
// A sector above 180 degrees is decomposed into separate convex collision
// pieces by CollisionShape3D (not by PhysicsServer3D::shape_create()).
class WedgeShape3D : public ConvexPolygonShape3D {
	GDCLASS(WedgeShape3D, ConvexPolygonShape3D);

	real_t radius = 1.0;
	real_t height = 1.0;
	real_t angle = 60.0;
	int radial_segments = 8;

	Vector<Ref<ConvexPolygonShape3D>> additional_shapes;

	void _rebuild();

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;

public:
	void set_radius(real_t p_radius);
	real_t get_radius() const;
	void set_height(real_t p_height);
	real_t get_height() const;
	void set_angle(real_t p_angle);
	real_t get_angle() const;
	void set_radial_segments(int p_segments);
	int get_radial_segments() const;

	void set_margin(real_t p_margin) override;
	void set_custom_solver_bias(real_t p_bias) override;

	const Vector<Ref<ConvexPolygonShape3D>> &get_additional_shapes() const;
	virtual Vector<Vector3> get_debug_mesh_lines() const override;
	virtual Ref<ArrayMesh> get_debug_arraymesh_faces(const Color &p_modulate) const override;
	virtual real_t get_enclosing_radius() const override;

	WedgeShape3D();
};
