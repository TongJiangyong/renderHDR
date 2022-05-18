#include "d3d11va_renderer.h"

D3D11VARenderer::D3D11VARenderer()
{

}

D3D11VARenderer::~D3D11VARenderer()
{

}

void D3D11VARenderer::RenderFrame(AVFrame* frame)
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (!d3d11_context_) {
		return;
	}

	ID3D11Texture2D* texture = (ID3D11Texture2D*)frame->data[0];
	int index = (int)frame->data[1];
	AVPixelFormat test = (AVPixelFormat)frame->format;
	D3D11_TEXTURE2D_DESC desc;
	if (AV_PIX_FMT_D3D11 == frame->format) {
		texture->GetDesc(&desc);
		if (pixel_format_ != DX::PIXEL_FORMAT_NV12 ||
			width_ != desc.Width || height_ != desc.Height) {
			if (!CreateTexture(desc.Width, desc.Height, DX::PIXEL_FORMAT_NV12)) {
				return;
			}
		}

	}
	else {
		//TODO fix this
		if (input_textures_[DX::PIXEL_PLANE_Y] == NULL) {
			if (!CreateTexture(frame->width, frame->height, DX::PIXEL_FORMAT_I420)) {
				return;
			}
		}
	}

	Begin();

	if (AV_PIX_FMT_D3D11 == frame->format) {
		ID3D11Texture2D* nv12_texture = input_textures_[DX::PIXEL_PLANE_NV12]->GetTexture();
		ID3D11ShaderResourceView* nv12_texture_y_srv = input_textures_[DX::PIXEL_PLANE_NV12]->GetNV12YShaderResourceView();
		ID3D11ShaderResourceView* nv12_texture_uv_srv = input_textures_[DX::PIXEL_PLANE_NV12]->GetNV12UVShaderResourceView();

		d3d11_context_->CopySubresourceRegion(
			nv12_texture,
			0,
			0,
			0,
			0,
			(ID3D11Resource*)texture,
			index,
			NULL);
		DX::D3D11RenderTexture* render_target = render_targets_[DX::PIXEL_SHADER_NV12_BT601].get();
		if (render_target) {
			render_target->Begin();
			render_target->PSSetTexture(0, nv12_texture_y_srv);
			render_target->PSSetTexture(1, nv12_texture_uv_srv);
			render_target->PSSetSamplers(0, linear_sampler_);
			render_target->PSSetSamplers(1, point_sampler_);
			render_target->Draw();
			render_target->End();
			render_target->PSSetTexture(0, NULL);
			render_target->PSSetTexture(1, NULL);
			output_texture_ = render_target;
		}
	}
	else {

		// TODO fix this
		ID3D11Texture2D* Y_texture = input_textures_[DX::PIXEL_PLANE_Y]->GetTexture();

		d3d11_context_->CopyResource(input_textures_[DX::PIXEL_PLANE_Y]->access_texture_, Y_texture);
		D3D11_MAPPED_SUBRESOURCE resource;
		UINT subresource = D3D11CalcSubresource(0, 0, 0);
		d3d11_context_->Map(input_textures_[DX::PIXEL_PLANE_Y]->access_texture_, subresource, D3D11_MAP_WRITE, 0, &resource);
		BYTE* dptr = reinterpret_cast<BYTE*>(resource.pData);
		memcpy(dptr, frame->data[0], frame->width * frame->height);
		d3d11_context_->Unmap(input_textures_[DX::PIXEL_PLANE_Y]->access_texture_, subresource);
		d3d11_context_->CopyResource(Y_texture, input_textures_[DX::PIXEL_PLANE_Y]->access_texture_);
		// ID3D11Texture2D* U_texture = input_textures_[DX::PIXEL_PLANE_U]->GetTexture();
		// ID3D11Texture2D* V_texture = input_textures_[DX::PIXEL_PLANE_V]->GetTexture();
		// d3d11_context_->UpdateSubresource(Y_texture, 0, NULL, frame->data[0], frame->linesize[0], 0);
		// d3d11_context_->UpdateSubresource(U_texture, 0, NULL, frame->data[1], frame->linesize[1], 0);
		// d3d11_context_->UpdateSubresource(V_texture, 0, NULL, frame->data[2], frame->linesize[2], 0);

		ID3D11ShaderResourceView* i420_texture_y_srv = input_textures_[DX::PIXEL_PLANE_Y]->GetShaderResourceView();
		ID3D11ShaderResourceView* i420_texture_u_srv = input_textures_[DX::PIXEL_PLANE_U]->GetShaderResourceView();
		ID3D11ShaderResourceView* i420_texture_v_srv = input_textures_[DX::PIXEL_PLANE_V]->GetShaderResourceView();

		DX::D3D11RenderTexture* render_target = render_targets_[DX::PIXEL_SHADER_YUV_BT709].get();
		if (render_target) {
			render_target->Begin();
			render_target->PSSetTexture(0, i420_texture_y_srv);
			render_target->PSSetTexture(1, i420_texture_u_srv);
			render_target->PSSetTexture(2, i420_texture_v_srv);
			render_target->PSSetSamplers(0, linear_sampler_);
			render_target->PSSetSamplers(1, point_sampler_);
			render_target->Draw();
			render_target->End();
			render_target->PSSetTexture(0, NULL);
			render_target->PSSetTexture(1, NULL);
			render_target->PSSetTexture(2, NULL);
			output_texture_ = render_target;
		}
	}


	// d3d11_context_->UpdateSubresource(nv12_texture_uv_srv, 0, NULL, frame->data[0], frame->linesize[0], 0);
	//d3d11_context_->UpdateSubresource(nv12_texture, 0, NULL, frame->data[0], frame->linesize[0], 0);



	Process();
	End();
}
