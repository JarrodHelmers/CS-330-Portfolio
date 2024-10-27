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
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
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

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"../../7-1_FinalProjectMilestones/Utilities/textures/coffee.jpg", "coffee");

	bReturn = CreateGLTexture(
		"../../7-1_FinalProjectMilestones/Utilities/textures/stainless.jpg", "stainless");

	bReturn = CreateGLTexture(
		"../../7-1_FinalProjectMilestones/Utilities/textures/Light-blond-oak.jpg", "oak");

	bReturn = CreateGLTexture(
		"../../7-1_FinalProjectMilestones/Utilities/textures/tissue.jpg", "mug");

	bReturn = CreateGLTexture(
		"../../7-1_FinalProjectMilestones/Utilities/textures/black-texture.jpg", "blktx");

	bReturn = CreateGLTexture(
		"../../7-1_FinalProjectMilestones/Utilities/textures/rubber.jpg", "rubber");

	bReturn = CreateGLTexture(
		"../../7-1_FinalProjectMilestones/Utilities/textures/drywall.jpg", "drywall");

	bReturn = CreateGLTexture(
		"../../7-1_FinalProjectMilestones/Utilities/textures/Kali-Linux_13.jpg", "Kali");

	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/



	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	metalMaterial.ambientStrength = 0.4f;
	metalMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	metalMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	metalMaterial.shininess = 22.0;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cementMaterial.ambientStrength = 0.2f;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";
	m_objectMaterials.push_back(cementMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL tileMaterial;
	tileMaterial.ambientColor = glm::vec3(0.2f, 0.3f, 0.4f);
	tileMaterial.ambientStrength = 0.3f;
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	tileMaterial.shininess = 25.0;
	tileMaterial.tag = "tile";
	m_objectMaterials.push_back(tileMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f);
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 0.5;
	clayMaterial.tag = "clay";
	m_objectMaterials.push_back(clayMaterial);

	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	plasticMaterial.ambientStrength = 0.5f;                    
	plasticMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f); 
	plasticMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f); 
	plasticMaterial.shininess = 32.0f;                          
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL lightplasticMaterial;
	lightplasticMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	lightplasticMaterial.ambientStrength = 0.5f;
	lightplasticMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	lightplasticMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f);
	lightplasticMaterial.shininess = 22.0f;
	lightplasticMaterial.tag = "lightplastic";
	m_objectMaterials.push_back(lightplasticMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/


void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 15.0f, -5.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 10.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.1f);

	m_pShaderManager->setVec3Value("lightSources[1].position", -10.0f, 15.0f, -5.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 30.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);

	m_pShaderManager->setVec3Value("lightSources[2].position", 10.0f, 15.0f, -5.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.02f, 0.02f, 0.02f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 30.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.6f);

	m_pShaderManager->setBoolValue("bUseLighting", true);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	// Load the meshes needed for the scene

	// Load the plane mesh for the ground or keyboard base
	m_basicMeshes->LoadPlaneMesh();

	// Load the cylinder mesh for the mug's body
	m_basicMeshes->LoadCylinderMesh();

	// Load the half torus mesh for the mug's handle
	m_basicMeshes->LoadTorusMesh(0.2f); // Adjust thickness as needed

	// Load the box mesh for the keyboard base
	m_basicMeshes->LoadBoxMesh();

	// Load the sphere mesh for the mouse body
	m_basicMeshes->LoadSphereMesh();

	// Load the tapered cylinder for the mouse tail or other parts if needed
	m_basicMeshes->LoadTaperedCylinderMesh();

	// Load any additional meshes as needed
}


