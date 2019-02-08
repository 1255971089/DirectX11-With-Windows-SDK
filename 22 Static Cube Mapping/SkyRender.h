﻿//***************************************************************************************
// SkyRender.h by X_Jun(MKXJun) (C) 2018-2019 All Rights Reserved.
// Licensed under the MIT License.
//
// 天空盒加载与渲染类
// Skybox loader and render classes.
//***************************************************************************************

#ifndef SKYRENDER_H
#define SKYRENDER_H

#include <vector>
#include <string>
#include "Effects.h"
#include "Camera.h"

class SkyRender
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;


	// 需要提供完整的天空盒贴图 或者 已经创建好的天空盒纹理.dds文件
	SkyRender(ComPtr<ID3D11Device> device, 
		ComPtr<ID3D11DeviceContext> deviceContext, 
		const std::wstring& cubemapFilename, 
		float skySphereRadius,		// 天空球半径
		bool generateMips = false);	// 默认不为静态天空盒生成mipmaps


	// 需要提供天空盒的六张正方形贴图
	SkyRender(ComPtr<ID3D11Device> device, 
		ComPtr<ID3D11DeviceContext> deviceContext, 
		const std::vector<std::wstring>& cubemapFilenames, 
		float skySphereRadius,		// 天空球半径
		bool generateMips = false);	// 默认不为静态天空盒生成mipmaps


	ComPtr<ID3D11ShaderResourceView> GetTextureCube();

	void Draw(ComPtr<ID3D11DeviceContext> deviceContext, SkyEffect& skyEffect, const Camera& camera);

private:
	void InitResource(ComPtr<ID3D11Device> device, float skySphereRadius);

private:
	ComPtr<ID3D11Buffer> mVertexBuffer;
	ComPtr<ID3D11Buffer> mIndexBuffer;

	UINT mIndexCount;

	ComPtr<ID3D11ShaderResourceView> mTextureCubeSRV;
};

#endif