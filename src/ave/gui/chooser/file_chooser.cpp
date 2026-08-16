#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <chrono>
#include <imgui.h>
#include <imgui_stdlib.h>
#include "ave/gui/chooser/file_chooser.h"

using std::cout;
using std::endl;
using std::string;

namespace fs = std::filesystem;

#ifdef _WIN32
#include <windows.h>
#endif


namespace ave {


void FileChooser::reset(bool all) {
    current_directory_ = fs::current_path();
    selected_path_ = "";
    selected_paths_.clear();

    column_widths_[0] = 0.4f;  // 名称
    column_widths_[1] = 0.2f;  // 类型
    column_widths_[2] = 0.15f; // 大小
    column_widths_[3] = 0.25f; // 修改时间

    file_name_input_.clear();

    if (all) {
        filters_ = {"所有文件 (*.*)"};
        current_filter_ = filters_[0];
        show_hidden_files_ = false;
        multiple_selection_ = false;
        selection_mode_ = SelectionMode::FilesOnly;
    }

    load_directory();
}

bool FileChooser::show() {
    if (!is_open_) {
        reset();
        ImGui::OpenPopup("选择文件");
        is_open_ = true;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal("选择文件", &is_open_, ImGuiWindowFlags_NoCollapse)) {
        ImGui::BeginChild("Sidebar", ImVec2(200, 0), ImGuiChildFlags_ResizeX);

        render_sidebar();
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("MainContent");

        render_toolbar();
        render_file_list();
        if( is_open_ )
            render_footer();

        ImGui::EndChild();

        ImGui::EndPopup();
    }
    return !is_open_ && !selected_paths_.empty();
}

void FileChooser::close() {
    is_open_ = false;
    ImGui::CloseCurrentPopup();
}

void FileChooser::render_toolbar() {
    if (ImGui::ArrowButton("##Up", ImGuiDir_Up))
        navigate_up();
    ImGui::SameLine();

    if (ImGui::Button("刷新"))
        load_directory();
    ImGui::SameLine();
    ImGui::PushItemWidth(300);
    std::string pathStr = current_directory_.string();
    if (ImGui::InputText("##Path", &pathStr, ImGuiInputTextFlags_EnterReturnsTrue))
        navigate_to(pathStr);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::PushItemWidth(150);
    if (ImGui::BeginCombo("##Filter", current_filter_.c_str())) {
        for (const auto &filter : filters_)
            if (ImGui::Selectable(filter.c_str(), filter == current_filter_)) {
                current_filter_ = filter;
                load_directory();
            }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Checkbox("显示隐藏", &show_hidden_files_))
        load_directory();
    ImGui::Separator();
}

void FileChooser::render_file_list() {
    ImGui::BeginChild("FileListHeader", ImVec2(0, 30), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGui::SetCursorPosX(0);
    ImGui::Text(" ");
    ImGui::SameLine();

    ImGui::SetCursorPosX(30);
    ImGui::Selectable("名称呢", false, ImGuiSelectableFlags_None, ImVec2(column_widths_[0] * ImGui::GetWindowWidth(), 0));
    ImGui::SameLine();

    ImGui::SetCursorPosX(30 + column_widths_[0] * ImGui::GetWindowWidth());
    ImGui::Selectable("类型呢", false, ImGuiSelectableFlags_None, ImVec2(column_widths_[1] * ImGui::GetWindowWidth(), 0));
    ImGui::SameLine();

    ImGui::SetCursorPosX(30 + (column_widths_[0] + column_widths_[1]) * ImGui::GetWindowWidth());
    ImGui::Selectable("大小呢", false, ImGuiSelectableFlags_None, ImVec2(column_widths_[2] * ImGui::GetWindowWidth(), 0));
    ImGui::SameLine();

    ImGui::SetCursorPosX(30 + (column_widths_[0] + column_widths_[1] + column_widths_[2]) * ImGui::GetWindowWidth());
    ImGui::Selectable("修改时间呢", false, ImGuiSelectableFlags_None, ImVec2(column_widths_[3] * ImGui::GetWindowWidth(), 0));

    ImGui::PopStyleVar();
    ImGui::EndChild();

    ImGui::BeginChild("FileList", ImVec2(0, -60), ImGuiChildFlags_Borders);

    for (FileItem &item : file_items_) {
        ImGui::PushID(&item);

        bool isSelected = item.is_selected;

        ImGui::SetCursorPosX(5);
        if (item.is_directory)
            ImGui::Text(">");
        else
            ImGui::Text(" ");
        ImGui::SameLine();

        ImGui::SetCursorPosX(30);
        if (ImGui::Selectable(item.name.c_str(), &isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                if (item.is_directory) {
                    navigate_to(item.path);
                    file_name_input_ = current_directory_.u8string();
                } else
                    select_item(item.path);
            } else if (ImGui::IsItemClicked()) {
                if (multiple_selection_ && ImGui::GetIO().KeyCtrl)
                    item.is_selected = !item.is_selected;
                else {
                    for (FileItem &other : file_items_)
                        other.is_selected = false;
                    item.is_selected = true;
                    selected_path_ = item.path;
                    file_name_input_ = item.name;
                }
            }
        }

        ImGui::SameLine();
        ImGui::SetCursorPosX(30 + column_widths_[0] * ImGui::GetWindowWidth());
        ImGui::Text("%s", item.type.c_str());

        ImGui::SameLine();
        ImGui::SetCursorPosX(30 + (column_widths_[0] + column_widths_[1]) * ImGui::GetWindowWidth());
        ImGui::Text("%s", item.size.c_str());

        ImGui::SameLine();
        ImGui::SetCursorPosX(30 + (column_widths_[0] + column_widths_[1] + column_widths_[2]) * ImGui::GetWindowWidth());
        ImGui::Text("%s", item.modified_time.c_str());

        ImGui::PopID();
        if( !is_open_ )
            break;
    }

    ImGui::EndChild();
}

void FileChooser::select_item(const fs::path& path) {
    if( fs::is_directory(path) && (selection_mode_==SelectionMode::FilesOnly
                                || selection_mode_==SelectionMode::SaveFile) ) {
        navigate_to(path);
        file_name_input_ = current_directory_.u8string();
        return;
    } else if( !fs::is_directory(path) && selection_mode_==SelectionMode::DirectoriesOnly )
        return;
    bool found = false;
    if (selection_mode_==SelectionMode::SaveFile)
        found = true;
    else
        for( FileItem& item : file_items_ )
            if( item.path == path) {
                found = true;
                item.is_selected = true;
            }
    if( !found )
        return;
    selected_path_ = path;
    file_name_input_ = path.filename().u8string();
    is_open_ = false;
    if( !multiple_selection_ )
        for( FileItem& item : file_items_ )
            item.is_selected = false;
}

void FileChooser::render_footer() {
    ImGui::Text("文件名:");
    ImGui::SameLine();
    ImGui::PushItemWidth(300);
    if ( ImGui::InputText("##FileName", &file_name_input_, ImGuiInputTextFlags_EnterReturnsTrue) )
        select_item(current_directory_ / file_name_input_);
    ImGui::PopItemWidth();

    if( !is_open_ )
        return;
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 160);

    const string btn_text = (selection_mode_==SelectionMode::SaveFile) ? "保存" : "打开";
    if( ImGui::Button(btn_text.c_str(), ImVec2(70, 0)) && !file_name_input_.empty() )
        select_item(current_directory_/file_name_input_);

    ImGui::SameLine();
    if (ImGui::Button("取消", ImVec2(70, 0))) {
        selected_path_ = "";
        is_open_ = false;
        for( FileItem& item : file_items_ )
            item.is_selected = false;
    }
}

void FileChooser::render_sidebar() {
    fs::path home;
    if (const char* home_env = std::getenv("HOME"))
        home = home_env;
    else if (const char* userprofile = std::getenv("USERPROFILE"))
        home = userprofile;

    if (!home.empty() && fs::exists(home)) {
        ImGui::Text("常用位置");
        ImGui::Separator();

        struct QuickLink { const char* label; const char* subdir; };
        QuickLink links[] = {
            {"桌面", "Desktop"},
            {"文档", "Documents"},
            {"下载", "Downloads"},
            {"图片", "Pictures"},
            {"音乐", "Music"},
            {"视频", "Videos"}
        };
        for (auto& link : links) {
            if (ImGui::Selectable(link.label)) {
                fs::path target = home / link.subdir;
                std::error_code ec;
                if (fs::exists(target, ec))
                    navigate_to(target);
            }
        }
        ImGui::Separator();
    }

#ifdef _WIN32
    ImGui::Text("驱动器");
    DWORD drives = GetLogicalDrives();
    for (char drive = 'A'; drive <= 'Z'; ++drive) {
        if (drives & (1 << (drive - 'A'))) {
            std::string drivePath = std::string(1, drive) + ":\\";
            std::string label = std::string(1, drive) + ":";
            if (ImGui::Selectable(label.c_str())) {
                navigate_to(drivePath);
            }
        }
    }
#endif
}

void FileChooser::load_directory() {
    file_items_.clear();

    try {
        for (const fs::directory_entry &entry : fs::directory_iterator(current_directory_)) {
            if (!show_hidden_files_) {
                std::string filename = entry.path().filename().string();
                if (!filename.empty() && filename[0] == '.')
                    continue;
            }

            if(selection_mode_ == SelectionMode::DirectoriesOnly && !entry.is_directory())
                continue;

            FileItem item;
            item.path = entry.path();
            item.name = entry.path().filename().string();
            item.is_directory = entry.is_directory();
            item.is_selected = false;

            if (item.is_directory)
                item.type = "文件夹";
            else if (entry.path().has_extension()) {
                item.type = entry.path().extension().string();
                if (!item.type.empty())
                    item.type = item.type.substr(1); // 移除点
                item.type = item.type + " 文件";
            }
            else
                item.type = "未知类型";

            if (!item.is_directory)
                item.size = get_file_size(entry.path());
            else
                item.size = "--";

            try {
                auto ftime = fs::last_write_time(entry.path());
                item.modified_time = format_time(ftime);
            } catch (...) {
                item.modified_time = "未知";
            }

            if (is_filter_match(entry.path()))
                file_items_.push_back(item);
        }
    } catch (const std::exception &e) {
        printf("Error loading directory: %s\n", e.what());
    }

    sort_items();
}

void FileChooser::sort_items() {
    std::sort(file_items_.begin(), file_items_.end(),
                [](const FileItem &a, const FileItem &b) {
                    if (a.is_directory != b.is_directory)
                        return a.is_directory > b.is_directory;
                    return a.name < b.name;
                });
}

void FileChooser::navigate_to(const fs::path &path) {
    if (fs::exists(path) && fs::is_directory(path)) {
        current_directory_ = fs::canonical(path);
        load_directory();
    }
}

void FileChooser::navigate_up() {
    if (current_directory_.has_parent_path())
        navigate_to(current_directory_.parent_path());
}

bool FileChooser::is_filter_match(const fs::path &path) const {
    if (current_filter_ == "所有文件 (*.*)")
        return true;

    std::string ext = path.extension().string();
    if (ext.empty())
        return false;

    return current_filter_.find(ext.substr(1)) != std::string::npos;
}

std::string FileChooser::get_file_size(const fs::path &path) {
    try {
        uintmax_t size = fs::file_size(path);
        const char *units[] = {"B", "KB", "MB", "GB", "TB"};
        int unitIndex = 0;
        double fileSize = static_cast<double>(size);

        while (fileSize >= 1024.0 && unitIndex < 4) {
            fileSize /= 1024.0;
            unitIndex++;
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << fileSize << " " << units[unitIndex];
        return ss.str();
    } catch (...) {
        return "未知";
    }
}

std::string FileChooser::format_time(const fs::file_time_type &time) {
    try {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            time - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        auto tt = std::chrono::system_clock::to_time_t(sctp);

        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &tt);
#else
        localtime_r(&tt, &tm);
#endif

        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M");
        return ss.str();
    } catch (...) {
        return "未知";
    }
}

std::vector<fs::path> FileChooser::get_selected_files() const {
    std::vector<fs::path> result;
    for (const auto &item : file_items_)
        if (item.is_selected)
            result.push_back(item.path);
    return result;
}

void FileChooser::set_filter(const std::string &filter) {
    filters_.clear();
    filters_.push_back(filter);
    current_filter_ = filter;
    load_directory();
}

void FileChooser::set_filters(const std::vector<std::string> &filters) {
    filters_ = filters;
    if (!filters_.empty()) {
        current_filter_ = filters_[0];
        load_directory();
    }
}


} // namespace ave
