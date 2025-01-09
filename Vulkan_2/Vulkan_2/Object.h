#pragma once

namespace Engine
{

}

namespace Resource
{
	class Mesh;
	class Material;
}

namespace Component
{
	class Transform;
}

class Object
{
public:
	Object();
	~Object();

	void AddMesh(const char* file);
	void AddMaterial(const char* file);

#pragma region Getters

	Resource::Mesh* GetMesh() { return _mesh; }
	Resource::Material* GetMaterial() { return _material; }
	Component::Transform* GetTransform() { return _transform; }

#pragma endregion

private:
	Resource::Mesh* _mesh;
	Resource::Material* _material;
	Component::Transform* _transform;
};