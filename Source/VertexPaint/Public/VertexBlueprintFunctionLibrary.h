// Copyright (C) Thyke. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VertexBlueprintFunctionLibrary.generated.h"

/**
 * Shape types to be used for vertex painting
 */
UENUM(BlueprintType)
enum class EVertexPaintShape : uint8
{
    Point UMETA(DisplayName = "Point"),
    Sphere UMETA(DisplayName = "Sphere"),
    Box UMETA(DisplayName = "Box"),
    Cylinder UMETA(DisplayName = "Cylinder")
};

/**
 * Color blending modes
 */
UENUM(BlueprintType)
enum class EVertexColorBlendMode : uint8
{
    Replace UMETA(DisplayName = "Replace"),
    Add UMETA(DisplayName = "Add"),
    Multiply UMETA(DisplayName = "Multiply"),
    Lerp UMETA(DisplayName = "Lerp")
};

/**
 * Vertex color override information
 */
USTRUCT(BlueprintType)
struct FVertexOverrideColorInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 VertexIndex = 0;

    UPROPERTY(BlueprintReadWrite)
    FColor OverrideColor = FColor::White;
};

/**
 * Vertex painting parameters
 */
USTRUCT(BlueprintType)
struct FVertexPaintParameters
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    EVertexPaintShape PaintShape = EVertexPaintShape::Sphere;
    
    UPROPERTY(BlueprintReadWrite)
    FVector Location = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadWrite)
    FVector Dimensions = FVector(100.0f, 100.0f, 100.0f);
    
    UPROPERTY(BlueprintReadWrite)
    FRotator Rotation = FRotator::ZeroRotator;
    
    UPROPERTY(BlueprintReadWrite)
    FLinearColor Color = FLinearColor::White;
    
    UPROPERTY(BlueprintReadWrite)
    EVertexColorBlendMode BlendMode = EVertexColorBlendMode::Replace;
    
    UPROPERTY(BlueprintReadWrite)
    float BlendStrength = 1.0f;
    
    UPROPERTY(BlueprintReadWrite)
    float Falloff = 0.5f;
    
    UPROPERTY(BlueprintReadWrite)
    bool bApplyToAllLODs = false;
};

/**
 * Vertex color state for Undo/Redo
 */
USTRUCT(BlueprintType)
struct FVertexPaintUndoRedoState
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FColor> ColorData;
    
    UPROPERTY()
    int32 LODIndex = 0;
};

/**
 * Blueprint Function Library for Runtime Vertex Painting
 */
UCLASS()
class VERTEXPAINT_API UVertexBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

    ///// BASIC FUNCTIONS /////
    
    /**
     * Paints a vertex at the specified index
     */
    UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "Paint Vertex Color By Index")
    static void PaintVertexColorByIndex(UStaticMeshComponent* StaticMeshComponent, FLinearColor Color, int32 Index, int32 LODIndex = 0);

    /**
     * Gets the vertex color data of a static mesh
     */
    UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "Get Static Mesh Vertex Colors")
    static TArray<FColor> GetStaticMeshVertexColors(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex = 0);

    /**
     * Overrides the colors of specified vertices
     */
    UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "Override Static Mesh Vertex Color")
    static void OverrideStaticMeshVertexColor(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, TArray<FVertexOverrideColorInfo> VertexOverrideColorInfos);

    /**
     * Colors vertices within the specified sphere
     */
    UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "Get Static Mesh Vertex Override Color Info In Sphere")
    static TArray<FVertexOverrideColorInfo> GetStaticMeshVertexOverrideColorInfoInSphere(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, FVector SphereWorldPosition, float Radius, FLinearColor OverrideColor);

    ///// NEW FUNCTIONS /////
    
    /**
     * Paints a specific region of the mesh
     */
    UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "Paint Mesh Region")
    static bool PaintMeshRegion(UStaticMeshComponent* StaticMeshComponent, 
                           EVertexPaintShape Shape,
                           FVector Location, 
                           FVector Dimensions, 
                           FRotator Rotation, 
                           FLinearColor Color, 
                           EVertexColorBlendMode BlendMode = EVertexColorBlendMode::Replace, 
                           float BlendStrength = 1.0f, 
                           float Falloff = 0.5f, 
                           int32 LODIndex = 0);

    /**
     * Paints the mesh using parameters
     */
    UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "Paint Mesh With Parameters")
    static bool PaintMeshWithParameters(UStaticMeshComponent* StaticMeshComponent, const FVertexPaintParameters& Parameters, int32 LODIndex = 0);

    /**
     * Blends two colors
     */
    UFUNCTION(BlueprintPure, Category = "Vertex", DisplayName = "Blend Vertex Colors")
    static FColor BlendVertexColors(FColor BaseColor, 
                                FColor BlendColor, 
                                EVertexColorBlendMode BlendMode = EVertexColorBlendMode::Replace, 
                                float BlendStrength = 1.0f);

    /**
     * Saves vertex color state (for Undo/Redo)
     */
    UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "Save Vertex Colors State")
    static FVertexPaintUndoRedoState SaveVertexColorsState(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex = 0);

    /**
     * Restores saved vertex color state
     */
    UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "Restore Vertex Colors State")
    static bool RestoreVertexColorsState(UStaticMeshComponent* StaticMeshComponent, const FVertexPaintUndoRedoState& State);

    /**
     * Resets all vertex colors of the mesh
     */
    UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "Reset Vertex Colors")
    static void ResetVertexColors(UStaticMeshComponent* StaticMeshComponent, FLinearColor ResetColor = FLinearColor::White, int32 LODIndex = -1);

    /**
     * Exports vertex colors as a texture
     */
    UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "Export Vertex Colors To Texture")
    static UTexture2D* ExportVertexColorsToTexture(UStaticMeshComponent* StaticMeshComponent, 
                                                int32 TextureWidth = 512, 
                                                int32 TextureHeight = 512, 
                                                int32 LODIndex = 0);

    /**
     * Imports vertex colors from a texture
     */
    UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "Import Vertex Colors From Texture")
    static bool ImportVertexColorsFromTexture(UStaticMeshComponent* StaticMeshComponent, 
                                          UTexture2D* Texture, 
                                          int32 LODIndex = 0);

    ///// HELPER FUNCTIONS /////
    
private:
    /**
     * Checks if the mesh is valid for painting
     */
    static bool ValidateMeshForPainting(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, int32& OutVertexCount);
    
    /**
     * Applies the color buffer
     */
    static void ApplyColorBuffer(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, const TArray<FColor>& VertexColors);
    
    /**
     * Checks if a vertex is within the specified area
     */
    static float GetVertexDistanceNormalizedToShape(const FVector& VertexPosition, EVertexPaintShape Shape, const FVector& Location, const FVector& Dimensions, const FRotator& Rotation);
};