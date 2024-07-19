#include "mesh.h"

StaticMesh::StaticMesh() : id(0)
{

}

bool StaticMesh::operator==(const StaticMesh & other) const
{
    return id == other.id;
}

DynamicMesh::DynamicMesh() : m_changed(false)
{

}

bool DynamicMesh::changed() const
{
    return m_changed;
}

void DynamicMesh::change_known()
{
    m_changed = false;
}

const Mesh & DynamicMesh::mesh() const
{
    return m_mesh;
}

Mesh & DynamicMesh::mesh()
{
    m_changed = true;
    return m_mesh;
}

const decltype(Mesh::vertices) & DynamicMesh::vertices() const { return mesh().vertices; }
const decltype(Mesh::indices) & DynamicMesh::indices() const { return mesh().indices; }
decltype(Mesh::vertices) & DynamicMesh::vertices() { return mesh().vertices; }
decltype(Mesh::indices) & DynamicMesh::indices() { return mesh().indices; }
