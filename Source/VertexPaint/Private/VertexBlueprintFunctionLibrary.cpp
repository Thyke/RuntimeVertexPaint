// Copyright (C) Thyke. All Rights Reserved.

#include "VertexBlueprintFunctionLibrary.h"
#include "StaticMeshComponentLODInfo.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "Engine/Texture2D.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VertexBlueprintFunctionLibrary)

bool UVertexBlueprintFunctionLibrary::ValidateMeshForPainting(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, int32& OutVertexCount)
{
    if (!StaticMeshComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("VertexPaint: Invalid StaticMeshComponent"));
        return false;
    }

    if (!StaticMeshComponent->GetStaticMesh())
    {
        UE_LOG(LogTemp, Warning, TEXT("VertexPaint: StaticMeshComponent has no StaticMesh"));
        return false;
    }

    const int32 LODNum = StaticMeshComponent->GetStaticMesh()->GetNumLODs();
    StaticMeshComponent->SetLODDataCount(LODNum, LODNum);

    if (!StaticMeshComponent->LODData.IsValidIndex(LODIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("VertexPaint: Invalid LOD index: %d, Max: %d"), LODIndex, LODNum - 1);
        return false;
    }

    OutVertexCount = StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[LODIndex].GetNumVertices();
    return true;
}

void UVertexBlueprintFunctionLibrary::ApplyColorBuffer(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, const TArray<FColor>& VertexColors)
{
    FStaticMeshComponentLODInfo& LODInfo = StaticMeshComponent->LODData[LODIndex];
    
    // Clear old buffer if exists
    if (LODInfo.OverrideVertexColors)
    {
        BeginReleaseResource(LODInfo.OverrideVertexColors);
        FlushRenderingCommands();
        delete LODInfo.OverrideVertexColors;
        LODInfo.OverrideVertexColors = nullptr;
    }
    
    // Create and initialize new buffer
    LODInfo.OverrideVertexColors = new FColorVertexBuffer;
    LODInfo.OverrideVertexColors->InitFromColorArray(VertexColors);
    BeginInitResource(LODInfo.OverrideVertexColors);
    
    StaticMeshComponent->MarkRenderStateDirty();
}

float UVertexBlueprintFunctionLibrary::GetVertexDistanceNormalizedToShape(const FVector& VertexPosition, EVertexPaintShape Shape, const FVector& Location, const FVector& Dimensions, const FRotator& Rotation)
{
    // Transform vertex position to shape's local coordinates
    FVector RelativePos = VertexPosition - Location;
    if (!Rotation.IsZero())
    {
        RelativePos = Rotation.UnrotateVector(RelativePos);
    }
    
    float NormalizedDistance = 1.0f;
    
    switch (Shape)
    {
    case EVertexPaintShape::Point:
        // For point, use direct distance
        NormalizedDistance = RelativePos.Size() / FMath::Max(0.1f, Dimensions.X);
        break;
        
    case EVertexPaintShape::Sphere:
        // For sphere, distance normalized by radius
        NormalizedDistance = RelativePos.Size() / FMath::Max(0.1f, Dimensions.X);
        break;
        
    case EVertexPaintShape::Box:
        // For box, maximum of normalized distances along each axis
        if (Dimensions.X > 0.0f && Dimensions.Y > 0.0f && Dimensions.Z > 0.0f)
        {
            FVector NormalizedPos = FVector(
                FMath::Abs(RelativePos.X) / (Dimensions.X * 0.5f),
                FMath::Abs(RelativePos.Y) / (Dimensions.Y * 0.5f),
                FMath::Abs(RelativePos.Z) / (Dimensions.Z * 0.5f)
            );
            NormalizedDistance = FMath::Max3(NormalizedPos.X, NormalizedPos.Y, NormalizedPos.Z);
        }
        break;
        
    case EVertexPaintShape::Cylinder:
        // For cylinder, distance normalized by radius (X) and height (Z)
        if (Dimensions.X > 0.0f && Dimensions.Z > 0.0f)
        {
            float RadialDist = FMath::Sqrt(RelativePos.X * RelativePos.X + RelativePos.Y * RelativePos.Y) / (Dimensions.X * 0.5f);
            float HeightDist = FMath::Abs(RelativePos.Z) / (Dimensions.Z * 0.5f);
            NormalizedDistance = FMath::Max(RadialDist, HeightDist);
        }
        break;
    }
    
    return NormalizedDistance;
}

