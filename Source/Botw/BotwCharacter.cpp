// Copyright Epic Games, Inc. All Rights Reserved.

#include "BotwCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/CapsuleComponent.h"
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
	UE_LOG(LogTemplateCharacter, Log, TEXT("Constructor: ABotwCharacter created with MovementComponent %s"), *GetNameSafe(MovementComponent));

	static ConstructorHelpers::FClassFinder<AActor> BlueprintFinder(TEXT("/Game/Characters/NPC/test_ai")); // Adjust the path

	if (GetCharacterMovement()) {

 		ACharacter* OwnerCharacter = Cast<ACharacter>(MovementComponent->GetOwner());

		if (OwnerCharacter) {
			AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
		}
	}

    if (BlueprintFinder.Succeeded()) {
        AI_BP = BlueprintFinder.Class;
    } else {

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
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("isPunching"));
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

void ABotwCharacter::DisableLeftClick()
{
    bDisableLeftClick = true;
}

void ABotwCharacter::EnableLeftClick()
{
    bDisableLeftClick = false;
}

void ABotwCharacter::CheckOverlapDuringPunch()
{
    TArray<AActor*> OverlappingActors;
    GetOverlappingActors(OverlappingActors);

    auto HandleSkeletalMeshComponent = [&](USkeletalMeshComponent* SkeletalMeshComp, AActor* Actor)
    {
        if (SkeletalMeshComp)
        {
            if (!SkeletalMeshComp->IsSimulatingPhysics())
            {
                SkeletalMeshComp->SetSimulatePhysics(true);
            }

            FVector ImpactNormal = GetActorForwardVector();
            FVector ImpactImpulse = ImpactNormal * 10000.0f; // Example force magnitude

            SkeletalMeshComp->SetAngularDamping(5.0f); // Adjust value as needed
            SkeletalMeshComp->SetLinearDamping(2.0f);  // Adjust value as needed

            FRotator PlayerRotation = GetActorRotation();
            FVector ForwardVector = UKismetMathLibrary::GetForwardVector(PlayerRotation);

            UE_LOG(LogTemp, Warning, TEXT("ImpactNormal %s"), *ImpactNormal.ToString());
            UE_LOG(LogTemp, Warning, TEXT("ImpactImpulse %s"), *ImpactImpulse.ToString());
            UE_LOG(LogTemp, Warning, TEXT("ForwardVector %s"), *ForwardVector.ToString());

            SkeletalMeshComp->AddImpulse(ForwardVector * 10000.0f, NAME_None, true);

            UE_LOG(LogTemp, Warning, TEXT("Triggered ragdoll on %s"), *Actor->GetName());
        }
    };

    for (AActor* Actor : OverlappingActors)
    {
        if (Actor)
        {
            UE_LOG(LogTemp, Warning, TEXT("Overlap with %s"), *Actor->GetName());

            ACharacter* Character = Cast<ACharacter>(Actor);
            if (Character)
            {
                USkeletalMeshComponent* SkeletalMeshComp = Character->GetMesh();
                HandleSkeletalMeshComponent(SkeletalMeshComp, Actor);
            }

            auto SkeletalMeshActor = Cast<ASkeletalMeshActor>(Actor);
            if (SkeletalMeshActor)
            {
                USkeletalMeshComponent* SkeletalMeshComp = SkeletalMeshActor->GetSkeletalMeshComponent();
                HandleSkeletalMeshComponent(SkeletalMeshComp, Actor);
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------

void ABotwCharacter::BeginPlay()
{
    Super::BeginPlay();

    GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("BEGIN PLAY"));

    // Example: Load your SoundWave asset
    USoundWave* BackgroundSound = LoadObject<USoundWave>(nullptr, TEXT("/Game/Audio/Diablo_Dark_Ambient_Music_for_Deep_Relaxation_and_Meditation.Diablo_Dark_Ambient_Music_for_Deep_Relaxation_and_Meditation"));

    if (BackgroundSound)
    {
        // Play the sound in 2D
        UGameplayStatics::PlaySound2D(this, BackgroundSound);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load sound: /Game/Audio/Diablo_Dark_Ambient_Music_for_Deep_Relaxation_and_Meditation.Diablo_Dark_Ambient_Music_for_Deep_Relaxation_and_Meditation"));
    }

    // Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }

        // Make the mouse cursor visible
        PlayerController->bShowMouseCursor = true;
        PlayerController->bEnableClickEvents = true;
        PlayerController->bEnableMouseOverEvents = true;

        // Lock the mouse to the viewport
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
        InputMode.SetHideCursorDuringCapture(false); // Ensure cursor stays visible
        PlayerController->SetInputMode(InputMode);
    }

    FistCollision = Cast<USphereComponent>(FindComponentByClass<USphereComponent>());

    // Initialize the AnimInstance
    if (GetMesh())
    {
        AnimInstance = GetMesh()->GetAnimInstance();
        if (!AnimInstance)
        {
            UE_LOG(LogTemplateCharacter, Error, TEXT("AnimInstance is null in BeginPlay for %s"), *GetNameSafe(this));
        }
    }
}


void ABotwCharacter::OnBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // Handle hit logic here
    UE_LOG(LogTemp, Warning, TEXT("(((((((HIT))))))) %s"), *OtherActor->GetName());
}

//////////////////////////////////////////////////////////////////////////
// Input

void ABotwCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ABotwCharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ABotwCharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABotwCharacter::Move);

        // Mouse Moving
		EnhancedInputComponent->BindAction(MouseMoveAction, ETriggerEvent::Triggered, this, &ABotwCharacter::MouseMove);

        // Middle Mouse
        EnhancedInputComponent->BindAction(MiddleMouse, ETriggerEvent::Started, this, &ABotwCharacter::OnMiddleMousePressed);
        EnhancedInputComponent->BindAction(MiddleMouse, ETriggerEvent::Completed, this, &ABotwCharacter::OnMiddleMouseReleased);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABotwCharacter::Look);

        // Bind the zoom action
        EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ABotwCharacter::ZoomCamera);

        EnhancedInputComponent->BindAction(LeftMouse, ETriggerEvent::Started, this, &ABotwCharacter::OnLeftMousePressed);
        EnhancedInputComponent->BindAction(LeftMouse, ETriggerEvent::Completed, this, &ABotwCharacter::OnLeftMouseReleased);
        EnhancedInputComponent->BindAction(RightMouse, ETriggerEvent::Started, this, &ABotwCharacter::OnRightMousePressed);
        EnhancedInputComponent->BindAction(RightMouse, ETriggerEvent::Completed, this, &ABotwCharacter::OnRightMouseReleased);
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
	ABotwCharacter* Character = Cast<ABotwCharacter>(MovementComponent->GetOwner());

	if (Character->IsPunching()) return;
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Determine movement direction
		FRotator YawRotation;

		if (bIsLeftMouseButtonDown)
		{
			// Use the character's facing direction when left mouse button is pressed
			YawRotation = FRotator(0, GetActorRotation().Yaw, 0);
		}
		else
		{
			// Use the controller's rotation otherwise
 			const FRotator Rotation = Controller->GetControlRotation();
			YawRotation = FRotator(0, Rotation.Yaw, 0);
		}

		// Get forward and right vectors
		FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Special handling for climbing
		if (MovementComponent->IsClimbing())
		{
			ForwardDirection = FVector::CrossProduct(MovementComponent->GetClimbSurfaceNormal(), -GetActorRightVector());
			RightDirection = FVector::CrossProduct(MovementComponent->GetClimbSurfaceNormal(), GetActorUpVector());
		}

		// Add movement input
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ABotwCharacter::MouseMove(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning,  TEXT("MouseMoveAction"));

    UE_LOG(LogTemp, Warning, TEXT("Left: %s, Right: %s, Middle: %s"),
       bIsLeftMouseButtonDown ? TEXT("Pressed") : TEXT("Not Pressed"),
       bIsRightMouseButtonDown ? TEXT("Pressed") : TEXT("Not Pressed"),
       bIsMiddleMouseButtonDown ? TEXT("Pressed") : TEXT("Not Pressed"));

    if (!(bIsLeftMouseButtonDown && bIsRightMouseButtonDown) && !bIsMiddleMouseButtonDown) return;

    UE_LOG(LogTemp, Warning,  TEXT("MouseMoveAction PAST CHECK"));

    GetCharacterMovement()->bOrientRotationToMovement = true;

	ABotwCharacter* Character = Cast<ABotwCharacter>(MovementComponent->GetOwner());

	if (Character->IsPunching()) return;
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Determine movement direction
		FRotator YawRotation;

        // Use the controller's rotation otherwise
        const FRotator Rotation = Controller->GetControlRotation();
        YawRotation = FRotator(0, Rotation.Yaw, 0);

		// Get forward and right vectors
		FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Special handling for climbing
		if (MovementComponent->IsClimbing())
		{
			ForwardDirection = FVector::CrossProduct(MovementComponent->GetClimbSurfaceNormal(), -GetActorRightVector());
			RightDirection = FVector::CrossProduct(MovementComponent->GetClimbSurfaceNormal(), GetActorUpVector());
		}

		// Add movement input
		AddMovementInput(ForwardDirection, 1.0);
		AddMovementInput(RightDirection, 0.0);
	}
}

