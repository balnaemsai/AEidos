// Fill out your copyright notice in the Description page of Project Settings.


#include "Entities/Page/PageCharacter.h"

// Sets default values
APageCharacter::APageCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APageCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APageCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APageCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