FColor UVertexBlueprintFunctionLibrary::BlendVertexColors(FColor BaseColor, FColor BlendColor, EVertexColorBlendMode BlendMode, float BlendStrength)
{
    FLinearColor LinearBase = FLinearColor::FromSRGBColor(BaseColor);
    FLinearColor LinearBlend = FLinearColor::FromSRGBColor(BlendColor);
    FLinearColor Result;
    
    // Limit blend strength to [0,1] range
    BlendStrength = FMath::Clamp(BlendStrength, 0.0f, 1.0f);
    
    switch (BlendMode)
    {
    case EVertexColorBlendMode::Replace:
        // Replace completely with new color (proportional to strength)
        Result = FLinearColor::LerpUsingHSV(LinearBase, LinearBlend, BlendStrength);
        break;
        
    case EVertexColorBlendMode::Add:
        // Add colors (proportional to strength)
        Result = LinearBase + (LinearBlend * BlendStrength);
        Result = Result.GetClamped(0.0f, 1.0f); // Clamp to [0,1] range
        break;
        
    case EVertexColorBlendMode::Multiply:
        // Multiply colors
        {
            FLinearColor Multiplier = FLinearColor::LerpUsingHSV(
                FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), // White (neutral for multiplication)
                LinearBlend,
                BlendStrength
            );
            Result = LinearBase * Multiplier;
        }
        break;
        
    case EVertexColorBlendMode::Lerp:
        // Linear interpolation
        Result = FLinearColor::LerpUsingHSV(LinearBase, LinearBlend, BlendStrength);
        break;
        
    default:
        Result = LinearBase;
        break;
    }
    
    // Preserve alpha value
    Result.A = LinearBase.A;
    
    return Result.ToFColor(true);
}

void UVertexBlueprintFunctionLibrary::PaintVertexColorByIndex(UStaticMeshComponent* StaticMeshComponent, FLinearColor Color, int32 Index, int32 LODIndex)
{
    int32 VertexNum = 0;
    if (!ValidateMeshForPainting(StaticMeshComponent, LODIndex, VertexNum))
    {
        return;
    }
    
    if (Index < 0 || Index >= VertexNum)
    {
        UE_LOG(LogTemp, Warning, TEXT("VertexPaint: Index out of range: %d, Vertex Count: %d"), Index, VertexNum);
        return;
    }
    
    TArray<FColor> VertexColors = GetStaticMeshVertexColors(StaticMeshComponent, LODIndex);
    VertexColors[Index] = Color.ToFColor(true);
    
    ApplyColorBuffer(StaticMeshComponent, LODIndex, VertexColors);
}

TArray<FColor> UVertexBlueprintFunctionLibrary::GetStaticMeshVertexColors(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex)
{
    TArray<FColor> VertexColors;
    int32 VertexNum = 0;
    
    if (!ValidateMeshForPainting(StaticMeshComponent, LODIndex, VertexNum))
    {
        return VertexColors;
    }
    
    VertexColors.SetNum(VertexNum);
    VertexColors.Init(FColor::White, VertexNum);
    
    FStaticMeshComponentLODInfo& LODInfo = StaticMeshComponent->LODData[LODIndex];
    if (LODInfo.OverrideVertexColors)
    {
        LODInfo.OverrideVertexColors->GetVertexColors(VertexColors);
    }
    else if (StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[LODIndex].bHasColorVertexData)
    {
        StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[LODIndex].VertexBuffers.ColorVertexBuffer.GetVertexColors(VertexColors);
    }
    
    return VertexColors;
}

