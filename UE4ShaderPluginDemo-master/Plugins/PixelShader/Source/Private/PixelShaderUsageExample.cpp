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

UTexture2D* FPixelShaderUsageExample::LoadTexFromPath(const FName& Path)
{
	if (Path == NAME_None) return NULL;

	return Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), NULL, *Path.ToString()));

}
FPixelShaderUsageExample::FPixelShaderUsageExample(ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	//static ConstructorHelpers::FObjectFinder<UTexture2D> InputTexObj(TEXT("Texture2D'Game/Textures/Texture.Texture'"));
	
	FString PathText = "Texture2D'/Game/Textures/Texture.Texture'";
	FName Path = *PathText;
	
	UTexture2D* InputTexObj = LoadTexFromPath(Path);
	
	FTexture2DMipMap* TextureMipMap = &InputTexObj->PlatformData->Mips[0];
	
	FColor* FormattedImageData = static_cast<FColor*>(InputTexObj->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_ONLY));
	int32 TextureWidth, TextureHeight;
	int i, j; //LOOP COUNTERS

	FVector2D MinimumVector;

	TextureWidth = TextureMipMap->SizeX;
	TextureHeight = TextureMipMap->SizeY;

	//ALLOCATION OF TEXTURE ARRAY
	FVector2D** TextureArray = new FVector2D*[TextureMipMap->SizeX];
	for (i = 0; i < TextureMipMap->SizeX - 1; ++i)
	{
		TextureArray[i] = new FVector2D[TextureMipMap->SizeY];
	}


	//CONVERSION FROM TEXTURE TO ARRAY OF VECTORS
	//DISTANCE MAP INITIALIZATION

	for (j = 0; j < TextureMipMap->SizeY - 1; ++j)
	{
		for (i = 0; i < TextureMipMap->SizeX - 1; ++i)
		{
			if (i >= 0 && i < TextureMipMap->SizeX && j >= 0 && j < TextureMipMap->SizeY)
			{
				if (FormattedImageData[j*TextureMipMap->SizeX + i].ToHex() == "FFFFFF")
				{
					TextureArray[i][j].Set(0, 0);
				}
				else
					if (FormattedImageData[j*TextureMipMap->SizeX + i].ToHex() == "000000")
					{
						TextureArray[i][j].Set(UINT32_MAX, UINT32_MAX);
					}
			}
		}
	}

	//FREE TEXTURE DATA MEMORY
	InputTexObj->PlatformData->Mips[0].BulkData.Unlock();

	//FIRST TEXTURE SCAN
	for (j = 0; j < TextureMipMap->SizeY - 1; ++j)
	{
		for (i = 0; i < TextureMipMap->SizeX - 1; ++i)
		{
			MinimumVector = std::min(TextureArray[i][j], TextureArray[i][j - 1] + (0, 1));
			TextureArray[i][j].Set(MinimumVector.X, MinimumVector.Y);
		}
		for (i = 1; i < TextureMipMap->SizeX - 1; ++i)
		{
			MinimumVector = std::min(TextureArray[i][j], TextureArray[i - 1][j] + (1, 0));
			TextureArray[i][j].Set(MinimumVector.X, MinimumVector.Y);
		}
		for (i = TextureMipMap->SizeX - 2; i > 0; i--)
		{
			MinimumVector = std::min(TextureArray[i][j], TextureArray[i + 1][j] + (1, 0));
			TextureArray[i][j].Set(MinimumVector.X, MinimumVector.Y);
		}
	}
	j = 0;
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
		for (i = TextureWidth - 2; i > 0; ++i)
		{
			MinimumVector = std::min(TextureArray[i][j], TextureArray[i + 1][j] + (1, 0));
			TextureArray[i][j].Set(MinimumVector.X, MinimumVector.Y);
		}
	}
	UTexture2D* OTexture = InputTexObj;

	OTexture->PlatformData->Mips[0] = *TextureMipMap;
	FColor* FormattedOutputData = static_cast<FColor*>(OTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
	for(j = 0; j < TextureHeight; ++j)
	{
		for (i = 0; i < TextureWidth; ++i)
		{
			FormattedOutputData->A = sqrtf(pow(TextureArray[i][j].X, 2) + pow(TextureArray[i][j].Y, 2));
		}
	}

	delete TextureArray;
	//GENERATE NEW DISTANCE FIELD TEXTURE USING NORMALIZED VALUES OF THE TEXTURE ARRAY AS ALPHA CHANNEL
	//FEED THIS TEXTURE INTO THE SHADER AND SHADER DO REST
/*
	//Post Processing

	//Check Function
	get (x1,x2), (y1,y2) and (z1,z2) from the distance map;
	check for singular cases;
	compute m,M,stepm,stepM;
	compute imax ;
	for i = 1 -> imax
	m = m+stepm; M = M+stepM
	for j = m -> M
	testp = p + (i,j);
	if d( testp , NOP(p) ) < d ( testp, NOP(testp))
	NOP( testp ) = NOP ( p);

	vector2 NOP(vector2[][] L, int i, int j, vector2 input)
	{
		vector2 output;

		output = L[i + input.x][j + input.y];
		return output;
	}
	//nearest object pixel (NOP)
	vector2 check(vector2 p, int quadrant)
	{
		///////////////////
		//DETERMINE X,Y,Z//
		///////////////////

		//Determine singular cases
		//if(x2 == z2)
		//if(x2 == (y2-1))
		//if(stepm == stepM)

		//Max-line coefficients
		stepM = (y1 - x1) / (x2 - y2 + 1);
		M = (stepM*(x1 + y1) - x2 - y2 + 1) / 2;
		//min-line coefficients
		stepm = (z1 - 1 - x1) / (x2 - z2);
		m = (stepm*(x1 + z1 - 1) - x2 - z2) / 2;

		iMax = (M - m) / (stepm - stepM);

		for (i = 1; i < imax; i++
		{
			m += stepm;
			M += stepM;
			for (j = m; j < M; j++)
			{
				testp = p + (i, j);
				if d(testp, NOP(L, i, j, p)) < p(testp, NOP(L, i, j, testp))
					NOP(L, i, j, testp) = NOP(L, i, j, p);
			}
		}
	}
	//Error Correction Algorithm
	For all pixel p but the last column
	if NOP(p) != NOP(p+(0,1))
	then
	if NOP(p) != NOP(p+(-1,0)) then check (p,1);
	if NOP(p) != NOP(p+(1,0)) then check (p,2);
	if NOP(p+(0,1)) != NOP(p+(1,1)) then check (p+(0,1),3);
	if NOP(p+(0,1)) != NOP(p+(-1,1)) then check (p+(0,1),4);
	

	for (i = 0; i < M - 2; i++)
	{
		for (j = 0; j < N - 1; j++)
		{
			if (NOP(L[i][j]) != NOP(L[i][j] + (0, 1)))
			{
				if (NOP(L, i, j, L[i][j]) != NOP(L, i, j, L[i][j] + (-1, 0)))
				{
					check(L[i][j], 1);
				}
				if (NOP(L, i, j, L[i][j]) != NOP(L, i, j, L[i][j] + (1, 0)))
				{
					check(L[i][j], 2);
				}
				if (NOP(L, i, j, L[i][j] + (0, 1)) != NOP(L, i, j, L[i][j] + (1, 1)))
				{
					check(L[i][j] + (0, 1), 3);
				}
				if (NOP(L, i, j, L[i][j] + (0, 1)) != NOP(L, i, j, L[i][j] + (-1, 1))
				{
					check(L[i][j] + (0, 1), 4);
				}
			}
		}
	}
*/

	FeatureLevel = ShaderFeatureLevel;
	
	ConstantParameters = FPixelShaderConstantParameters();
	VariableParameters = FPixelShaderVariableParameters();
	
	bMustRegenerateSRV = false;
	bIsPixelShaderExecuting = false;
	bIsUnloading = false;
	bSave = false;

	CurrentTexture = NULL;
	CurrentRenderTarget = NULL;
	TextureParameterSRV = NULL;
}

FPixelShaderUsageExample::~FPixelShaderUsageExample()
{
	bIsUnloading = true;
}

void FPixelShaderUsageExample::ExecutePixelShader(UTextureRenderTarget2D* RenderTarget, FTexture2DRHIRef InputTexture)
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
	//VariableParameters.EndColor = FVector4(EndColor.R / 255.0, EndColor.G / 255.0, EndColor.B / 255.0, EndColor.A / 255.0);
	//VariableParameters.TextureParameterBlendFactor = TextureParameterBlendFactor;
	VariableParameters.BASE_COLOR = false;
	VariableParameters.OUTER_GLOW = false;
	VariableParameters.SOFT_EDGES = false;
	VariableParameters.OUTLINE = false;

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

FTexture2DRHIRef FPixelShaderUsageExample::GetTexture()
{
	return this->OutputTexture;
}
