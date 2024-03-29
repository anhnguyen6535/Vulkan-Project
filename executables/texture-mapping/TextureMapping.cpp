#include "BedrockLog.hpp"
#include "BedrockPath.hpp"
#include "LogicalDevice.hpp"
#include "pipeline/LinePipeline.hpp"
#include "render_pass/DisplayRenderPass.hpp"
#include "render_resource/DepthRenderResource.hpp"
#include "render_resource/MSAA_RenderResource.hpp"
#include "render_resource/SwapChainRenderResource.hpp"
#include "BedrockMath.hpp"
#include "BufferTracker.hpp"
#include "camera/PerspectiveCamera.hpp"
#include "utils/LineRenderer.hpp"
#include "UI.hpp"
#include "pipeline/FlatShadingPipeline.hpp"
#include "ImportTexture.hpp"
#include "camera/ObserverCamera.hpp"
#include "utils/PointRenderer.hpp"

#include <glm/glm.hpp>
#include <filesystem>

#include "ImportObj.hpp"
#include <camera/ArcballCamera.hpp>
#include <iostream>

using namespace MFA;

//--------- IMGUI -------------//
bool renderWireframe = false;
bool colorEnabled = false;
bool aoEnabled = false;
bool perlinEnabled = false;
int m = 24;
bool extrinsicEnabled = false;
bool rotationEnabled = false;
int angleX = 0;
int angleY = 0;
int angleZ = 0;
float scale = 1;
bool reset = false;

//-----other variables ------//
float oldScale = 1;
glm::vec3 camPosition;

void UI_Loop()
{
	auto ui = UI::Instance;
	ui->BeginWindow("Settings");
	ImGui::Checkbox("Wireframe enable", &renderWireframe);
	ImGui::Checkbox("Base color enable", &colorEnabled);
	ImGui::Checkbox("Ao enable", &aoEnabled);
	ImGui::Checkbox("Perlin enable", &perlinEnabled);
	if (perlinEnabled) ImGui::InputInt("m", &m);
	ImGui::InputFloat("scale", &scale, 0.5f);
	ImGui::Checkbox("Extrinsic", &extrinsicEnabled);
	ImGui::Checkbox("Rotate", &rotationEnabled);
	ImGui::InputInt("Rotate x", &angleX);
	ImGui::InputInt("Rotate y", &angleY);
	ImGui::InputInt("Rotate z", &angleZ);
	ImGui::Checkbox("Reset", &reset);

	(m > 0) ? m : -m;	// handle m input
	ui->EndWindow();

};

class FlagMesh
{
public:

	using Triangle = std::tuple<int, int, int>;

	explicit FlagMesh(
		std::shared_ptr<FlatShadingPipeline> pipeline,
		std::shared_ptr<FlatShadingPipeline> wireframePipeline,
		glm::vec4 const& color,
		std::vector<glm::vec3> const& vertices,
		std::vector<Triangle> const& triangles,         // Indices and intensity
		std::vector<glm::vec2> const & uvs,
		std::vector<glm::vec3> const & normals
	)
		: _pipeline(std::move(pipeline))
		, _wireframePipeline(std::move(wireframePipeline))
		, _color(color)
	{

		CreateIndexBuffer(triangles);

		CreateVertexBuffer(vertices, uvs, normals);
		
		CreateGpuTexture();

		CreateMaterial();

		CreateDescriptorSet();
	}

	// Update Object Transformation
	void objectUpdateConstants() {
		glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		// Combine rotations 
		glm::mat4 combinedRotation;
		

		if (rotationEnabled) {
			rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(float(angleX)), glm::vec3(1.0f, 0.0f, 0.0f));
			rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(float(angleY)), glm::vec3(0.0f, 1.0f, 0.0f));
			rotationZ = glm::rotate(glm::mat4(1.0f), glm::radians(float(angleZ)), glm::vec3(0.0f, 0.0f, 1.0f));

