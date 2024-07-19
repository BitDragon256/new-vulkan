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
class ImmutableMesh : Mesh
{
public:

	ImmutableMesh();
};

// a mesh that checks if its vertices and indices changed
class MutableMesh
{
public:

	MutableMesh();
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
