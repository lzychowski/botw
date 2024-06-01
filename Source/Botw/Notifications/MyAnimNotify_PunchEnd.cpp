#include "MyAnimNotify_PunchEnd.h"
#include "../BotwCharacter.h"

void UMyAnimNotify_PunchEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (MeshComp && MeshComp->GetOwner())
    {
        ABotwCharacter* Character = Cast<ABotwCharacter>(MeshComp->GetOwner());
        if (Character)
        {
            Character->bIsPunching = false;
        }
    }
}