void UVertexBlueprintFunctionLibrary::OverrideStaticMeshVertexColor(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, TArray<FVertexOverrideColorInfo> VertexOverrideColorInfos)
{
    int32 VertexNum = 0;
    if (!ValidateMeshForPainting(StaticMeshComponent, LODIndex, VertexNum))
    {
        return;
    }
    
    TArray<FColor> VertexColors = GetStaticMeshVertexColors(StaticMeshComponent, LODIndex);
    
    for (const FVertexOverrideColorInfo& VertexOverrideColorInfo : VertexOverrideColorInfos)
    {
        if (VertexColors.IsValidIndex(VertexOverrideColorInfo.VertexIndex))
        {
            VertexColors[VertexOverrideColorInfo.VertexIndex] = VertexOverrideColorInfo.OverrideColor;
        }
    }
    
    ApplyColorBuffer(StaticMeshComponent, LODIndex, VertexColors);
}

TArray<FVertexOverrideColorInfo> UVertexBlueprintFunctionLibrary::GetStaticMeshVertexOverrideColorInfoInSphere(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, FVector SphereWorldPosition, float Radius, FLinearColor OverrideColor)
{
    TArray<FVertexOverrideColorInfo> VertexOverrideColorInfos;
    int32 VertexNum = 0;
    
    if (!ValidateMeshForPainting(StaticMeshComponent, LODIndex, VertexNum))
    {
        return VertexOverrideColorInfos;
    }
    
    const FTransform StaticMeshWorldTransform = StaticMeshComponent->GetComponentTransform();
    const FVector SphereLocationInMeshTransform = UKismetMathLibrary::InverseTransformLocation(StaticMeshWorldTransform, SphereWorldPosition);
    const FPositionVertexBuffer& VertexPositionBuffer = StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[LODIndex].VertexBuffers.PositionVertexBuffer;
    
    for (int32 VertexIndex = 0; VertexIndex < VertexNum; VertexIndex++)
    {
        const FVector VertexPosition = static_cast<UE::Math::TVector4<double>>(VertexPositionBuffer.VertexPosition(VertexIndex));
        const float Distance = UKismetMathLibrary::Vector_Distance(VertexPosition, SphereLocationInMeshTransform);
        const float NormalizedDistance = Distance / Radius;
        
        if (NormalizedDistance <= 1.0f)
        {
            FVertexOverrideColorInfo VertexOverrideColorInfo;
            VertexOverrideColorInfo.VertexIndex = VertexIndex;
            VertexOverrideColorInfo.OverrideColor = OverrideColor.ToFColor(true);
            VertexOverrideColorInfos.Add(VertexOverrideColorInfo);
        }
    }
    
    return VertexOverrideColorInfos;
}

bool UVertexBlueprintFunctionLibrary::PaintMeshRegion(UStaticMeshComponent* StaticMeshComponent, EVertexPaintShape Shape, FVector Location, FVector Dimensions, FRotator Rotation, FLinearColor Color, EVertexColorBlendMode BlendMode, float BlendStrength, float Falloff, int32 LODIndex)
{
    int32 VertexNum = 0;
    if (!ValidateMeshForPainting(StaticMeshComponent, LODIndex, VertexNum))
    {
        return false;
    }
    
    // Transform world location to mesh's local coordinates
    const FTransform StaticMeshWorldTransform = StaticMeshComponent->GetComponentTransform();
    const FVector LocationInMeshTransform = UKismetMathLibrary::InverseTransformLocation(StaticMeshWorldTransform, Location);
    const FRotator RotationInMeshTransform = UKismetMathLibrary::InverseTransformRotation(StaticMeshWorldTransform, Rotation);
    
    // Get vertex positions and current colors
    const FPositionVertexBuffer& VertexPositionBuffer = StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[LODIndex].VertexBuffers.PositionVertexBuffer;
    TArray<FColor> VertexColors = GetStaticMeshVertexColors(StaticMeshComponent, LODIndex);
    
    // Limit falloff value
    Falloff = FMath::Clamp(Falloff, 0.01f, 0.99f);
    
    // Process each vertex
    bool bAnyVertexPainted = false;
    for (int32 VertexIndex = 0; VertexIndex < VertexNum; VertexIndex++)
    {
        const FVector VertexPosition = static_cast<UE::Math::TVector4<double>>(VertexPositionBuffer.VertexPosition(VertexIndex));
        
        // Calculate normalized distance based on shape
        const float NormalizedDistance = GetVertexDistanceNormalizedToShape(
            VertexPosition, 
            Shape, 
            LocationInMeshTransform, 
            Dimensions, 
            RotationInMeshTransform
        );
        
        // Paint if inside the shape
        if (NormalizedDistance <= 1.0f)
        {
            bAnyVertexPainted = true;
            
            // Calculate falloff (edges are less affected)
            float DistanceAlpha = 1.0f;
            if (NormalizedDistance > Falloff)
            {
                // Linear interpolation between falloff and 1.0
                DistanceAlpha = 1.0f - ((NormalizedDistance - Falloff) / (1.0f - Falloff));
            }
            
            // Calculate effective blend value
            float EffectiveBlendStrength = BlendStrength * DistanceAlpha;
            
            // Blend colors
            VertexColors[VertexIndex] = BlendVertexColors(
                VertexColors[VertexIndex], 
                Color.ToFColor(true), 
                BlendMode, 
                EffectiveBlendStrength
            );
        }
    }
    
    if (bAnyVertexPainted)
    {
        ApplyColorBuffer(StaticMeshComponent, LODIndex, VertexColors);
    }
    
    return bAnyVertexPainted;
}

