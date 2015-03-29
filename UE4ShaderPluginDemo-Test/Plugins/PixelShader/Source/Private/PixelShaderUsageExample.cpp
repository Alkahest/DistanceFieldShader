/******************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2015 Fredrik Lindh
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
******************************************************************************/

#include "PixelShaderPrivatePCH.h"
#include "RHIStaticStates.h"
#include <algorithm>

//It seems to be the convention to expose all vertex declarations as globals, and then reference them as externs in the headers where they are needed.
//It kind of makes sense since they do not contain any parameters that change and are purely used as their names suggest, as declarations :)
TGlobalResource<FTextureVertexDeclaration> GTextureVertexDeclaration;

FPixelShaderUsageExample::FPixelShaderUsageExample(FColor StartColor, ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	FeatureLevel = ShaderFeatureLevel;

	ConstantParameters = FPixelShaderConstantParameters();
	ConstantParameters.StartColor = FVector4(StartColor.R / 255.0, StartColor.G / 255.0, StartColor.B / 255.0, StartColor.A / 255.0);
	
	VariableParameters = FPixelShaderVariableParameters();

	FString PathText = "Texture2D'/Game/Textures/Texture.Texture'";
	FName Path = *PathText;

	UTexture2D* InputTexObj = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), NULL, *Path.ToString()));

	FTexture2DMipMap* TextureMipMap = &InputTexObj->PlatformData->Mips[0];

	FColor* FormattedImageData = static_cast<FColor*>(InputTexObj->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_ONLY));
	int32 TextureWidth, TextureHeight;
	int i, j; //LOOP COUNTERS

	FVector2D MinimumVector;

	TextureWidth = TextureMipMap->SizeX;
	TextureHeight = TextureMipMap->SizeY;
	
	//ALLOCATION AND INITIALIZATION OF TEXTURE ARRAY
	FVector2D** TextureArray = new FVector2D*[TextureWidth];
	for (i = 0; i < TextureWidth; ++i)
	{
		TextureArray[i] = new FVector2D[TextureHeight];
		for (j = 0; j < TextureHeight - 1; ++j)
		{
			if (i >= 0 && i < TextureWidth && j >= 0 && j < TextureHeight)
			{
				if (FormattedImageData[j*TextureWidth + i].ToHex() == "FFFFFFFF")
				{
					TextureArray[i][j].Set(0, 0);
				}
				else if (FormattedImageData[j*TextureWidth + i].ToHex() == "000000FF")
				{
					TextureArray[i][j].Set(UINT32_MAX, UINT32_MAX);
				}
			}
		}
	}

	InputTexObj->PlatformData->Mips[0].BulkData.Unlock();

	//FIRST TEXTURE SCAN
	for (j = 0; j < TextureHeight - 1; ++j)
	{
		for (i = 0; i < TextureWidth - 1; ++i)
		{
			MinimumVector = std::min(TextureArray[i][j], TextureArray[i][j - 1] + (0, 1));
			TextureArray[i][j].Set(MinimumVector.X, MinimumVector.Y);
		}
		for (i = 1; i < TextureWidth - 1; ++i)
		{
			MinimumVector = std::min(TextureArray[i][j], TextureArray[i - 1][j] + (1, 0));
			TextureArray[i][j].Set(MinimumVector.X, MinimumVector.Y);
		}
		for (i = TextureWidth - 2; i > 0; i--)
		{
			MinimumVector = std::min(TextureArray[i][j], TextureArray[i + 1][j] + (1, 0));
			TextureArray[i][j].Set(MinimumVector.X, MinimumVector.Y);
		}
	}

	//SECOND TEXTURE SCAN
	for (j = TextureHeight - 2; j > 0; --j)
	{
		for (i = 0; i < TextureWidth - 1; ++i)
		{
			MinimumVector = std::min(TextureArray[i][j], TextureArray[i][j - 1] + (0, 1));
			TextureArray[i][j].Set(MinimumVector.X, MinimumVector.Y);
		}
		for (i = 1; i < TextureWidth - 1; ++i)
		{
			MinimumVector = std::min(TextureArray[i][j], TextureArray[i - 1][j] + (1, 0));
			TextureArray[i][j].Set(MinimumVector.X, MinimumVector.Y);
		}
		for (i = TextureWidth - 2; i > 0; i--)
		{
			MinimumVector = std::min(TextureArray[i][j], TextureArray[i + 1][j] + (1, 0));
			TextureArray[i][j].Set(MinimumVector.X, MinimumVector.Y);
		}
	}

	TResourceArray<FColor>* OutArray = new TResourceArray < FColor >;
	for (i = 0; i < TextureWidth; ++i)
	{
		for (j = 0; j < TextureHeight - 1; ++j)
		{
			if (i >= 0 && i < TextureWidth && j >= 0 && j < TextureHeight)
			{
				float alpha;
				float length = sqrtf((TextureArray[i][j].X * TextureArray[i][j].X) + (TextureArray[i][j].Y*TextureArray[i][j].Y));
				if (length > 0)
				{
					TextureArray[i][j] /= length;

					alpha = sqrtf((TextureArray[i][j].X * TextureArray[i][j].X) + (TextureArray[i][j].Y*TextureArray[i][j].Y));
				}
				else
				{
					alpha = length;
				}
				OutArray->Add(FColor(0.0f, 0.0f, 0.0f, alpha));
			}
		}
	}

	bMustRegenerateSRV = false;
	bIsPixelShaderExecuting = false;
	bIsUnloading = false;
	bSave = false;


	EPixelFormat mFormat = PF_R8G8B8A8;

	FResourceArrayInterface* InArrayData(OutArray);

	FRHIResourceCreateInfo CreateInfo(InArrayData);

	FTexture2DRHIRef OutTexture = RHICreateTexture2D(TextureWidth, TextureHeight, mFormat, 1, 1, TexCreate_ShaderResource|TexCreate_UAV, CreateInfo);

	CurrentTexture = OutTexture;

	CurrentRenderTarget = NULL;
	TextureParameterSRV = NULL;
}