void ABotwCharacter::Look(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning,  TEXT("Look"));

    FVector2D LookAxisVector = Value.Get<FVector2D>();

    UE_LOG(LogTemp, Warning,  TEXT("bIsLeftMouseButtonDown: %s"), bIsLeftMouseButtonDown ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Warning,  TEXT("bIsRightMouseButtonDown: %s"), bIsRightMouseButtonDown ? TEXT("true") : TEXT("false"));

    if (Controller != nullptr)
    {
        if (bIsLeftMouseButtonDown && !bIsRightMouseButtonDown)
        {
            UE_LOG(LogTemp, Warning,  TEXT("Left but no right"));

            // Rotate the camera freely around the character
            AddControllerYawInput(LookAxisVector.X);
            AddControllerPitchInput(LookAxisVector.Y);

           UE_LOG(LogTemp, Warning,  TEXT("Look LookAxisVector.Y: %f"), LookAxisVector.Y);
        }
        else if (bIsMiddleMouseButtonDown || bIsRightMouseButtonDown || (bIsLeftMouseButtonDown && bIsRightMouseButtonDown))
        {
            UE_LOG(LogTemp, Warning,  TEXT("Left and right OR right"));

            // Rotate the character on X-axis (yaw)
            FRotator NewCharacterRotation = GetActorRotation();
            NewCharacterRotation.Yaw += LookAxisVector.X;
            SetActorRotation(NewCharacterRotation);

            // Adjust the controller's rotation to update the camera independently
            FRotator ControllerRotation = Controller->GetControlRotation();
            ControllerRotation.Yaw += LookAxisVector.X; // Camera yaw follows the mouse
            //ControllerRotation.Pitch = FMath::Clamp(ControllerRotation.Pitch - LookAxisVector.Y, -80.0f, 80.0f); // Clamp pitch
            ControllerRotation.Pitch -= LookAxisVector.Y;
            Controller->SetControlRotation(ControllerRotation);

            UE_LOG(LogTemp, Warning,  TEXT("Look ControllerRotation: %f"), LookAxisVector.Y);
            UE_LOG(LogTemp, Warning,  TEXT("Look ControllerRotation: %f"), ControllerRotation.Pitch);
        }
    }
}

