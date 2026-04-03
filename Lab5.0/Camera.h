#pragma once

#include <DirectXMath.h>
#include <windows.h>
#include <algorithm>
#include <cmath>

using namespace DirectX;

template<typename T>
static inline T Clamp(T val, T lo, T hi)
{
    return val < lo ? lo : (val > hi ? hi : val);
}


class OrbitalCamera
{
public:
    float sensitivity = 0.005f;   
    float panSpeed = 0.002f;  
    float zoomSpeed = 0.5f;     
    float keySpeed = 30.0f; 
    float minRadius = 0.5f;
    float maxRadius = 500.0f;

    OrbitalCamera() = default;
    OrbitalCamera(float radius, float yaw, float pitch)
        : mRadius(radius), mYaw(yaw), mPitch(pitch) {}


    void OnMouseDown(WPARAM btn, int x, int y)
    {
        mLastX = x; mLastY = y;
        if (btn & MK_LBUTTON) mDraggingL = true;
        if (btn & MK_RBUTTON) mDraggingR = true;
        SetCapture(nullptr); // захват мыши снаружи
    }

    void OnMouseUp(WPARAM btn)
    {
        if (!(btn & MK_LBUTTON)) mDraggingL = false;
        if (!(btn & MK_RBUTTON)) mDraggingR = false;
        ReleaseCapture();
    }

    void OnMouseMove(WPARAM btn, int x, int y)
    {
        int dx = x - mLastX;
        int dy = y - mLastY;
        mLastX = x; mLastY = y;

        if (mDraggingL)
        {
            // Вращение
            mYaw += dx * sensitivity;
            mPitch += dy * sensitivity;
        }
        else if (mDraggingR)
        {
            // Pan в плоскости камеры
            XMVECTOR right = GetRight();
            XMVECTOR up = GetUp();

            XMVECTOR delta = XMVectorScale(right, -(float)dx * panSpeed * mRadius)
                + XMVectorScale(up, +(float)dy * panSpeed * mRadius);

            XMStoreFloat3(&mTarget,
                XMVectorAdd(XMLoadFloat3(&mTarget), delta));
        }
    }

    void OnMouseWheel(int delta)
    {
        float sign = (delta > 0) ? -1.0f : 1.0f;
        mRadius = Clamp(mRadius + sign * zoomSpeed * mRadius * 0.1f,
            minRadius, maxRadius);
    }


    void Update(float dt)
    {
        // Zoom: W / S
        if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8000)
            mRadius -= keySpeed * dt;
        if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8000)
            mRadius += keySpeed * dt;

        // Горизонтальный pan: A / D
        XMVECTOR right = GetRight();
        if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000)
        {
            XMVECTOR delta = XMVectorScale(right, -keySpeed * dt);
            XMStoreFloat3(&mTarget, XMVectorAdd(XMLoadFloat3(&mTarget), delta));
        }
        if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000)
        {
            XMVECTOR delta = XMVectorScale(right, +keySpeed * dt);
            XMStoreFloat3(&mTarget, XMVectorAdd(XMLoadFloat3(&mTarget), delta));
        }

        // Вертикальный pan: Q / E
        if (GetAsyncKeyState('Q') & 0x8000)
        {
            XMVECTOR up = GetWorldUp();
            XMVECTOR delta = XMVectorScale(up, -keySpeed * dt);
            XMStoreFloat3(&mTarget, XMVectorAdd(XMLoadFloat3(&mTarget), delta));
        }
        if (GetAsyncKeyState('E') & 0x8000)
        {
            XMVECTOR up = GetWorldUp();
            XMVECTOR delta = XMVectorScale(up, +keySpeed * dt);
            XMStoreFloat3(&mTarget, XMVectorAdd(XMLoadFloat3(&mTarget), delta));
        }
    }

    XMMATRIX GetViewMatrix() const
    {
        XMVECTOR eye = GetEyePosition();
        XMVECTOR target = XMLoadFloat3(&mTarget);
        XMVECTOR up = XMVectorSet(0, 1, 0, 0);
        return XMMatrixLookAtLH(eye, target, up);
    }

    XMFLOAT3 GetEyePosW() const
    {
        XMFLOAT3 pos;
        XMStoreFloat3(&pos, GetEyePosition());
        return pos;
    }

private:
    float      mYaw = 0.5f;
    float      mPitch = 0.4f;
    float      mRadius = 6.0f;
    XMFLOAT3   mTarget = { 0.f, 0.f, 0.f };

    bool mDraggingL = false;
    bool mDraggingR = false;
    int  mLastX = 0, mLastY = 0;

    XMVECTOR GetEyePosition() const
    {
        float x = mRadius * cosf(mPitch) * sinf(mYaw);
        float y = mRadius * sinf(mPitch);
        float z = mRadius * cosf(mPitch) * cosf(mYaw);

        XMFLOAT3 target = mTarget;
        return XMVectorSet(target.x + x, target.y + y, target.z + z, 1.f);
    }

    // Правый вектор камеры (для pan)
    XMVECTOR GetRight() const
    {
        XMVECTOR forward = XMVectorSubtract(
            XMLoadFloat3(&mTarget), GetEyePosition());
        forward = XMVector3Normalize(forward);
        XMVECTOR worldUp = XMVectorSet(0, 1, 0, 0);
        return XMVector3Normalize(XMVector3Cross(forward, worldUp));
    }

    // Верхний вектор камеры (для pan)
    XMVECTOR GetUp() const
    {
        XMVECTOR forward = XMVectorSubtract(
            XMLoadFloat3(&mTarget), GetEyePosition());
        forward = XMVector3Normalize(forward);
        return XMVector3Normalize(XMVector3Cross(GetRight(), forward));
    }

    XMVECTOR GetWorldUp() const { return XMVectorSet(0, 1, 0, 0); }
};