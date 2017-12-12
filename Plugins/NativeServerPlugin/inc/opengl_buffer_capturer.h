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

#include <wrl\client.h>
#include <wrl\wrappers\corewrappers.h>
#include <freeglut.h>
#include <glut.h>
#include "glext.h"

#include "buffer_capturer.h"

namespace StreamingToolkit
{
	// Provides DirectX implementation of the BufferCapturer class.
	class OpenGLBufferCapturer : public BufferCapturer
	{
	public:
		explicit OpenGLBufferCapturer(UINT width, UINT height);

		virtual ~OpenGLBufferCapturer() {}

		void Initialize() override;

		void SendFrame(webrtc::VideoFrame video_frame) override;

		void SendFrame();

	private:
		UINT buffer_width;
		UINT buffer_height;
	};
}
