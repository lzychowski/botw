#include "MyAnimNotify_PunchStart.h"
#include "../BotwCharacter.h"

void UMyAnimNotify_PunchStart::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (MeshComp && MeshComp->GetOwner())
    {
        UE_LOG(LogTemp, Error, TEXT("passed Notify first if"));
        ABotwCharacter* Character = Cast<ABotwCharacter>(MeshComp->GetOwner());
        if (Character)
        {
            Character->SetPunching(true);
        }
    }
}