FPixelShaderUsageExample::~FPixelShaderUsageExample()
{
	bIsUnloading = true;
}

void FPixelShaderUsageExample::ExecutePixelShader(UTextureRenderTarget2D* RenderTarget, FTexture2DRHIRef InputTexture, FColor EndColor, float TextureParameterBlendFactor)
{
	if (bIsUnloading || bIsPixelShaderExecuting) //Skip this execution round if we are already executing
	{
		return;
	}

	bIsPixelShaderExecuting = true;

	if (TextureParameter != InputTexture)
	{
		bMustRegenerateSRV = true;
	}

	//Now set our runtime parameters!
	VariableParameters.EndColor = FVector4(EndColor.R / 255.0, EndColor.G / 255.0, EndColor.B / 255.0, EndColor.A / 255.0);
	VariableParameters.TextureParameterBlendFactor = TextureParameterBlendFactor;
	VariableParameters.BASE_COLOR_VALUE = FVector4(FColor::Red.R / 255.0, FColor::Red.G / 255.0, FColor::Red.B / 255.0, FColor::Red.A / 255.0);

	VariableParameters.OUTER_GLOW = false;
	VariableParameters.SOFT_EDGES = false;
	VariableParameters.OUTLINE = false;
	VariableParameters.BASE_COLOR = false;

	VariableParameters.OUTLINE_MIN_VALUE_0 = 1.0f;
	VariableParameters.OUTLINE_MIN_VALUE_1 = 1.0f;
	VariableParameters.OUTLINE_MAX_VALUE_0 = 1.0f;
	VariableParameters.OUTLINE_MAX_VALUE_1 = 1.0f;
	VariableParameters.SOFT_EDGE_MIN = 1.0f;
	VariableParameters.SOFT_EDGE_MAX = 1.0f;
	VariableParameters.OUTER_GLOW_MIN_D_VALUE = 1.0f;
	VariableParameters.OUTER_GLOW_MAX_D_VALUE = 1.0f;
	VariableParameters.GLOW_UV_OFFSET = FVector2D(1.0f, 1.0f);
	VariableParameters.OUTER_GLOW_COLOR = FVector4(FColor::Yellow.R / 255.0, FColor::Yellow.G / 255.0, FColor::Yellow.B / 255.0, FColor::Yellow.A / 255.0);
	VariableParameters.OUTLINE_COLOR = FVector4(FColor::Black.R / 255.0, FColor::Black.G / 255.0, FColor::Black.B / 255.0, FColor::Black.A / 255.0);

	CurrentRenderTarget = RenderTarget;
	TextureParameter = InputTexture;
	//This macro sends the function we declare inside to be run on the render thread. What we do is essentially just send this class and tell the render thread to run the internal render function as soon as it can.
	//I am still not 100% Certain on the thread safety of this, if you are getting crashes, depending on how advanced code you have in the start of the ExecutePixelShader function, you might have to use a lock :)
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		FPixelShaderRunner,
		FPixelShaderUsageExample*, PixelShader, this,
		{
			PixelShader->ExecutePixelShaderInternal();
		}
	);
}

