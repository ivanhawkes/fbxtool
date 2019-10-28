# FBXTool

This is a tool I have adapted from [link another project](https://github.com/L0rdCha0s/FBXJointRenamer) to help me process animated characters and animations into a format that is more friendly for CRYENGINE to deal with.

At present it has a couple of simple functions:

* bulk rename bones, using a configuration file
* dump out a list of the bones, and their pre and post rotations
* rename the first animation in an FBX file so it matches the actual file name - I use this to bulk rename animations, since it can work in conjunction with a command line or regex based renaming utility.
* apply a couple of fixes specific to Mixamo characters

In progress is work on:

* adding IK bones to the skeleton
* adding properly named physics and ragdoll proxies

# Example Use

```
.\bin\x64\Debug\fbxtool.exe -i human_female.fbx -o renamed\human_female.fbx -v -j .\mixamo-to-autodesk.json -f
```

Reads an input file called human_female.fbx and outputs to a different folder a file with the same name. Don't try and output to the same folder, unless you give the file a new name!

Joint naming is taken from the mixamo-to-autodesk.json file. We apply fixes appropriate for Mixamo models.

Run the program with with -h to see the command lines options on offer e.g.

```
.\bin\x64\Debug\fbxtool.exe -h
```

# Building the Code

I am using Visual Studio 2017 for the solution, though I have set the project to use settings suitable for Visual Studio 2015 users to make things a little easier for people who haven't upgraded yet.

You will require a copy of the FBX SDK from Autodesk installed onto your machine. The project is using version 2018.1.1 but is compatible with some other versions. You can either download and install [link FBX SDK 2018.1.1](https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2018-1-1) or use another version if you already have that installed. If you choose to use a different version you will need to change the project "VC++ Directories" to include the install folder for your version.
