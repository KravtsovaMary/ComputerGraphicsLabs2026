// Shaders.hlsl

cbuffer PerObjectCB : register(b0)
{
    float4x4 gWorld;
    float4x4 gWorldViewProj;

    float3 gLightPosW;
    float  pad0;

    float3 gEyePosW;
    float  pad1;

    float4 gDiffuseColor;
    float4 gSpecColorPower;

    float  gUVOffsetX;
    float  gUVOffsetY;
    float  gUVTileX;
    float  gUVTileY;

    int    gUseTexture;
    float3 pad2;

    float  gSpotIntensity;      // интенсивность точки
    float  gSpotSpeed;          // текущая скорость для отладки
    float2 gSpotPosition;       // позиция точки на текстуре (UV координаты)
};

Texture2D    gDiffuseMap : register(t0);
SamplerState gSampler    : register(s0);

struct VertexIn
{
    float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC    : TEXCOORD;
    float4 Color   : COLOR;
};

struct VertexOut
{
    float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC    : TEXCOORD;
    float4 Color   : COLOR;
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;

    vout.PosH   = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    vout.PosW   = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.NormalW = normalize(mul(vin.NormalL, (float3x3)gWorld));

    float2 uv;
    uv.x = vin.TexC.x * gUVTileX + gUVOffsetX;
    uv.y = vin.TexC.y * gUVTileY + gUVOffsetY;
    vout.TexC = uv;

    vout.Color = vin.Color;

    return vout;
}

float4 PSMain(VertexOut pin) : SV_TARGET
{
    float3 normal   = normalize(pin.NormalW);
    float3 lightDir = normalize(gLightPosW - pin.PosW);
    float3 viewDir  = normalize(gEyePosW   - pin.PosW);
    float3 halfVec  = normalize(lightDir + viewDir);

    float4 baseColor;
    if (gUseTexture)
    {
        baseColor = gDiffuseMap.Sample(gSampler, pin.TexC);
    }
    else
    {
        baseColor = pin.Color * gDiffuseColor;
    }

    // Добавляем яркую точку для отслеживания скорости вращения
    float2 spotPos = gSpotPosition;
    
    // Делаем точку движущейся вместе с текстурой
    // Корректируем позицию точки с учетом UVOffset
    float2 adjustedSpotPos = spotPos;
    adjustedSpotPos.x = frac(adjustedSpotPos.x - gUVOffsetX / gUVTileX);
    adjustedSpotPos.y = frac(adjustedSpotPos.y - gUVOffsetY / gUVTileY);
    
    // Вычисляем расстояние до точки в UV-пространстве
    float2 distToSpot = abs(pin.TexC - adjustedSpotPos);
    // Учитываем зацикливание текстуры (wrap mode)
    distToSpot = min(distToSpot, 1.0 - distToSpot);
    float spotDistance = length(distToSpot);
    
    // Параметры точки
    float spotRadius = 0.05f;          // радиус точки
    float spotHardness = 0.02f;         // резкость границ
    
    // Создаем яркую точку с плавными краями
    float spotMask = 1.0 - smoothstep(spotRadius - spotHardness, spotRadius, spotDistance);
    
    // Добавляем ореол вокруг точки для лучшей видимости
    float glowRadius = 0.15f;
    float glowMask = 1.0 - smoothstep(glowRadius - spotHardness * 2, glowRadius, spotDistance);
    glowMask *= 0.3f; // полупрозрачный ореол
    
    // Яркий желто-белый цвет для точки
    float3 spotColor = float3(1.0, 0.9, 0.5); // теплый желтый
    
    // Добавляем эффект пульсации (опционально)
    float pulse = 1.0 + 0.2 * sin(gUVOffsetX * 50.0); // пульсация яркости
    
    // Смешиваем точку с основным цветом
    float3 mixedColor = baseColor.rgb;
    
    // Добавляем ореол
    mixedColor = lerp(mixedColor, spotColor * 1.5, glowMask * gSpotIntensity * 0.5);
    
    // Добавляем саму точку
    mixedColor = lerp(mixedColor, spotColor * (2.0 + pulse), spotMask * gSpotIntensity);
    
    // Добавляем светящийся след для отслеживания направления движения
    float trailLength = 0.2f;
    float trailWidth = 0.03f;
    
    // Создаем след в направлении движения (по U координате)
    float trailDistU = abs(pin.TexC.x - adjustedSpotPos.x);
    trailDistU = min(trailDistU, 1.0 - trailDistU);
    
    // След появляется только в определенном диапазоне по U
    float trailMask = 0.0;
    if (trailDistU < trailLength && abs(pin.TexC.y - adjustedSpotPos.y) < trailWidth)
    {
        trailMask = (1.0 - trailDistU / trailLength) * 0.3f;
    }

    mixedColor = lerp(mixedColor, spotColor * 1.2, trailMask * gSpotIntensity);

    float  diff    = max(dot(normal, lightDir), 0.0f);
    float3 diffuse = diff * mixedColor;

    float  spec    = pow(max(dot(normal, halfVec), 0.0f), gSpecColorPower.w);
    float3 specular = spec * gSpecColorPower.xyz;

    float3 ambient = 0.15f * mixedColor;

    float3 finalColor = ambient + diffuse + specular;

    return float4(finalColor, baseColor.a);
}