bool UVertexBlueprintFunctionLibrary::PaintMeshWithParameters(UStaticMeshComponent* StaticMeshComponent, const FVertexPaintParameters& Parameters, int32 LODIndex)
{
    if (Parameters.bApplyToAllLODs)
    {
        // Paint all LODs
        bool bSuccess = false;
        int32 LODCount = StaticMeshComponent->GetStaticMesh()->GetNumLODs();
        
        for (int32 LOD = 0; LOD < LODCount; LOD++)
        {
            bool bLODSuccess = PaintMeshRegion(
                StaticMeshComponent,
                Parameters.PaintShape,
                Parameters.Location,
                Parameters.Dimensions,
                Parameters.Rotation,
                Parameters.Color,
                Parameters.BlendMode,
                Parameters.BlendStrength,
                Parameters.Falloff,
                LOD
            );
            
            bSuccess |= bLODSuccess;
        }
        
        return bSuccess;
    }
    else
    {
        // Paint only the specified LOD
        return PaintMeshRegion(
            StaticMeshComponent,
            Parameters.PaintShape,
            Parameters.Location,
            Parameters.Dimensions,
            Parameters.Rotation,
            Parameters.Color,
            Parameters.BlendMode,
            Parameters.BlendStrength,
            Parameters.Falloff,
            LODIndex
        );
    }
}

FVertexPaintUndoRedoState UVertexBlueprintFunctionLibrary::SaveVertexColorsState(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex)
{
    FVertexPaintUndoRedoState State;
    State.LODIndex = LODIndex;
    
    int32 VertexNum = 0; // Fixed this line - temporary int variable defined
    if (ValidateMeshForPainting(StaticMeshComponent, LODIndex, VertexNum))
    {
        State.ColorData = GetStaticMeshVertexColors(StaticMeshComponent, LODIndex);
    }
    
    return State;
}

bool UVertexBlueprintFunctionLibrary::RestoreVertexColorsState(UStaticMeshComponent* StaticMeshComponent, const FVertexPaintUndoRedoState& State)
{
    int32 VertexNum = 0;
    if (!ValidateMeshForPainting(StaticMeshComponent, State.LODIndex, VertexNum))
    {
        return false;
    }
    
    if (State.ColorData.Num() != VertexNum)
    {
        UE_LOG(LogTemp, Warning, TEXT("VertexPaint: Can't restore state - vertex count mismatch"));
        return false;
    }
    
    ApplyColorBuffer(StaticMeshComponent, State.LODIndex, State.ColorData);
    return true;
}

