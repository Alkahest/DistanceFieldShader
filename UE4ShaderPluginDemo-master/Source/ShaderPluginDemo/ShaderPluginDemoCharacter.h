#pragma once
#include "GameFramework/Character.h"
#include "PixelShaderUsageExample.h"
#include "ShaderPluginDemoCharacter.generated.h"

class UInputComponent;

UCLASS(config=Game)
class AShaderPluginDemoCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	class USkeletalMeshComponent* Mesh1P;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;
public:
	AShaderPluginDemoCharacter(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

protected:	
	void OnFire();
	void MoveForward(float Val);
	void MoveRight(float Val);
	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);
	
protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	virtual void Tick(float DeltaSeconds) override;
	// End of APawn interface

	bool EnableTouchscreenMovement(UInputComponent* InputComponent);

public:
	FORCEINLINE class USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	/************************************************************************/
	/* Plugin Shader Demo variables!                                        */
	/************************************************************************/
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShaderDemo)
		FColor PixelShaderTopLeftColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShaderDemo)
		UMaterialInterface* MaterialToApplyToClickedObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShaderDemo)
		UTextureRenderTarget2D* RenderTarget;

private:
	FPixelShaderUsageExample* PixelShading;

	float EndColorBuildup;
	float EndColorBuildupDirection;
	float TotalElapsedTime;

	void SavePixelShaderOutput();
};