			if (!extrinsicEnabled) {
				combinedRotation = rotationZ * rotationY * rotationX;
				_model = _model * combinedRotation;
			}
			else {
				combinedRotation = rotationX * rotationY * rotationZ;
				_model = combinedRotation * _model;
			}
		}
		
		if (scale != oldScale) {
			oldScale = scale;
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3{ scale, scale, scale });
			std::cout << "SCALE: " << scale << std::endl;
			_model = scaleMatrix * _model;
			
		}	

		// reset all gui
		if (reset) {
			colorEnabled = false;
			aoEnabled = false;
			perlinEnabled = false;
			rotationEnabled = false;
			extrinsicEnabled = false;
			renderWireframe = false;
			scale = 1.0f;
			m = 24.0f;
			_model = glm::mat4(1.0f);
			oldScale = 1;
			reset = false;
		}
	}

	void Render(RT::CommandRecordState& recordState)
	{
		if (renderWireframe == true)
		{
			_wireframePipeline->BindPipeline(recordState);
		}
		else
		{
			_pipeline->BindPipeline(recordState);
		}
		
		objectUpdateConstants();		// handle object transformation

		RB::AutoBindDescriptorSet(
			recordState, 
			RB::UpdateFrequency::PerGeometry, 
			_perGeometryDescriptorSet.descriptorSets[0]
		);

		_pipeline->SetPushConstants(
			recordState,
			FlatShadingPipeline::PushConstants{
				.model = _model,
				.hasBaseColor = colorEnabled ? 1 : 0,
				.hasAo = aoEnabled ? 1 : 0,
				.hasPerlin = perlinEnabled ? 1 : 0,
				.m = m,
				.camPosition = camPosition
			}
		);
		
		RB::BindIndexBuffer(
			recordState,
			*_indexBuffer,
			0,
			VK_INDEX_TYPE_UINT16
		);

		RB::BindVertexBuffer(
			recordState,
			*_vertexBuffer,
			0,
			0
		);

		RB::DrawIndexed(
			recordState,
			_indexCount
		);
	}

