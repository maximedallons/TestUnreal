// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

bool isDashOnCooldown = false;

ATestCharacter::ATestCharacter() {
	
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = true;	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 350.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// FPS Camera
	HeadCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("HeadCamera"));
	HeadCamera->SetRelativeLocation(FVector(0.f, 10.f, 0.f)); // Position the camera
	HeadCamera->bUsePawnControlRotation = true;
	HeadCamera->SetupAttachment(GetMesh(), "head");

	// TPS Camera
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 300.0f;
	SpringArm->bUsePawnControlRotation = true;
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void ATestCharacter::BeginPlay() {
	Super::BeginPlay();

	HeadCamera->SetActive(false);
	FollowCamera->SetActive(true);
	
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller)) {
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) {
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

///////////////////////////////     Input     ///////////////////////////////////////
void ATestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATestCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATestCharacter::Look);
		EnhancedInputComponent->BindAction(SwitchCameraAction, ETriggerEvent::Triggered, this, &ATestCharacter::SwitchCamera);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ATestCharacter::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ATestCharacter::StopSprinting);
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &ATestCharacter::Dash);
	}
	else {
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ATestCharacter::Move(const FInputActionValue& Value) {
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr) {
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ATestCharacter::Sprint() {
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void ATestCharacter::StopSprinting() {
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ATestCharacter::Dash() {
	if (isDashOnCooldown) {
		return;
	}
	isDashOnCooldown = true;
	FTimerHandle DashCooldownTimer;
	GetWorldTimerManager().SetTimer(DashCooldownTimer, this, &ATestCharacter::ResetDashCooldown, DashCooldown, false);
	const FVector ForwardDir = this->GetActorRotation().Vector();
	LaunchCharacter(ForwardDir * DashDistance, true, true);
}

void ATestCharacter::ResetDashCooldown() {
	isDashOnCooldown = false;
}


void ATestCharacter::Look(const FInputActionValue& Value) {
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	if (Controller != nullptr) {
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ATestCharacter::SwitchCamera() {
	if (HeadCamera->IsActive()) {
		HeadCamera->SetActive(false);
		FollowCamera->SetActive(true);
	}
	else {
		HeadCamera->SetActive(true);
		FollowCamera->SetActive(false);
	}
}
