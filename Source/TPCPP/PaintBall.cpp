// Fill out your copyright notice in the Description page of Project Settings.

#include "Engine/DecalActor.h"
#include "PaintBall.h"

// Sets default values
APaintBall::APaintBall()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Sphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
	Sphere->OnComponentHit.AddDynamic(this, &APaintBall::OnHit);
	RootComponent = Sphere;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComponent"));
	ProjectileMovement->UpdatedComponent = Sphere;
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.2f;

	InitialLifeSpan = 10.f;
	
	Decal = CreateDefaultSubobject<UMaterialInterface>(TEXT("Decal"));
}

// Called when the game starts or when spawned
void APaintBall::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APaintBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APaintBall::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherComp != nullptr)
	{
		ADecalActor* DecalActor = GetWorld()->SpawnActor<ADecalActor>(Hit.Location, Hit.Normal.Rotation());
		DecalActor->SetLifeSpan(6.f);
		DecalActor->SetActorRotation(Hit.Normal.Rotation());
		DecalActor->SetDecalMaterial(Decal);
		DecalActor->GetDecal()->DecalSize = FVector(100.f, 100.f, 100.f);
		DecalActor->GetDecal()->SetFadeOut(1.f, 5.f);
	}

	Destroy();
}