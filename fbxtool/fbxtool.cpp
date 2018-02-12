#include "stdafx.h"

#include "fbxtool.h"
#include <cstdio>
#include <stdlib.h>
#include <fbxsdk.h>
#include <map>
#include <iostream>
#include <fstream>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "Common.h"
#include "DisplayCommon.h"
#include "GeometryUtility.h"
#include "clara.hpp"
#include "tinydir.h"

using namespace clara;

const std::string physicsSuffix = "__phys";
const std::string ragdollSuffix = "__ragdoll";


struct SJointEnhancement
{
	std::string oldName;
	std::string newName;
	std::string physicsProxy;
	std::string ragdollProxy;
	std::string primitiveType;
	std::string parentNode;
};


std::map<std::string, SJointEnhancement> jointMap;
bool isVerbose { false };
bool applyMixamoFixes { false };


// Multiply a quaternion by a vector.

FbxVector4 QMulV(const FbxQuaternion& q, const FbxVector4& v)
{
	FbxVector4 out;
	FbxVector4 r2;

	r2.mData[0] = (q.mData[1] * v.mData[2] - q.mData[2] * v.mData[1]) + q.mData[3] * v.mData[0];
	r2.mData [1] = (q.mData[2] * v.mData[0] - q.mData[0] * v.mData[2]) + q.mData[3] * v.mData[1];
	r2.mData [2] = (q.mData[0] * v.mData[1] - q.mData[1] * v.mData[0]) + q.mData[3] * v.mData[2];

	out.mData[0] = (r2.mData[2] * q.mData[1] - r2.mData[1] * q.mData[2]);
	out.mData[0] += out.mData[0] + v.mData[0];
	out.mData[1] = (r2.mData[0] * q.mData[2] - r2.mData[2] * q.mData[0]);
	out.mData[1] += out.mData[1] + v.mData[1];
	out.mData[2] = (r2.mData[1] * q.mData[0] - r2.mData[0] * q.mData[1]);
	out.mData[2] += out.mData[2] + v.mData[2];

	return out;
}



void RenameSkeleton(FbxScene* pFbxScene, FbxNode* pFbxNode, std::string indexName, std::map<std::string, SJointEnhancement> jointMap)
{
	FbxSkeleton* lSkeleton = (FbxSkeleton*)pFbxNode->GetNodeAttribute();

	// See if we have a new name for the joint.
	if (jointMap.find(indexName) != jointMap.end())
	{
		FbxString stringName = jointMap [indexName].newName.c_str();

		if (isVerbose)
			DisplayString("NEW NAME: " + stringName);
		pFbxNode->SetName(stringName);
	}
	else
	{
		DisplayString("Name: ", pFbxNode->GetName());
	}

	DisplayInt("  Skeleton Type: ", lSkeleton->GetSkeletonType());

	auto transform = pFbxNode->LclTranslation.Get();
	Display3DVector("  Transform: ", transform);
	auto rotation = pFbxNode->LclRotation.Get();
	Display3DVector("  Rotation: ", rotation);

	auto preRotation = pFbxNode->PreRotation.Get();
	Display3DVector("  Pre-Rotation: ", preRotation);
	auto postRotation = pFbxNode->PostRotation.Get();
	Display3DVector("  Post-Rotation: ", postRotation);
	DisplayString("");

	FbxAMatrix& worldTM = pFbxNode->EvaluateGlobalTransform();
	Display3DVector("  World Rotation: ", worldTM.GetR());
	Display3DVector("  World Translation: ", worldTM.GetT());

	if (strcmp(pFbxNode->GetName(), "Hips") == 0)
	{
		// Zero the hips, Mixamo leaves them offset slightly.
		transform.mData [0] = 0.0f;
		transform.mData [2] = 0.0f;
		pFbxNode->LclTranslation.Set(transform);

		// Apply a rotation.
		//double temp = preRotation.mData [1];
		//preRotation.mData [1] = preRotation.mData [2];
		//preRotation.mData [2] = -temp;
		//pFbxNode->PreRotation.Set(preRotation);
	}
}


