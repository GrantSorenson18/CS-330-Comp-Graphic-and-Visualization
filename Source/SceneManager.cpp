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

void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;
	
	//bReturn = CreateGLTexture("resources/textures/Plastic.jpg",
	//	"ClockBase");
	//bReturn = CreateGLTexture("resources/textures/Gems.jpg",
	//	"Gems");
	//bReturn = CreateGLTexture("resources/textures/Gold.jpg",
	//	"Gold");
	//bReturn = CreateGLTexture("resources/textures/Wood.jpg",
	//	"Wood");
	bReturn = CreateGLTexture("resources/textures/ExerciseTape.jpg",
		"Ball");
	bReturn = CreateGLTexture("resources/textures/Glass.jpg",
		"Glass");
	bReturn = CreateGLTexture("resources/textures/BrownPlastic.jpg",
		"BrownPlastic");
	bReturn = CreateGLTexture("resources/textures/GreenScreen.jpg",
		"GreenScreen");
	bReturn = CreateGLTexture("resources/textures/Book.jpg",
		"Book");
	bReturn = CreateGLTexture("resources/textures/RedPlasticTop.jpg",
		"RedTop");


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
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
	//I will recreate the timer, which will require a box and prism in its most basic form
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();


}

/***********************************************************
* DefineObjectMaterials()
 *
* This method is used for configuring the various material
* settings for all of the objects in the 3D scene.
***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	//Base Plane object material
	OBJECT_MATERIAL baseMaterial;
	baseMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	baseMaterial.ambientStrength = 0.4f;
	baseMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	baseMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	baseMaterial.shininess = 0.0;
	baseMaterial.tag = "Base";
	m_objectMaterials.push_back(baseMaterial);

	//Ball object material
	OBJECT_MATERIAL tapeMaterial;
	tapeMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
	tapeMaterial.ambientStrength = 0.4f;
	tapeMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	tapeMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	tapeMaterial.shininess = 0.0;
	tapeMaterial.tag = "Tape";
	m_objectMaterials.push_back(tapeMaterial);

	//Plastic object material
	//must be brown (0.259, 0.18, 0.027)
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.259f, 0.18f, 0.027);
	plasticMaterial.ambientStrength = 0.4f;
	plasticMaterial.diffuseColor = glm::vec3(0.522f, 0.369f, 0.059f);
	plasticMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	plasticMaterial.shininess = 3.0;
	plasticMaterial.tag = "Plastic";
	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL redTopMaterial;
	redTopMaterial.ambientColor = glm::vec3(0.259f, 0.18f, 0.027);
	redTopMaterial.ambientStrength = 0.4f;
	redTopMaterial.diffuseColor = glm::vec3(0.522f, 0.369f, 0.059f);
	redTopMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	redTopMaterial.shininess = 3.0;
	redTopMaterial.tag = "Red";
	m_objectMaterials.push_back(redTopMaterial);

	//Clock Screen object material(wood for now)
	OBJECT_MATERIAL screenMaterial;
	screenMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	screenMaterial.ambientStrength = 0.4f;
	screenMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	screenMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	screenMaterial.shininess = 1.0;
	screenMaterial.tag = "Screen";
	m_objectMaterials.push_back(screenMaterial);

	//Book object material
	OBJECT_MATERIAL BookMaterial;
	BookMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
	BookMaterial.ambientStrength = 0.4f;
	BookMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	BookMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	BookMaterial.shininess = 0.0;
	BookMaterial.tag = "BookFace";
	m_objectMaterials.push_back(BookMaterial);
}

/*
* SetupSceneLights()
* 
* This method is used for setting up scene lights by providing property values for individual
* light sources.
*/
void SceneManager::SetupSceneLights()
{
	//First scene light, white light hovering above scene
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 5.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.75f);

	m_pShaderManager->setBoolValue("bUseLighting", true);


}


/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	//Plane that the items sit on
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

	SetShaderTexture("Glass");
	SetShaderMaterial("Base");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

/*CLOCK START*/
	// The box is a rectangle whos long side faces the camera, so augment size respectivly
	scaleXYZ = glm::vec3(6.0f, 2.0f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//The timer is the object closest to the camera in the sceene, so place it slightly forward on the z
	positionXYZ = glm::vec3(0.0f, 1.0f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//most of the timer is blue, so apply color accordingly
	//SetShaderColor(0, 0, 1, 1);
	SetShaderTexture("BrownPlastic");
	SetShaderMaterial("Plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// The prism must match the lengh of the box, which is they coordiante. the edge needs to make a 90 degree angle with the ground, because this cant be done with the prism at the current 
	//moment i will elongate the edge to make it appear as if it touches the ground at a slant from the cameras perspective
	scaleXYZ = glm::vec3(1.2f, 6.0f, 1.9f);

	// set the XYZ rotation for the mesh
	//we need to rotate the prism so it juts out toward the camera. this will require a 90 degree rotation on the Zto get it sideways, and a -115 degree roation on the x to face the edge
	XrotationDegrees = -105.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	//The timer is the object closest to the camera in the sceene, so place it slightly forward on the z
	positionXYZ = glm::vec3(0.0f, 1.10f, 6.25f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//make the prism a slightly lighter blue to differentiate the shapes
	SetShaderTexture("BrownPlastic");
	SetShaderMaterial("Plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();


	//The screen for the ball will be a box that clips into the prism and textured to look like the clock screen

	//clock screen should fit within the prism
	scaleXYZ = glm::vec3(3.0f, 1.5f, 0.5f);

	//Make the rotation fit the prism surface
	XrotationDegrees = 60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the screen slightly inside the clock
	positionXYZ = glm::vec3(0.0f, 1.10f, 6.25f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set teh screen texture and material
	SetShaderTexture("GreenScreen");
	SetShaderMaterial("Screen");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/*CLOCK END*/

	/*
	* EXCERCISE BALL START
	*/
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the ball behind the timer and adjust for scale 
	positionXYZ = glm::vec3(-1.0f, 2.0f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set mesh shader texture and material
	SetShaderTexture("Ball");
	SetShaderMaterial("Tape");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	//EXCERCISE BALL END

	/*BOOK START*/
	//The book should be the largest element
	scaleXYZ = glm::vec3(7.0f, 7.0f, 2.0f);

	//Book should lay flat and have 90 rotation on x
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Place the book in the back right of the scene
	positionXYZ = glm::vec3(6.0f, 1.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set teh screen texture and material
	SetShaderTexture("Book");
	SetShaderMaterial("BookFace");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/*BOOK END*/

	/*PEANUT BUTTER JAR START*/
	//peanut butter jar Base
	scaleXYZ = glm::vec3(2.0f, 3.0f, 2.0f);

	//Book should lay flat and have 90 rotation on x
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Place the book in the back right of the scene
	positionXYZ = glm::vec3(-6.5f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set teh screen texture and material
	SetShaderTexture("BrownPlastic");
	SetShaderMaterial("Plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

//peanut butter jar Top
	scaleXYZ = glm::vec3(2.0f, 1.0f, 2.0f);

	//Book should lay flat and have 90 rotation on x
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Place the book in the back right of the scene
	positionXYZ = glm::vec3(-6.5f, 3.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set teh screen texture and material
	SetShaderTexture("RedTop");
	SetShaderMaterial("Plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/*PEANUT BUTTER JAR END*/
}
