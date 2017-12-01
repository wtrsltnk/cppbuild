
#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <iostream>

#define PATH_SEPERATOR_CHR '\\'
#define PATH_SEPERATOR_STR "\\"
#define PATH_ALT_SEPERATOR_CHR '/'
#define PATH_ALT_SEPERATOR_STR "/"

namespace cppbuild
{
    bool buildScriptOn = false;
    bool installScriptOn = false;
    bool runScriptOn = false;
    bool cleanScriptOn = false;

    void init(int argc, char* argv[])
    {
        if (argc == 1) cppbuild::buildScriptOn = true;

        for (int i = 0; i < argc; i++)
        {
            if (std::string(argv[i]) == "build") cppbuild::buildScriptOn = true;
            if (std::string(argv[i]) == "install") cppbuild::installScriptOn = true;
            if (std::string(argv[i]) == "run") cppbuild::runScriptOn = true;
            if (std::string(argv[i]) == "clean") cppbuild::cleanScriptOn = true;
        }
    } 

    std::string pathCombine(std::initializer_list<const std::string> files)
    {
        std::string result;

        for (auto file : files)
        {
            if (result != "") result += PATH_ALT_SEPERATOR_STR;
            result += file;
        }

        std::replace(result.begin(), result.end(), PATH_ALT_SEPERATOR_CHR, PATH_SEPERATOR_CHR);

        return result;
    }

    void cleanPathSeperators(std::string& filename)
    {
        std::replace(filename.begin(), filename.end(), PATH_ALT_SEPERATOR_CHR, PATH_SEPERATOR_CHR);
    }

    std::string extractFilename(const std::string& fullPath)
    {
        std::string p = fullPath;
        cleanPathSeperators(p);
        return p.substr(p.find_last_of(PATH_SEPERATOR_CHR) + 1);
    }
    
    std::string extractFilepath(const std::string& fullPath)
    {
        std::string p = fullPath;
        cleanPathSeperators(p);
        return p.substr(0, p.find_last_of(PATH_SEPERATOR_CHR));
    }
    
    class Folder
    {
        std::string _folder;
        std::map<std::string, Folder> _folders;
        std::vector<std::string> _files;
    public:
        Folder(const std::string& folder)
            : _folder(folder)
        {
            cleanPathSeperators(this->_folder);

            auto split = this->_folder.find(PATH_SEPERATOR_CHR);
            if (split != std::string::npos)
            {
                auto rest = this->_folder.substr(split + 1);
                this->_folders.insert(std::make_pair(rest, Folder(rest)));
                this->_folder = this->_folder.substr(0, split);
            }
        }

        const std::map<std::string, Folder>& folders() const { return this->_folders; }
        const std::vector<std::string>& files() const { return this->_files; }

        Folder& files(std::initializer_list<const char*> files)
        {
            for (auto file : files) _files.push_back(file);

            return *this;
        }

        Folder& folder(const Folder& folder)
        {
            _folders.insert(std::make_pair(folder._folder, folder));

            return *this;
        }

        Folder& folder(const std::string& name, std::initializer_list<const char*> files)
        {
            _folders.insert(std::make_pair(name, Folder(name).files(files)));

            return *this;
        }

        // TODO test this in combo with folders, not sure it works
        Folder& operator += (const Folder& other)
        {
            for (auto file : other._files) this->_files.push_back(file);
            for (auto folder : other._folders)
            {
                auto result = _folders.insert(std::make_pair(folder.first, folder.second));
                if (!result.second) result.first->second += folder.second;
            }

            return *this;
        }

        std::vector<std::string> allFiles(const std::string& path = "")
        {
            auto localPath = path != "" ? pathCombine({ path, _folder }) : _folder;
            std::vector<std::string> result;
            for (auto file : _files) result.push_back(localPath != "" ? pathCombine({ localPath, file }) : file);
            for (auto folder : _folders)
            {
                auto files = folder.second.allFiles(localPath);
                for (auto file : files) result.push_back(file);
            }

            return result;
        }

        std::vector<std::string> allFolders(const std::string& path = "")
        {
            auto localPath = path != "" ? pathCombine({ path, _folder }) : _folder;
            std::vector<std::string> result;
            if (_folder != "") result.push_back(localPath);
            for (auto folder : _folders)
            {
                auto subFolders = folder.second.allFolders(localPath);
                for (auto subFolder : subFolders) result.push_back(subFolder);
            }

            return result;
        }

