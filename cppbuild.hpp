
#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <regex>

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
    bool testScriptOn = false;
    std::string targetToUse;

    void init(int argc, char* argv[])
    {
        if (argc == 1)
        {
            cppbuild::buildScriptOn = true;
        }
        else if (argc > 1)
        {
            if (std::string(argv[1]) == "build") cppbuild::buildScriptOn = true;
            if (std::string(argv[1]) == "install") cppbuild::installScriptOn = true;
            if (std::string(argv[1]) == "run") cppbuild::runScriptOn = true;
            if (std::string(argv[1]) == "clean") cppbuild::cleanScriptOn = true;
            if (std::string(argv[1]) == "test") cppbuild::testScriptOn = true;
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
    protected:
        virtual const std::string buildRoot() const
        {
            return "build";
        }

    public:
        Folder(const std::string& folder)
            : _folder(folder)
        {
            cleanPathSeperators(_folder);

            auto split = _folder.find(PATH_SEPERATOR_CHR);
            if (split != std::string::npos)
            {
                auto rest = _folder.substr(split + 1);
                _folders.insert(std::make_pair(rest, Folder(rest)));
                _folder = _folder.substr(0, split);
            }
        }

        const std::map<std::string, Folder>& folders() const { return _folders; }
        const std::vector<std::string>& files() const { return _files; }

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
            for (auto file : other._files) _files.push_back(file);
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

        void createFolder(const std::string& path)
        {
            std::cout << "if not exist " << path << " ("
                      << "    echo Creating folder \"" << path << "\"\n" 
                      << "    mkdir " << path << "\n"
                      << ")\n";
        }

        void createAllFolders(const std::string& path = "")
        {
            // create all folders needed to compile to
            for (auto folder : allFolders(path))
            {
                createFolder(pathCombine({ buildRoot(), "obj", folder }));
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
            if (_targetType == TargetTypes::Executable) return pathCombine({ buildRoot(), _project + ".exe" });
            if (_targetType == TargetTypes::SharedLibrary) return pathCombine({ buildRoot(), "lib" + _project + ".dll" });
            if (_targetType == TargetTypes::StaticLibrary) return pathCombine({ buildRoot(), "lib" + _project + ".a" });

            return pathCombine({ buildRoot() });
        }

        std::string outputLibraries() const
        {
            std::string libraries;

            // Gather all library include directories into one string
            for (auto file : _libraries) libraries += " -l" + file;
            
            return libraries;
        }

        std::string outputLibraryDirs() const
        {
            std::string dirs;

            // Gather all library include directories into one string
            for (auto dir : _libraryDirs) dirs += " -L" + dir;
            
            return dirs;
        }

        std::string outputIncludeDirs() const
        {
            std::string dirs;

            // Gather all header include directories into one string
            for (auto dir : _includeDirs) dirs += " -I" + dir;
            
            return dirs;
        }

        std::string outputAllObjectFiles()
        {
            std::string objectFiles;

            // Add all object files
            for (auto file : allFiles())
            {
                std::string objfile = pathCombine({ buildRoot(), "obj", file + ".o" });
                objfile = std::regex_replace(objfile, std::regex("\\.\\."), "__");

                objectFiles += std::string(" ") + objfile;
            }

            return objectFiles;
        }

        void outputCopyFile(const std::string& outputFile, const std::string& destinationPath)
        {
            std::cout << "echo Copying \"" << outputFile << "\"\n" ;
            std::cout << "copy " << outputFile << " " << pathCombine({ destinationPath, extractFilename(outputFile) }) << "\n";
        }

        void outputDeleteFile(const std::string& filename)
        {
            std::cout << "echo Removing \"" << filename << "\"\n" ;
            std::cout << "del " << filename << "\n";
        }

        void outputCompileFile(const std::string& file, int percentage)
        {
            std::string objfile = pathCombine({ buildRoot(), "obj", file + ".o" });
            objfile = std::regex_replace(objfile, std::regex("\\.\\."), "__");

            std::cout << "echo [" << std::setfill(' ') << std::setw(3) << percentage << "%%] Compiling \"" << file << "\"\n" 
                      << "g++" << _compilerFlags << " " << file << " -c -o " << objfile << outputIncludeDirs() << "\n";
        }

        void generateBuildScript()
        {
            // Gather all folders that need to be created
            std::set<std::string> foldersToCreate;
            for (auto folder : allFolders())
            {
                foldersToCreate.insert(pathCombine({ buildRoot(), "obj", folder }));
            }

            for (auto file : allFiles())
            {
                std::string objfile = extractFilepath(pathCombine({ buildRoot(), "obj", file }));
                foldersToCreate.insert(std::regex_replace(objfile, std::regex("\\.\\."), "__"));
            }

            for (auto folder : foldersToCreate)
            {
                createFolder(folder);
            }

            // Compile all given files into their own object file
            for (int i = 0; i < allFiles().size(); i++)
            {
                outputCompileFile(allFiles()[i], int((double(i + 1) / double(allFiles().size() + 1)) * 100));
            }

            if (_targetType == TargetTypes::Executable)
            {
                std::cout << "echo [" << std::setfill(' ') << std::setw(3) << 100 << "%%] Linking executable \"" << outputFile() << "\"\n" 
                          << "g++" << _linkerFlags << " -o " << outputFile() << outputAllObjectFiles() << outputLibraryDirs() << outputLibraries() << "\n";
            }
            else if (_targetType == TargetTypes::SharedLibrary)
            {
                std::cout << "echo Linking shared library \"" << outputFile() << "\"\n"
                          << "g++" << _linkerFlags << " -shared -o " << outputFile() << outputAllObjectFiles() << outputLibraryDirs() << outputLibraries() << "\n";
            }
            else if (_targetType == TargetTypes::StaticLibrary)
            {
                std::cout << "echo Linking static library \"" << outputFile() << "\"\n"
                          << "ar rvs " << outputFile() << outputAllObjectFiles() << "\n";
            }
        }

        void generateInstallScript()
        {
            // Determine target destination and create all folders needed for that
            auto fullTargetDestination = pathCombine({ _installDir, _targetInstallDir });

            // Copy the target file(exe/a/dll) to the install folder
           outputCopyFile(outputFile(), fullTargetDestination); 
            
            // For all destinations, copy the install files
            for (auto pair : _installFiles)
            {
                // Determine the destination for this file list and create all folders for this
                auto fullDestination = pathCombine({ _installDir, pair.first });

                // Copy all files for this destination
                auto files = pair.second.allFiles();
                for (auto file : files)
                {
                    outputCopyFile(file, fullDestination); 
                }
            }
        }

        void generateRunScript()
        {
            if (_targetType == TargetTypes::Executable)
            {
                // Run the executable
                std::cout << "call " << outputFile() << "\n";
            }
            else
            {
                // Show a message that the user is trying to run the target executable
                std::cout << "echo This target is not an executable.\n"; 
            }
        }

        void generateCleanScript()
        {
            // Remove all the independant object files
            for (auto file : allFiles()) outputDeleteFile(pathCombine({ buildRoot(), "obj", extractFilename(file) + ".o" }));

            // Remove the target file
            outputDeleteFile(outputFile());
        }
        
        void generateTestScript()
        {
        }

    public:
        Target(const std::string& name, TargetTypes targetType = TargetTypes::Executable)
            : Folder(""), _project(name), _targetType(targetType), _installDir(""), 
                _compilerFlags(" --std=c++11"), _linkerFlags("")
        { }

        virtual ~Target()
        {
            std::cout << "echo off"<< "\n";
            std::cout << "if \"%1\" == \"\" goto build" << _project << "\n";
            std::cout << "if \"%1\" == \"" << _project << "\" goto build" << _project << "\n";
            std::cout << "echo Skipping target \"" << _project << "\".\n";
            std::cout << "goto done" << _project << "\n";
            std::cout << ":build" << _project << "\n";
            if (buildScriptOn)
            {
                std::cout << "echo Executing build script for target \"" << _project << "\"\n";
                generateBuildScript();
            }
            if (installScriptOn)
            {
                std::cout << "echo Executing install script for target \"" << _project << "\"\n";
                generateInstallScript();
            }
            if (runScriptOn)
            {
                std::cout << "echo Executing run script for target \"" << _project << "\"\n";
                generateRunScript();
            }
            if (cleanScriptOn)
            {
                std::cout << "echo Executing clean script for target \"" << _project << "\"\n";
                generateCleanScript();
            }
            if (testScriptOn)
            {
                std::cout << "echo Executing test script for target \"" << _project << "\"\n";
                generateTestScript();
            }
            std::cout << ":done" << _project << "\n";
            std::cout << "echo.\n";
        }

        Target& includeDirs(std::initializer_list<const char*> dirs)
        {
            for (auto dir : dirs) _includeDirs.push_back(dir);

            return *this;
        }

        Target& libraries(std::initializer_list<const char*> files)
        {
            for (auto file : files) _libraries.push_back(file);

            return *this;
        }

        Target& libraryDirs(std::initializer_list<const char*> dirs)
        {
            for (auto dir : dirs) _libraryDirs.push_back(dir);

            return *this;
        }

        Target& installDir(const std::string& folder)
        {
            _installDir = folder;
            cleanPathSeperators(_installDir);

            return *this;
        }

        Target& installFiles(const std::string& destination, std::initializer_list<const char*> files)
        {
            return installFiles(destination, Folder("").files(files));
        }

        Target& installTargetInto(const std::string& target)
        {
            _targetInstallDir = target;
            cleanPathSeperators(_targetInstallDir);

            return *this;
        }

        Target& installFiles(const std::string& destination, const Folder& folder)
        {
            auto dest = destination;
            cleanPathSeperators(dest);

            auto result = _installFiles.insert(std::make_pair(dest, folder));

            if (!result.second) result.first->second += folder;

            return *this;
        }

        Target& compilerFlags(std::initializer_list<const char*> flags)
        {
            for (auto flag : flags) _compilerFlags += std::string(" ") + flag;

            return *this;
        }

        Target& linkerFlags(std::initializer_list<const char*> flags)
        {
            for (auto flag : flags) _linkerFlags += std::string(" ") + flag;

            return *this;
        }
    };

} // namespace cppbuild
