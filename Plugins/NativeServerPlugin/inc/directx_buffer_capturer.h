/*
 *  Copyright (c) 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#pragma once

#include <d3d11_4.h>
#include <wrl\client.h>
#include <wrl\wrappers\corewrappers.h>

#include "buffer_capturer.h"

#define MAX_STAGING_BUFFERS		10

namespace StreamingToolkit
{
	// Provides DirectX implementation of the BufferCapturer class.
	class DirectXBufferCapturer : public BufferCapturer
	{
	public:
		explicit DirectXBufferCapturer(ID3D11Device* d3d_device);

		virtual ~DirectXBufferCapturer() {}

		void Initialize() override;

		void SendFrame(ID3D11Texture2D* frame_buffer);

	private:
		ID3D11Texture2D* UpdateStagingBuffer(ID3D11Texture2D* frame_buffer);

		Microsoft::WRL::ComPtr<ID3D11Device> d3d_device_;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_context_;
		ID3D11Texture2D* staging_frame_buffers_[MAX_STAGING_BUFFERS];
	};
}