private:

	void CreateIndexBuffer(std::vector<Triangle> const& triangles)
	{
		std::vector<uint16_t> indices{};
		for (auto& [idx1, idx2, idx3] : triangles)
		{
			indices.emplace_back(idx1);
			indices.emplace_back(idx2);
			indices.emplace_back(idx3);
		}
		Alias const indicesAlias{ indices.data(), indices.size() };

		auto const* device = LogicalDevice::Instance;

		auto const commandBuffer = RB::BeginSingleTimeCommand(
			device->GetVkDevice(),
			device->GetGraphicCommandPool()
		);

		auto const indexStageBuffer = RB::CreateStageBuffer(
			device->GetVkDevice(),
			device->GetPhysicalDevice(),
			indicesAlias.Len(),
			1
		);

		_indexBuffer = RB::CreateIndexBuffer(
			device->GetVkDevice(),
			device->GetPhysicalDevice(),
			commandBuffer,
			*indexStageBuffer->buffers[0],
			indicesAlias
		);

		RB::EndAndSubmitSingleTimeCommand(
			device->GetVkDevice(),
			device->GetGraphicCommandPool(),
			device->GetGraphicQueue(),
			commandBuffer
		);

		_indexCount = static_cast<int>(indices.size());
	}

	void CreateVertexBuffer(std::vector<glm::vec3> const & positions, std::vector<glm::vec2> const & uvs, std::vector<glm::vec3> const & normals)
	{
		auto device = LogicalDevice::Instance->GetVkDevice();
		auto physicalDevice = LogicalDevice::Instance->GetPhysicalDevice();
		auto graphicCommandPool = LogicalDevice::Instance->GetGraphicCommandPool();
		auto graphicQueue = LogicalDevice::Instance->GetGraphicQueue();

		auto const commandBuffer = RB::BeginSingleTimeCommand(
			device,
			graphicCommandPool
		);

		// Vertices and normals should be updated every frame
		std::vector<FlatShadingPipeline::Vertex> vertices(positions.size());
		for (int i = 0; i < static_cast<int>(positions.size()); ++i)
		{
			vertices[i].position = positions[i];
			vertices[i].baseColorUV = uvs[i];
			vertices[i].normal = normals[i];
		}

		Alias const alias{ vertices.data(), vertices.size() };

		auto const stageBuffer = RB::CreateStageBuffer(
			device,
			physicalDevice,
			alias.Len(),
			1
		);

		_vertexBuffer = RB::CreateVertexBuffer(
			device,
			physicalDevice,
			commandBuffer,
			*stageBuffer->buffers[0],
			alias
		);

		RB::EndAndSubmitSingleTimeCommand(
			device,
			graphicCommandPool,
			graphicQueue,
			commandBuffer
		);
	}

	// Generate a grid [0,15]^2 contains random values
	std::vector<std::vector<float>> GenerateGrid(int size) {
		std::vector<std::vector<float>> grid(size, std::vector<float>(size));

		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				grid[i][j] = Math::Random(0, 255);
			}
		}
		return grid;
	}

	void CreateGpuTexture()
	{
		//auto const path = Path::Instance->Get("models/Flag_of_Canada.png");
		auto const path = Path::Instance->Get("models/chess_bishop/bishop.colour.white.png");
		MFA_ASSERT(std::filesystem::exists(path));
		auto const cpuTexture = Importer::UncompressedImage(path);

		// Changed: load ao image
		auto const aoPath = Path::Instance->Get("models/chess_bishop/bishop.ao.png");
		MFA_ASSERT(std::filesystem::exists(aoPath));
		auto const aoCpuTexture = Importer::UncompressedImage(aoPath);

		int width = 16;
		int height = 16;
		int components = 4;
		std::vector<std::vector<float>> grid = GenerateGrid(width);
		auto const blob = Memory::AllocSize(width * height * components);
		auto* ptr = blob->As<uint8_t>();
		for (int i = 0; i < width * height * components; ++i)
		{
			ptr[i] = Math::Random(0, 255);
		}

		for (int y = 0; y < 16; y++) {
			for (int x = 0; x < 16; x++) {
				ptr[y * 4 * 16 + x * 4] = grid[y][x]; 	  //R
				ptr[y * 4 * 16 + x * 4 + 1] = grid[y][x]; //G
				ptr[y * 4 * 16 + x * 4 + 2] = grid[y][x]; //B
				ptr[y * 4 * 16 + x * 4 + 3] = 255;
			}
		}

		auto proCpuTexture = Importer::InMemoryTexture(
			*blob,
			width,
			height,
			Asset::Texture::Format::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR,
			components
		);

		auto const* device = LogicalDevice::Instance;
		auto const commandBuffer = RB::BeginSingleTimeCommand(
			device->GetVkDevice(),
			device->GetGraphicCommandPool()
		);

		auto [texture, stagingBuffer] = RB::CreateTexture(
			*cpuTexture,
			LogicalDevice::Instance->GetVkDevice(),
			LogicalDevice::Instance->GetPhysicalDevice(),
			commandBuffer
		);

		// Changed: create ao texture 
		auto [aoTexture, aoStagingBuffer] = RB::CreateTexture(
			*aoCpuTexture,
			LogicalDevice::Instance->GetVkDevice(),
			LogicalDevice::Instance->GetPhysicalDevice(),
			commandBuffer
		);

		// Changed: create procedural texture from random grid 
		auto [proTexture, proStagingBuffer] = RB::CreateTexture(
			*proCpuTexture,
			LogicalDevice::Instance->GetVkDevice(),
			LogicalDevice::Instance->GetPhysicalDevice(),
			commandBuffer
		);

		_texture = texture;
		_aoTexture = aoTexture; // Changed
		_proTexture = proTexture;

		RB::EndAndSubmitSingleTimeCommand(
			device->GetVkDevice(),
			device->GetGraphicCommandPool(),
			device->GetGraphicQueue(),
			commandBuffer
		);
	}


	void CreateMaterial()
	{
		FlatShadingPipeline::Material material{
			.color = _color,
			.hasBaseColorTexture = 1
		};

		_material = RB::CreateLocalUniformBuffer(
			LogicalDevice::Instance->GetVkDevice(),
			LogicalDevice::Instance->GetPhysicalDevice(),
			sizeof(material),
			1
		);

		auto stageBuffer = RB::CreateStageBuffer(
			LogicalDevice::Instance->GetVkDevice(), 
			LogicalDevice::Instance->GetPhysicalDevice(), 
			sizeof(material), 
			1
		);

		auto commandBuffer = RB::BeginSingleTimeCommand(
			LogicalDevice::Instance->GetVkDevice(), 
			LogicalDevice::Instance->GetGraphicCommandPool()
		);

		RB::UpdateHostVisibleBuffer(
			LogicalDevice::Instance->GetVkDevice(), 
			*stageBuffer->buffers[0], 
			Alias(material)
		);

		RB::UpdateLocalBuffer(
			commandBuffer, 
			*_material->buffers[0], 
			*stageBuffer->buffers[0]
		);

		RB::EndAndSubmitSingleTimeCommand(
			LogicalDevice::Instance->GetVkDevice(),
			LogicalDevice::Instance->GetGraphicCommandPool(),
			LogicalDevice::Instance->GetGraphicQueue(),
			commandBuffer
		);
	}

	// Changed
	void CreateDescriptorSet()
	{
		_perGeometryDescriptorSet = _pipeline->CreatePerGeometryDescriptorSetGroup(*_material->buffers[0], *_texture, *_aoTexture, *_proTexture);		// pass aoTexture to descriptorSet
	}

	std::shared_ptr<PointRenderer> _pointRenderer{};
	std::shared_ptr<LineRenderer> _lineRenderer{};
	std::shared_ptr<FlatShadingPipeline> _pipeline{};
	std::shared_ptr<FlatShadingPipeline> _wireframePipeline{};
	glm::vec4 _color{};

	glm::mat4 _model = glm::identity<glm::mat4>();
	
	int _indexCount{};
	std::shared_ptr<RT::BufferAndMemory> _indexBuffer{};

	std::shared_ptr<RT::BufferAndMemory> _vertexBuffer{};

	std::shared_ptr<RT::GpuTexture> _texture{};
	std::shared_ptr<RT::GpuTexture> _aoTexture{};
	std::shared_ptr<RT::GpuTexture> _proTexture{};
	std::shared_ptr<RT::BufferGroup> _material{};
	
	RT::DescriptorSetGroup _perGeometryDescriptorSet;
};

