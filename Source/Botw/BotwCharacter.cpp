// Copyright Epic Games, Inc. All Rights Reserved.

#include "BotwCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "MyCharacterMovementComponent.h" // Include the header here
#include "Kismet/KismetMathLibrary.h"


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ABotwCharacter

ABotwCharacter::ABotwCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UMyCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	UE_LOG(LogTemplateCharacter, Error, TEXT("BEGIN PLAY"), *GetNameSafe(this));

	// Set this character to call Tick() every frame
    PrimaryActorTick.bCanEverTick = true;
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	bIsPunching = false;

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
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

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
	MovementComponent = Cast<UMyCharacterMovementComponent>(GetCharacterMovement());

	static ConstructorHelpers::FClassFinder<AActor> BlueprintFinder(TEXT("/Game/Characters/NPC/test_ai")); // Adjust the path

    if (BlueprintFinder.Succeeded()) {
        AI_BP = BlueprintFinder.Class;
    } else {
        UE_LOG(LogTemp, Error, TEXT("Could not find Blueprint class at /Game/Characters/NPC/test_ai"));
    }
}

//----------------------------------------------------------------------------------------------------------

void ABotwCharacter::SetPunching(bool bPunching)
{
    bIsPunching = bPunching;

    if (bIsPunching)
    {
        // Call the overlap check immediately when punch starts
        CheckOverlapDuringPunch();
    }
}

bool ABotwCharacter::IsPunching() const
{
    return bIsPunching;
}

void ABotwCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsPunching)
    {
        CheckOverlapDuringPunch();
    }
}

void ABotwCharacter::CheckOverlapDuringPunch()
{
    TArray<AActor*> OverlappingActors;
    GetOverlappingActors(OverlappingActors);

    for (AActor* Actor : OverlappingActors)
    {
        //if (Actor && Actor->IsA(AI_BP)) // Replace with your skeletal mesh class
		if (Actor) // Replace with your skeletal mesh class
        {
			// Handle overlap logic here
			UE_LOG(LogTemp, Warning, TEXT("Overlap with %s"), *Actor->GetName());

			UE_LOG(LogTemp, Warning, TEXT("getting skeletal mesh actor"));
            auto SkeletalMeshActor = Cast<ASkeletalMeshActor>(Actor);
			UE_LOG(LogTemp, Warning, TEXT("got skeletal mesh actor"));

            if (SkeletalMeshActor && SkeletalMeshActor->GetSkeletalMeshComponent())
            {
				UE_LOG(LogTemp, Warning, TEXT("skeletal mesh actor has a skeletal mesh component"));
                USkeletalMeshComponent* SkeletalMeshComp = SkeletalMeshActor->GetSkeletalMeshComponent();

                if (!SkeletalMeshComp->IsSimulatingPhysics())
                {
                    SkeletalMeshComp->SetSimulatePhysics(true);
                }

                FVector ImpactNormal = GetActorForwardVector();
                FVector ImpactImpulse = ImpactNormal * 10000.0f; // Example force magnitude

                SkeletalMeshComp->SetAngularDamping(5.0f); // Adjust value as needed
                SkeletalMeshComp->SetLinearDamping(2.0f);  // Adjust value as needed
                
				//FVector socketNormal = SkeletalMeshComp->GetSocketLocation("hand_rSocket");
				FRotator PlayerRotation = GetActorRotation();
				// Convert the rotation to a direction vector
				FVector ForwardVector = UKismetMathLibrary::GetForwardVector(PlayerRotation);

				UE_LOG(LogTemp, Warning, TEXT("ImpactNormal %s"), *ImpactNormal.ToString());
				UE_LOG(LogTemp, Warning, TEXT("ImpactImpulse %s"), *ImpactImpulse.ToString());
				UE_LOG(LogTemp, Warning, TEXT("ForwardVector %s"), *ForwardVector.ToString());

				// Apply the impulse to the skeletal mesh component at the center of mass
				SkeletalMeshComp->AddImpulse(ForwardVector * 10000.0f, NAME_None, true);

                UE_LOG(LogTemp, Warning, TEXT("Triggered ragdoll on %s"), *Actor->GetName());
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------

void ABotwCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("BEGIN PLAY"));

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	FistCollision = Cast<USphereComponent>(FindComponentByClass<USphereComponent>());

	// if (FistCollision){
	// 	FistCollision->OnComponentBeginOverlap.AddDynamic(this, &ABotwCharacter::OnBoxBeginOverlap);
	// 	FistCollision->OnComponentHit.AddDynamic(this, &ABotwCharacter::OnBoxHit);
	// }
}

void ABotwCharacter::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("bIsPunching %s"), ( bIsPunching ? TEXT("true") : TEXT("false") ));
	if (!bIsPunching) return; // Only proceed if the character is punching

    if (bHasHandledOverlap) return;
    bHasHandledOverlap = true;
	
	// Handle overlap logic here
    UE_LOG(LogTemp, Warning, TEXT("Overlap with %s"), *OtherActor->GetName());

	if (OtherActor && OtherActor->IsA(AI_BP)) { // Replace with your skeletal mesh class
        auto SkeletalMeshActor = Cast<ASkeletalMeshActor>(OtherActor);
		UE_LOG(LogTemp, Warning, TEXT("skeletal mesh actor"));
        if (SkeletalMeshActor && SkeletalMeshActor->GetSkeletalMeshComponent()) {

			USkeletalMeshComponent* SkeletalMeshComp = SkeletalMeshActor->GetSkeletalMeshComponent();

			if (!SkeletalMeshComp->IsSimulatingPhysics()) {
				//UE_LOG(LogTemp, Warning, TEXT("IsSimulatingPhysics: %s"), SkeletalMeshComp->IsSimulatingPhysics() ? TEXT("true") : TEXT("false"));
				SkeletalMeshComp->SetSimulatePhysics(true);
				//UE_LOG(LogTemp, Warning, TEXT("IsSimulatingPhysics %s"), SkeletalMeshComp->IsSimulatingPhysics() ? TEXT("true") : TEXT("false"));
			}

			FVector ImpactNormal = SweepResult.ImpactNormal;
			FVector Normal = SweepResult.Normal;

			// Increase angular damping to reduce spinning
            SkeletalMeshComp->SetAngularDamping(5.0f); // Adjust value as needed
            SkeletalMeshComp->SetLinearDamping(2.0f);  // Adjust value as needed

 			FVector ImpactImpulse = -ImpactNormal * 10000.0f; // Example force magnitude

			//FVector socketNormal = SkeletalMeshComp->GetSocketLocation("hand_rSocket");
			FRotator PlayerRotation = GetActorRotation();
			// Convert the rotation to a direction vector
			FVector ForwardVector = UKismetMathLibrary::GetForwardVector(PlayerRotation);

			UE_LOG(LogTemp, Warning, TEXT("ImpactNormal %s"), *ImpactNormal.ToString());
			UE_LOG(LogTemp, Warning, TEXT("ImpactImpulse %s"), *ImpactImpulse.ToString());
			UE_LOG(LogTemp, Warning, TEXT("ForwardVector %s"), *ForwardVector.ToString());


			// Apply the impulse to the skeletal mesh component at the center of mass
			SkeletalMeshComp->AddImpulse(ForwardVector * 10000.0f, NAME_None, true);
			
            UE_LOG(LogTemp, Warning, TEXT("Triggered ragdoll on %s"), *OtherActor->GetName());
        }
	}

	// Reset flag after a short delay or next frame
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
        bHasHandledOverlap = false;
    });
}

