# Fast Non-Intrusive File Indexing

## Features
### Must have
- Fast: reduce needed bandwidth if used to access a distant server
- Non-intrusive: files/folders should not be modified (especially their metadata)
- Easily expandable: add features, formats, networks connectivity and so on
- Handle complex filtering and sorting
- Keep it simple

### Could be great
- Preview: low-quality rendering to reduce bandwidth for fast glance
- Folder and Album (Virtual Folder) handling
- Automatic Album: based on metada
- Handle multiple sources at the same time

## Class Diagram
<img src="https://raw.githubusercontent.com/abadiet/FNIFI/refs/heads/main/resources/architecture.svg?raw=true" style="max-width: 100%;">

## Dependencies
- [SXEval](https://github.com/abadiet/SXEval): S-expression interpreter (includded as a submodule)
- [Exiv2](https://exiv2.orgl): Image metadata library
- [Samba](https://www.samba.org): Server Message Block implementation
