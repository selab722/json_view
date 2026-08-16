#ifndef AVE_GUI_WINDOW_SAVE__CONFIG_H
#define AVE_GUI_WINDOW_SAVE__CONFIG_H

#include <string>
#include <fstream>
#include <unordered_map>
#include <algorithm>


namespace {

void trim( std::string& str ) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(),
        [](unsigned char ch) { return !std::isspace(ch); }));
    str.erase(std::find_if(str.rbegin(), str.rend(),
        [](unsigned char ch) { return !std::isspace(ch); }).base(), str.end());
}

}

namespace ave {


class SaveConfig {
public:
    std::string file;
    std::unordered_map<std::string, std::string> values;

    SaveConfig( std::string file_ ) : file(file_) {}

    void read( int& xpos, int& ypos, int& width, int& height ) {
        if( file.empty() )
            return;

        values.clear();

        std::ifstream fin(file);
        if( !fin.is_open() )
            return;

        std::string line;
        while( std::getline(fin, line) ) try {
            size_t pos = line.find('=');
            if( pos == std::string::npos )
                continue;
            std::string key = line.substr(0, pos);
            trim(key);
            std::string value = line.substr(pos+1);
            trim(value);
            if( key == "xpos" )
                xpos = std::stoi(value);
            else if( key == "ypos" )
                ypos = std::stoi(value);
            else if( key == "width" )
                width = std::stoi(value);
            else if( key == "height" )
                height = std::stoi(value);
            else
                values[key] = value;
        } catch (...) {}
    }

    void write( int xpos, int ypos, int width, int height ) const {
        if( file.empty() )
            return;
        std::ofstream fout(file, std::ios::out);
        if( !fout.is_open() )
            return;

        fout<<"xpos="<<xpos<<std::endl;
        fout<<"ypos="<<ypos<<std::endl;
        fout<<"width="<<width<<std::endl;
        fout<<"height="<<height<<std::endl;

        for (const auto& [key, value] : values)
            fout << key << "=" << value << std::endl;
    }

    SaveConfig(const SaveConfig&) = default;
    SaveConfig(SaveConfig&&) = default;
    SaveConfig& operator=(const SaveConfig&) = default;
    SaveConfig& operator=(SaveConfig&&) = default;

};


}

#endif  // #ifndef GUI_SAVE__CONFIG_H