// ZOOM

void ABotwCharacter::ZoomCamera(const FInputActionValue& Value)
{
    if (CameraBoom)
    {
        float AxisValue = Value.Get<float>(); // Get the axis value from the mouse wheel

        float NewTargetArmLength = CameraBoom->TargetArmLength - (AxisValue * ZoomSpeed * GetWorld()->GetDeltaSeconds());
        CameraBoom->TargetArmLength = FMath::Clamp(NewTargetArmLength, MinZoomDistance, MaxZoomDistance);
    }
}

// LOOK FUNCTIONS

void ABotwCharacter::OnLeftMousePressed()
{
    UE_LOG(LogTemp, Warning,  TEXT("OnLeftMousePressed"));

    bIsLeftMouseButtonDown = true;

    UE_LOG(LogTemp, Warning,  TEXT("OnLeftMousePressed past 2 button check"));

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        // Save the original cursor position
        PlayerController->GetMousePosition(OriginalCursorPosition.X, OriginalCursorPosition.Y);
        
        // Hide the cursor and lock it for seamless camera rotation
        PlayerController->bShowMouseCursor = false;

        FInputModeGameOnly InputMode; // Game-only mode to capture the mouse
        PlayerController->SetInputMode(InputMode);
    }

    // Disable movement-based rotation while LMB is pressed
    if (!bIsRightMouseButtonDown) // Only disable if RMB is not pressed
    {
        GetCharacterMovement()->bOrientRotationToMovement = false;
    }
}

