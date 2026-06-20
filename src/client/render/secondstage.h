// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#pragma once
#include "pipeline.h"

/**
 *  Step to apply post-processing filter to the rendered image
 */
class PostProcessingStep : public RenderStep
{
public:
	/**
	 * Construct a new PostProcessingStep object
	 *
	 * @param shader_id ID of the shader in IShaderSource
	 * @param texture_map Map of textures to be chosen from the render source
	 */
	PostProcessingStep(u32 shader_id, const std::vector<u8> &texture_map);


	void setRenderSource(RenderSource *source) override;
	void setRenderTarget(RenderTarget *target) override;
	void reset(PipelineContext &context) override;
	void run(PipelineContext &context) override;

	/**
	 * Configure bilinear filtering for a specific texture layer
	 *
	 * @param index Index of the texture layer
	 * @param value true to enable the bilinear filter, false to disable
	 */
	void setBilinearFilter(u8 index, bool value);
private:
	u32 shader_id;
	std::vector<u8> texture_map;
	RenderSource *source { nullptr };
	RenderTarget *target { nullptr };
	video::SMaterial material;

	void configureMaterial();
};


class ResolveMSAAStep : public TrivialRenderStep
{
public:
	ResolveMSAAStep(TextureBufferOutput *_msaa_fbo, TextureBufferOutput *_target_fbo) :
			msaa_fbo(_msaa_fbo), target_fbo(_target_fbo) {};
	void run(PipelineContext &context) override;

private:
	TextureBufferOutput *msaa_fbo;
	TextureBufferOutput *target_fbo;
};


class CustomPostProcessingStep : public RenderStep
{
public:
	CustomPostProcessingStep(Client *client, RenderPipeline *pipeline);

	void setRenderSource(RenderSource *source) override;
	void setRenderTarget(RenderTarget *target) override;
	void reset(PipelineContext &context) override;
	void run(PipelineContext &context) override;

private:
	Client *m_client;
	RenderPipeline *m_pipeline;
	RenderSource *m_source { nullptr };
	RenderTarget *m_target { nullptr };
	video::SMaterial m_material;

	TextureBuffer *m_pingpong_buffer { nullptr };
	TextureBufferOutput *m_pingpong_outputs[2] { nullptr, nullptr };

	void configureMaterial();
	void ensurePingPongBuffer(PipelineContext &context);
};

RenderStep *addPostProcessing(RenderPipeline *pipeline, RenderStep *previousStep, v2f scale, Client *client);
