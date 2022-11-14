// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VertexLib.generated.h"

USTRUCT(BlueprintType)
struct FVertexOverrideColorInfo
{
	GENERATED_BODY()

		UPROPERTY(BlueprintReadWrite)
		int32 VertexIndex;

	UPROPERTY(BlueprintReadWrite)
		FColor OverrideColor;
};

/**
 *
 */
UCLASS()
class MYPROJECT_API UVertexLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
		UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "PaintVertexColorByIndex")
		static void PaintVertexColorByIndex(UStaticMeshComponent* StaticMeshComponent, FLinearColor Color, int32 Index, int32 LODIndex);

	UFUNCTION(Category = "Vertex", DisplayName = "Get Static Mesh Vertex Colors")
		static TArray<FColor> GetStaticMeshVertexColors(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex);

	UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "Override Static Mesh Vertex Color")
		static void OverrideStaticMeshVertexColor(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, TArray<FVertexOverrideColorInfo> VertexOverrideColorInfos);

	UFUNCTION(BlueprintCallable, Category = "Vertex", DisplayName = "Get Static Mesh Vertex Override Color Info In Sphere")
		static TArray<FVertexOverrideColorInfo> GetStaticMeshVertexOverrideColorInfoInSphere(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, FVector SphereWorldPosition, float Radius, FLinearColor OverrideColor);
};
