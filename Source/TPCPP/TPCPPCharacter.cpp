// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPCPPCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "PaintBall.h"

//////////////////////////////////////////////////////////////////////////
// ATPCPPCharacter

ATPCPPCharacter::ATPCPPCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
	JumpMaxCount = 2;

	MaxHealth = 100;
	Health = MaxHealth;
	NotGrabing = true;
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SceneComponent->SetRelativeLocation(FVector(80.f, 30.f, 0.f));
	SceneComponent->SetupAttachment(RootComponent);

	BulletSpawn = CreateDefaultSubobject<USceneComponent>(TEXT("BulletSpawn"));
	BulletSpawn->SetRelativeLocation(FVector(80.f, 0.f, 60.f));
	BulletSpawn->SetupAttachment(RootComponent);
	
}

//////////////////////////////////////////////////////////////////////////
// Input

void ATPCPPCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &ATPCPPCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATPCPPCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ATPCPPCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ATPCPPCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ATPCPPCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ATPCPPCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ATPCPPCharacter::OnResetVR);

	PlayerInputComponent->BindAction("PickUp", IE_Pressed, this, &ATPCPPCharacter::OnPickupPressed);
	PlayerInputComponent->BindAction("PickUp", IE_Released, this, &ATPCPPCharacter::OnPickupReleased);
	PlayerInputComponent->BindAction("Shoot", IE_Released, this, &ATPCPPCharacter::Shooting);
}


void ATPCPPCharacter::OnResetVR()
{
	// If TPCPP is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in TPCPP.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ATPCPPCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void ATPCPPCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void ATPCPPCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ATPCPPCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ATPCPPCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ATPCPPCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ATPCPPCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Health <= 0)
	{
		UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
	}
}

void ATPCPPCharacter::OnPickupPressed()
{
	FVector Location;
	FRotator Rotation;
	FHitResult Hit(ForceInit);

	GetController()->GetPlayerViewPoint(Location, Rotation);

	FVector Start = Location + (Rotation.Vector() * 300);
	FVector End = Location + (Rotation.Vector() * 700);

	FCollisionQueryParams TraceParams = FCollisionQueryParams(FName(TEXT("Trace")), false, this);
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, TraceParams);
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.0f);

	if (bHit && Hit.GetActor()->IsRootComponentMovable())
	{
		HitRes = Hit.GetActor();
		
		DrawDebugBox(GetWorld(), Hit.ImpactPoint, FVector(8.f, 8.f, 8.f), FColor::Green, false, 2.f);
		if (NotGrabing)
		{
			PickedUp = Cast<UStaticMeshComponent>(HitRes->GetComponentByClass(UStaticMeshComponent::StaticClass()));
			if (PickedUp == nullptr)
				return;
			PickedUp->SetSimulatePhysics(false);

			FAttachmentTransformRules Attach(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
			HitRes->AttachToComponent(SceneComponent, Attach);
			
			
			NotGrabing = false;
		}
	}
}

void ATPCPPCharacter::OnPickupReleased()
{
	if (!NotGrabing)
	{
		FDetachmentTransformRules Dettach(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false);
		HitRes->DetachAllSceneComponents(SceneComponent, Dettach);
		if (PickedUp == nullptr)
			return;
		PickedUp->SetSimulatePhysics(true);
		PickedUp = nullptr;
		HitRes = nullptr;
		NotGrabing = true;
	}
}

void ATPCPPCharacter::Shooting()
{
	GetWorld()->SpawnActor<APaintBall>(PaintBall, BulletSpawn->GetComponentLocation(), FollowCamera->GetComponentRotation());
}
