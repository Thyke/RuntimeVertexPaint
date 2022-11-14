#include "VertexLib.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

void UVertexLib::PaintVertexColorByIndex(UStaticMeshComponent* StaticMeshComponent, FLinearColor Color, int32 Index, int32 LODIndex)
{
	if (!StaticMeshComponent || !StaticMeshComponent->GetStaticMesh())
	{
		if (!StaticMeshComponent)
		{
			return;
		}
		else if (!StaticMeshComponent->GetStaticMesh())
		{
			return;
		}
		return;
	}
	const int32 LODNum = StaticMeshComponent->GetStaticMesh()->GetNumLODs();
	StaticMeshComponent->SetLODDataCount(LODNum, LODNum);
	if (!StaticMeshComponent->LODData.IsValidIndex(LODIndex))
	{
		return;
	}
	FStaticMeshComponentLODInfo& LODInfo = StaticMeshComponent->LODData[LODIndex];
	const int32 VertexNum = StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[LODIndex].GetNumVertices();

	if (Index < 0 || Index >= VertexNum)
	{
		return;
	}
	TArray<FColor> VertexColors;
	VertexColors.SetNum(VertexNum);
	if (LODInfo.OverrideVertexColors)
	{
		LODInfo.OverrideVertexColors->GetVertexColors(VertexColors);
	}
	else
	{
		if (StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[LODIndex].bHasColorVertexData)
		{
			StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[LODIndex].VertexBuffers.ColorVertexBuffer.GetVertexColors(VertexColors);
		}
		else
		{
			VertexColors.Init(FColor::White, VertexNum);
		}
	}
	VertexColors[Index] = Color.ToFColor(true);
	LODInfo.OverrideVertexColors = new FColorVertexBuffer;
	LODInfo.OverrideVertexColors->InitFromColorArray(VertexColors);
	//VertexColor için material bağla, base color girişinde kesinlikle VertexColor node bağlı olmalı
	BeginInitResource(LODInfo.OverrideVertexColors);
	StaticMeshComponent->MarkRenderStateDirty();
	StaticMeshComponent->bDisallowMeshPaintPerInstance = true;
}

TArray<FColor> UVertexLib::GetStaticMeshVertexColors(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex)
{
	TArray<FColor> VertexColors;
	if (!StaticMeshComponent)
	{
			return VertexColors;
	}

	if (!StaticMeshComponent->GetStaticMesh())
	{
			return VertexColors;
	}

	const int32 LODNum = StaticMeshComponent->GetStaticMesh()->GetNumLODs();
	StaticMeshComponent->SetLODDataCount(LODNum, LODNum);

	if (!StaticMeshComponent->LODData.IsValidIndex(LODIndex))
	{
			return VertexColors;
	}
	const int32 VertexNum = StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[LODIndex].GetNumVertices();
	VertexColors.SetNum(VertexNum);//otomatik olarak mavi, siyah ile doldurulur
	VertexColors.Init(FColor::White, VertexNum);
	FStaticMeshComponentLODInfo& LODInfo = StaticMeshComponent->LODData[LODIndex];
	if (LODInfo.OverrideVertexColors)
	{
		LODInfo.OverrideVertexColors->GetVertexColors(VertexColors);
	}
	return VertexColors;
}

void UVertexLib::OverrideStaticMeshVertexColor(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, TArray<FVertexOverrideColorInfo> VertexOverrideColorInfos)
{
	if (!StaticMeshComponent)
	{
			return;
	}

	if (!StaticMeshComponent->GetStaticMesh())
	{
			return;
	}

	const int32 LODNum = StaticMeshComponent->GetStaticMesh()->GetNumLODs();
	StaticMeshComponent->SetLODDataCount(LODNum, LODNum);

	if (!StaticMeshComponent->LODData.IsValidIndex(LODIndex))
	{
			return;
	}
	FStaticMeshComponentLODInfo& LODInfo = StaticMeshComponent->LODData[LODIndex];
	TArray<FColor> VertexColors = GetStaticMeshVertexColors(StaticMeshComponent, LODIndex);

	for (const FVertexOverrideColorInfo VertexOverrideColorInfo : VertexOverrideColorInfos)
	{
		if (VertexColors.IsValidIndex(VertexOverrideColorInfo.VertexIndex))
		{
			VertexColors[VertexOverrideColorInfo.VertexIndex] = VertexOverrideColorInfo.OverrideColor;
		}
	}
	LODInfo.OverrideVertexColors = new FColorVertexBuffer;
	LODInfo.OverrideVertexColors->InitFromColorArray(VertexColors);
	BeginInitResource(LODInfo.OverrideVertexColors);
	StaticMeshComponent->MarkRenderStateDirty();
	StaticMeshComponent->bDisallowMeshPaintPerInstance = true;
}

TArray<FVertexOverrideColorInfo> UVertexLib::GetStaticMeshVertexOverrideColorInfoInSphere(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, FVector SphereWorldPosition, float Radius, FLinearColor OverrideColor)
{
	TArray<FVertexOverrideColorInfo> VertexOverrideColorInfos;
	if (!StaticMeshComponent || !StaticMeshComponent->GetStaticMesh())
	{
		if (!StaticMeshComponent)
		{
//			return log
		}
		else if (!StaticMeshComponent->GetStaticMesh())
		{
//			return log
		}
		return VertexOverrideColorInfos;
	}
	const int32 LODNum = StaticMeshComponent->GetStaticMesh()->GetNumLODs();
	StaticMeshComponent->SetLODDataCount(LODNum, LODNum);
	if (!StaticMeshComponent->LODData.IsValidIndex(LODIndex))
	{
			return VertexOverrideColorInfos;
	}
	const FTransform StaticMeshWorldTransform = StaticMeshComponent->GetComponentTransform();
	const FVector SphereLocationInMeshTransform = UKismetMathLibrary::InverseTransformLocation(StaticMeshWorldTransform, SphereWorldPosition);
	const FPositionVertexBuffer& VertexPositionBuffer = StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[LODIndex].VertexBuffers.PositionVertexBuffer;
	const int VertexNum = VertexPositionBuffer.GetNumVertices();
	for (int32 VertexIndex = 0; VertexIndex < VertexNum; VertexIndex++)
	{
		const FVector VertexPosition = static_cast<UE::Math::TVector4<double>>(VertexPositionBuffer.VertexPosition(VertexIndex));
		const float Distance = UKismetMathLibrary::Vector_Distance(VertexPosition, SphereLocationInMeshTransform);
		const float NormalizedDistance = Distance / Radius;
		if (NormalizedDistance <= 1)
		{
			FVertexOverrideColorInfo VertexOverrideColorInfo;

			VertexOverrideColorInfo.VertexIndex = VertexIndex;
			VertexOverrideColorInfo.OverrideColor = OverrideColor.ToFColor(true);
			VertexOverrideColorInfos.Add(VertexOverrideColorInfo);
		}
	}

	return VertexOverrideColorInfos;
}