void CalculateBoundingBox(Importer::ObjModel& objModel) {
	glm::vec3 minBound = objModel.vertices[0].position;
	glm::vec3 maxBound = objModel.vertices[0].position;

	for (int i = 0; i < objModel.vertices.size(); i++) {
		minBound = glm::min(minBound, objModel.vertices[i].position);
		maxBound = glm::max(maxBound, objModel.vertices[i].position);
	}

	glm::vec3 offset = (minBound + maxBound) / 2.0f;

	//Calculate size of the bounding box
	glm::vec3 boundingBoxSize = maxBound - minBound;

	// Determine the maximum dimension of the bounding box
	float maxDimension = glm::compMax(boundingBoxSize);

	//Calculate the scaling factor for uniform scaling
	float scaleFactor = 2.0f / maxDimension;

	for (int i = 0; i < objModel.vertices.size(); i++) {
		objModel.vertices[i].position.x -= offset.x;
		objModel.vertices[i].position.y -= offset.y;
		objModel.vertices[i].position.z -= offset.z;

		objModel.vertices[i].position *= scaleFactor;
	}
}


std::shared_ptr<FlagMesh> GenerateFlag(
	std::shared_ptr<FlatShadingPipeline> const& pipeline,
	std::shared_ptr<FlatShadingPipeline> const& wireframePipeline
)
{
	std::vector<glm::vec3> vertices{};
	std::vector<std::tuple<int, int, int>> triangles{};   // Used to construct the growth tensors
	std::vector<glm::vec2> uvs{};
	std::vector<glm::vec3> normals{};

	Importer::ObjModel objModel{};
	bool success = Importer::LoadObj(Path::Instance->Get("models/chess_bishop/bishop.obj"), objModel);
	MFA_ASSERT(success == true);

	for (int i = 0; i < objModel.indices.size(); i += 3)
	{
		triangles.emplace_back(std::tuple{ objModel.indices[i], objModel.indices[i + 1], objModel.indices[i + 2] });
	}

	CalculateBoundingBox(objModel);		// calculate bounding box to center obj 

	for (auto & vertex : objModel.vertices)
	{
		vertices.emplace_back(vertex.position);
		uvs.emplace_back(vertex.uv);
		normals.emplace_back(vertex.normal);
	}

	/*int xCount = 2;
	int yCount = 2;

	glm::vec3 start{ -1.5f, -1.0f, 3.0f };
	glm::vec3 end{ 1.5f, 1.0f, 3.0f };

	for (int j = 0; j < xCount; ++j)
	{
		float xT = static_cast<float>(j) / static_cast<float>(xCount - 1);
		for (int k = 0; k < yCount; ++k)
		{
			float yT = static_cast<float>(k) / static_cast<float>(yCount - 1);
			uvs.emplace_back(xT, 1.0f - yT);
			vertices.emplace_back(glm::vec3{
				glm::mix(start.x, end.x, xT),
				glm::mix(start.y, end.y, yT),
				end.z,
			});
		}
	}

	for (int j = 0; j < xCount - 1; ++j)
	{
		int currX = j * yCount;
		int nextX = currX + yCount;

		for (int i = 0; i < yCount - 1; ++i)
		{
			triangles.emplace_back(std::tuple{ currX + i, currX + i + 1, nextX + i + 1 });
			triangles.emplace_back(std::tuple{ currX + i,  nextX + i + 1, nextX + i});
		}
	}*/

	return std::make_shared<FlagMesh>(
		pipeline,
		wireframePipeline,
		glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f },
		vertices,
		triangles,
		uvs,
		normals
	);

}

