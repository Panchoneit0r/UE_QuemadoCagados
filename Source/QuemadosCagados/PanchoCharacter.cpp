// Fill out your copyright notice in the Description page of Project Settings.


#include "PanchoCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "JimmyBall.h"

// Sets default values
APanchoCharacter::APanchoCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

}

void APanchoCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr && !death)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void APanchoCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APanchoCharacter::Jumped()
{
	if(!death)
	{
		ACharacter::Jump();
	}
}

void APanchoCharacter::ChangeCamera(const FInputActionValue& Value)
{
	if(death)
	{
		float inputValue = Value.Get<float>();
		actualCamera+= inputValue;
		if(actualCamera > 3)
		{
			actualCamera = 0;
		}
		else if(actualCamera < 0)
		{
			actualCamera = 3;
		}
		APlayerController* PlayerController = Cast<APlayerController>(Controller);

		PlayerController->SetViewTargetWithBlend(Cameras[actualCamera]);
	}
}

// Called when the game starts or when spawned
void APanchoCharacter::BeginPlay()
{
	Super::BeginPlay();

	currentHealth = maxHealth;
	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
}

void APanchoCharacter::Damaged(float _damage)
{
	if(!death)
	{
		currentHealth -= _damage;
		setCurrentHealth(currentHealth);
	
		/*if(currentHealth <= 0.0f)
		{
			DeathSystem();
		}
		*/
	}
}

void APanchoCharacter::ChangePowerBall(TSubclassOf<AActor> newPowerBall)
{
	PowerBall = newPowerBall;
}

void APanchoCharacter::Death()
{
	death = true;
}

void APanchoCharacter::Respawn(FVector respawnPosition)
{
	death = false;
	setCurrentHealth(maxHealth);
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	PlayerController->SetViewTargetWithBlend(this);
	SetActorLocation(respawnPosition);
}

// Called every frame
void APanchoCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APanchoCharacter::setCurrentHealth(float newHealth)
{
	currentHealth = newHealth;
	OnHealthUpdate();
}

void APanchoCharacter::setCameras(TArray<AActor*> newCameras)
{
	Cameras = newCameras;
}

void APanchoCharacter::OnHealthUpdate()
{
	//Client-specific functionality
	if (IsLocallyControlled())
	{
		FString healthMessage = FString::Printf(TEXT("You now have %f health remaining."), currentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);

		if (currentHealth <= 0)
		{
			FString deathMessage = FString::Printf(TEXT("You have been killed."));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, deathMessage);
		}
	}

	//Server-specific functionality
	if (GetLocalRole() == ROLE_Authority)
	{
		FString healthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), currentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);
	}

	
	if (currentHealth <= 0)
	{
		DeathSystem();
	}
}

void APanchoCharacter::StartFire()
{
	if (!bIsFiringWeapon && !death)
	{
		bIsFiringWeapon = true;
		UWorld* World = GetWorld();
		World->GetTimerManager().SetTimer(FiringTimer, this, &APanchoCharacter::StopFire, FireRate, false);
		HandleFire();
	}
	// Handle firing projectiles
	//PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AJimmyCharacter::StartFire);
	//no se que haga esto
}

void APanchoCharacter::StopFire()
{
	bIsFiringWeapon = false;
}

void APanchoCharacter::HandleFire_Implementation()
{
	FVector spawnLocation = GetActorLocation() + ( GetActorRotation().Vector()  * 100.0f ) + (GetActorUpVector() * 50.0f);
	FRotator spawnRotation = GetActorRotation();

	FActorSpawnParameters spawnParameters;
	spawnParameters.Instigator = GetInstigator();
	spawnParameters.Owner = this;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance != nullptr)
	{
		AnimInstance->Montage_Play(AttackAnim, 2.0f);
	}
	
	if(PowerBall)
	{
		AActor* projectile = GetWorld()->SpawnActor<AActor>(PowerBall, spawnLocation, spawnRotation, spawnParameters);
		PowerBall = nullptr;
	}
	else
	{
		AActor* projectile = GetWorld()->SpawnActor<AActor>(ProjectileClass, spawnLocation, spawnRotation, spawnParameters);
	}
	
}

void APanchoCharacter::OnRep_CurrentHealth()
{	
	OnHealthUpdate();
}

void APanchoCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Replicate current health.
	DOREPLIFETIME(APanchoCharacter, currentHealth);
}

// Called to bind functionality to input
void APanchoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &APanchoCharacter::Jumped);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APanchoCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APanchoCharacter::Look);

		//ChangeCamera
		EnhancedInputComponent->BindAction(ChangeCameraAction, ETriggerEvent::Started, this, &APanchoCharacter::ChangeCamera);

		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &APanchoCharacter::StartFire);
	}

}