void ABotwCharacter::OnLeftMouseReleased()
{
    UE_LOG(LogTemp, Warning,  TEXT("OnLeftMouseReleased"));

    bIsLeftMouseButtonDown = false;

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (!bIsRightMouseButtonDown && !bIsMiddleMouseButtonDown) {
            // Restore the cursor position
            PlayerController->SetMouseLocation(OriginalCursorPosition.X, OriginalCursorPosition.Y);
            // Restore the cursor and unlock it
            PlayerController->bShowMouseCursor = true;
            
            FInputModeGameAndUI InputMode; // Restore the previous mode
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
            InputMode.SetHideCursorDuringCapture(false);
            PlayerController->SetInputMode(InputMode);
        }
    }

    // Re-enable movement-based rotation only if RMB is not pressed
    if (!bIsRightMouseButtonDown && !bIsMiddleMouseButtonDown)
    {
        GetCharacterMovement()->bOrientRotationToMovement = true;
    }
}

void ABotwCharacter::OnRightMousePressed()
{
    UE_LOG(LogTemp, Warning,  TEXT("OnRightMousePressed"));

    bIsRightMouseButtonDown = true;

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        // Save the original cursor position
        PlayerController->GetMousePosition(OriginalCursorPosition.X, OriginalCursorPosition.Y);

        // Hide the cursor and lock it for seamless rotation
        PlayerController->bShowMouseCursor = false;

        FInputModeGameOnly InputMode; // Game-only mode to capture the mouse
        PlayerController->SetInputMode(InputMode);
    }

    FRotator CameraRotation = Controller->GetControlRotation();
    FRotator NewCharacterRotation = GetActorRotation();
    NewCharacterRotation.Yaw = CameraRotation.Yaw;
    SetActorRotation(NewCharacterRotation);
}

void ABotwCharacter::OnRightMouseReleased()
{
    UE_LOG(LogTemp, Warning,  TEXT("OnRightMouseReleased"));

    bIsRightMouseButtonDown = false;

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        // Restore the cursor and unlock it
        if (!bIsLeftMouseButtonDown && !bIsMiddleMouseButtonDown) {
            // Restore the cursor position
            PlayerController->SetMouseLocation(OriginalCursorPosition.X, OriginalCursorPosition.Y);
            // Restore the cursor and unlock it
            PlayerController->bShowMouseCursor = true;

            FInputModeGameAndUI InputMode; // Restore the previous mode
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
            InputMode.SetHideCursorDuringCapture(false);
            PlayerController->SetInputMode(InputMode);
        }
    }

    // Re-enable movement-based rotation only if LMB is not pressed
    if (!bIsLeftMouseButtonDown && !bIsMiddleMouseButtonDown)
    {
        GetCharacterMovement()->bOrientRotationToMovement = true;
    }
}

