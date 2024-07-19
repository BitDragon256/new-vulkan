#pragma once
#ifndef MESH_H
#define MESH_H

#include <vector>

#include "nve_types.h"

class Material;

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<Index> indices;
};

// a mesh that has no idea if its vertices and indices changed
struct StaticMesh : Mesh
{
	REF(Material) material;
	size_t id; // mesh id to uniquely identify the mesh

	StaticMesh();
	bool operator==(const StaticMesh &) const;
};

// a mesh that checks if its vertices and indices changed
struct DynamicMesh
{
	REF(Material) material;

	DynamicMesh();
	[[nodiscard]] bool changed() const;
	// tell the mesh that its changing is known (changed is now false)
	void change_known();
	// pure access to the mesh
	[[nodiscard]] const Mesh& mesh() const;
	// access to the mesh in anticipation of change
	[[nodiscard]] Mesh& mesh();

	[[nodiscard]] const decltype(Mesh::vertices)& vertices() const;
	[[nodiscard]] const decltype(Mesh::indices)& indices() const;

	[[nodiscard]] decltype(Mesh::vertices)& vertices();
	[[nodiscard]] decltype(Mesh::indices)& indices();
private:
	bool m_changed;
	Mesh m_mesh;
};

#endif //MESH_H
