///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
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

	modelView = translation * rotationZ * rotationY * rotationX * scale;

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

		// ensure no leftover texture blending affects this draw
		m_pShaderManager->setFloatValue("blendFactor", 0.0f);
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

		// neutralize any prior two-texture/color blend
		m_pShaderManager->setSampler2DValue("blendTexture", textureID);
		m_pShaderManager->setFloatValue("blendFactor", 0.0f); // no mix

	}
}



void SceneManager::SetShaderTextures(
	std::string textureTag, std::string textureTag2)
{
	
	//I do not know what to put here to mix two textures

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


//Used to load textures into memory for the scene.
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	//fan texture
	bReturn = CreateGLTexture("textures/black plastic.jpg", "blackPlastic");
	//Stand texture
	bReturn = CreateGLTexture("textures/wood.jpg", "stand");
	//fan blade texture
	bReturn = CreateGLTexture("textures/fanblade.jpg", "blade");
	//wall texture
	bReturn = CreateGLTexture("textures/wall.jpg", "wall");
	//motherboard texture
	bReturn = CreateGLTexture("textures/microchip.jpg", "chip");
	//gpu texture
	bReturn = CreateGLTexture("textures/gpu.jpg", "gpu");



	BindGLTextures();

}


//*************DefineObjects********************//
void SceneManager::DefineObjectMaterials()
{

	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(5.0f, 5.0f, 5.0f);
	plasticMaterial.specularColor = glm::vec3(5.0f, 5.0f, 5.0f);
	plasticMaterial.shininess = 30.0;
	plasticMaterial.tag = "plastic";

	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	woodMaterial.shininess = 0.1;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL middleMaterial;
	middleMaterial.diffuseColor = glm::vec3(.0f, 0.0f, 0.0f);
	middleMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	middleMaterial.shininess = 5.0;
	middleMaterial.tag = "middle";

	m_objectMaterials.push_back(middleMaterial);
	

	//mootherboard material
	OBJECT_MATERIAL motherMaterial;
	motherMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.3f);
	motherMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	motherMaterial.shininess = 20.03;
	motherMaterial.tag = "mother";

	m_objectMaterials.push_back(motherMaterial);
	
	//glass panels
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

}