void EnhanceSkeleton(FbxScene* pFbxScene, FbxNode* pFbxNode, std::string indexName, std::map<std::string, SJointEnhancement> jointMap)
{
	FbxSkeleton* pFBXSkeleton = (FbxSkeleton*)pFbxNode->GetNodeAttribute();

	// Check to see if there is any enhancement data in the JSON file for this joint.
	if (jointMap.find(indexName) != jointMap.end())
	{
		//SJointEnhancement joint = jointMap [indexName];

		//// Physics, yo!
		//if (!joint.physicsProxy.empty())
		//{
		//	FBXSDK_printf("PHYSICS for %s\n", joint.newName.c_str());
		//	auto physName = joint.newName + physicsSuffix;
		//	FbxDouble3 trans { 0.0f, 0.0f, 0.0f };
		//	auto newNode = CreateCube(pFbxScene, physName.c_str(), trans);
		//	
		//	// Create a mesh object
		//	//FbxMesh* pMesh = FbxMesh::Create(pFbxScene, physName.c_str());

		//	// Add it to the scene.
		//	if (auto pFbxNode = pFbxScene->FindNodeByName(joint.newName.c_str()))
		//	{
		//		pFbxNode->AddChild(newNode);

		//		// Set the mesh as the node attribute of the node
		//		//pFbxNode->SetNodeAttribute(newNode);
		//	}
		//}

		// Ragdoll.
		//if (!joint.ragdollProxy.empty())
		//{
		//	FBXSDK_printf("RAGDOLL for %s\n", joint.newName.c_str());
		//	auto physName = joint.newName + ragdollSuffix;
		//	auto transform = pFbxNode->LclTranslation.Get();
		//	auto newNode = CreateCube(pFbxScene, physName.c_str(), transform);

		//	newNode->LclRotation.Set(pFbxNode->LclRotation.Get());
		//	newNode->PreRotation.Set(pFbxNode->PreRotation.Get());
		//	newNode->PostRotation.Set(pFbxNode->PostRotation.Get());

		//	// Add it to the scene.
		//	if (auto pFbxNode = pFbxScene->FindNodeByName(joint.parentNode.c_str()))
		//	{
		//		pFbxNode->AddChild(newNode);
		//	}
		//	else
		//	{
		//		FbxNode* sceneRootNode = pFbxScene->GetRootNode();

		//		if (sceneRootNode)
		//		{
		//			sceneRootNode->AddChild(newNode);
		//		}
		//	}
		//}
	}
}


void DoSkeletonStuff(FbxScene* pFbxScene, FbxNode* pFbxNode, std::map<std::string, SJointEnhancement> jointMap)
{
	if (jointMap.find(std::string(pFbxNode->GetName())) != jointMap.end())
	{
		// We're renaming the skeleton on the fly, so it's important to remember what the node 'was' called and use that for all lookups in the map.
		auto indexName = jointMap [std::string(pFbxNode->GetName())].oldName;

		// Rename to new skeleton standard.
		RenameSkeleton(pFbxScene, pFbxNode, indexName, jointMap);
		EnhanceSkeleton(pFbxScene, pFbxNode, indexName, jointMap);
	}
}


void InterateContent(FbxScene* pFbxScene, FbxNode* pFbxNode)
{
	FbxNodeAttribute::EType attributeType;

	if (pFbxNode->GetNodeAttribute() == nullptr)
	{
		FBXSDK_printf("NULL Node Attribute\n\n");
	}
	else
	{
		attributeType = (pFbxNode->GetNodeAttribute()->GetAttributeType());

		switch (attributeType)
		{
			case FbxNodeAttribute::eSkeleton:
				DoSkeletonStuff(pFbxScene, pFbxNode, jointMap);
				break;

			default:
				break;
		}
	}

	for (int i = 0; i < pFbxNode->GetChildCount(); i++)
	{
		InterateContent(pFbxScene, pFbxNode->GetChild(i));
	}
}


void InterateContent(FbxScene* pFbxScene)
{
	FbxNode* sceneRootNode = pFbxScene->GetRootNode();

	if (sceneRootNode)
	{
		for (int i = 0; i < sceneRootNode->GetChildCount(); i++)
		{
			InterateContent(pFbxScene, sceneRootNode->GetChild(i));
		}
	}
}


