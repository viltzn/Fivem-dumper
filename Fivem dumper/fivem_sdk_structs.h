#pragma once
#include <windows.h>
#include <cstdint>

// ============================================================================
// FiveM / GTA V Reconstructed SDK Struct Layouts & Offsets
// Reconstructed for @Bombeule Offset Dumper & Engine Analyzer
// Architecture: x64 (__fastcall ABI)
// ============================================================================

#pragma pack(push, 1)

// 3D Vector Representation
struct Vector3 {
    float x; // 0x00
    float y; // 0x04
    float z; // 0x08
    float _pad; // 0x0C (16-byte alignment)
};

// 4x4 View / Projection Transformation Matrix
struct Matrix4x4 {
    float m[4][4];
};

// ----------------------------------------------------------------------------
// 1. Top-Level Singletons & Engine Interfaces
// ----------------------------------------------------------------------------

// CWorld Struct Layout
struct CWorld {
    uint8_t _pad0[0x8];       // 0x0000 - VTable
    struct CPed* LocalPlayer; // 0x0008 - Pointer to Local CPed Instance
};

// ----------------------------------------------------------------------------
// 2. Weapon & Handling Substructures
// ----------------------------------------------------------------------------

struct CWeaponInfo {
    uint8_t _pad0[0x10];       // 0x0000
    uint32_t Hash;             // 0x0010 - Weapon Hash ID
    uint8_t _pad1[0x4];        // 0x0014
    float BulletDamage;        // 0x0018 - Base Bullet Damage
    uint8_t _pad2[0x4];        // 0x001C
    float WeaponRange;         // 0x0020 - Maximum Firing Distance
    uint8_t _pad3[0x54];       // 0x0024
    float Spread;              // 0x0078 - Accuracy / Spread Multiplier
    float Recoil;              // 0x007C - Recoil Kick Multiplier
};

struct CWeapon {
    uint8_t _pad0[0x20];            // 0x0000
    CWeaponInfo* WeaponInfo;        // 0x0020 - Pointer to Weapon Metadata
    uint8_t _pad1[0x34];            // 0x0028
    uint32_t ClipAmmo;              // 0x005C - Current Magazine Ammo
    uint32_t AmmoCount;             // 0x0060 - Reserve Ammo
};

struct CWeaponManager {
    uint8_t _pad0[0x20];            // 0x0000
    CWeaponInfo* CurrentWeaponInfo; // 0x0020 - Active Weapon Metadata
    uint8_t _pad1[0x40];            // 0x0028
    CWeapon* CurrentWeapon;         // 0x0068 - Active Weapon Instance
};

struct CHandlingData {
    uint8_t _pad0[0x0C];       // 0x0000
    float Mass;                // 0x000C - Vehicle Mass (kg)
    float InitialDragCoeff;    // 0x0010 - Air Resistance Coefficient
    uint8_t _pad1[0x2C];       // 0x0014
    Vector3 CenterOfMass;      // 0x0040 - Center of Gravity Vector
    uint8_t _pad2[0x24];       // 0x0050
    float InitialDriveForce;   // 0x0074 - Acceleration / Engine Force
    uint8_t _pad3[0x24];       // 0x0078
    float BrakeForce;          // 0x009C - Braking Power
};

// ----------------------------------------------------------------------------
// 3. Vehicle & Ped Entities
// ----------------------------------------------------------------------------

struct CVehicle {
    uint8_t _pad0[0x30];       // 0x0000 - VTable & Transform Header
    Vector3 Position;          // 0x0030 - Vehicle World Position
    uint8_t _pad1[0x24C];      // 0x003C
    float EngineHealth;        // 0x0280 - Engine Damage Health (0.0 - 1000.0)
    float BodyHealth;          // 0x0284 - Bodywork Health (0.0 - 1000.0)
    uint8_t _pad2[0x18];       // 0x0288
    float FuelTankHealth;      // 0x02A0 - Fuel Tank Health
    uint8_t _pad3[0x52C];      // 0x02A4
    CHandlingData* Handling;   // 0x07D0 - Pointer to Vehicle Handling Data
    Vector3 Velocity;          // 0x07D8 - 3D Velocity Vector
};

struct CPlayerInfo {
    uint8_t _pad0[0x0C0];      // 0x0000
    float Stamina;             // 0x00C0 - Player Sprint Stamina
    uint8_t _pad1[0x3C];       // 0x00C4
    float WantedLevel;         // 0x0100 - Wanted Level Intensity
};

