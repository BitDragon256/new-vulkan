#include "mesh.h"

ImmutableMesh::ImmutableMesh()
= default;

MutableMesh::MutableMesh() : m_changed(false)
{

}

bool MutableMesh::changed() const
{
    return m_changed;
}

void MutableMesh::change_known()
{
    m_changed = false;
}

const Mesh & MutableMesh::mesh() const
{
    return m_mesh;
}

Mesh & MutableMesh::mesh()
{
    m_changed = true;
    return m_mesh;
}

const decltype(Mesh::vertices) & MutableMesh::vertices() const { return mesh().vertices; }
const decltype(Mesh::indices) & MutableMesh::indices() const { return mesh().indices; }
decltype(Mesh::vertices) & MutableMesh::vertices() { return mesh().vertices; }
decltype(Mesh::indices) & MutableMesh::indices() { return mesh().indices; }