/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Ground Plane
	{
		glm::vec3 scaleXYZ = glm::vec3(10.0f, 10.0f, 5.0f);
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		//SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f); // Grey color
		SetShaderTexture("oak");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("wood");

		m_basicMeshes->DrawPlaneMesh();
	}

	// Draw the Coffee Mug Outer Body (without top)
	{
		glm::vec3 scaleXYZ = glm::vec3(0.8f, 1.2f, 0.8f);
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(-4.0f, -0.03f, -0.5f);

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		//SetShaderColor(0.8f, 0.7f, 0.5f, 1.0f);
		SetShaderTexture("mug");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("clay");

		// Draw cylinder without top cap
		m_basicMeshes->DrawCylinderMesh(false, true, true);
	}

	// Draw the Coffee Mug Inner Surface
	{
		glm::vec3 innerScaleXYZ = glm::vec3(0.75f, 1.15f, 0.75f); // Slightly smaller and shorter
		float innerXrotationDegrees = 0.0f;
		float innerYrotationDegrees = 0.0f;
		float innerZrotationDegrees = 0.0f;
		glm::vec3 innerPositionXYZ = glm::vec3(-4.0f, -0.04f, -0.5f); // Slightly higher

		SetTransformations(innerScaleXYZ, innerXrotationDegrees, innerYrotationDegrees, innerZrotationDegrees, innerPositionXYZ);

		//SetShaderColor(0.9f, 0.8f, 0.7f, 1.0f);
		SetShaderTexture("coffee");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("glass");

		// Draw inner cylinder without bottom cap
		m_basicMeshes->DrawCylinderMesh(true, false, true);
	}

	// Draw the Coffee Mug Handle using TorusMesh
	{
		glm::vec3 scaleXYZ = glm::vec3(0.4f, 0.4f, 0.4f);
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(-3.2f, 0.6f, -0.5f); // Adjusted position

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		//SetShaderColor(0.8f, 0.7f, 0.5f, 1.0f);
		SetShaderTexture("mug");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("clay");

		m_basicMeshes->DrawTorusMesh();
	}

	// Keyboard Base
	{
		glm::vec3 scaleXYZ = glm::vec3(4.0f, 0.2f, 2.0f);
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(0.0f, 0.0f, 2.0f);

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		//SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);
		SetShaderTexture("stainless");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("metal");

		m_basicMeshes->DrawBoxMesh();
	}

	// Keyboard Keys
	for (int i = 0; i < 5; i++) // Number of rows of keys
	{
		for (int j = 0; j < 12; j++) // Number of keys per row
		{
			// Reduced key size for better spacing
			glm::vec3 scaleXYZ = glm::vec3(0.25f, 0.25f, 0.25f); // Smaller keys
			float XrotationDegrees = 0.0f;
			float YrotationDegrees = 1.5f; // Match keyboard base tilt
			float ZrotationDegrees = 0.0f;

			// Proper spacing between keys
			glm::vec3 positionXYZ = glm::vec3(-1.65f + j * 0.3f, 0.05f, 1.5f + i * 0.3f); // Added spacing between keys

			SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

			SetShaderTexture("blktx");
			SetTextureUVScale(1.0, 1.0);
			SetShaderMaterial("lightplastic");

			m_basicMeshes->DrawBoxMesh();
		}
	}

	// Mouse Body
	{
		glm::vec3 scaleXYZ = glm::vec3(0.5f, 0.2f, 0.8f);
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 15.0f; // Rotate 30 degrees around Y-axis
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(4.0f, 0.2f, 2.5f);

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);
		SetShaderTexture("blktx");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("lightplastic");

		m_basicMeshes->DrawSphereMesh();
	}

	// Mouse Buttons
	// Mouse Wheel
	{
		glm::vec3 scaleXYZ = glm::vec3(0.1f, 0.1f, 0.15f);
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 15.0f; // Match mouse body rotation
		float ZrotationDegrees = 90.0f;
		glm::vec3 positionXYZ = glm::vec3(3.9f, 0.31f, 2.0f);

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		//SetShaderColor(0.3f, 0.3f, 0.3f, 1.0f);
		SetShaderTexture("rubber");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("lightplastic");

		m_basicMeshes->DrawCylinderMesh();
	}

	// Wall
	{
		glm::vec3 scaleXYZ = glm::vec3(40.0f, 30.0f, 0.2f); // Wide and tall wall
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(0.0f, 2.5f, -6.0f); // Positioned behind the desk

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light grey for the wall
		SetShaderTexture("drywall");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("cement");

		m_basicMeshes->DrawBoxMesh();
	}

	// Side Wall
	{
		glm::vec3 scaleXYZ = glm::vec3(40.0f, 30.0f, 0.2f); // Wide and tall wall
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 90.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(20.0f, 2.5f, 10.0f);

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light grey for the wall
		SetShaderTexture("drywall");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("cement");

		m_basicMeshes->DrawBoxMesh();
	}


	// Monitor Body
	{
		glm::vec3 scaleXYZ = glm::vec3(9.0f, 4.5f, 0.3f); // Wide and thin monitor
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(0.0f, 5.0f, -3.5f); // Raised position on the desk

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark grey/black color
		SetShaderTexture("blktx");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("plastic");

		m_basicMeshes->DrawBoxMesh();
	}

	// Monitor Screen
	{
		glm::vec3 scaleXYZ = glm::vec3(8.8f, 4.3f, 0.3f); // Wide and thin monitor
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(0.0f, 5.0f, -3.48f); // Raised position on the desk

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark grey/black color
		SetShaderTexture("Kali");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("glass");

		m_basicMeshes->DrawBoxMesh();
	}

	// Monitor Stand (Pole)
	{
		glm::vec3 scaleXYZ = glm::vec3(0.3f, 4.0f, 0.3f); // Thin pole for the stand
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(0.0f, 0.2f, -3.7f); // Positioned under the monitor

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		//SetShaderColor(0.3f, 0.3f, 0.3f, 1.0f); // Slightly lighter than the monitor
		SetShaderTexture("stainless");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("metal");

		m_basicMeshes->DrawCylinderMesh();
	}

	// Monitor Stand (Base)
	{
		glm::vec3 scaleXYZ = glm::vec3(4.0f, 0.3f, 2.0f); // Flat, wide base
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 0.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(0.0f, 0.2f, -3.5f); // Positioned on the desk

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		//SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f); // Dark grey/black color
		SetShaderTexture("stainless");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("metal");

		m_basicMeshes->DrawBoxMesh();
	}

}
