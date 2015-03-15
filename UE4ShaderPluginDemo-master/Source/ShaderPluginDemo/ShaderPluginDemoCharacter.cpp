#include "ShaderPluginDemo.h"
#include "ShaderPluginDemoCharacter.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/InputSettings.h"
#include "Engine.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

AShaderPluginDemoCharacter::AShaderPluginDemoCharacter(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	FirstPersonCameraComponent = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->AttachParent = GetCapsuleComponent();
	FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 64.f);
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	GunOffset = FVector(100.0f, 30.0f, 10.0f);

	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);			
	Mesh1P->AttachParent = FirstPersonCameraComponent;
	Mesh1P->RelativeLocation = FVector(0.f, 0.f, -150.f);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;

	//ShaderPluginDemo variables
	TotalElapsedTime = 0;
}

//Since we need the featurelevel, we need to create the shaders from beginplay, and not in the ctor.
void AShaderPluginDemoCharacter::BeginPlay()
{
	PixelShading = new FPixelShaderUsageExample(GetWorld()->Scene->GetFeatureLevel());
}

//Do not forget cleanup :)
void AShaderPluginDemoCharacter::BeginDestroy()
{
	Super::BeginDestroy();

	if (NULL != PixelShading)
	{
		delete PixelShading;
	}
}

//The idea here is to run the execute functions once per frame. Since we need to pass the compute shader through our pixel shader to get a format that the higher level
//system can understand, we do that here.
void AShaderPluginDemoCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TotalElapsedTime += DeltaSeconds;

	if (NULL != PixelShading)
	{
		EndColorBuildup = FMath::Clamp(EndColorBuildup + DeltaSeconds * EndColorBuildupDirection, 0.0f, 1.0f);
		if (EndColorBuildup >= 1.0 || EndColorBuildup <= 0)
		{
			EndColorBuildupDirection *= -1;
		}
		

		FTexture2DRHIRef Texture = NULL;
		//PixelShading
		Texture = PixelShading->GetTexture();
		PixelShading->ExecutePixelShader(RenderTarget, Texture);
	}
}

void AShaderPluginDemoCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	check(InputComponent);

	//ShaderPluginDemo Specific input mappings
	InputComponent->BindAction("SavePixelShaderOutput", IE_Pressed, this, &AShaderPluginDemoCharacter::SavePixelShaderOutput);
	//ShaderPluginDemo Specific input mappings

	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	InputComponent->BindAction("Fire", IE_Pressed, this, &AShaderPluginDemoCharacter::OnFire);
	
	InputComponent->BindAxis("MoveForward", this, &AShaderPluginDemoCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AShaderPluginDemoCharacter::MoveRight);

	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AShaderPluginDemoCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AShaderPluginDemoCharacter::LookUpAtRate);
}

//Saving functions
void AShaderPluginDemoCharacter::SavePixelShaderOutput()
{
	PixelShading->Save();
}

void AShaderPluginDemoCharacter::OnFire()
{
	//Try to set a texture to the object we hit!
	FHitResult HitResult; 
	FVector StartLocation = FirstPersonCameraComponent->GetComponentLocation();
	FRotator Direction = FirstPersonCameraComponent->GetComponentRotation();
	FVector EndLocation = StartLocation + Direction.Vector() * 10000;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingle(HitResult, StartLocation, EndLocation, ECC_Visibility, QueryParams))
	{
		TArray<UStaticMeshComponent*> StaticMeshComponents = TArray<UStaticMeshComponent*>();
		AActor* HitActor = HitResult.GetActor();

		if (NULL != HitActor)
		{
			HitActor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
			for (int32 i = 0; i < StaticMeshComponents.Num(); i++)
			{
				UStaticMeshComponent* CurrentStaticMeshPtr = StaticMeshComponents[i];
				CurrentStaticMeshPtr->SetMaterial(0, MaterialToApplyToClickedObject);
				UMaterialInstanceDynamic* MID = CurrentStaticMeshPtr->CreateAndSetMaterialInstanceDynamic(0);
				UTexture* CastedRenderTarget = Cast<UTexture>(RenderTarget);
				MID->SetTextureParameterValue("InputTexture", CastedRenderTarget);
			}
		}
	}
}

void AShaderPluginDemoCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AShaderPluginDemoCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AShaderPluginDemoCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShaderPluginDemoCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}