void UVertexBlueprintFunctionLibrary::ResetVertexColors(UStaticMeshComponent* StaticMeshComponent, FLinearColor ResetColor, int32 LODIndex)
{
    if (!StaticMeshComponent || !StaticMeshComponent->GetStaticMesh())
    {
        return;
    }
    
    const int32 LODCount = StaticMeshComponent->GetStaticMesh()->GetNumLODs();
    StaticMeshComponent->SetLODDataCount(LODCount, LODCount);
    
    // Reset all LODs
    if (LODIndex < 0)
    {
        for (int32 LOD = 0; LOD < LODCount; LOD++)
        {
            int32 VertexNum = 0;
            if (ValidateMeshForPainting(StaticMeshComponent, LOD, VertexNum))
            {
                TArray<FColor> VertexColors;
                VertexColors.Init(ResetColor.ToFColor(true), VertexNum);
                ApplyColorBuffer(StaticMeshComponent, LOD, VertexColors);
            }
        }
    }
    // Reset only the specified LOD
    else if (LODIndex < LODCount)
    {
        int32 VertexNum = 0;
        if (ValidateMeshForPainting(StaticMeshComponent, LODIndex, VertexNum))
        {
            TArray<FColor> VertexColors;
            VertexColors.Init(ResetColor.ToFColor(true), VertexNum);
            ApplyColorBuffer(StaticMeshComponent, LODIndex, VertexColors);
        }
    }
}

UTexture2D* UVertexBlueprintFunctionLibrary::ExportVertexColorsToTexture(UStaticMeshComponent* StaticMeshComponent, int32 TextureWidth, int32 TextureHeight, int32 LODIndex)
{
    int32 VertexNum = 0;
    if (!ValidateMeshForPainting(StaticMeshComponent, LODIndex, VertexNum))
    {
        return nullptr;
    }
    
    TArray<FColor> VertexColors = GetStaticMeshVertexColors(StaticMeshComponent, LODIndex);
    if (VertexColors.Num() == 0)
    {
        return nullptr;
    }
    
    // Create new texture
    UTexture2D* Texture = UTexture2D::CreateTransient(TextureWidth, TextureHeight, PF_B8G8R8A8);
    if (!Texture)
    {
        return nullptr;
    }
    
    // Configure texture settings
    Texture->MipGenSettings = TMGS_NoMipmaps;
    Texture->CompressionSettings = TC_VectorDisplacementmap; // To preserve RGBA values
    Texture->SRGB = false;
    Texture->Filter = TF_Nearest;
    Texture->AddressX = TA_Wrap;
    Texture->AddressY = TA_Wrap;
    
    // Write vertex colors to texture
    FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
    void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
    
    // Fill texture (a more intelligent UV mapping could be done in a real application)
    FColor* Pixels = static_cast<FColor*>(TextureData);
    
    for (int32 y = 0; y < TextureHeight; y++)
    {
        for (int32 x = 0; x < TextureWidth; x++)
        {
            int32 PixelIndex = y * TextureWidth + x;
            
            // Simple mapping (real UV mapping should be added)
            int32 VertexIndex = (PixelIndex % VertexNum);
            
            Pixels[PixelIndex] = VertexColors[VertexIndex];
        }
    }
    
    Mip.BulkData.Unlock();
    Texture->UpdateResource();
    
    return Texture;
}

bool UVertexBlueprintFunctionLibrary::ImportVertexColorsFromTexture(UStaticMeshComponent* StaticMeshComponent, UTexture2D* Texture, int32 LODIndex)
{
    if (!Texture)
    {
        UE_LOG(LogTemp, Warning, TEXT("VertexPaint: Invalid texture for import"));
        return false;
    }
    
    int32 VertexNum = 0;
    if (!ValidateMeshForPainting(StaticMeshComponent, LODIndex, VertexNum))
    {
        return false;
    }
    
    // Get current vertex colors
    TArray<FColor> VertexColors = GetStaticMeshVertexColors(StaticMeshComponent, LODIndex);
    
    // Get texture data
    FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
    FColor* TextureData = static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_ONLY));
    
    const int32 TextureWidth = Texture->GetSizeX();
    const int32 TextureHeight = Texture->GetSizeY();
    const int32 TotalPixels = TextureWidth * TextureHeight;
    
    // Process vertex colors from texture
    // Note: In a real application, UV mapping should be used, this is a simple example
    for (int32 VertexIndex = 0; VertexIndex < VertexNum; VertexIndex++)
    {
        if (VertexIndex < TotalPixels)
        {
            VertexColors[VertexIndex] = TextureData[VertexIndex];
        }
    }
    
    Mip.BulkData.Unlock();
    
    // Apply new colors
    ApplyColorBuffer(StaticMeshComponent, LODIndex, VertexColors);
    
    return true;
}