int main()
{
	MFA_LOG_DEBUG("Loading...");

	auto path = Path::Instantiate();

	LogicalDevice::InitParams params
	{
		.windowWidth = 1000,
		.windowHeight = 1000,
		.resizable = true,
		.fullScreen = false,
		.applicationName = "Flag app"
	};

	auto const device = LogicalDevice::Instantiate(params);
	assert(device->IsValid() == true);

	{
		auto swapChainResource = std::make_shared<SwapChainRenderResource>();
		auto depthResource = std::make_shared<DepthRenderResource>();
		auto msaaResource = std::make_shared<MSSAA_RenderResource>();
		auto displayRenderPass = std::make_shared<DisplayRenderPass>(
			swapChainResource,
			depthResource,
			msaaResource
			);

		auto const ui = std::make_shared<UI>(displayRenderPass);

		auto cameraBuffer = RB::CreateHostVisibleUniformBuffer(
			device->GetVkDevice(),
			device->GetPhysicalDevice(),
			sizeof(glm::mat4),
			device->GetMaxFramePerFlight()
		);

		ArcballCamera camera{};
		camera.Setposition({ 0.0f, 0.0f, 5.0f });

		auto ComputeViewProjectionMat4 = [&camera]()->glm::mat4
		{
			return camera.GetViewProjection();
		};

		auto cameraBufferTracker = std::make_shared<HostVisibleBufferTracker<glm::mat4>>(cameraBuffer, ComputeViewProjectionMat4());

		device->ResizeEventSignal2.Register([&cameraBufferTracker, &ComputeViewProjectionMat4]()->void
			{
				cameraBufferTracker->SetData(ComputeViewProjectionMat4());
			}
		);

		auto defaultSampler = RB::CreateSampler(LogicalDevice::Instance->GetVkDevice(), {});
		
		auto const pipeline = std::make_shared<FlatShadingPipeline>(
			displayRenderPass, 
			cameraBuffer, 
			defaultSampler,
			FlatShadingPipeline::Params {
				.maxSets = 100,
				.cullModeFlags = VK_CULL_MODE_NONE
			}
		);
		auto const wireframePipeline = std::make_shared<FlatShadingPipeline>(
			displayRenderPass,
			cameraBuffer,
			defaultSampler,
			FlatShadingPipeline::Params {
				.maxSets = 100,
				.cullModeFlags = VK_CULL_MODE_NONE,
				.polygonMode = VK_POLYGON_MODE_LINE
			}
		);
		
		auto flagMesh = GenerateFlag(pipeline, wireframePipeline);

		ui->UpdateSignal.Register(UI_Loop);

		static const uint32_t FixedDeltaTimeMs = 1000 / 120;
		static const float PBD_FixedDeltaTimeSec = static_cast<float>(FixedDeltaTimeMs) / 1000.0f;
		SDL_GL_SetSwapInterval(0);
		SDL_Event e;
		uint32_t deltaTimeMs = FixedDeltaTimeMs;
		float deltaTimeSec = PBD_FixedDeltaTimeSec;
		uint32_t startTime = SDL_GetTicks();
		
		bool shouldQuit = false;

		while (shouldQuit == false)
		{
			//Handle events
			while (SDL_PollEvent(&e) != 0)
			{
				//User requests quit
				if (e.type == SDL_QUIT)
				{
					shouldQuit = true;
				}
			}

			device->Update();

			camera.Update(deltaTimeSec);
			if (camera.IsDirty())
			{
				cameraBufferTracker->SetData(camera.GetViewProjection());
			}

			ui->Update();

			auto recordState = device->AcquireRecordState(swapChainResource->GetSwapChainImages().swapChain);
			if (recordState.isValid == true)
			{
				device->BeginCommandBuffer(
					recordState,
					RT::CommandBufferType::Compute
				);
				device->EndCommandBuffer(recordState);

				device->BeginCommandBuffer(
					recordState,
					RT::CommandBufferType::Graphic
				);

				cameraBufferTracker->Update(recordState);

				displayRenderPass->Begin(recordState);

				camPosition = camera.Getposition();
				flagMesh->Render(recordState);
				ui->Render(recordState, deltaTimeSec);

				displayRenderPass->End(recordState);
				device->EndCommandBuffer(recordState);

				device->SubmitQueues(recordState);
				device->Present(recordState, swapChainResource->GetSwapChainImages().swapChain);
			}

			deltaTimeMs = SDL_GetTicks() - startTime;
			if (FixedDeltaTimeMs > deltaTimeMs)
			{
				SDL_Delay(FixedDeltaTimeMs - deltaTimeMs);
			}
			deltaTimeMs = SDL_GetTicks() - startTime;
			deltaTimeSec = static_cast<float>(deltaTimeMs) / 1000.0f;
			startTime = SDL_GetTicks();
		}

		device->DeviceWaitIdle();
	
	}

	return 0;
}