void DisplayMetaData(FbxScene* pFbxScene)
{
	FbxDocumentInfo* sceneInfo = pFbxScene->GetSceneInfo();

	if (sceneInfo)
	{
		FBXSDK_printf("\n\n--------------------\nMeta-Data\n--------------------\n\n");
		FBXSDK_printf("    Title: %s\n", sceneInfo->mTitle.Buffer());
		FBXSDK_printf("    Subject: %s\n", sceneInfo->mSubject.Buffer());
		FBXSDK_printf("    Author: %s\n", sceneInfo->mAuthor.Buffer());
		FBXSDK_printf("    Keywords: %s\n", sceneInfo->mKeywords.Buffer());
		FBXSDK_printf("    Revision: %s\n", sceneInfo->mRevision.Buffer());
		FBXSDK_printf("    Comment: %s\n", sceneInfo->mComment.Buffer());

		FbxThumbnail* thumbnail = sceneInfo->GetSceneThumbnail();
		if (thumbnail)
		{
			FBXSDK_printf("    Thumbnail:\n");

			switch (thumbnail->GetDataFormat())
			{
				case FbxThumbnail::eRGB_24:
					FBXSDK_printf("        Format: RGB\n");
					break;
				case FbxThumbnail::eRGBA_32:
					FBXSDK_printf("        Format: RGBA\n");
					break;
			}

			switch (thumbnail->GetSize())
			{
				default:
					break;
				case FbxThumbnail::eNotSet:
					FBXSDK_printf("        Size: no dimensions specified (%ld bytes)\n", thumbnail->GetSizeInBytes());
					break;
				case FbxThumbnail::e64x64:
					FBXSDK_printf("        Size: 64 x 64 pixels (%ld bytes)\n", thumbnail->GetSizeInBytes());
					break;
				case FbxThumbnail::e128x128:
					FBXSDK_printf("        Size: 128 x 128 pixels (%ld bytes)\n", thumbnail->GetSizeInBytes());
			}
		}
	}
}


/**
Rename the first animation in the set. Use case: you have a bunch of single animation FBX files, but the internal
animations have names like 'Take 001' or 'Unreal Take'. This will rename only the first animation, but it will also
emit the names of any other animations as debug.

\param [in,out]	pFbxScene  If non-null, the scene.
\param 		   	newName The new name for the first animation take.
**/
void RenameFirstAnimation(FbxScene* pFbxScene, const char* newName)
{
	for (int i = 0; i < pFbxScene->GetSrcObjectCount<FbxAnimStack>(); i++)
	{
		FbxAnimStack* pFbxAnimStack = pFbxScene->GetSrcObject<FbxAnimStack>(i);

		FbxString lOutputString = "Animation Stack Name: ";
		lOutputString += "\nRenamed from ";
		lOutputString += pFbxAnimStack->GetName();
		lOutputString += " to ";
		lOutputString += newName;
		lOutputString += "\n\n";
		FBXSDK_printf(lOutputString);

		// Rename only the first animation.
		if (i == 0)
			pFbxAnimStack->SetName(newName);
	}
}


void ApplyMixamoFixes(FbxManager* pFbxManager, FbxScene* pFbxScene)
{
	FbxNode* sceneRootNode = pFbxScene->GetRootNode();

	// Fix stupid names.
	if (auto pFbxNode = pFbxScene->FindNodeByName("default"))
		pFbxNode->SetName("Eyes");
	if (auto pFbxNode = pFbxScene->FindNodeByName("Tops"))
		pFbxNode->SetName("Top");
	if (auto pFbxNode = pFbxScene->FindNodeByName("Bottoms"))
		pFbxNode->SetName("Bottom");

	// Add a new node and move the meshes onto it.
	FbxNode* meshNode = FbxNode::Create(pFbxScene, "Meshes");
	sceneRootNode->AddChild(meshNode);
	if (auto pFbxNode = pFbxScene->FindNodeByName("Body"))
		meshNode->AddChild(pFbxNode);
	if (auto pFbxNode = pFbxScene->FindNodeByName("Bottom"))
		meshNode->AddChild(pFbxNode);
	if (auto pFbxNode = pFbxScene->FindNodeByName("Eyes"))
		meshNode->AddChild(pFbxNode);
	if (auto pFbxNode = pFbxScene->FindNodeByName("Eyelashes"))
		meshNode->AddChild(pFbxNode);
	if (auto pFbxNode = pFbxScene->FindNodeByName("Hair"))
		meshNode->AddChild(pFbxNode);
	if (auto pFbxNode = pFbxScene->FindNodeByName("Top"))
		meshNode->AddChild(pFbxNode);
	if (auto pFbxNode = pFbxScene->FindNodeByName("Shoes"))
		meshNode->AddChild(pFbxNode);

	// We also need a 'RootProxy' for scaling at some point.
	FbxString rootProxyName("RootProxy");
	FbxNode* skeletonRootProxyNode = FbxNode::Create(pFbxScene, rootProxyName.Buffer());
	sceneRootNode->AddChild(skeletonRootProxyNode);

	// Add a new node called 'Root' to parent the existing skeleton onto.
	FbxString rootName("Root");
	FbxSkeleton* skeletonRootAttribute = FbxSkeleton::Create(pFbxScene, rootName);
	skeletonRootAttribute->SetSkeletonType(FbxSkeleton::eRoot);
	FbxNode* skeletonRootNode = FbxNode::Create(pFbxScene, rootName.Buffer());
	skeletonRootNode->SetNodeAttribute(skeletonRootAttribute);
	skeletonRootProxyNode->AddChild(skeletonRootNode);

	// Reparent the hips.
	if (auto pFbxNode = pFbxScene->FindNodeByName("Hips"))
	{
		skeletonRootNode->AddChild(pFbxNode);
	}
}


