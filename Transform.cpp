#include "Transform.h"

using namespace DirectX;


Transform::Transform()
{
	// Start with an identity matrix and basic transform data
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTransposeMatrix, XMMatrixIdentity());

	position = XMFLOAT3(0, 0, 0);
	pitchYawRoll = XMFLOAT3(0, 0, 0);
	scale = XMFLOAT3(1, 1, 1);

	// No need to recalc yet
	matricesDirty = false;

	parent = NULL;
}

void Transform::MoveAbsolute(float x, float y, float z)
{
	position.x += x;
	position.y += y;
	position.z += z;
	matricesDirty = true;
	MarkChildTransformsDirty();
}

void Transform::MoveRelative(float x, float y, float z)
{
	// Create a direction vector from the params
	// and a rotation quaternion
	XMVECTOR movement = XMVectorSet(x, y, z, 0);
	XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));

	// Rotate the movement by the quaternion
	XMVECTOR dir = XMVector3Rotate(movement, rotQuat);

	// Add and store, and invalidate the matrices
	XMStoreFloat3(&position, XMLoadFloat3(&position) + dir);
	matricesDirty = true;
	MarkChildTransformsDirty();
}

void Transform::Rotate(float p, float y, float r)
{
	pitchYawRoll.x += p;
	pitchYawRoll.y += y;
	pitchYawRoll.z += r;
	matricesDirty = true;
	MarkChildTransformsDirty();
}

void Transform::Scale(float x, float y, float z)
{
	scale.x *= x;
	scale.y *= y;
	scale.z *= z;
	matricesDirty = true;
	MarkChildTransformsDirty();
}

void Transform::SetPosition(float x, float y, float z)
{
	position.x = x;
	position.y = y;
	position.z = z;
	matricesDirty = true;
	MarkChildTransformsDirty();
}

void Transform::SetRotation(float p, float y, float r)
{
	pitchYawRoll.x = p;
	pitchYawRoll.y = y;
	pitchYawRoll.z = r;
	matricesDirty = true;
	MarkChildTransformsDirty();
}

void Transform::SetScale(float x, float y, float z)
{
	scale.x = x;
	scale.y = y;
	scale.z = z;
	matricesDirty = true;
	MarkChildTransformsDirty();
}

DirectX::XMFLOAT3 Transform::GetPosition() { return position; }

DirectX::XMFLOAT3 Transform::GetPitchYawRoll() { return pitchYawRoll; }

DirectX::XMFLOAT3 Transform::GetScale() { return scale; }


DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	UpdateMatrices();
	return worldMatrix;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	UpdateMatrices();
	return worldMatrix;
}

void Transform::AddChild(Transform* child)
{
	if (child == NULL) return;
	for (size_t i = 0; i < children.size(); i++)
	{
		if (children[i] == child) return;
	}
	
	children.push_back(child);
	child->parent = this;
	child->matricesDirty = true;
	child->MarkChildTransformsDirty();

}

void Transform::RemoveChild(Transform* child)
{
	unsigned int index = IndexOfChild(child);
	if (index == -1) return;
	children.erase(children.begin()+index);
	child->SetParent(NULL);
}

void Transform::SetParent(Transform* newParent)
{
	parent = newParent;
	if (parent != NULL)
	{
		newParent->children.push_back(this);
	}
	matricesDirty = true;
	MarkChildTransformsDirty();
}

Transform* Transform::GetParent()
{
	return parent;
}

Transform* Transform::GetChild(unsigned int index)
{
	if (index > children.size() || index < 0) return NULL;
	return children[index];
}

int Transform::IndexOfChild(Transform* child)
{
	int index = -1;
	for (size_t i = 0; i < children.size(); i++)
	{
		if (children[i] == child) index = i;
	}
	return index;
}

unsigned int Transform::GetChildCount()
{
	return children.size();
}

void Transform::UpdateMatrices()
{
	// Are the matrices out of date (dirty)?
	if (matricesDirty)
	{
		// Create the three transformation pieces
		XMMATRIX trans = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
		XMMATRIX rot = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));
		XMMATRIX sc = XMMatrixScalingFromVector(XMLoadFloat3(&scale));

		// Combine and store the world
		XMMATRIX wm = sc * rot * trans;
		if (parent)
		{
			XMFLOAT4X4 pW4X4 = parent->GetWorldMatrix();
			XMMATRIX pW = XMLoadFloat4x4(&pW4X4);
			wm *= pW;
		}
		XMStoreFloat4x4(&worldMatrix, wm);

		// Invert and transpose, too
		XMStoreFloat4x4(&worldInverseTransposeMatrix, XMMatrixInverse(0, XMMatrixTranspose(wm)));

		// All set
		matricesDirty = false;
	}
}

void Transform::MarkChildTransformsDirty()
{
	for (size_t i = 0; i < children.size(); i++)
	{
		children[i]->matricesDirty = true;
		children[i]->MarkChildTransformsDirty();
	}
}
