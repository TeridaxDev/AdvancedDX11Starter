#include "Renderer.h"

#include "Extensions/imgui/imgui.h"
#include "Extensions/imgui/backends/imgui_impl_dx11.h"
#include "Extensions/imgui/backends/imgui_impl_win32.h"
#include "AssetLoader.h"

#include <DirectXMath.h>

using namespace DirectX;

Renderer::Renderer(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV, Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV, unsigned int windowWidth, unsigned int windowHeight, Sky* sky, std::vector<GameEntity*>& entities, std::vector<Light>& lights)
	: device(device), context(context), swapChain(swapChain), backBufferRTV(backBufferRTV), depthBufferDSV(depthBufferDSV),
	windowWidth(windowWidth), windowHeight(windowHeight), sky(sky),
	entities(entities), lights(lights)
{

	ambientNonPBR = XMFLOAT3(0.1f, 0.1f, 0.25f);

	// Set up the ssao offsets (count must match shader!)
	for (int i = 0; i < ARRAYSIZE(ssaoOffsets); i++)
	{
		ssaoOffsets[i] = DirectX::XMFLOAT4(
			(float)rand() / RAND_MAX * 2 - 1,	// -1 to 1
			(float)rand() / RAND_MAX * 2 - 1,	// -1 to 1
			(float)rand() / RAND_MAX,			// 0 to 1
			0);

		XMVECTOR v = XMVector3Normalize(XMLoadFloat4(&ssaoOffsets[i]));

		// Scale up over the array
		float scale = (float)i / ARRAYSIZE(ssaoOffsets);
		XMVECTOR scaleVector = XMVectorLerp(
			XMVectorSet(0.1f, 0.1f, 0.1f, 1),
			XMVectorSet(1, 1, 1, 1),
			scale * scale);

		XMStoreFloat4(&ssaoOffsets[i], v * scaleVector);

	}

	CreateGenericRenderTarget(windowWidth, windowHeight, sceneColorsRTV, sceneColorsSRV);
	CreateGenericRenderTarget(windowWidth, windowHeight, sceneNormalsRTV, sceneNormalsSRV);
	CreateGenericRenderTarget(windowWidth, windowHeight, sceneAmbientRTV, sceneAmbientSRV);
	CreateGenericRenderTarget(windowWidth, windowHeight, sceneDepthRTV, sceneDepthSRV, DXGI_FORMAT_R32_FLOAT);
	CreateGenericRenderTarget(windowWidth, windowHeight, ssaoResultRTV, ssaoResultSRV);
	CreateGenericRenderTarget(windowWidth, windowHeight, ssaoBlurRTV, ssaoBlurSRV);

}

void Renderer::PostResize(unsigned int windowWidth, unsigned int windowHeight, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV, Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV)
{
	Renderer::backBufferRTV = backBufferRTV;
	Renderer::depthBufferDSV = depthBufferDSV;
	Renderer::windowWidth = windowWidth;
	Renderer::windowHeight = windowHeight;

	sceneColorsRTV.Reset();
	sceneColorsSRV.Reset();
	sceneNormalsRTV.Reset();
	sceneNormalsSRV.Reset();
	sceneAmbientRTV.Reset();
	sceneAmbientSRV.Reset();
	sceneDepthRTV.Reset();
	sceneDepthSRV.Reset();

	CreateGenericRenderTarget(windowWidth, windowHeight, sceneColorsRTV, sceneColorsSRV);
	CreateGenericRenderTarget(windowWidth, windowHeight, sceneNormalsRTV, sceneNormalsSRV);
	CreateGenericRenderTarget(windowWidth, windowHeight, sceneAmbientRTV, sceneAmbientSRV);
	CreateGenericRenderTarget(windowWidth, windowHeight, sceneDepthRTV, sceneDepthSRV, DXGI_FORMAT_R32_FLOAT);
	CreateGenericRenderTarget(windowWidth, windowHeight, ssaoResultRTV, ssaoResultSRV);
	CreateGenericRenderTarget(windowWidth, windowHeight, ssaoBlurRTV, ssaoBlurSRV);

}

