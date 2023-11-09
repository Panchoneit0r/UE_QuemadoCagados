// Fill out your copyright notice in the Description page of Project Settings.


#include "JimmyCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "JimmyBall.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

//Initialize the player's Health
	

// Replicated Properties

   void AJimmyCharacter::GetLifetimeReplicatedProps(TArray <FLifetimeProperty>& OutLifetimeProps) const
   {
   	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   	//Replicate current health.
   	DOREPLIFETIME(AJimmyCharacter, CurrentHealth);
   }

void AJimmyCharacter::OnHealthUpdate()
   {
   	//Client-specific functionality
   	if (IsLocallyControlled())
   	{
   		FString healthMessage = FString::Printf(TEXT("You now have %f health remaining."), CurrentHealth);
   		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);

   		if (CurrentHealth <= 0)
   		{
   			FString deathMessage = FString::Printf(TEXT("You have been killed."));
   			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, deathMessage);
   		}
   	}

   	//Server-specific functionality
   	if (GetLocalRole() == ROLE_Authority)
   	{
   		FString healthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), CurrentHealth);
   		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);
   	}

   	//Functions that occur on all machines.
   	/*
		   Any special functionality that should occur as a result of damage or death should be placed here.
	   */
   }

void AJimmyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

   	// Set up action bindings
   	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {

   		//Looking
   		EnhancedInputComponent->BindAction(LookFire, ETriggerEvent::Triggered, this, &AJimmyCharacter::StartFire);

   	}
   	
}

void AJimmyCharacter::OnRep_CurrentHealth()
   {
   	OnHealthUpdate();
   }

float AJimmyCharacter::TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
   {
   	float damageApplied = CurrentHealth - DamageTaken;
   	SetCurrentHealth(damageApplied);
   	return damageApplied;
   }

void AJimmyCharacter::SetCurrentHealth(float healthValue)
   {
   	if (GetLocalRole() == ROLE_Authority)
   	{
   		CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);
   		OnHealthUpdate();
   	}
   }

void AJimmyCharacter::StartFire()
   {
   	if (!bIsFiringWeapon)
   	{
   		bIsFiringWeapon = true;
   		UWorld* World = GetWorld();
   		World->GetTimerManager().SetTimer(FiringTimer, this, &AJimmyCharacter::StopFire, FireRate, false);
   		HandleFire();
   	}
   	// Handle firing projectiles
   	//PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AJimmyCharacter::StartFire);
	//no se que haga esto
   	
   }

void AJimmyCharacter::StopFire()
   {
   	bIsFiringWeapon = false;
   }

void AJimmyCharacter::HandleFire_Implementation()
   {
   	FVector spawnLocation = GetActorLocation() + ( GetActorRotation().Vector()  * 100.0f ) + (GetActorUpVector() * 50.0f);
   	FRotator spawnRotation = GetActorRotation();

   	FActorSpawnParameters spawnParameters;
   	spawnParameters.Instigator = GetInstigator();
   	spawnParameters.Owner = this;

   	AJimmyBall* spawnedProjectile = GetWorld()->SpawnActor<AJimmyBall>(ProjectileClass, spawnLocation, spawnRotation, spawnParameters);
   }

