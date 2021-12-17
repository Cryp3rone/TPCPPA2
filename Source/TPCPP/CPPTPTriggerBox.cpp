// Fill out your copyright notice in the Description page of Project Settings.


#include "CPPTPTriggerBox.h"
#include "TPCPPCharacter.h"
#include "Math/UnrealMathUtility.h"
#include "Containers/UnrealString.h"

ACPPTPTriggerBox::ACPPTPTriggerBox()
{
	OnActorBeginOverlap.AddDynamic(this, &ACPPTPTriggerBox::OnOverlapBegin);
}

void ACPPTPTriggerBox::BeginPlay()
{
	Super::BeginPlay();
}

void ACPPTPTriggerBox::OnOverlapBegin(class AActor* OverlappedActor, class AActor* OtherActor)
{
	if (OtherActor && (OtherActor != this))
	{
		ATPCPPCharacter* Character = Cast<ATPCPPCharacter>(OtherActor);
		if (Character == nullptr)
			return;

		Character->Health = Character->Health + changeVal;
		Character->Health = FMath::Clamp(Character->Health, 0, Character->MaxHealth);

		GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Blue, FString::FromInt(Character->Health));
	}
}
