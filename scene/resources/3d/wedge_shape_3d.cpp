/**************************************************************************/
/*  wedge_shape_3d.cpp                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2026 Godot Engine contributors.                          */
/* SPDX-License-Identifier: MIT                                           */
/**************************************************************************/

#include "wedge_shape_3d.h"

#include "core/object/class_db.h"
#include "scene/resources/mesh.h"

namespace {
// Angle is measured around +Y, with zero pointing along local +Z.
Vector3 wedge_arc_point(real_t p_radius, real_t p_y, real_t p_degrees) {
	const real_t radians = Math::deg_to_rad(p_degrees);
	return Vector3(Math::sin(radians) * p_radius, p_y, Math::cos(radians) * p_radius);
}

void push_triangle(Vector<Vector3> &r_vertices, Vector<Color> &r_colors, const Vector3 &p_a, const Vector3 &p_b, const Vector3 &p_c, const Color &p_color) {
	r_vertices.push_back(p_a);
	r_vertices.push_back(p_b);
	r_vertices.push_back(p_c);
	r_colors.push_back(p_color);
	r_colors.push_back(p_color);
	r_colors.push_back(p_color);
}
} // namespace

void WedgeShape3D::_rebuild() {
	// More than 180 degrees is concave, except for the full 360-degree disk.
	// Concave sectors are split into convex pieces of at most 90 degrees.
	const int piece_count = angle >= 360.0 ? 1 : (angle > 180.0 ? (int)Math::ceil(angle / 90.0) : 1);
	const real_t collision_angle = MAX(angle, (real_t)0.1); // Degenerate 0-degree RID is never attached.
	const real_t half_height = height * 0.5;
	Vector<Vector3> primary_points;
	Vector<Ref<ConvexPolygonShape3D>> new_additional_shapes;

	for (int piece = 0; piece < piece_count; piece++) {
		const real_t from = -collision_angle * 0.5 + collision_angle * piece / piece_count;
		const real_t to = -collision_angle * 0.5 + collision_angle * (piece + 1) / piece_count;
		const int steps = MAX(2, (int)Math::ceil(radial_segments * (to - from) / 360.0));

		Vector<Vector3> piece_points;
		piece_points.push_back(Vector3(0, half_height, 0));
		piece_points.push_back(Vector3(0, -half_height, 0));
		for (int i = 0; i <= steps; i++) {
			const real_t degrees = Math::lerp(from, to, (real_t)i / steps);
			piece_points.push_back(wedge_arc_point(radius, half_height, degrees));
			piece_points.push_back(wedge_arc_point(radius, -half_height, degrees));
		}

		if (piece == 0) {
			primary_points = piece_points;
		} else {
			Ref<ConvexPolygonShape3D> extra;
			extra.instantiate();
			extra->set_points(piece_points);
			extra->set_margin(get_margin());
			extra->set_custom_solver_bias(get_custom_solver_bias());
			new_additional_shapes.push_back(extra);
		}
	}

	// Publish all pieces before signalling that the shape has changed.
	additional_shapes = new_additional_shapes;
	ConvexPolygonShape3D::set_points(primary_points);
}

void WedgeShape3D::set_radius(real_t p_radius) {
	ERR_FAIL_COND_MSG(p_radius <= 0.0, "WedgeShape3D radius must be greater than zero.");
	if (Math::is_equal_approx(radius, p_radius)) {
		return;
	}
	radius = p_radius;
	_rebuild();
}

real_t WedgeShape3D::get_radius() const {
	return radius;
}

void WedgeShape3D::set_height(real_t p_height) {
	ERR_FAIL_COND_MSG(p_height <= 0.0, "WedgeShape3D height must be greater than zero.");
	if (Math::is_equal_approx(height, p_height)) {
		return;
	}
	height = p_height;
	_rebuild();
}

real_t WedgeShape3D::get_height() const {
	return height;
}

void WedgeShape3D::set_angle(real_t p_angle) {
	ERR_FAIL_COND_MSG(p_angle < 0.0 || p_angle > 360.0, "WedgeShape3D angle must be between 0 and 360 degrees.");
	if (Math::is_equal_approx(angle, p_angle)) {
		return;
	}
	angle = p_angle;
	_rebuild();
}

real_t WedgeShape3D::get_angle() const {
	return angle;
}

void WedgeShape3D::set_radial_segments(int p_segments) {
	ERR_FAIL_COND_MSG(p_segments < 8 || p_segments > 256, "WedgeShape3D radial_segments must be between 8 and 256.");
	if (radial_segments == p_segments) {
		return;
	}
	radial_segments = p_segments;
	_rebuild();
}

int WedgeShape3D::get_radial_segments() const {
	return radial_segments;
}

void WedgeShape3D::set_margin(real_t p_margin) {
	Shape3D::set_margin(p_margin);
	for (const Ref<ConvexPolygonShape3D> &piece : additional_shapes) {
		piece->set_margin(p_margin);
	}
}

void WedgeShape3D::set_custom_solver_bias(real_t p_bias) {
	Shape3D::set_custom_solver_bias(p_bias);
	for (const Ref<ConvexPolygonShape3D> &piece : additional_shapes) {
		piece->set_custom_solver_bias(p_bias);
	}
}

const Vector<Ref<ConvexPolygonShape3D>> &WedgeShape3D::get_additional_shapes() const {
	return additional_shapes;
}