void AddNewJoint(FbxManager* pFbxManager, FbxScene* pFbxScene, const char* nodeName, const char* parentNodeName,
	int offsetType = 0,
	FbxVector4 offset = FbxVector4(0.0f, 0.0f, 0.0f),
	FbxQuaternion rotation = FbxQuaternion()
)
{
	FbxNode* sceneRootNode = pFbxScene->GetRootNode();

	// Add a new node  and parent it.
	FbxString newNodeName(nodeName);
	FbxSkeleton* skeletonRootAttribute = FbxSkeleton::Create(pFbxScene, newNodeName);
	skeletonRootAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);

	FbxNode* skeletonNode = FbxNode::Create(pFbxScene, newNodeName.Buffer());
	skeletonNode->SetNodeAttribute(skeletonRootAttribute);
	
	// The default offset is fine in most cases.
	skeletonNode->LclTranslation.Set(offset);

	// Parent the node.
	if (auto pParentNode = pFbxScene->FindNodeByName(parentNodeName))
	{
		// Foot plane weights.
		if (offsetType == 1)
		{
			// These are meant to point upwards and be approximately 100 units in length. Taking the inverse of the world
			// rotation and multiplying it by an up vector will give us a second vector pointing in the right direction. 
			FbxAMatrix& worldTM = pParentNode->EvaluateGlobalTransform();
			FbxQuaternion rot = worldTM.GetQ();
			rot.Inverse();			
			FbxVector4 newVector = QMulV(rot, FbxVector4 { 0.0f, 1.0f, 0.0f });
			skeletonNode->LclTranslation.Set(newVector * 100.0f);
		}

		// Foot target - should be at 0 on the Z axis.
		if (offsetType == 2)
		{
			// These are meant to point straight down and be level with the ground plane. Taking the inverse of the world
			// rotation and multiplying it by an down vector will give us a second vector pointing in the right direction. Just
			// multiply that by the world height of the foot and you're set. 
			FbxAMatrix& worldTM = pParentNode->EvaluateGlobalTransform();
			FbxQuaternion rot = worldTM.GetQ();
			rot.Inverse();
			FbxVector4 trans = worldTM.GetT();
			double length = trans.mData [1];
			FbxVector4 newVector = QMulV(rot, FbxVector4 { 0.0f, -1.0f, 0.0f });
			skeletonNode->LclTranslation.Set(newVector * length);
		}

		// Add this to it's proper parent.
		pParentNode->AddChild(skeletonNode);
	}
}