void FPixelShaderUsageExample::ExecutePixelShaderInternal()
{
	check(IsInRenderingThread());

	if (bIsUnloading) //If we are about to unload, so just clean up the SRV :)
	{
		if (NULL != TextureParameterSRV)
		{
			TextureParameterSRV.SafeRelease();
			TextureParameterSRV = NULL;
		}

		return;
	}

	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();

	//If our input texture reference has changed, we need to recreate our SRV
	if (bMustRegenerateSRV)
	{
		bMustRegenerateSRV = false;

		if (NULL != TextureParameterSRV)
		{
			TextureParameterSRV.SafeRelease();
			TextureParameterSRV = NULL;
		}

		TextureParameterSRV = RHICreateShaderResourceView(TextureParameter, 0);
	}

	//This is where the magic happens
	CurrentTexture = CurrentRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture();
	SetRenderTarget(RHICmdList, CurrentTexture, FTextureRHIRef());
	RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
	RHICmdList.SetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
	
	static FGlobalBoundShaderState BoundShaderState;
	TShaderMapRef<FVertexShaderExample> VertexShader(GetGlobalShaderMap(FeatureLevel));
	TShaderMapRef<FPixelShaderDeclaration> PixelShader(GetGlobalShaderMap(FeatureLevel));

	SetGlobalBoundShaderState(RHICmdList, FeatureLevel, BoundShaderState, GTextureVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	PixelShader->SetSurfaces(RHICmdList, TextureParameterSRV);
	PixelShader->SetUniformBuffers(RHICmdList, ConstantParameters, VariableParameters);

	// Draw a fullscreen quad that we can run our pixel shader on
	FTextureVertex Vertices[4];
	Vertices[0].Position = FVector4(-1.0f, 1.0f, 0, 1.0f);
	Vertices[1].Position = FVector4(1.0f, 1.0f, 0, 1.0f);
	Vertices[2].Position = FVector4(-1.0f, -1.0f, 0, 1.0f);
	Vertices[3].Position = FVector4(1.0f, -1.0f, 0, 1.0f);
	Vertices[0].UV = FVector2D(0, 0);
	Vertices[1].UV = FVector2D(1, 0);
	Vertices[2].UV = FVector2D(0, 1);
	Vertices[3].UV = FVector2D(1, 1);

	DrawPrimitiveUP(RHICmdList, PT_TriangleStrip, 2, Vertices, sizeof(Vertices[0]));
	
	PixelShader->UnbindBuffers(RHICmdList);
	
	if (bSave) //Save to disk if we have a save request!
	{
		bSave = false;

		SaveScreenshot(RHICmdList);
	}

	bIsPixelShaderExecuting = false;
}

void FPixelShaderUsageExample::SaveScreenshot(FRHICommandListImmediate& RHICmdList)
{
	check(IsInRenderingThread());

	TArray<FColor> Bitmap;

	FReadSurfaceDataFlags ReadDataFlags;
	ReadDataFlags.SetLinearToGamma(false);
	ReadDataFlags.SetOutputStencil(false);
	ReadDataFlags.SetMip(0); //No mip supported ofc!
	
	//This is pretty straight forward. Since we are using a standard format, we can use this convenience function instead of having to lock rect.
	RHICmdList.ReadSurfaceData(CurrentTexture, FIntRect(0, 0, CurrentTexture->GetSizeX(), CurrentTexture->GetSizeY()), Bitmap, ReadDataFlags);

	// if the format and texture type is supported
	if (Bitmap.Num())
	{
		// Create screenshot folder if not already present.
		IFileManager::Get().MakeDirectory(*FPaths::ScreenShotDir(), true);

		const FString ScreenFileName(FPaths::ScreenShotDir() / TEXT("VisualizeTexture"));

		uint32 ExtendXWithMSAA = Bitmap.Num() / CurrentTexture->GetSizeY();

		// Save the contents of the array to a bitmap file. (24bit only so alpha channel is dropped)
		FFileHelper::CreateBitmap(*ScreenFileName, ExtendXWithMSAA, CurrentTexture->GetSizeY(), Bitmap.GetData());

		UE_LOG(LogConsoleResponse, Display, TEXT("Content was saved to \"%s\""), *FPaths::ScreenShotDir());
	}
	else
	{
		UE_LOG(LogConsoleResponse, Error, TEXT("Failed to save BMP, format or texture type is not supported"));
	}
}