void ABotwCharacter::OnMiddleMousePressed()
{
    UE_LOG(LogTemp, Warning,  TEXT("OnMiddleMousePressed"));
    bIsMiddleMouseButtonDown = true;

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        // Save the original cursor position
        PlayerController->GetMousePosition(OriginalCursorPosition.X, OriginalCursorPosition.Y);

        // Hide the cursor and lock it for seamless rotation
        PlayerController->bShowMouseCursor = false;

        FInputModeGameOnly InputMode; // Game-only mode to capture the mouse
        PlayerController->SetInputMode(InputMode);
    }

    FRotator CameraRotation = Controller->GetControlRotation();
    FRotator NewCharacterRotation = GetActorRotation();
    NewCharacterRotation.Yaw = CameraRotation.Yaw;
    SetActorRotation(NewCharacterRotation);
}

void ABotwCharacter::OnMiddleMouseReleased()
{
    UE_LOG(LogTemp, Warning,  TEXT("OnMiddleMouseReleased"));
    bIsMiddleMouseButtonDown = false;

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        // Restore the cursor and unlock it
        if (!(bIsLeftMouseButtonDown && bIsRightMouseButtonDown)) {
            // Restore the cursor position
            PlayerController->SetMouseLocation(OriginalCursorPosition.X, OriginalCursorPosition.Y);
            // Restore the cursor and unlock it
            PlayerController->bShowMouseCursor = true;

            FInputModeGameAndUI InputMode; // Restore the previous mode
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
            InputMode.SetHideCursorDuringCapture(false);
            PlayerController->SetInputMode(InputMode);
        }
    }

    // Re-enable movement-based rotation only if LMB is not pressed
    if (!(bIsLeftMouseButtonDown && bIsRightMouseButtonDown))
    {
        GetCharacterMovement()->bOrientRotationToMovement = true;
    }
}


// LOOK FUNCTIONS END

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

	ABotwCharacter* Character = Cast<ABotwCharacter>(MovementComponent->GetOwner());

	UE_LOG(LogTemp, Warning, TEXT("BEFORE IF: bIsPunching %s"), Character->IsPunching() ? TEXT("true") : TEXT("false"));

    if (Character && Punching_UE_Montage && !Character->IsPunching())
    {
		UE_LOG(LogTemp, Warning, TEXT("AFTER IF: bIsPunching %s"), Character->IsPunching() ? TEXT("true") : TEXT("false"));

		if (!AnimInstance)
        {
            AnimInstance = GetMesh()->GetAnimInstance();
            if (!AnimInstance)
            {
                UE_LOG(LogTemplateCharacter, Error, TEXT("AnimInstance is null in Attack for %s"), *GetNameSafe(this));
                return;
            }
        }

        AnimInstance->Montage_Play(Punching_UE_Montage);

		UE_LOG(LogTemp, Warning, TEXT("ice cream and beans"));

        // Set up a notification or callback to reset the flag when the montage ends
        FOnMontageEnded MontageEndedDelegate;
        MontageEndedDelegate.BindUObject(this, &ABotwCharacter::OnPunchingMontageEnded);
        AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, Punching_UE_Montage);
    }
}

void ABotwCharacter::OnPunchingMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("[OnPunchingMontageEnded]"));
    if (Montage && Montage == Punching_UE_Montage && MovementComponent)
    {

    }
    else
    {
        if (!Montage)
        {

        }
        else if (Montage != Punching_UE_Montage)
        {

        }

        if (!MovementComponent)
        {

        }
    }
}