void AddIkJoints(FbxManager* pFbxManager, FbxScene* pFbxScene)
{
	// Foot planting.
	AddNewJoint(pFbxManager, pFbxScene, "Bip01 planeTargetRight", "RightFoot", 2, FbxVector4(0.0f, 0.0f, 0.0f));
	AddNewJoint(pFbxManager, pFbxScene, "Bip01 planeWeightRight", "RightFoot", 1, FbxVector4(0.0f, 100.0f, 0.0f));
	AddNewJoint(pFbxManager, pFbxScene, "Bip01 planeTargetLeft", "LeftFoot", 2, FbxVector4(0.0f, 0.0f, 0.0f));
	AddNewJoint(pFbxManager, pFbxScene, "Bip01 planeWeightLeft", "LeftFoot", 1, FbxVector4(0.0f, 100.0f, 0.0f));

	// Hands - weapon bones and positioning IK.
	AddNewJoint(pFbxManager, pFbxScene, "RightHandIK", "RightHand", 0, FbxVector4(0.0f, 20.0f, 0.0f));
	AddNewJoint(pFbxManager, pFbxScene, "LeftHandIK", "LeftHand", 0, FbxVector4(0.0f, 20.0f, 0.0f));

	// Looking.
	AddNewJoint(pFbxManager, pFbxScene, "Bip01 Look", "Head", 0, FbxVector4(0.0f, 0.0f, 6.0f));

	// Camera. Just place it approximately where it typically goes for now.
	AddNewJoint(pFbxManager, pFbxScene, "Camera", "Head", 0, FbxVector4(0.0f, 8.3f, 7.4f));
}


bool ProcessFile(FbxManager* pFbxManager, FbxScene* pFbxScene, FbxString fbxInFilePath, FbxString fbxOutFilePath)
{
	bool result = false;

	// Load the scene if there is one.
	if (!fbxInFilePath.IsEmpty())
	{
		FBXSDK_printf("\n\nFile: %s\n\n", fbxInFilePath.Buffer());

		if (LoadScene(pFbxManager, pFbxScene, fbxInFilePath))
		{
			// Switch to a Z-Up co-ordinate system.
			FbxAxisSystem max;
			max.ConvertScene(pFbxScene);

			// Display the scene.
			DisplayMetaData(pFbxScene);
			InterateContent(pFbxScene);

			if (applyMixamoFixes)
				ApplyMixamoFixes(pFbxManager, pFbxScene);

			// Add a set of joints for IK management.
			AddIkJoints(pFbxManager, pFbxScene);

			// We really only want the base part of the filename. This code is windows specific and MS compiler specific.
			char fname [255];
			char ext [20];
			_splitpath_s(fbxInFilePath, nullptr, 0, nullptr, 0, fname, sizeof(fname), ext, sizeof(ext));

			// Optionally, rename the animation to the filename.
			RenameFirstAnimation(pFbxScene, fname);

			// Save a copy of the scene to a new file.
			result = SaveScene(pFbxManager, pFbxScene, fbxOutFilePath);

			if (result == false)
				FBXSDK_printf("\n\nAn error occurred while saving the scene...\n");
		}
		else
		{
			FBXSDK_printf("\n\nAn error occurred while loading the scene...");
		}
	}
	else
	{
		FBXSDK_printf("\n\nUsage: ImportScene <FBX file name>\n\n");
	}

	return result;
}


int ReadJointFile(std::string &jointMetaFilePath)
{
	// Read joints file.
	std::ifstream jointStream(jointMetaFilePath);
	std::stringstream lineStream;
	if (jointStream)
	{
		lineStream << jointStream.rdbuf();
		jointStream.close();
	}
	std::string jointString = lineStream.str();

	rapidjson::Document jointJSONDocument;

	if (jointJSONDocument.Parse(jointString.c_str()).HasParseError())
	{
		auto error = jointJSONDocument.GetParseError();
		std::cerr << "JSON Read Error: " << error << std::endl;
		return 1;
	}
	else
	{
		const rapidjson::Value& joints = jointJSONDocument ["joints"];
		assert(joints.IsArray());
		for (rapidjson::SizeType i = 0; i < joints.Size(); i++)
		{
			assert(joints [i].IsObject());

			SJointEnhancement newJoint;

			// Load up our map with the data in the JSON file.
			newJoint.oldName = joints [i] ["old-name"].GetString();
			newJoint.newName = joints [i] ["new-name"].GetString();

			if (joints [i].HasMember("physics-proxy"))
			{
				newJoint.physicsProxy = joints [i] ["physics-proxy"].GetString();
			}

			if (joints [i].HasMember("ragdoll-proxy"))
			{
				newJoint.ragdollProxy = joints [i] ["ragdoll-proxy"].GetString();
			}

			if (joints [i].HasMember("primitive-type"))
			{
				newJoint.primitiveType = joints [i] ["primitive-type"].GetString();
			}

			if (joints [i].HasMember("parent-node"))
			{
				newJoint.parentNode = joints [i] ["parent-node"].GetString();
			}

			jointMap [newJoint.oldName] = newJoint;
		}
	}

	return 0;
}