        void createAllFolders(const std::string& path = "")
        {
            // create all folders needed to compile to
            for (auto folder : this->allFolders(path))
            {
                std::cout << "echo Creating folder \"" << pathCombine({ "build", "obj", folder }) << "\"" << std::endl 
                          << "if not exist " << pathCombine({ "build", "obj", folder }) << "("
                          << "    mkdir " << pathCombine({ "build", "obj", folder }) << std::endl
                          << ")" << std::endl;
            }
        }
    };

    enum class TargetTypes
    {
        Executable,
        SharedLibrary,
        StaticLibrary
    };

    class Target : public Folder
    {
        std::string _project;
        TargetTypes _targetType;
        std::vector<std::string> _includeDirs;
        std::vector<std::string> _libraries;
        std::vector<std::string> _libraryDirs;
        std::string _installDir;
        std::string _targetInstallDir;
        std::map<std::string, Folder> _installFiles;
        std::string _compilerFlags;
        std::string _linkerFlags;

        std::string outputFile() const
        {
            if (this->_targetType == TargetTypes::Executable) return pathCombine({ "build", this->_project + ".exe" });
            if (this->_targetType == TargetTypes::SharedLibrary) return pathCombine({ "build", "lib" + this->_project + ".dll" });
            if (this->_targetType == TargetTypes::StaticLibrary) return pathCombine({ "build", "lib" + this->_project + ".a" });

            return pathCombine({ "build", this->_project });
        }

        std::string outputLibraries() const
        {
            std::string libraries;

            // Gather all library include directories into one string
            for (auto file : this->_libraries) libraries += " -l" + file;
            
            return libraries;
        }

        std::string outputLibraryDirs() const
        {
            std::string dirs;

            // Gather all library include directories into one string
            for (auto dir : this->_libraryDirs) dirs += " -L" + dir;
            
            return dirs;
        }

        std::string outputIncludeDirs() const
        {
            std::string dirs;

            // Gather all header include directories into one string
            for (auto dir : this->_includeDirs) dirs += " -I" + dir;
            
            return dirs;
        }

        std::string outputAllObjectFiles()
        {
            std::string objectFiles;

            // Add all object files
            for (auto file : this->allFiles()) objectFiles += std::string(" ") + pathCombine({ "build", "obj", file + ".o" });

            return objectFiles;
        }

        void outputCopyFile(const std::string& outputFile, const std::string& destinationPath)
        {
            std::cout << "echo Copying \"" << outputFile << "\"" << std::endl ;
            std::cout << "copy " << outputFile << " " << pathCombine({ destinationPath, extractFilename(outputFile) }) << std::endl;
        }

        void outputDeleteFile(const std::string& filename)
        {
            std::cout << "echo Removing \"" << filename << "\"" << std::endl ;
            std::cout << "del " << filename << std::endl;
        }

        void outputCompileFile(const std::string& file)
        {
            auto fileName = extractFilename(file);
            auto filePath = extractFilepath(file);
            
            std::cout << "echo Compiling \"" << file << "\"" << std::endl 
                      << "g++" << this->_compilerFlags << " " << file << " -c -o " << pathCombine({ "build", "obj", file + ".o" }) << this->outputIncludeDirs() << std::endl;
        }

        Target& generateBuildScript()
        {
            auto outputFile = this->outputFile();

            this->createAllFolders();

            // Compile all given files into their own object file
            for (auto file : this->allFiles()) this->outputCompileFile(file);
            
            if (_targetType == TargetTypes::Executable)
            {
                std::cout << "echo Linking executable \"" << outputFile << "\"" << std::endl 
                          << "g++" << this->_linkerFlags << " -o " << outputFile << this->outputAllObjectFiles() << this->outputLibraryDirs() << this->outputLibraries() << std::endl;
            }
            else if (_targetType == TargetTypes::SharedLibrary)
            {
                std::cout << "echo Linking shared library \"" << outputFile << "\"" << std::endl
                          << "g++" << this->_linkerFlags << " -shared -o " << outputFile << this->outputAllObjectFiles() << this->outputLibraryDirs() << this->outputLibraries() << std::endl;
            }
            else if (_targetType == TargetTypes::StaticLibrary)
            {
                std::cout << "echo Linking static library \"" << outputFile << "\"" << std::endl
                          << "ar rvs " << outputFile << this->outputAllObjectFiles() << std::endl;
            }
        }

