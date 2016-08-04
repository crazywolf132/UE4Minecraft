// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UE4_Minecraft.h"
#include "UE4_MinecraftCharacter.h"
#include "UE4_MinecraftProjectile.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/InputSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AUE4_MinecraftCharacter

AUE4_MinecraftCharacter::AUE4_MinecraftCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->RelativeLocation = FVector(-39.56f, 1.75f, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->RelativeRotation = FRotator(1.9f, -19.19f, 5.2f);
	Mesh1P->RelativeLocation = FVector(-0.5f, -4.4f, -155.7f);

	// Create a gun mesh component
	FP_WieldedItem = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_WieldedItem"));
	FP_WieldedItem->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_WieldedItem->bCastDynamicShadow = false;
	FP_WieldedItem->CastShadow = false;
	// FP_WieldedItem->SetupAttachment(Mesh1P, TEXT("GripPoint"));

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_WieldedItem);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 30.0f, 10.0f);

	Reach = 250.f;

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P are set in the
	// derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

void AUE4_MinecraftCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	FP_WieldedItem->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint")); //Attach gun mesh component to Skeleton, doing it here because the skelton is not yet created in the constructor
}

//////////////////////////////////////////////////////////////////////////
// Input

void AUE4_MinecraftCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// set up gameplay key bindings
	check(InputComponent);

	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	//InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AUE4_MinecraftCharacter::TouchStarted);
	if (EnableTouchscreenMovement(InputComponent) == false)
	{
		InputComponent->BindAction("Interact", IE_Pressed, this, &AUE4_MinecraftCharacter::OnHit);
		InputComponent->BindAction("Interact", IE_Released, this, &AUE4_MinecraftCharacter::EndHit);
	}

	InputComponent->BindAxis("MoveForward", this, &AUE4_MinecraftCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AUE4_MinecraftCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AUE4_MinecraftCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AUE4_MinecraftCharacter::LookUpAtRate);
}

void AUE4_MinecraftCharacter::OnFire()
{
	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}

}

void AUE4_MinecraftCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AUE4_MinecraftCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnFire();
	}
	TouchItem.bIsPressed = false;
}

void AUE4_MinecraftCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if ((TouchItem.bIsPressed == true) && (TouchItem.FingerIndex == FingerIndex))
	{
		if (TouchItem.bIsPressed)
		{
			if (GetWorld() != nullptr)
			{
				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
				if (ViewportClient != nullptr)
				{
					FVector MoveDelta = Location - TouchItem.Location;
					FVector2D ScreenSize;
					ViewportClient->GetViewportSize(ScreenSize);
					FVector2D ScaledDelta = FVector2D(MoveDelta.X, MoveDelta.Y) / ScreenSize;
					if (FMath::Abs(ScaledDelta.X) >= 4.0 / ScreenSize.X)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.X * BaseTurnRate;
						AddControllerYawInput(Value);
					}
					if (FMath::Abs(ScaledDelta.Y) >= 4.0 / ScreenSize.Y)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.Y * BaseTurnRate;
						AddControllerPitchInput(Value);
					}
					TouchItem.Location = Location;
				}
				TouchItem.Location = Location;
			}
		}
	}
}

void AUE4_MinecraftCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AUE4_MinecraftCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AUE4_MinecraftCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AUE4_MinecraftCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AUE4_MinecraftCharacter::EnableTouchscreenMovement(class UInputComponent* InputComponent)
{
	bool bResult = false;
	if (FPlatformMisc::GetUseVirtualJoysticks() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		bResult = true;
		InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AUE4_MinecraftCharacter::BeginTouch);
		InputComponent->BindTouch(EInputEvent::IE_Released, this, &AUE4_MinecraftCharacter::EndTouch);
		InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AUE4_MinecraftCharacter::TouchUpdate);
	}
	return bResult;
}
void AUE4_MinecraftCharacter::CheckForBlocks() {
	FHitResult LineTraceHit;
	FVector StartTrace = FirstPersonCameraComponent->GetComponentLocation();
	FVector EndTrace = (FirstPersonCameraComponent->GetForwardVector() * Reach) + StartTrace;
	
	FCollisionQueryParams CQP;
	CQP.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(LineTraceHit, StartTrace, EndTrace, ECollisionChannel::ECC_WorldDynamic, CQP);

	ABlock* PotentialBlock = Cast<ABlock>(LineTraceHit.GetActor());
	if (PotentialBlock == NULL) {
		CurrentBlock = nullptr;
		return;
	}
	else {
		CurrentBlock = PotentialBlock;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, *CurrentBlock->GetName());
	}
}
void AUE4_MinecraftCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
	CheckForBlocks();
}
void AUE4_MinecraftCharacter::PlayHitAnim() {
	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}

}
void AUE4_MinecraftCharacter::OnHit() {
	PlayHitAnim();

	if (CurrentBlock != nullptr) {

		bIsBreaking = true;

		float TimeBetweenBreaks = ((CurrentBlock->Resistance) / 100.f) / 2.f;

		GetWorld()->GetTimerManager().SetTimer(BlockBreakingHandle, this, &AUE4_MinecraftCharacter::BreakBlock, TimeBetweenBreaks, true);
		GetWorld()->GetTimerManager().SetTimer(HitAnimHandle, this, &AUE4_MinecraftCharacter::BreakBlock, 0.4f, true);
	}
}
void AUE4_MinecraftCharacter::EndHit() {
	GetWorld()->GetTimerManager().ClearTimer (BlockBreakingHandle);
	GetWorld()->GetTimerManager().ClearTimer(HitAnimHandle);
	bIsBreaking = false;
	if (CurrentBlock != nullptr) {
		CurrentBlock->ResetBlock();
	}
}
void AUE4_MinecraftCharacter::BreakBlock() {
	if (bIsBreaking && CurrentBlock != nullptr && !CurrentBlock->IsPendingKill()) {
		CurrentBlock->Break();
	}
}