void ABotwCharacter::OnBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // Handle hit logic here
    UE_LOG(LogTemp, Warning, TEXT("(((((((HIT))))))) %s"), *OtherActor->GetName());

	// if (OtherActor && OtherActor->IsA(AI_BP)) {
    //     auto SkeletalMeshActor = Cast<ASkeletalMeshActor>(OtherActor);
    //     if (SkeletalMeshActor && SkeletalMeshActor->GetSkeletalMeshComponent()) {
    //         //keletalMeshActor->GetSkeletalMeshComponent()->SetSimulatePhysics(true);
    //         //E_LOG(LogTemp, Warning, TEXT("Ragdoll triggered by hit with %s"), *OtherActor->GetName());
    //     }
    // }
}

//////////////////////////////////////////////////////////////////////////
// Input

void ABotwCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ABotwCharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ABotwCharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABotwCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABotwCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

	PlayerInputComponent->BindAction("Climb", IE_Pressed, this, &ABotwCharacter::Climb);
	PlayerInputComponent->BindAction("Cancel Climb", IE_Pressed, this, &ABotwCharacter::CancelClimb);
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ABotwCharacter::Attack);
}

void ABotwCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		//FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		FVector ForwardDirection;

		if (MovementComponent->IsClimbing())
		{
			ForwardDirection = FVector::CrossProduct(MovementComponent->GetClimbSurfaceNormal(), -GetActorRightVector());
		}
		else
		{
			//ForwardDirection = GetControlOrientationMatrix().GetUnitAxis(EAxis::X);
			ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		}

		// get right vector 
		//const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		FVector RightDirection;

		if (MovementComponent->IsClimbing())
		{
			RightDirection = FVector::CrossProduct(MovementComponent->GetClimbSurfaceNormal(), GetActorUpVector());
		}
		else
		{
			//RightDirection = GetControlOrientationMatrix().GetUnitAxis(EAxis::Y);
			RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		}

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ABotwCharacter::Look(const FInputActionValue& Value)
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

void ABotwCharacter::Climb()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("+++ CLIMB +++"));
	MovementComponent->TryClimbing();
}

void ABotwCharacter::CancelClimb()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("+++ CANCEL CLIMB +++"));
	MovementComponent->CancelClimbing();
}

void ABotwCharacter::Attack()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("+++ ATTACK +++"));
	MovementComponent->Attack();
}
