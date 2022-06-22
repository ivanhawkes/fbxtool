# FBXTool

This is a tool I have adapted from [link another project](https://github.com/L0rdCha0s/FBXJointRenamer) to help me process animated characters and animations into a format that is more friendly for CRYENGINE to deal with.

A lot of sources for animations have unique names for the same basic set of bones, and this can add a lot of time to an otherwise automatic task.

At present it has a couple of simple functions:

* bulk rename bones, using a configuration file
* dump out a list of the bones, and their pre and post rotations
* rename the first animation in an FBX file so it matches the actual file name - I use this to bulk rename animations, since it can work in conjunction with a command line or regex based renaming utility.
* apply a couple of fixes specific to Mixamo characters

In progress is work on:

* adding IK bones to the skeleton (CRYENGINE SPECIFIC)
* adding properly named physics and ragdoll proxies (CRYENGINE SPECIFIC)

# Example Use

```
.\bin\x64\Debug\fbxtool.exe -i human_female.fbx -o renamed\human_female.fbx -v -j .\mixamo-to-autodesk.json -f
```

Reads an input file called human_female.fbx and outputs to a different folder a file with the same name. Don't try and output to the same folder, unless you give the file a new name!

Joint naming is taken from the mixamo-to-autodesk.json file. We apply fixes appropriate for Mixamo models. There are a couple of joint naming files already created. It's quite easy to make your own as well.

Run the program with with -h to see the command lines options on offer e.g.

```
.\bin\x64\Debug\fbxtool.exe -h
```

# Building the Code

I am using Visual Studio 2022 for the solution. It should work with a standard install of VS2022 as at 22 June 2022. NOTE: Some of the libraries have been updated from the older versions to match what I have installed with VS. It should be pretty simple to install those libraries if you are missing them, or switch to a different library for your project.

You will require a copy of the FBX SDK from Autodesk installed onto your machine. The project is using version 2018.1.1 but is code compatible with some older versions. You can either download and install [link FBX SDK 2018.1.1](https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2018-1-1) or use another version if you already have that installed. If you choose to use a different version you will need to change the project "VC++ Directories" include and library settings to include the header files and libraries.

By default, FBX SDK 2018.1.1 will want to install the libraries at:

```
C:\Program Files\Autodesk\FBX\FBX SDK\2018.0
```

I suggest you use the default installation location, as that is where I always install SDKs.

If you choose to install them elsewhere be sure to edit the project settings and make sure the "VC++ Include Directories" and ""VC++ Library Directories" " are set correctly. Mine are set to:

```
INCLUDES:     C:\Program Files\Autodesk\FBX\FBX SDK\2018.0\include;
LIBRARIES:    C:\Program Files\Autodesk\FBX\FBX SDK\2018.0\lib\vs2015\$(PlatformTarget)\$(Configuration);
```