int main(int argc, char** argv)
{
	bool didEverythingSucceed { true };
	bool isBulk { false };
	std::string inFilePath;
	std::string outFilePath;
	std::string jointMetaFilePath;

	int width = 0;
	std::string name;
	bool doIt = false;
	std::string command;

	auto cli =
		Opt(inFilePath, "input path")
		["-i"] ["--input-files"]
		("path to the input file(s)")
		| Opt(outFilePath, "output path")
		["-o"] ["--output-files"]
		("path for the output file(s)")
		| Opt(isBulk) ["-b"] ["--bulk"]
		("Bulk process more than one file?")
		| Opt(isVerbose)
		["-v"] ["--verbose"]("Output verbose information")
		| Opt(jointMetaFilePath, "Joint meta file")
		["-j"] ["--joints"]
		| Opt(applyMixamoFixes)
		["-f"] ["--fixamo"]
		("Apply fixes to Mixamo model");

	auto result = cli.parse(Args(argc, argv));
	if (!result) {
		std::cerr << "Error in command line: " << result.errorMessage() << std::endl;
		cli.writeToStream(std::cout);
		exit(1);
	}

	FbxString fbxInFilePath { inFilePath.c_str() };
	FbxManager* pFbxManager = nullptr;
	FbxScene* pFbxScene = nullptr;

	// Prepare the FBX SDK.
	InitializeSdkObjects(pFbxManager, pFbxScene);
	FBXSDK_printf("\n");

	if (jointMetaFilePath.length() > 0)
	{
		if (int error = ReadJointFile(jointMetaFilePath) != 0)
		{
			std::cerr << "Joint file was mal-formed JSON." << std::endl;
			exit(error);
		}
	}

	if (!isBulk)
	{
		// Default output is to the same file as the input.
		FbxString fbxOutFilePath;
		if (outFilePath.length() > 0)
			fbxOutFilePath = outFilePath.c_str();
		else
			fbxOutFilePath = fbxInFilePath;

		// There's only one file to process.
		didEverythingSucceed = didEverythingSucceed && ProcessFile(pFbxManager, pFbxScene, fbxInFilePath, fbxOutFilePath);
	}
	else
	{
		bool bAbort = false;
		if (outFilePath.length() == 0)
		{
			std::cerr << "Error: You must supply an output path for bulk conversions." << std::endl;
			cli.writeToStream(std::cout);
			exit(1);
		}

		// Default to using the present directory if no file path was supplied.
		TCHAR dirPath [255];
		size_t pReturnValue;
		if (fbxInFilePath.IsEmpty())
			lstrcpyW(dirPath, L".");
		else
			mbstowcs_s(&pReturnValue, dirPath, inFilePath.c_str(), inFilePath.length());

		tinydir_dir dir;
		if (tinydir_open(&dir, dirPath) != -1)
		{
			while ((dir.has_next) && (!bAbort))
			{
				tinydir_file file;
				if (tinydir_readfile(&dir, &file) != -1)
				{
					if (lstrcmpiW(file.extension, L"FBX") == 0)
					{
						printf("%ls\n", file.name);
						char inFilePath [255];
						wcstombs_s(&pReturnValue, inFilePath, file.name, wcslen(file.name));

						// Root of the output path.
						FbxString fbxOutFilePath { outFilePath.c_str() };
						fbxOutFilePath.Append(inFilePath, strlen(inFilePath));

						ProcessFile(pFbxManager, pFbxScene, inFilePath, fbxOutFilePath);
					}

					if (tinydir_next(&dir) == -1)
					{
						FBXSDK_printf("Error getting next file");
						bAbort = true;
					}
				}
				else
				{
					FBXSDK_printf("Error getting file");
					bAbort = true;
				}
			}
		}
		else
		{
			FBXSDK_printf("Folder not found.");
		}

		tinydir_close(&dir);
	}

	// Destroy all objects created by the FBX SDK.
	FBXSDK_printf("\n");
	DestroySdkObjects(pFbxManager, didEverythingSucceed);

	return 0;
}
