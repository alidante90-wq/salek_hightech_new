# SALEK HIGHTECH — GitHub Windows Build

## 1. Create the repository

Create a new GitHub repository, for example:

`salek-hightech`

Upload **all files and folders** from this project.

Important: `.github/workflows/build-windows.yml` must be included.

## 2. Run the build

Open:

`Actions` → `Build SALEK HIGHTECH (Windows)` → `Run workflow`

GitHub will use a Windows 2022 runner and build:

- VST3
- Standalone EXE

## 3. Download the result

After the workflow succeeds:

`Actions` → the completed run → `Artifacts`

Download:

`SALEK-HIGHTECH-Windows.zip`

## 4. Create a release

To automatically create a GitHub Release, push a version tag:

`v0.1.0`

The workflow will attach:

`SALEK-HIGHTECH-Windows.zip`

to the release.

## Note about JUCE

CMakeLists.txt currently fetches JUCE from GitHub using FetchContent.
The Windows runner therefore needs internet access during configuration.