void Renderer::Render(Camera* camera, int lightCount, SimpleVertexShader* lightVS, SimplePixelShader* lightPS, Mesh* lightMesh)
{

	// Background color for clearing
	const float color[4] = { 0, 0, 0, 1 };
	const float depth[4] = { 1, 0, 0, 0 };

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV.Get(), color);
	context->ClearRenderTargetView(sceneColorsRTV.Get(), color);
	context->ClearRenderTargetView(sceneNormalsRTV.Get(), color);
	context->ClearRenderTargetView(sceneAmbientRTV.Get(), color);
	context->ClearRenderTargetView(sceneDepthRTV.Get(), depth);
	context->ClearRenderTargetView(ssaoResultRTV.Get(), color);
	context->ClearRenderTargetView(ssaoBlurRTV.Get(), color);
	context->ClearDepthStencilView(
		depthBufferDSV.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);


	ID3D11RenderTargetView* renderTargets[4] = {};
	renderTargets[0] = sceneColorsRTV.Get();
	renderTargets[1] = sceneNormalsRTV.Get();
	renderTargets[2] = sceneAmbientRTV.Get();
	renderTargets[3] = sceneDepthRTV.Get();

	context->OMSetRenderTargets(4, renderTargets, depthBufferDSV.Get());

	// Draw all of the entities
	for (auto ge : entities)
	{
		// Set the "per frame" data
		// Note that this should literally be set once PER FRAME, before
		// the draw loop, but we're currently setting it per entity since 
		// we are just using whichever shader the current entity has.  
		// Inefficient!!!
		SimplePixelShader* ps = ge->GetMaterial()->GetPS();
		ps->SetData("Lights", (void*)(&lights[0]), sizeof(Light) * lightCount);
		ps->SetInt("LightCount", lightCount);
		ps->SetFloat3("CameraPosition", camera->GetTransform()->GetPosition());
		ps->SetInt("SpecIBLTotalMipLevels", sky->IBLGetMipLevels());
		ps->SetFloat3("AmbientNonPBR", ambientNonPBR);
		ps->CopyBufferData("perFrame");

		ps->SetShaderResourceView("IrradianceIBLMap", sky->IBLGetIrradianceMap());
		ps->SetShaderResourceView("SpecularIBLMap", sky->IBLGetConvolvedSpecularMap());
		ps->SetShaderResourceView("BrdfLookUpMap", sky->IBLGetBRDFLookupTexture());

		// Draw the entity
		ge->Draw(context, camera);
	}

	// Draw the light sources
	DrawPointLights(camera, lightCount, lightVS, lightPS, lightMesh);

	// Draw the sky
	sky->Draw(camera);

	// Lastly, get the final color results to the screen!
	Assets& assets = Assets::GetInstance();
	SimpleVertexShader* vs = assets.GetVertexShader("FullscreenVS.cso");
	vs->SetShader();


	// Set up ssao render pass
	renderTargets[0] = ssaoResultRTV.Get();
	renderTargets[1] = 0;
	renderTargets[2] = 0;
	renderTargets[3] = 0;
	context->OMSetRenderTargets(4, renderTargets, 0);

	SimplePixelShader* ssaoPS = assets.GetPixelShader("SsaoPS.cso");
	ssaoPS->SetShader();

	// Calculate the inverse of the camera matrices
	XMFLOAT4X4 invView, invProj, view = camera->GetView(), proj = camera->GetProjection();
	XMStoreFloat4x4(&invView, XMMatrixInverse(0, XMLoadFloat4x4(&view)));
	XMStoreFloat4x4(&invProj, XMMatrixInverse(0, XMLoadFloat4x4(&proj)));
	ssaoPS->SetMatrix4x4("invViewMatrix", invView);
	ssaoPS->SetMatrix4x4("invProjMatrix", invProj);
	ssaoPS->SetMatrix4x4("viewMatrix", view);
	ssaoPS->SetMatrix4x4("projectionMatrix", proj);
	ssaoPS->SetData("offsets", ssaoOffsets, sizeof(XMFLOAT4) * ARRAYSIZE(ssaoOffsets));
	ssaoPS->SetFloat("ssaoRadius", ssaoRadius);
	ssaoPS->SetInt("ssaoSamples", ssaoSamples);
	ssaoPS->SetFloat2("randomTextureScreenScale", XMFLOAT2(windowWidth / 4.0f, windowHeight / 4.0f));
	ssaoPS->CopyAllBufferData();

	ssaoPS->SetShaderResourceView("Normals", sceneNormalsSRV);
	ssaoPS->SetShaderResourceView("Depths", sceneDepthSRV);
	ssaoPS->SetShaderResourceView("Random", assets.GetTexture("random"));

	context->Draw(3, 0);


	// Set up blur (assuming all other targets are null here)
	renderTargets[0] = ssaoBlurRTV.Get();
	context->OMSetRenderTargets(1, renderTargets, 0);

	SimplePixelShader* ps = assets.GetPixelShader("SsaoBlurPS.cso");
	ps->SetShader();
	ps->SetShaderResourceView("SSAO", ssaoResultSRV);
	ps->SetFloat2("pixelSize", XMFLOAT2(1.0f / windowWidth, 1.0f / windowHeight));
	ps->CopyAllBufferData();
	context->Draw(3, 0);


	// Re-enable back buffer (assuming all other targets are null here)
	renderTargets[0] = backBufferRTV.Get();
	context->OMSetRenderTargets(1, renderTargets, 0);

	ps = assets.GetPixelShader("SsaoCombinePS.cso");
	ps->SetShader();
	ps->SetShaderResourceView("SceneColorsNoAmbient", sceneColorsSRV);
	ps->SetShaderResourceView("Ambient", sceneAmbientSRV);
	ps->SetShaderResourceView("SSAOBlur", ssaoBlurSRV);
	ps->SetFloat2("pixelSize", XMFLOAT2(1.0f / windowWidth, 1.0f / windowHeight));
	ps->CopyAllBufferData();
	context->Draw(3, 0);

	// Draw ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present the back buffer to the user
	//  - Puts the final frame we're drawing into the window so the user can see it
	//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)
	swapChain->Present(0, 0);

	// Due to the usage of a more sophisticated swap chain,
	// the render target must be re-bound after every call to Present()
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());


	// Unbind all SRVs at the end of the frame so they're not still bound for input
	// when we begin the MRTs of the next frame
	ID3D11ShaderResourceView* nullSRVs[16] = {};
	context->PSSetShaderResources(0, 16, nullSRVs);

}

