#include "pch.h"
#include "Transform.h"

Component::Transform::Transform()
	: _position(0, 0, 0)
	, _rotation(0, 0, 0)
	, _scale(1, 1, 1)
{
}

Component::Transform::~Transform()
{
}
