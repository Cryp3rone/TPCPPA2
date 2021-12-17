// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "CPPTPTriggerBox.generated.h"

/**
 * 
 */
UCLASS()
class TPCPP_API ACPPTPTriggerBox : public ATriggerBox
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	int changeVal;

	ACPPTPTriggerBox();
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(class AActor* OverlappedActor, class AActor* OtherActor);
};