void Renderer::DrawPointLights(Camera* camera, int lightCount, SimpleVertexShader* lightVS, SimplePixelShader* lightPS, Mesh* lightMesh)
{
	// Turn on these shaders
	lightVS->SetShader();
	lightPS->SetShader();

	// Set up vertex shader
	lightVS->SetMatrix4x4("view", camera->GetView());
	lightVS->SetMatrix4x4("projection", camera->GetProjection());

	for (int i = 0; i < lightCount; i++)
	{
		Light light = lights[i];

		// Only drawing points, so skip others
		if (light.Type != LIGHT_TYPE_POINT)
			continue;

		// Calc quick scale based on range
		// (assuming range is between 5 - 10)
		float scale = light.Range / 10.0f;

		// Make the transform for this light
		DirectX::XMMATRIX rotMat = DirectX::XMMatrixIdentity();
		DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(scale, scale, scale);
		DirectX::XMMATRIX transMat = DirectX::XMMatrixTranslation(light.Position.x, light.Position.y, light.Position.z);
		DirectX::XMMATRIX worldMat = scaleMat * rotMat * transMat;

		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4X4 worldInvTrans;
		DirectX::XMStoreFloat4x4(&world, worldMat);
		DirectX::XMStoreFloat4x4(&worldInvTrans, XMMatrixInverse(0, XMMatrixTranspose(worldMat)));

		// Set up the world matrix for this light
		lightVS->SetMatrix4x4("world", world);
		lightVS->SetMatrix4x4("worldInverseTranspose", worldInvTrans);

		// Set up the pixel shader data
		DirectX::XMFLOAT3 finalColor = light.Color;
		finalColor.x *= light.Intensity;
		finalColor.y *= light.Intensity;
		finalColor.z *= light.Intensity;
		lightPS->SetFloat3("Color", finalColor);

		// Copy data
		lightVS->CopyAllBufferData();
		lightPS->CopyAllBufferData();

		// Draw
		lightMesh->SetBuffersAndDraw(context);
	}
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetSceneColorsSRV()
{
	return sceneColorsSRV;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetSceneNormalsSRV()
{
	return sceneNormalsSRV;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetSceneAmbientSRV()
{
	return sceneAmbientSRV;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetSceneDepthSRV()
{
	return sceneDepthSRV;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetSSAO()
{
	return ssaoResultSRV;
}

void Renderer::CreateGenericRenderTarget(unsigned int width, unsigned int height, Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, DXGI_FORMAT colorFormat)
{

	Microsoft::WRL::ComPtr<ID3D11Texture2D> rtTexture;

	D3D11_TEXTURE2D_DESC textDesc = {};

	textDesc.Width = width;
	textDesc.Height = height;
	textDesc.ArraySize = 1;
	textDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textDesc.Format = colorFormat;
	textDesc.MipLevels = 1;
	textDesc.MiscFlags = 0;
	textDesc.SampleDesc.Count = 1;
	device->CreateTexture2D(&textDesc, 0, rtTexture.GetAddressOf());

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};

	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Format = textDesc.Format;
	device->CreateRenderTargetView(rtTexture.Get(), &rtvDesc, rtv.GetAddressOf());

	device->CreateShaderResourceView(rtTexture.Get(), 0, srv.GetAddressOf());


}
