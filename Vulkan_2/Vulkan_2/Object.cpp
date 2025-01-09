#include "pch.h"
#include "Object.h"
#include "Transform.h"
#include "Mesh.h"
#include "Material.h"

Object::Object()
{
	_transform = new Component::Transform();
}

Object::~Object()
{
	delete _transform;
	delete _mesh;
	delete _material;
}

void Object::AddMesh(const char* file)
{
	_mesh = new Resource::Mesh(file);
}

void Object::AddMaterial(const char* file)
{
	_material = new Resource::Material(file);
}
