# Fast Non-Intrusive File Indexing

## Features
### Must have
- [x] Fast: reduce needed bandwidth if used to access a distant server
- [x] Non-intrusive: files/folders should not be modified (especially their metadata)
- [x] Easily expandable: add features, formats, networks connectivity and so on
- [x] Handle complex filtering and sorting
- [x] Keep it simple

### Could be great
- [x] Preview: low-quality rendering to reduce bandwidth for fast glance
- [ ] Folder and Album (Virtual Folder) handling
- [ ] Automatic Album: based on metada
- [x] Handle multiple sources at the same time

## Class Diagram
<img src="https://raw.githubusercontent.com/abadiet/FNIFI/refs/heads/main/resources/architecture.svg?raw=true" style="max-width: 100%;">

## Dependencies
- [SXEval](https://github.com/abadiet/SXEval): S-expression interpreter (don't need to be installed as it is includded as a submodule)
- [Exiv2](https://exiv2.orgl): Image metadata library
- [Samba](https://www.samba.org): Server Message Block implementation
- [OpenCV](https://opencv.org/)

## Examples
Check the [examples](https://github.com/abadiet/FNIFI/tree/main/examples) folder or the Swift application [FNIFI-Apple](https://github.com/abadiet/FNIFI-Apple) for more.
