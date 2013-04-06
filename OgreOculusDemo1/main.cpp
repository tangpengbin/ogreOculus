/// Copyright (C) 2013 Kojack
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
/// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
/// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
/// DEALINGS IN THE SOFTWARE.

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "ogre.h"
#include "../OgreOculus/OgreOculus.h"
#include "OIS.h"
#include "../OgreOculus/euler.h"

using namespace Ogre;

const float g_eyeHeight = 1.7f;
const float g_walkSpeed = 5.0f;
const float g_turnSpeed = 0.003f;

Ogre::Euler g_bodyOrientation;
Vector3 g_bodyPosition(0,0,0);
Quaternion g_headOrientation(1,0,0,0);
bool g_flying=false;
Ogre::Euler g_sinbadLook;
Ogre::Bone *g_sinbadHead;
OIS::InputManager *g_inputManager = 0;
OIS::Keyboard *g_keyboard = 0;
OIS::Mouse *g_mouse = 0;



INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
{
	try
	{
		// Create Root
	#ifdef _DEBUG
		Root *root = new Ogre::Root("plugins_d.cfg","config.cfg","ogre_d.log");
	#else
		Root *root = new Ogre::Root("plugins.cfg","config.cfg","ogre.log");
	#endif
		RenderWindow *window;

		// Display the configuration dialog
		if(!root->showConfigDialog())
		{
			// The user selected cancel, so clean up and exit
			delete root;
			return 1;
		}

		// Initialise the ogre root. This creates a window (or fullscreen)
		window = root->initialise(true, "Oculus");

		// Create a scene manager (the octree manager in this case)
		SceneManager *sceneManager = root->createSceneManager("OctreeSceneManager");

		// Add some paths to the resource group manager. This is where ogre will search for resources (meshes, textures, etc)
		Ogre::ResourceGroupManager::getSingleton().addResourceLocation("media/Sinbad.zip","Zip");
		Ogre::ResourceGroupManager::getSingleton().addResourceLocation("media/sibenik.zip","Zip");
		Ogre::ResourceGroupManager::getSingleton().addResourceLocation("media","FileSystem");
		Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

		Oculus oculus;
		oculus.setupOculus();
		oculus.setupOgre(sceneManager, window);

		oculus.getCameraNode()->setPosition(0.0f,1.7f,10.0f);

		OIS::ParamList pl;
		size_t windowHnd = 0;
		std::ostringstream windowHndStr;

		window->getCustomAttribute("WINDOW", &windowHnd);
		windowHndStr << windowHnd;
		pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

		g_inputManager = OIS::InputManager::createInputSystem( pl );

		g_keyboard = static_cast<OIS::Keyboard*>(g_inputManager->createInputObject( OIS::OISKeyboard, true ));
		g_mouse = static_cast<OIS::Mouse*>(g_inputManager->createInputObject( OIS::OISMouse, true ));

		// Add a light.
		Light *light = sceneManager->createLight("light");
		light->setType(Light::LT_DIRECTIONAL);
		light->setDirection(-0.577,-0.577,-0.577);
		light->setDiffuseColour(ColourValue::White);
		sceneManager->setAmbientLight(ColourValue(0.4f,0.4f,0.4f));

		// Add the level mesh.
		SceneNode *node;
		Entity *ent;

		ent = sceneManager->createEntity("sibenik.mesh");
		node = sceneManager->getRootSceneNode()->createChildSceneNode();
		node->attachObject(ent);

		ent = sceneManager->createEntity("sinbad.mesh");
		ent->getAnimationState("Dance")->setEnabled(true);
		node = sceneManager->getRootSceneNode()->createChildSceneNode();
		node->setScale(0.25f,0.25f,0.25f);
		node->setPosition(10.7, 1.25, 0);
		node->yaw(Ogre::Degree(90));
		node->attachObject(ent);

		g_bodyPosition = Vector3(15.8f, g_eyeHeight, 0.0f);
		g_bodyOrientation.yaw(Ogre::Degree(90));

		// Get the current time.
		unsigned long last = root->getTimer()->getMilliseconds();
		unsigned long now=last;
	
		// Loop while the ogre window is open
		while(!window->isClosed())
		{
			MSG msg;
			// Check for windows messages
			while( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			// Find out how much time has passed (delta time)
			last = now;
			now = root->getTimer()->getMilliseconds();
			float deltaT = (now-last)/1000.0f;

			g_keyboard->capture();
			g_mouse->capture();
			Vector3 translate(0,0,0);
			if(g_keyboard->isKeyDown(OIS::KC_W))
			{
				translate.z = -g_walkSpeed;
			}
			if(g_keyboard->isKeyDown(OIS::KC_S))
			{
				translate.z = g_walkSpeed;
			}
			if(g_keyboard->isKeyDown(OIS::KC_A))
			{
				translate.x = -g_walkSpeed;
			}
			if(g_keyboard->isKeyDown(OIS::KC_D))
			{
				translate.x = g_walkSpeed;
			}
			if(g_keyboard->isKeyDown(OIS::KC_ESCAPE))
			{
				break;
			}
			if(g_keyboard->isKeyDown(OIS::KC_1))
			{
				g_flying = false;
			}
			if(g_keyboard->isKeyDown(OIS::KC_2))
			{
				g_flying = true;
			}
			if(g_keyboard->isKeyDown(OIS::KC_R))
			{
				g_bodyOrientation.setPitch(Ogre::Radian(0)).setRoll(Ogre::Radian(0));
			}

			g_bodyOrientation.yaw(Radian(-g_mouse->getMouseState().X.rel*g_turnSpeed));
			if(!oculus.isOculusReady())
			{
				g_bodyOrientation.pitch(Radian(-g_mouse->getMouseState().Y.rel*g_turnSpeed));
				g_bodyOrientation.limitPitch(Ogre::Degree(90));
			}
			g_bodyPosition+= g_bodyOrientation*(translate*deltaT);

			// Add deltaT to the ninja's animation (animations don't update automatically)
			ent->getAnimationState("Dance")->addTime(deltaT);

			if(!g_flying)
			{
				g_bodyPosition.y = g_eyeHeight;
			}
			oculus.getCameraNode()->setPosition(g_bodyPosition);
			oculus.getCameraNode()->setOrientation(g_bodyOrientation.toQuaternion() * oculus.getOrientation());

			// Render the scene
			root->renderOneFrame();
			// Perform a 1ms sleep. This stops the app from hogging the cpu, and also stops some gfx cards (like my previous one) from overheating
			Sleep(1);
		}

		// Clean up and exit
		oculus.shutDownOgre();
		oculus.shutDownOculus();

		delete root;
	}
	catch( Ogre::Exception& e )
	{
		MessageBox( NULL, e.getFullDescription().c_str(), "An Ogre exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}
	catch(...)
	{
		MessageBox( NULL, "An exception has occured!", "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}
	return 0;
}
