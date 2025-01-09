#pragma once

namespace Engine
{
	
}

namespace Resource
{

}

namespace Component
{
	class Transform
	{
	public:
		Transform();
		~Transform();

#pragma region Getters

		glm::vec3 GetPosition() { return _position; }
		glm::vec3 GetRotation() { return _rotation; }
		glm::vec3 GetScale() { return _scale; }

		glm::mat4 GetModelMatrix()
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, _position);
			model = glm::rotate(model, glm::radians(_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, glm::radians(_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, _scale);
			return model;
		}

#pragma endregion

#pragma region Setters

		void SetPosition(glm::vec3 position) { _position = position; }
		void SetRotation(glm::vec3 rotation) { _rotation = rotation; }
		void SetScale(glm::vec3 scale) { _scale = scale; }

#pragma endregion

	private:
		glm::vec3 _position;
		glm::vec3 _rotation;
		glm::vec3 _scale;
	};
}