void SceneManager::SetupSceneLights()
{

		m_pShaderManager->setBoolValue(g_UseLightingName, true);

	//main light
	m_pShaderManager->setVec3Value("pointLights[0].position", -4.0f, 8.0f, 5.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);


	//fan light 1
	m_pShaderManager->setVec3Value("pointLights[1].position", 4.0f, 11.5f, -3.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", .01f, .01f, .01f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", .4f, .4f, .4f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", .0f, .0f, .0f);
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	//fan light 2
	m_pShaderManager->setVec3Value("pointLights[2].position", 4.0f, 7.25f, -3.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", .01f, .01f, .01f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", .0f, .0f, .0f);
	m_pShaderManager->setFloatValue("pointLights[2].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[2].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[2].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);

	//fan light 3
	m_pShaderManager->setVec3Value("pointLights[3].position", 4.0f, 3.0f, -3.0f);
	m_pShaderManager->setVec3Value("pointLights[3].ambient", .01f, .01f, .01f);
	m_pShaderManager->setVec3Value("pointLights[3].diffuse", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("pointLights[3].specular", .0f, .0f, .0f);
	m_pShaderManager->setFloatValue("pointLights[3].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[3].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[3].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", true);

	//fan light 4
	m_pShaderManager->setVec3Value("pointLights[4].position", -7.0f, 11.5f, 0.5f);
	m_pShaderManager->setVec3Value("pointLights[4].ambient", .01f, .01f, .01f);
	m_pShaderManager->setVec3Value("pointLights[4].diffuse", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("pointLights[4].specular", .0f, .0f, .0f);
	m_pShaderManager->setFloatValue("pointLights[4].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[4].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[4].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[4].bActive", true);

	//fan light 5
	m_pShaderManager->setVec3Value("pointLights[5].position", -3.5f, 13.0f, 0.5f);
	m_pShaderManager->setVec3Value("pointLights[5].ambient", .01f, .01f, .01f);
	m_pShaderManager->setVec3Value("pointLights[5].diffuse", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("pointLights[5].specular", .0f, .0f, .0f);
	m_pShaderManager->setFloatValue("pointLights[5].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[5].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[5].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[5].bActive", true);

	//fan light 6
	m_pShaderManager->setVec3Value("pointLights[6].position", 1.0f, 13.0f, 0.5f);
	m_pShaderManager->setVec3Value("pointLights[6].ambient", .01f, .01f, .01f);
	m_pShaderManager->setVec3Value("pointLights[6].diffuse", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("pointLights[6].specular", .0f, .0f, .0f);
	m_pShaderManager->setFloatValue("pointLights[6].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[6].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[6].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[6].bActive", true);




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
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	LoadSceneTextures();

	DefineObjectMaterials();

	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadExtraTorusMesh1();
	m_basicMeshes->LoadExtraTorusMesh2();

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
	/*** and drawing all the basic 3D shapes.	                    ***/
	/***                                                            ***/
	/*** Stand for the computer to rest on                          ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 15.0f);

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
	
	//Sets the stands texture
	SetShaderTexture("stand");
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	



	/****************************************************************/


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 25.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//wall texture
	SetShaderTexture("wall");
	
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	//My added code//
	// 
	// Matrix for fan parts position
	//(6.0f, 11.5f, -2.5f)
	//(2.0f, 11.5f, -2.5f)
	//(4.0f, 9.5f, -2.5f)
	//(4.0f, 13.5f, -2.5f)
	//(4.0f, 11.5f, -3.0f)
	//(2.0f, 13.5f, -3.0f)
	//(2.0f, 9.5f, -3.0f)
	//(6.0f, 13.5f, -3.0f)
	//(6.0f, 9.5f, -3.0f)
	//(4.0f, 11.5f, -2.1f)
	//(4.0f, 10.5f, -2.5f)
	//(4.75f, 12.25f, -2.5f)
	//(3.25f, 12.25f, -2.5f)
	// 
	// 
	//********************************************//
	//********************************************//
	// Computer Fan
	// (6.0f, 11.5f, -2.5f)
	// (2.0f, 11.5f, -2.5f)
	// (4.0f, 9.5f, -2.5f)
	// (4.0f, 13.5f, -2.5f)
	// (4.0f, 11.5f, -3.0f)
	// 
	
	//add texture for all parts of the fan
	SetShaderTexture("blackPlastic");
	SetShaderMaterial("plastic");


	//Set the scale for all four sides
	scaleXYZ = glm::vec3(0.25f, 4.0f, 1.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(6.0f, 11.5f, -2.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//Draw the mesh


	m_basicMeshes->DrawBoxMesh();



	//Set the XYZposition
	positionXYZ = glm::vec3(2.0f, 11.5f, -2.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 9.5f, -2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 13.5f, -2.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//*****************************//
	//Middle Cylinder of the fan
	//Set the scale
	scaleXYZ = glm::vec3(0.65f, 1.0f, 0.65f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 11.5f, -3.0f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("middle");
	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();
	
	//********************************************//



	//********************************************//
	//********************************************//
	//Corner Cylinders
	//
	// matrix for position
	//(2.0f, 13.5f, -3.0f)
	//(2.0f, 9.5f, -3.0f)
	//(6.0f, 13.5f, -3.0f)
	//(6.0f, 9.5f, -3.0f)
	// 
	// 
	//
	//****** Top Left ******//
	// 
	SetShaderMaterial("plastic");
	//Set the scale
	scaleXYZ = glm::vec3(0.13f, 1.0f, 0.13f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(2.0f, 13.5f, -3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//****** Bottom left corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(2.0f, 9.5f, -3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//****** Top Right corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(6.0f, 13.5f, -3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	 
	//****** Bottom right corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(6.0f, 9.5f, -3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();



	//********************************************//
	//********************************************//
	//Supports
	// 
	// Matrix for position
	// (4.0f, 11.5f, -2.1f)
	//
	// 
	// 
	// 
	//Set the scale
	scaleXYZ = glm::vec3(4.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 11.5f, -2.15f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 36.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(4.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 72.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(4.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 108.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(5.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 144.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//********************************************//





	//********************************************//
	//********************************************//
	//Fan blades
	// 
	// Matrix for position
	//(4.0f, 10.5f, -2.5f)
	//(4.75f, 12.25f, -2.5f)
	//(3.25f, 12.25f, -2.5f)
	//
	//
	SetShaderTexture("blade");
	//Set the scale
	scaleXYZ = glm::vec3(.45f, 0.75f, 0.1f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 10.5f, -2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -45.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.75f, 12.25f, -2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 45.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(3.25f, 12.25f, -2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();

	//********************************************//




	// 
	// Matrix for fan parts position
	//(6.0f, 7.25f, -2.5f)
	//(2.0f, 7.25f, -2.5f)
	//(4.0f, 5.25f, -2.5f)
	//(4.0f, 9.25f, -2.5f)
	//(4.0f, 7.25f, -3.0f)
	//(2.0f, 9.25f, -3.0f)
	//(2.0f, 5.25f, -3.0f)
	//(6.0f, 9.25f, -3.0f)
	//(6.0f, 5.25f, -3.0f)
	//(4.0f, 7.25f, -2.15f)
	//(4.0f, 6.25f, -2.5f)
	//(4.75f, 8.0f, -2.5f)
	//(3.25f, 8.0f, -2.5f)
	// 
	// 
	//********************************************//
	//********************************************//
	// Computer Fan
	// (6.0f, 11.5f, -2.5f)
	// (2.0f, 11.5f, -2.5f)
	// (4.0f, 9.5f, -2.5f)
	// (4.0f, 13.5f, -2.5f)
	// (4.0f, 11.5f, -3.0f)
	// 

	//add texture for all parts of the fan
	SetShaderTexture("blackPlastic");
	SetShaderMaterial("plastic");


	//Set the scale for all four sides
	scaleXYZ = glm::vec3(0.25f, 4.0f, 1.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(6.0f, 7.25f, -2.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//Draw the mesh


	m_basicMeshes->DrawBoxMesh();



	//Set the XYZposition
	positionXYZ = glm::vec3(2.0f, 7.25f, -2.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 5.25f, -2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 9.25f, -2.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//*****************************//
	//Middle Cylinder of the fan
	//Set the scale
	scaleXYZ = glm::vec3(0.65f, 1.0f, 0.65f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 7.25f, -3.0f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	//********************************************//



	//********************************************//
	//********************************************//
	//Corner Cylinders
	//
	// matrix for position
	//(2.0f, 13.5f, -3.0f)
	//(2.0f, 9.5f, -3.0f)
	//(6.0f, 13.5f, -3.0f)
	//(6.0f, 9.5f, -3.0f)
	// 
	// 
	//
	//****** Top Left ******//
	//Set the scale
	scaleXYZ = glm::vec3(0.13f, 1.0f, 0.13f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(2.0f, 9.25f, -3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//****** Bottom left corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(2.0f, 5.25f, -3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//****** Top Right corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(6.0f, 9.25f, -3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();



	//****** Bottom right corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(6.0f, 5.25f, -3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();



	//********************************************//
	//********************************************//
	//Supports
	// 
	// Matrix for position
	// (4.0f, 11.5f, -2.15f)
	//
	// 
	// 
	// 
	//Set the scale
	scaleXYZ = glm::vec3(4.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 7.25f, -2.15f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 36.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(4.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 72.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(4.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 108.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(5.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 144.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//********************************************//





	//********************************************//
	//********************************************//
	//Fan blades
	// 
	// Matrix for position
	//(4.0f, 10.5f, -2.5f)
	//(4.75f, 12.25f, -2.5f)
	//(3.25f, 12.25f, -2.5f)
	//
	//
	SetShaderTexture("blade");
	//Set the scale
	scaleXYZ = glm::vec3(.45f, 0.75f, 0.1f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 6.25f, -2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -45.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.75f, 8.0f, -2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 45.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(3.25f, 8.0f, -2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();

	//********************************************//


	// 
	// Matrix for fan parts position
	//(6.0f, 3.0f, -2.5f)
	//(2.0f, 3.0f, -2.5f)
	//(4.0f, 1.0f, -2.5f)
	//(4.0f, 5.0f, -2.5f)
	//(4.0f, 3.0f, -3.0f)
	//(2.0f, 5.0f, -3.0f)
	//(2.0f, 1.0f, -3.0f)
	//(6.0f, 5.0f, -3.0f)
	//(6.0f, 1.0f, -3.0f)
	//(4.0f, 3.0f, -2.15f)
	//(4.0f, 2.0f, -2.5f)
	//(4.75f, 3.75f, -2.5f)
	//(3.25f, 3.75f, -2.5f)
	// 
	// 
	//********************************************//
	//********************************************//
	// Computer Fan
	// 
	// 

	//add texture for all parts of the fan
	SetShaderTexture("blackPlastic");
	SetShaderMaterial("plastic");


	//Set the scale for all four sides
	scaleXYZ = glm::vec3(0.25f, 4.0f, 1.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(6.0f, 3.0f, -2.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//Draw the mesh


	m_basicMeshes->DrawBoxMesh();



	//Set the XYZposition
	positionXYZ = glm::vec3(2.0f, 3.0f, -2.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 1.0f, -2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 5.0f, -2.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//*****************************//
	//Middle Cylinder of the fan
	//Set the scale
	scaleXYZ = glm::vec3(0.65f, 1.0f, 0.65f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 3.0f, -3.0f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	//********************************************//



	//********************************************//
	//********************************************//
	//Corner Cylinders
	// 
	// 
	//
	//****** Top Left ******//
	//Set the scale
	scaleXYZ = glm::vec3(0.13f, 1.0f, 0.13f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(2.0f, 5.0f, -3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//****** Bottom left corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(2.0f, 1.0f, -3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//****** Top Right corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(6.0f, 5.0f, -3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();



	//****** Bottom right corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(6.0f, 1.0f, -3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();



	//********************************************//
	//********************************************//
	//Supports
	//
	// 
	// 
	// 
	//Set the scale
	scaleXYZ = glm::vec3(4.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 3.0f, -2.15f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 36.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(4.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 72.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(4.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 108.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(5.0f, 0.1f, 0.25f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 144.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//********************************************//



	//********************************************//
	//********************************************//
	//Fan blades
	// 
	//
	SetShaderTexture("blade");
	//Set the scale
	scaleXYZ = glm::vec3(.45f, 0.75f, 0.1f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 2.0f, -2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -45.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.75f, 3.75f, -2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 45.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(3.25f, 3.75f, -2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();

	//********************************************//

		// Matrix for fan parts position
	//(-6.5f, 11.5f, -1.5f)
	//(-6.5f, 11.5f, 2.5f)
	//(-6.5f, 9.5f, 0.5f)
	//(-6.5f, 13.5f, 0.5f)
	//(-7.0f, 11.5f, 0.5f)
	//(-7.0f, 13.5f, 2.5f)
	//(-7.0f, 9.5f, 2.5f)
	//(-7.0f, 13.5f, -1.5f)
	//(-7.0f, 9.5f, -1.5f)
	//(-6.15f, 11.5f, 0.5f)
	//(-6.5f, 10.5f, 0.5f)
	//(-6.5f, 12.25f, -0.25f)
	//(-6.5, 12.25f, 1.25f)
	// 





	SetShaderTexture("blackPlastic");
	SetShaderMaterial("plastic");


	//Set the scale for all four sides
	scaleXYZ = glm::vec3(0.25f, 4.0f, 1.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(-6.5f, 11.5f, -1.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(-6.5f, 11.5f, 2.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//Draw the mesh


	m_basicMeshes->DrawBoxMesh();



	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-6.5f, 9.5f, 0.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Set the XYZposition
	positionXYZ = glm::vec3(-6.5f, 13.5f, 0.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//*****************************//
	//Middle Cylinder of the fan
	//Set the scale
	scaleXYZ = glm::vec3(0.65f, 1.0f, 0.65f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(-7.0f, 11.5f, 0.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	//********************************************//



	//********************************************//
	//********************************************//
	//Corner Cylinders
	//
	// matrix for position
	//(2.0f, 13.5f, -3.0f)
	//(2.0f, 9.5f, -3.0f)
	//(6.0f, 13.5f, -3.0f)
	//(6.0f, 9.5f, -3.0f)
	// 
	// 
	//
	//****** Top Left ******//
	//Set the scale
	scaleXYZ = glm::vec3(0.13f, 1.0f, 0.13f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-7.0f, 13.5f, 2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//****** Bottom left corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(-7.0f, 9.5f, 2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//****** Top Right corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(-7.0f, 13.5f, -1.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();



	//****** Bottom right corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(-7.0f, 9.5f, -1.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();



	//********************************************//
	//********************************************//
	//Supports
	// 
	// Matrix for position
	//(-6.15f, 11.5f, 0.5f)
	//
	// 
	// 
	// 
	//Set the scale
	scaleXYZ = glm::vec3(0.25f, 0.1f, 4.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-6.15f, 11.5f, 0.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.25f, 0.1f, 5.0f);

	//Set the XYZ rotation
	XrotationDegrees = 36.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.25f, 0.1f, 4.0f);

	//Set the XYZ rotation
	XrotationDegrees = 72.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(0.25f, 0.1f, 4.0f);

	//Set the XYZ rotation
	XrotationDegrees = 108.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(0.25f, 0.1f, 5.0f);

	//Set the XYZ rotation
	XrotationDegrees = 144.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//********************************************//





	//********************************************//
	//********************************************//
	//Fan blades
	// 
	// Matrix for position
	//(-6.5f, 10.5f, 0.5f)
	//(-6.5f, 12.25f, -0.25f)
	//(-6.5, 12.25f, 1.25f)
	//
	//
	SetShaderTexture("blade");
	//Set the scale
	scaleXYZ = glm::vec3(.1f, 0.75f, 0.45f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-6.5f, 10.5f, 0.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set the XYZ rotation
	XrotationDegrees = -45.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-6.5f, 12.25f, -0.25f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set the XYZ rotation
	XrotationDegrees = 45.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(-6.5, 12.25f, 1.25f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();

	//********************************************//






	//********************************************//

		// Matrix for fan parts position
	//(-3.5f, 13.5f, -1.5f)
	//(-3.5f, 13.5f, 2.5f)
	//(-1.5f, 13.5f, 0.5f)
	//(-5.5f, 13.5f, 0.5f)
	//(-3.5f, 13.0f, 0.5f)
	//(-5.5f, 13.0f, 2.5f)
	//(-1.5f, 13.0f, 2.5f)
	//(-5.5f, 13.0f, -1.5f)
	//(-1.5f, 13.0f, -1.5f)
	//(-3.5f, 13.15f, 0.5f)
	//(-4.5f, 13.5f, 0.5f)
	//(-3.0f, 13.5f, 1.25f)
	//(-3.0, 13.5f, -0.25f)
	// 





	SetShaderTexture("blackPlastic");
	SetShaderMaterial("plastic");


	//Set the scale for all four sides
	scaleXYZ = glm::vec3(0.25f, 4.0f, 1.0f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(-3.5f, 13.5f, -1.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//Draw the mesh


	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(-3.5f, 13.5f, 2.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//Draw the mesh


	m_basicMeshes->DrawBoxMesh();



	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-1.5f, 13.5f, 0.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Set the XYZposition
	positionXYZ = glm::vec3(-5.5f, 13.5f, 0.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//*****************************//
	//Middle Cylinder of the fan
	//Set the scale
	scaleXYZ = glm::vec3(0.65f, 1.0f, 0.65f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(-3.5f, 13.0f, 0.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	//********************************************//



	//********************************************//
	//********************************************//
	//Corner Cylinders
	//
	// matrix for position
	//(-5.5f, 13.0f, 2.5f)
	//(-1.5f, 13.0f, 2.5f)
	//(-5.5f, 13.0f, -1.5f)
	//(-1.5f, 13.0f, -1.5f)
	// 
	// 
	//
	//****** Top Left ******//
	//Set the scale
	scaleXYZ = glm::vec3(0.13f, 1.0f, 0.13f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-5.5f, 13.0f, 2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//****** Bottom left corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(-1.5f, 13.0f, 2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//****** Top Right corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(-5.5f, 13.0f, -1.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();



	//****** Bottom right corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(-1.5f, 13.0f, -1.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();



	//********************************************//
	//********************************************//
	//Supports
	// 
	// Matrix for position
	//(-3.5f, 13.15f, 0.5f)
	//
	// 
	// 
	// 
	//Set the scale
	scaleXYZ = glm::vec3(0.1f, 0.25f, 4.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-3.5f, 13.15f, 0.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.1f, 0.25f, 5.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 36.0f;
	ZrotationDegrees = 0.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.1f, 0.25f, 4.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 72.0f;
	ZrotationDegrees = 0.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(0.1f, 0.25f, 4.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 108.0f;
	ZrotationDegrees = 0.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(0.1f, 0.25f, 5.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 144.0f;
	ZrotationDegrees = 0.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//********************************************//

	//********************************************//
	//********************************************//
	//Fan blades
	// 
	// Matrix for position
	//(-4.5f, 13.5f, 0.5f)
	//(-3.0f, 13.5f, 1.25f)
	//(-3.0, 13.5f, -0.25f)
	//
	//
	SetShaderTexture("blade");
	//Set the scale
	scaleXYZ = glm::vec3(.75f, 0.1f, 0.45f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-4.5f, 13.5f, 0.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-3.0f, 13.5f, 1.25f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(-3.0, 13.5f, -0.25f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();

	//********************************************//




	// Matrix for fan parts position
	//(1.0f, 13.5f, 2.5f)
	//(1.0f, 13.5f, -1.5f)
	//(3.0f, 13.5f, 0.5f)
	//(-1.0f, 13.5f, 0.5f)
	//(1.0f, 13.0f, 0.5f)
	//(-5.5f, 13.0f, 2.5f)
	//(-1.5f, 13.0f, 2.5f)
	//(-5.5f, 13.0f, -1.5f)
	//(-1.5f, 13.0f, -1.5f)
	//(-3.5f, 13.15f, 0.5f)
	//(-4.5f, 13.5f, 0.5f)
	//(-3.0f, 13.5f, 1.25f)
	//(-3.0, 13.5f, -0.25f)
	// 





	SetShaderTexture("blackPlastic");
	SetShaderMaterial("plastic");


	//Set the scale for all four sides
	scaleXYZ = glm::vec3(0.25f, 4.0f, 1.0f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(1.0f, 13.5f, 2.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//Draw the mesh


	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(1.0f, 13.5f, -1.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//Draw the mesh


	m_basicMeshes->DrawBoxMesh();




	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(3.0f, 13.5f, 0.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Set the XYZposition
	positionXYZ = glm::vec3(-1.0f, 13.5f, 0.5f);

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//*****************************//
	//Middle Cylinder of the fan
	//Set the scale
	scaleXYZ = glm::vec3(0.65f, 1.0f, 0.65f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(1.0f, 13.0f, 0.5f); 

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	//********************************************//



	//********************************************//
	//********************************************//
	//Corner Cylinders
	//
	// matrix for position
	//(-1.0f, 13.0f, 2.5f)
	//(3.0f, 13.0f, 2.5f)
	//(-1.0f, 13.0f, -1.5f)
	//(3.0f, 13.0f, -1.5f)
	// 
	// 
	//
	//****** Top Left ******//
	//Set the scale
	scaleXYZ = glm::vec3(0.13f, 1.0f, 0.13f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-1.0f, 13.0f, 2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//****** Bottom left corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(3.0f, 13.0f, 2.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//****** Top Right corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(-1.0f, 13.0f, -1.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();



	//****** Bottom right corner ******//

	//Set the XYZposition
	positionXYZ = glm::vec3(3.0f, 13.0f, -1.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();



	//********************************************//
	//********************************************//
	//Supports
	// 
	// Matrix for position
	//(1.0f, 13.15f, 0.5f)
	//
	// 
	// 
	// 
	//Set the scale
	scaleXYZ = glm::vec3(0.1f, 0.25f, 4.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(1.0f, 13.15f, 0.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.1f, 0.25f, 5.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 36.0f;
	ZrotationDegrees = 0.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.1f, 0.25f, 4.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 72.0f;
	ZrotationDegrees = 0.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(0.1f, 0.25f, 4.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 108.0f;
	ZrotationDegrees = 0.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(0.1f, 0.25f, 5.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 144.0f;
	ZrotationDegrees = 0.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//********************************************//

	//********************************************//
	//********************************************//
	//Fan blades
	// 
	// Matrix for position
	//(0.0f, 13.5f, 0.5f)
	//(1.5f, 13.5f, 1.25f)
	//(1.5, 13.5f, -0.25f)
	//
	//
	SetShaderTexture("blade");
	//Set the scale
	scaleXYZ = glm::vec3(.75f, 0.1f, 0.45f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(0.0f, 13.5f, 0.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(1.5f, 13.5f, 1.25f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZposition
	positionXYZ = glm::vec3(1.5, 13.5f, -0.25f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();

	//********************************************//






	//**************Case************//
	//Matrix for position//
	//(0.0f, 7.5f, -3.0f)
	//(0.0f, 14.0f, 0.0f)
	//(-7.0f, 7.5f, 0.0f)
	//(0.0f, 1.0f, 0.0f)
	//(-6.0f, 0.75f, 2.9f)
	//(6.0f, 0.75f, 2.9f)
	//(6.0f, 0.75f, -2.9f)
	//(-6.0f, 0.75f, -2.9f)
	// 
	// 
	// 
	// Set shader texture for case
	SetShaderTexture("blackPlastic");
	//Set the scale
	scaleXYZ = glm::vec3(14.0f, 13.0f, 0.1f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(0.0f, 7.5f, -3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 0.1, 0.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();



	//Set the scale
	scaleXYZ = glm::vec3(14.0f, 0.1f, 6.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(0.0f, 14.0f, 0.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 0.1, 0.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Set the scale
	scaleXYZ = glm::vec3(0.1f, 13.0f, 6.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-7.0f, 7.5f, 0.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 0.1, 0.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();
	
	//Set the scale
	scaleXYZ = glm::vec3(14.0f, 0.1f, 6.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(0.0f, 1.0f, 0.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 0.1, 0.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(1.5f, 0.5f, 0.1f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-6.0f, 0.75f, 2.9f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 1.0, 0.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();



	//Set the scale
	scaleXYZ = glm::vec3(1.5f, 0.5f, 0.1f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(6.0f, 0.75f, 2.9f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 1.0, 0.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();



	//Set the scale
	scaleXYZ = glm::vec3(1.5f, 0.5f, 0.1f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(6.0f, 0.75f, -2.9f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 1.0, 0.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();



	//Set the scale
	scaleXYZ = glm::vec3(1.5f, 0.5f, 0.1f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-6.0f, 0.75f, -2.9f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 1.0, 0.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(8.5f, 3.0f, 5.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-2.7f, 2.5f, -0.5f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 1.0, 0.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();






	//***********MotherBoard*********//

	SetShaderTexture("chip");
	SetShaderMaterial("mother");
	//Set the scale
	scaleXYZ = glm::vec3(8.0f, 9.0f, 0.1f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-2.5f, 9.0f, -2.8f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 1.0, 0.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//*****************************************************//



	//*******GPU*************//



	SetShaderTexture("gpu");
	SetShaderMaterial("plastic");
	//Set the scale
	scaleXYZ = glm::vec3(8.5f, 1.5f, 3.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-2.75f, 6.0f, -1.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 1.0, 0.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();




	//*********************************************//

	//**********RAM*******************//

	SetShaderTexture("blackPlastic");
	SetShaderMaterial("plastic");
	//Set the scale
	scaleXYZ = glm::vec3(.3f, 4.0f, 1.5f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-0.5f, 9.0f, -2.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 1.0, 0.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Set the XYZposition
	positionXYZ = glm::vec3(0.5f, 9.0f, -2.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.1, 1.0, 0.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(.3f, 4.0f, 0.3f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-0.5f, 9.0f, -1.1f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0);
	//SetShaderMaterial("plastic");
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Set the XYZposition
	positionXYZ = glm::vec3(0.5f, 9.0f, -1.1f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.1, 1.1, 1.1, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//************CPU fan**********//

	
	//Set scale
	scaleXYZ = glm::vec3(0.85f, 2.0f, 0.85f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-3.0f, 9.5f, -2.75f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.01, 0.01, 0.01, 1.0);
	//SetShaderMaterial("plastic");
	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	//Set scale
	scaleXYZ = glm::vec3(0.40f, 2.0f, 0.40f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-3.0f, 9.5f, -2.55f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.01, 0.01, 0.01, 1.0);
	SetShaderTexture("blackPlastic");
	SetShaderMaterial("plastic");
	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();



	//Set scale
	scaleXYZ = glm::vec3(1.0f, 1.0f, 4.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-3.0f, 9.5f, -1.75f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.01, 0.01, 0.01, 1.0);
	SetShaderTexture("blackPlastic");
	SetShaderMaterial("plastic");
	//Draw the mesh
	m_basicMeshes->DrawTorusMesh();

	///**********Middle bars of CPU**************///

	//Set scale
	scaleXYZ = glm::vec3(1.75f, .02f, 0.2f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-3.0f, 9.5f, -0.75f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.0, .5, 0.5, 1.0);
	SetShaderTexture("blade");
	//SetShaderMaterial("plastic");
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 10.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 20.0f;

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 30.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 40.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 50.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 60.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 70.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 80.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 100.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 110.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 120.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 130.0f;



	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();



	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 140.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 150.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 160.0f;

	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 170.0f;


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//***************************//

	//***************cooling tube************************//

	

	//Set scale
	scaleXYZ = glm::vec3(0.5f, 1.0f, 0.5f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-3.0f, 7.5f, -2.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.01, 0.01, 0.01, 1.0);
	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	//Set scale
	scaleXYZ = glm::vec3(0.5f, 1.0f, 0.5f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 40.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-3.0f, 7.5f, -2.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//Set scale
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 40.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-3.0f, 7.5f, -2.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();



	//Set scale
	scaleXYZ = glm::vec3(0.5f, 1.75f, 0.5f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 40.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-2.4f, 7.5f, -1.3f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//Set scale
	scaleXYZ = glm::vec3(0.5f, 2.90f, 0.5f);

	//Set the XYZ rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-1.4, 7.5f, 0.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	//Set scale
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(-1.33f, 7.5f, 0.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set scale
	scaleXYZ = glm::vec3(0.5f, 1.0f, 0.5f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 13.0f, 0.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();



	//Set scale
	scaleXYZ = glm::vec3(0.5f, 3.0f, 0.5f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 10.0f, 0.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//Set scale
	scaleXYZ = glm::vec3(0.5f, 1.0f, 0.5f);

	//Set the XYZ rotation
	XrotationDegrees = 45.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(3.2f, 9.25f, 0.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();


	//Set scale
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(4.0f, 10.0f, 0.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();


	//Set scale
	scaleXYZ = glm::vec3(0.5f, 2.75f, 0.5f);

	//Set the XYZ rotation
	XrotationDegrees = 45.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(1.45f, 7.5f, 0.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	
	//Set scale
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(1.45f, 7.5f, 0.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Draw the mesh
	m_basicMeshes->DrawSphereMesh();





	//********************Glass panels********************//
	//*******************MUST BE LAST*******************//

	//Set scale
	scaleXYZ = glm::vec3(14.0f, 13.0f, 0.1f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(0.0f, 7.5f, 3.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 0.2);
	SetShaderMaterial("glass");
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();


	//Set the scale
	scaleXYZ = glm::vec3(0.1f, 13.0f, 6.0f);

	//Set the XYZ rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	//Set the XYZposition
	positionXYZ = glm::vec3(7.0f, 7.5f, 0.0f);


	//Set the transformation into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 0.2);
	SetShaderMaterial("glass");
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();



}

