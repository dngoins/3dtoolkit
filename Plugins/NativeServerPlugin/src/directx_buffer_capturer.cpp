#include "pch.h"

#include "directx_buffer_capturer.h"
#include "plugindefs.h"

#include "webrtc/modules/video_coding/codecs/h264/h264_encoder_impl.h"

using namespace Microsoft::WRL;
using namespace StreamingToolkit;

DirectXBufferCapturer::DirectXBufferCapturer(ID3D11Device* d3d_device) :
	d3d_device_(d3d_device)
{
	memset(staging_frame_buffers_, 0, MAX_STAGING_BUFFERS * sizeof(ID3D11Texture2D*));
}

void DirectXBufferCapturer::Initialize()
{
	// Gets the device context.
	d3d_device_->GetImmediateContext(&d3d_context_);

#ifdef MULTITHREAD_PROTECTION
	// Enables multithread protection.
	ID3D11Multithread* multithread;
	d3d_device_->QueryInterface(IID_PPV_ARGS(&multithread));
	multithread->SetMultithreadProtected(true);
	multithread->Release();
#endif // MULTITHREAD_PROTECTION

	// Initializes NVIDIA encoder.
	webrtc::H264EncoderImpl::SetDevice(d3d_device_.Get());
	webrtc::H264EncoderImpl::SetContext(d3d_context_.Get());
}

void DirectXBufferCapturer::SendFrame(ID3D11Texture2D* frame_buffer)
{
	if (!running_ || !sinks_.size())
	{
		return;
	}

	// Updates staging frame buffer.
	ID3D11Texture2D* staging_buffer = UpdateStagingBuffer(frame_buffer);

	// Returns if there is no available staging buffer.
	if (!staging_buffer)
	{
		return;
	}

	for each (auto sink in sinks_)
	{
		// Creates webrtc frame buffer.
		D3D11_TEXTURE2D_DESC desc;
		frame_buffer->GetDesc(&desc);
		rtc::scoped_refptr<webrtc::I420Buffer> buffer =
			webrtc::I420Buffer::Create(desc.Width, desc.Height);

		// For software encoder, converting to supported video format.
		if (use_software_encoder_)
		{
			D3D11_MAPPED_SUBRESOURCE mapped;
			if (SUCCEEDED(d3d_context_.Get()->Map(
				staging_buffer, 0, D3D11_MAP_READ, 0, &mapped)))
			{
				libyuv::ABGRToI420(
					(uint8_t*)mapped.pData,
					desc.Width * 4,
					buffer.get()->MutableDataY(),
					buffer.get()->StrideY(),
					buffer.get()->MutableDataU(),
					buffer.get()->StrideU(),
					buffer.get()->MutableDataV(),
					buffer.get()->StrideV(),
					desc.Width,
					desc.Height);

				d3d_context_->Unmap(staging_buffer, 0);
			}
		}

		// Updates time stamp.
		auto time_stamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();

		// Creates video frame buffer.
		auto frame = webrtc::VideoFrame(buffer, kVideoRotation_0, time_stamp);
		frame.set_ntp_time_ms(clock_->CurrentNtpInMilliseconds());
		frame.set_rotation(VideoRotation::kVideoRotation_0);

		// For hardware encoder, setting the video frame texture.
		if (!use_software_encoder_)
		{
			staging_buffer->AddRef();
			frame.SetID3D11Texture2D(staging_buffer);
		}

		// Sending video frame.
		sink->OnFrame(frame);
	}
}

ID3D11Texture2D* DirectXBufferCapturer::UpdateStagingBuffer(ID3D11Texture2D* frame_buffer)
{
	// Finds any available staging frame buffer.
	int buffer_id = 0;
	do
	{
		if (!staging_frame_buffers_[buffer_id])
		{
			break;
		}
		else
		{
			staging_frame_buffers_[buffer_id]->AddRef();
			int ref_count = staging_frame_buffers_[buffer_id]->Release();
			if (ref_count == 1)
			{
				break;
			}
		}
	}
	while (++buffer_id < MAX_STAGING_BUFFERS);

	if (buffer_id != MAX_STAGING_BUFFERS)
	{
		// Gets frame buffer description.
		D3D11_TEXTURE2D_DESC desc;
		frame_buffer->GetDesc(&desc);

		// Lazily creates staging frame buffer.
		if (!staging_frame_buffers_[buffer_id])
		{
			D3D11_TEXTURE2D_DESC staging_frame_buffer_desc = { 0 };
			staging_frame_buffer_desc.ArraySize = 1;
			staging_frame_buffer_desc.Format = desc.Format;
			staging_frame_buffer_desc.Width = desc.Width;
			staging_frame_buffer_desc.Height = desc.Height;
			staging_frame_buffer_desc.MipLevels = 1;
			staging_frame_buffer_desc.SampleDesc.Count = 1;
			staging_frame_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			staging_frame_buffer_desc.Usage = D3D11_USAGE_STAGING;
			d3d_device_->CreateTexture2D(
				&staging_frame_buffer_desc, nullptr, &staging_frame_buffers_[buffer_id]);
		}
		else // Resizes if needed.
		{
			D3D11_TEXTURE2D_DESC staging_frame_buffer_desc;
			staging_frame_buffers_[buffer_id]->GetDesc(&staging_frame_buffer_desc);
			if (staging_frame_buffer_desc.Width != desc.Width ||
				staging_frame_buffer_desc.Height != desc.Height)
			{
				staging_frame_buffer_desc.Width = desc.Width;
				staging_frame_buffer_desc.Height = desc.Height;
				staging_frame_buffers_[buffer_id]->Release();
				d3d_device_->CreateTexture2D(&staging_frame_buffer_desc, nullptr,
					&staging_frame_buffers_[buffer_id]);
			}
		}

		// Copies the frame buffer to the staging one.
		d3d_context_->CopyResource(staging_frame_buffers_[buffer_id], frame_buffer);

		return staging_frame_buffers_[buffer_id];
	}

	return nullptr;
}