struct CPed {
    uint8_t _pad0[0x28];            // 0x0000
    Vector3 Position;               // 0x0028 - Ped 3D World Position
    uint8_t _pad1[0x254];           // 0x0038
    CPlayerInfo* PlayerInfo;        // 0x028C - Player Controller Pointer (Null if NPC)
    uint8_t _pad2[0x10];            // 0x0294
    float Health;                   // 0x02A4 - Current Ped Health
    float MaxHealth;                // 0x02A8 - Maximum Ped Health Capacity
    uint8_t _pad3[0xA58];           // 0x02AC
    CVehicle* CurrentVehicle;       // 0x0D04 - Mounted Vehicle Pointer
    uint8_t _pad4[0x94];            // 0x0D0C
    float Armor;                    // 0x0DA0 - Ped Body Armor Points
    uint8_t _pad5[0x310];           // 0x0DA4
    CWeaponManager* WeaponManager;  // 0x10B4 - Inventory / Active Weapon Manager
};

// ----------------------------------------------------------------------------
// 4. Replay Interface & Generic Entity Pools
// ----------------------------------------------------------------------------

struct PoolDescriptor {
    uintptr_t PoolBase;   // 0x00 - Base pointer to allocated memory array
    uint32_t MaxCapacity; // 0x08 - Maximum elements (e.g. 256 Peds, 300 Vehicles)
    uint32_t ElementSize; // 0x0C - Stride / byte size per slot
    uintptr_t HandleArray;// 0x10 - Active slots bitfield / handle map
    uint32_t CurrentCount;// 0x18 - Currently active spawned entity count
};

struct CReplayInterface {
    uint8_t _pad0[0x18];
    uintptr_t PedPoolHeader;    // 0x0018 - Pointer to Ped Pool Structure
    uint8_t _pad1[0x38];
    uintptr_t VehiclePoolHeader;// 0x0058 - Pointer to Vehicle Pool Structure
    uint8_t _pad2[0x10];
    uintptr_t ObjectPoolHeader; // 0x0070 - Pointer to Object Pool Structure
    uint8_t _pad3[0x28];
    uintptr_t PickupPoolHeader; // 0x00A0 - Pointer to Pickup Item Pool Structure
};

// ----------------------------------------------------------------------------
// 5. Viewport & Camera Engine Structures
// ----------------------------------------------------------------------------

struct CViewport {
    uint8_t _pad0[0x24C0];
    Matrix4x4 ViewMatrix; // 0x24C0 - 4x4 View-Projection Transformation Matrix
};

struct CCamera {
    uint8_t _pad0[0x0D0];
    Vector3 Position;     // 0x00D0 - Camera Position Vector
    uint8_t _pad1[0x030];
    Vector3 Rotation;     // 0x010C - Camera Euler Angles (Pitch, Roll, Yaw)
    uint8_t _pad2[0x010];
    float FOV;            // 0x0128 - Field of View angle
};

#pragma pack(pop)

// ----------------------------------------------------------------------------
// 6. 3D World-To-Screen Matrix Math Transformation Helper
// ----------------------------------------------------------------------------

inline bool WorldToScreen(const Vector3& worldPos, const Matrix4x4& viewMatrix, int screenWidth, int screenHeight, Vector3& screenPos) {
    // Transpose matrix multiplication for GTA V view matrix layout
    float w = viewMatrix.m[0][3] * worldPos.x + viewMatrix.m[1][3] * worldPos.y + viewMatrix.m[2][3] * worldPos.z + viewMatrix.m[3][3];
    if (w < 0.001f) {
        return false; // Point is behind the camera plane
    }

    float x = viewMatrix.m[0][0] * worldPos.x + viewMatrix.m[1][0] * worldPos.y + viewMatrix.m[2][0] * worldPos.z + viewMatrix.m[3][0];
    float y = viewMatrix.m[0][1] * worldPos.x + viewMatrix.m[1][1] * worldPos.y + viewMatrix.m[2][1] * worldPos.z + viewMatrix.m[3][1];

    float invW = 1.0f / w;
    x *= invW;
    y *= invW;

    float halfWidth = static_cast<float>(screenWidth) * 0.5f;
    float halfHeight = static_cast<float>(screenHeight) * 0.5f;

    screenPos.x = halfWidth + (0.5f * x * static_cast<float>(screenWidth) + 0.5f);
    screenPos.y = halfHeight - (0.5f * y * static_cast<float>(screenHeight) + 0.5f);
    screenPos.z = w;

    return true;
}
