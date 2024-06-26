// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Components/SphereComponent.h"
//#include "MyCharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h" // For class finding
#include "BotwCharacter.generated.h"

// Forward declaration
class UMyCharacterMovementComponent;

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ABotwCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

public:
	ABotwCharacter(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure)
	FORCEINLINE UMyCharacterMovementComponent* GetCustomCharacterMovement() const { return MovementComponent; }
	
	UFUNCTION()
	void OnBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* FistCollision; // This is the variable name

	bool bHasHandledOverlap = false;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
    bool bIsPunching;

    UFUNCTION(BlueprintCallable, Category = "Character")
    void SetPunching(bool bPunching);

    UFUNCTION(BlueprintCallable, Category = "Character")
    bool IsPunching() const;

    virtual void Tick(float DeltaTime) override;

private:
    void CheckOverlapDuringPunch();

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	TSubclassOf<AActor> AI_BP;

	// void MoveForward(float Value);
	
	// void MoveRight(float Value);

			
protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly)
	UMyCharacterMovementComponent* MovementComponent;

	void Climb();

	void CancelClimb();

	void Attack();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