Vector<Vector3> WedgeShape3D::get_debug_mesh_lines() const {
	Vector<Vector3> lines;
	if (angle <= 0.0) {
		return lines;
	}

	const real_t half_height = height * 0.5;
	const int steps = MAX(2, (int)Math::ceil(radial_segments * angle / 360.0));
	for (int i = 0; i < steps; i++) {
		const real_t a = -angle * 0.5 + angle * i / steps;
		const real_t b = -angle * 0.5 + angle * (i + 1) / steps;
		lines.push_back(wedge_arc_point(radius, half_height, a));
		lines.push_back(wedge_arc_point(radius, half_height, b));
		lines.push_back(wedge_arc_point(radius, -half_height, a));
		lines.push_back(wedge_arc_point(radius, -half_height, b));
	}

	// A 360-degree slice is a cylinder: no exposed radial planes.
	const int vertical_count = angle >= 360.0 ? 0 : 2;
	for (int i = 0; i < vertical_count; i++) {
		const real_t a = angle >= 360.0 ? i * 90.0 : (i == 0 ? -angle * 0.5 : angle * 0.5);
		lines.push_back(wedge_arc_point(radius, -half_height, a));
		lines.push_back(wedge_arc_point(radius, half_height, a));
	}
	if (angle < 360.0) {
		for (int side = 0; side < 2; side++) {
			const real_t a = side == 0 ? -angle * 0.5 : angle * 0.5;
			for (int sign = -1; sign <= 1; sign += 2) {
				lines.push_back(Vector3(0, sign * half_height, 0));
				lines.push_back(wedge_arc_point(radius, sign * half_height, a));
			}
		}
		lines.push_back(Vector3(0, -half_height, 0));
		lines.push_back(Vector3(0, half_height, 0));
	}
	return lines;
}

Ref<ArrayMesh> WedgeShape3D::get_debug_arraymesh_faces(const Color &p_modulate) const {
	Ref<ArrayMesh> mesh = memnew(ArrayMesh);
	if (angle <= 0.0) {
		return mesh;
	}

	Vector<Vector3> vertices;
	Vector<Color> colors;
	const real_t half_height = height * 0.5;
	const int steps = MAX(2, (int)Math::ceil(radial_segments * angle / 360.0));
	const Vector3 top_center(0, half_height, 0);
	const Vector3 bottom_center(0, -half_height, 0);

	for (int i = 0; i < steps; i++) {
		const real_t a = -angle * 0.5 + angle * i / steps;
		const real_t b = -angle * 0.5 + angle * (i + 1) / steps;
		const Vector3 top_a = wedge_arc_point(radius, half_height, a);
		const Vector3 top_b = wedge_arc_point(radius, half_height, b);
		const Vector3 bottom_a = wedge_arc_point(radius, -half_height, a);
		const Vector3 bottom_b = wedge_arc_point(radius, -half_height, b);

		push_triangle(vertices, colors, top_center, top_a, top_b, p_modulate);
		push_triangle(vertices, colors, bottom_center, bottom_b, bottom_a, p_modulate);
		push_triangle(vertices, colors, bottom_a, bottom_b, top_a, p_modulate);
		push_triangle(vertices, colors, top_a, bottom_b, top_b, p_modulate);
	}

	if (angle < 360.0) {
		const Vector3 top_start = wedge_arc_point(radius, half_height, -angle * 0.5);
		const Vector3 bottom_start = wedge_arc_point(radius, -half_height, -angle * 0.5);
		const Vector3 top_end = wedge_arc_point(radius, half_height, angle * 0.5);
		const Vector3 bottom_end = wedge_arc_point(radius, -half_height, angle * 0.5);
		push_triangle(vertices, colors, bottom_center, bottom_start, top_center, p_modulate);
		push_triangle(vertices, colors, top_center, bottom_start, top_start, p_modulate);
		push_triangle(vertices, colors, bottom_center, top_center, bottom_end, p_modulate);
		push_triangle(vertices, colors, top_center, top_end, bottom_end, p_modulate);
	}

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_COLOR] = colors;
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	return mesh;
}

real_t WedgeShape3D::get_enclosing_radius() const {
	return Vector2(radius, height * 0.5).length();
}

void WedgeShape3D::_validate_property(PropertyInfo &p_property) const {
	if (p_property.name == "points") {
		// Generated automatically from radius/height/angle; do not serialize.
		p_property.usage = PROPERTY_USAGE_NONE;
	}
}

void WedgeShape3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &WedgeShape3D::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &WedgeShape3D::get_radius);
	ClassDB::bind_method(D_METHOD("set_height", "height"), &WedgeShape3D::set_height);
	ClassDB::bind_method(D_METHOD("get_height"), &WedgeShape3D::get_height);
	ClassDB::bind_method(D_METHOD("set_angle", "angle"), &WedgeShape3D::set_angle);
	ClassDB::bind_method(D_METHOD("get_angle"), &WedgeShape3D::get_angle);
	ClassDB::bind_method(D_METHOD("set_radial_segments", "segments"), &WedgeShape3D::set_radial_segments);
	ClassDB::bind_method(D_METHOD("get_radial_segments"), &WedgeShape3D::get_radial_segments);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius", PROPERTY_HINT_RANGE, "0.001,100,0.001,or_greater,suffix:m"), "set_radius", "get_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height", PROPERTY_HINT_RANGE, "0.001,100,0.001,or_greater,suffix:m"), "set_height", "get_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "angle", PROPERTY_HINT_RANGE, "0,360,0.1,degrees"), "set_angle", "get_angle");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "radial_segments", PROPERTY_HINT_RANGE, "8,256,1"), "set_radial_segments", "get_radial_segments");
}

WedgeShape3D::WedgeShape3D() {
	_rebuild();
}