        void generateInstallScript()
        {
            auto outputFile = this->outputFile();
            
            // Determine target destination and create all folders needed for that
            auto fullTargetDestination = pathCombine({ _installDir, this->_targetInstallDir });

            // Copy the target file(exe/a/dll) to the install folder
           this->outputCopyFile(outputFile, fullTargetDestination); 
            
            // For all destinations, copy the install files
            for (auto pair : _installFiles)
            {
                // Determine the destination for this file list and create all folders for this
                auto fullDestination = pathCombine({ _installDir, pair.first });

                // Copy all files for this destination
                auto files = pair.second.allFiles();
                for (auto file : files)
                {
                    this->outputCopyFile(file, fullDestination); 
                }
            }
        }

        void generateRunScript()
        {
            if (this->_targetType == TargetTypes::Executable)
            {
                // Run the executable
                std::cout << "call " << this->outputFile() << std::endl;
            }
            else
            {
                // Show a message that the user is trying to run the target executable
                std::cout << "echo This target is not an executable." << std::endl; 
            }
        }

        void generateCleanScript()
        {
            // Remove all the independant object files
            for (auto file : this->allFiles()) this->outputDeleteFile(pathCombine({ "build", "obj", extractFilename(file) + ".o" }));

            // Remove the target file
            this->outputDeleteFile(this->outputFile());
        }

    public:
        Target(const std::string& name, TargetTypes targetType = TargetTypes::Executable)
            : Folder(""), _project(name), _targetType(targetType), _installDir(""), 
                _compilerFlags(" --std=c++11"), _linkerFlags("")
        { }

        virtual ~Target()
        {
            std::cout << "echo off"<< std::endl;
            if (buildScriptOn)
            {
                std::cout << "echo." << std::endl << "echo Executing build script for target \"" << this->_project << "\"" << std::endl;
                generateBuildScript();
            }
            if (installScriptOn)
            {
                std::cout << "echo." << std::endl << "echo Executing install script for target \"" << this->_project << "\"" << std::endl;
                generateInstallScript();
            }
            if (runScriptOn)
            {
                std::cout << "echo." << std::endl << "echo Executing run script for target \"" << this->_project << "\"" << std::endl;
                generateRunScript();
            }
            if (cleanScriptOn)
            {
                std::cout << "echo." << std::endl << "echo Executing clean script for target \"" << this->_project << "\"" << std::endl;
                generateCleanScript();
            }
        }

        Target& includeDirs(std::initializer_list<const char*> dirs)
        {
            for (auto dir : dirs) this->_includeDirs.push_back(dir);

            return *this;
        }

        Target& libraries(std::initializer_list<const char*> files)
        {
            for (auto file : files) this->_libraries.push_back(file);

            return *this;
        }

        Target& libraryDirs(std::initializer_list<const char*> dirs)
        {
            for (auto dir : dirs) this->_libraryDirs.push_back(dir);

            return *this;
        }

        Target& installDir(const std::string& folder)
        {
            this->_installDir = folder;
            cleanPathSeperators(this->_installDir);

            return *this;
        }

        Target& installFiles(const std::string& destination, std::initializer_list<const char*> files)
        {
            return installFiles(destination, Folder("").files(files));
        }

        Target& installTargetInto(const std::string& target)
        {
            this->_targetInstallDir = target;
            cleanPathSeperators(this->_targetInstallDir);

            return *this;
        }

        Target& installFiles(const std::string& destination, const Folder& folder)
        {
            auto dest = destination;
            cleanPathSeperators(dest);

            auto result = this->_installFiles.insert(std::make_pair(dest, folder));

            if (!result.second) result.first->second += folder;

            return *this;
        }

        Target& compilerFlags(std::initializer_list<const char*> flags)
        {
            for (auto flag : flags) this->_compilerFlags += std::string(" ") + flag;

            return *this;
        }

        Target& linkerFlags(std::initializer_list<const char*> flags)
        {
            for (auto flag : flags) this->_linkerFlags += std::string(" ") + flag;

            return *this;
        }
    };

} // namespace cppbuild
