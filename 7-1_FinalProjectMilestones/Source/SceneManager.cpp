///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/
void SceneManager::LoadSceneTextures()
{
	CreateGLTexture("../../Utilities/textures/rusticwood.jpg", "wood");
	CreateGLTexture("../../Utilities/textures/cheese_top.jpg", "leaves");
	CreateGLTexture("../../Utilities/textures/knife_handle.jpg", "ground");

	BindGLTextures();
}

void SceneManager::DefineObjectMaterials()
{
	// matte material for floor and tree
	OBJECT_MATERIAL matteMaterial;
	matteMaterial.tag = "matte";
	matteMaterial.ambientStrength = 0.35f;
	matteMaterial.ambientColor = glm::vec3(0.55f, 0.55f, 0.55f);
	matteMaterial.diffuseColor = glm::vec3(0.85f, 0.85f, 0.85f);
	matteMaterial.specularColor = glm::vec3(0.20f, 0.20f, 0.20f);
	matteMaterial.shininess = 12.0f;
	m_objectMaterials.push_back(matteMaterial);

	// shiny material for ornaments
	OBJECT_MATERIAL shinyMaterial;
	shinyMaterial.tag = "shiny";
	shinyMaterial.ambientStrength = 0.25f;
	shinyMaterial.ambientColor = glm::vec3(0.7f, 0.7f, 0.7f);
	shinyMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	shinyMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	shinyMaterial.shininess = 72.0f;  // higher = tighter sparkle
	m_objectMaterials.push_back(shinyMaterial);
}

void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Light 0 – main (tree glow, warm)
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 2.6f, -2.2f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.15f, 0.15f, 0.15f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.95f, 0.85f, 0.65f); // warm
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 0.95f, 0.85f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 20.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.8f);

	// Light 1 – fill (cool, soft)
	m_pShaderManager->setVec3Value("lightSources[1].position", -3.0f, 3.0f, 2.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.05f, 0.05f, 0.08f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.30f, 0.35f, 0.55f); // cool
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.40f, 0.45f, 0.65f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.4f);

	// Light source 2: off
	m_pShaderManager->setVec3Value("lightSources[2].position", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.0f);

	// Light source 3: off
	m_pShaderManager->setVec3Value("lightSources[3].position", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.0f);
}

void SceneManager::PrepareScene()
{
	// load scene textures first
	LoadSceneTextures();

	// define object materials
	DefineObjectMaterials();

	// set up scene lighting
	SetupSceneLights();

	// load the basic meshes used in the scene
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("matte");
	SetShaderTexture("ground");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// merry christmas
	// Christmas Tree

	// Tree trunk
	scaleXYZ = glm::vec3(0.35f, 1.3f, 0.35f);
	positionXYZ = glm::vec3(0.0f, 0.0f, -2.4f);

	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,
		positionXYZ);

	SetShaderMaterial("matte");
	SetShaderTexture("wood");
	SetTextureUVScale(12.0f, 12.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Bottom tree section
	scaleXYZ = glm::vec3(1.6f, 2.4f, 1.6f);
	positionXYZ = glm::vec3(0.0f, 1.0f, -2.4f);

	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,
		positionXYZ);

	SetShaderMaterial("matte");
	SetShaderColor(0.0f, 0.45f, 0.0f, 1.0f);
	m_basicMeshes->DrawConeMesh();

	// Middle tree section
	scaleXYZ = glm::vec3(1.2f, 2.0f, 1.2f);
	positionXYZ = glm::vec3(0.0f, 2.3f, -2.4f);

	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,
		positionXYZ);

	SetShaderMaterial("matte");
	SetShaderColor(0.0f, 0.60f, 0.0f, 1.0f);
	m_basicMeshes->DrawConeMesh();

	// Top tree section
	scaleXYZ = glm::vec3(0.8f, 1.5f, 0.8f);
	positionXYZ = glm::vec3(0.0f, 3.3f, -2.4f);

	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,
		positionXYZ);

	SetShaderMaterial("matte");
	SetShaderColor(0.15f, 0.80f, 0.15f, 1.0f);
	m_basicMeshes->DrawConeMesh();

	//Suprise Gift Box
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);
	positionXYZ = glm::vec3(1.6f, 0.25f, -1.8f);

	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,
		positionXYZ);

	SetShaderMaterial("matte");
	SetShaderColor(0.8f, 0.0f, 0.0f, 1.0f); // red box
	m_basicMeshes->DrawBoxMesh();

	// Ribbon (vertical stripe)
	scaleXYZ = glm::vec3(0.05f, 0.55f, 0.55f);
	positionXYZ = glm::vec3(1.6f, 0.25f, -1.8f);

	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,
		positionXYZ);

	SetShaderMaterial("shiny");
	SetShaderColor(1.0f, 1.0f, 0.0f, 1.0f); // gold ribbon
	m_basicMeshes->DrawBoxMesh();

	// Ribbon (horizontal stripe)
	scaleXYZ = glm::vec3(0.55f, 0.05f, 0.55f);
	positionXYZ = glm::vec3(1.6f, 0.25f, -1.8f);

	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,
		positionXYZ);

	SetShaderMaterial("shiny");
	SetShaderColor(1.0f, 1.0f, 0.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
	
	//Bling
		// Ornament 1 - top left
	scaleXYZ = glm::vec3(0.28f, 0.28f, 0.28f);
	positionXYZ = glm::vec3(-0.45f, 2.7f, -1.65f);

	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,
		positionXYZ);

	SetShaderMaterial("shiny");
	SetShaderColor(1.0f, 0.0f, 0.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Ornament 2 - middle right
	scaleXYZ = glm::vec3(0.28f, 0.28f, 0.28f);
	positionXYZ = glm::vec3(0.55f, 1.8f, -1.55f);

	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,
		positionXYZ);

	SetShaderMaterial("shiny");
	SetShaderColor(1.0f, 0.0f, 0.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Ornament 3 - bottom left
	scaleXYZ = glm::vec3(0.30f, 0.30f, 0.30f);
	positionXYZ = glm::vec3(-0.35f, 0.95f, -1.45f);

	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,
		positionXYZ);

	SetShaderMaterial("shiny");
	SetShaderColor(1.0f, 0.2f, 0.2f, 1.0f);
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	//AI use acknowledgment: I used ChatGPT to help troubleshoot OpenGL transformations, object composition, and debugging during this assignment. 
	// I reviewed, modified, and finalized all code independently to ensure it meets the assignment requirements.
}
