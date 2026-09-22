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
using std::vector;

namespace fs = std::filesystem;

#ifdef _WIN32
// windows.h 默认定义 min/max 宏，会破坏本文件中 std::max 等调用（MSVC 报 C2589）。
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif


namespace ave {


void FileChooser::reset(bool all) {
    is_open_ = false;
    current_directory_ = fs::current_path();
    selected_path_ = "";

    column_widths_[0] = 0.4f;  // 名称
    column_widths_[1] = 0.2f;  // 类型
    column_widths_[2] = 0.15f; // 大小
    column_widths_[3] = 0.25f; // 修改时间

    file_name_input_.clear();

    if( all ) {
        filters_ = {"所有文件 (*.*)"};
        current_filter_ = filters_[0];
        show_hidden_files_ = false;
        multiple_selection_ = false;
        selection_mode_ = SelectionMode::FilesOnly;
    }

    load_directory();
}

bool FileChooser::show() {
    if( !is_open_ ) {
        reset(false);
        ImGui::OpenPopup("选择文件");
        is_open_ = true;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.8f, 0.9f, 0.8f, 1));
    if( ImGui::BeginPopupModal("选择文件", &is_open_, ImGuiWindowFlags_NoCollapse) ) {

        render_toolbar();

        render_sidebar();
        ImGui::SameLine();

        ImGui::BeginChild("MainContent");

        render_file_list();
        if( is_open_ )
            render_footer();

        ImGui::EndChild();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();

    return !is_open_ && !selected_path_.empty();
}

void FileChooser::close() {
    is_open_ = false;
    ImGui::CloseCurrentPopup();
}

void FileChooser::render_toolbar() {
    if( ImGui::ArrowButton("##Up", ImGuiDir_Up) )
        navigate_up();
    ImGui::SameLine();

    if( ImGui::Button("刷新") )
        load_directory();
    ImGui::SameLine();

    // 路径框占满一行：为右侧的 Filter 组合框和"显示隐藏"复选框预留宽度
    const ImGuiStyle &style = ImGui::GetStyle();
    const float filter_width = 150.0f;
    const float hidden_text_width = ImGui::CalcTextSize("显示隐藏").x;
    const float checkbox_width = ImGui::GetFrameHeight() + style.ItemInnerSpacing.x + hidden_text_width;
    // 路径框与其后每个控件之间的间隔，以及组合框与复选框之间的间隔
    const float reserve = filter_width + checkbox_width + style.ItemSpacing.x * 2.0f;

    // 负值宽度表示"从右边界向左预留 reserve 像素"，随窗口宽度自适应
    ImGui::SetNextItemWidth(-reserve);
    if( ImGui::InputText("##Path", &top_dir_input_, ImGuiInputTextFlags_EnterReturnsTrue) )
        navigate_to(top_dir_input_);
    ImGui::SameLine();

    ImGui::SetNextItemWidth(filter_width);
    if( ImGui::BeginCombo("##Filter", current_filter_.c_str()) ) {
        for( const auto &filter : filters_ )
            if( ImGui::Selectable(filter.c_str(), filter == current_filter_) ) {
                current_filter_ = filter;
                load_directory();
            }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("显示隐藏", &show_hidden_files_))
        load_directory();
    ImGui::Separator();
}

void FileChooser::render_file_list() {
    float availHeight = std::max(ImGui::GetContentRegionAvail().y - 60.0f, 100.0f);

    ImGui::BeginChild("FileListTable", ImVec2(0, availHeight), ImGuiChildFlags_Borders);

    if( ImGui::BeginTable("FileTable", 4,
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
                          ImGuiTableFlags_NoSavedSettings) ) {

        ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthStretch, column_widths_[0]);
        ImGui::TableSetupColumn("类型", ImGuiTableColumnFlags_WidthStretch, column_widths_[1]);
        ImGui::TableSetupColumn("大小", ImGuiTableColumnFlags_WidthStretch, column_widths_[2]);
        ImGui::TableSetupColumn("修改时间", ImGuiTableColumnFlags_WidthStretch, column_widths_[3]);
        ImGui::TableHeadersRow();

        for (FileItem &item : file_items_) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            ImGui::Text("%c", item.is_directory ? '>' : ' ');
            ImGui::SameLine();

            ImGui::PushID(&item);
            if (ImGui::Selectable(item.name.c_str(), item.is_selected,
                                  ImGuiSelectableFlags_SpanAllColumns |
                                  ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    if (item.is_directory)
                        navigate_to(current_directory_/item.path);
                    else
                        select_item(item.path);
                } else
                    single_click(item, ImGui::GetIO().KeyCtrl);
            }
            ImGui::PopID();

            // 第二列：类型
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", item.type.c_str());

            // 第三列：大小
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", item.size.c_str());

            // 第四列：修改时间
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", item.modified_time.c_str());
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

void FileChooser::single_click( FileItem& item, bool ctrl_down ) {
    if( !is_selectable(item.path) )
        return;
    if( multiple_selection_ && ctrl_down ) {
        item.is_selected = !item.is_selected;
        if( item.is_selected && selected_path_.empty() )
            selected_path_ = current_directory_ / item.path;
    } else {
        for( FileItem &other : file_items_ )
            other.is_selected = false;
        file_name_input_ = item.name;
        item.is_selected = true;
        if( multiple_selection_ && selected_path_.empty() )
            selected_path_ = current_directory_ / item.path;
    }
}

void FileChooser::select_item(const fs::path& path) {
    if( !is_selectable(path) ) {
        if( fs::is_directory(current_directory_ / path) )
            navigate_to(current_directory_ / path);
        return;
    }
    if( path.empty() ) {
        if( multiple_selection_ ) {
            is_open_ = false;
            if( selected_path_.empty() && fs::is_directory(current_directory_) ) {
                selected_path_ = current_directory_;
                file_items_.clear();
            }
        } else if( fs::is_directory(current_directory_) ) {
            is_open_ = false;
            selected_path_ = current_directory_;
            file_items_.clear();
        }
        return;
    }
    if( !fs::is_directory(current_directory_/ path) && selection_mode_==SelectionMode::DirectoriesOnly )
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
    selected_path_ = current_directory_ / path;
    is_open_ = false;
    if( !multiple_selection_ )
        file_items_.clear();
}

void FileChooser::render_footer() {
    ImGui::Text("文件名:");
    ImGui::SameLine();

    // 文件名框占满一行：为右侧的"打开/保存"与"取消"两个按钮预留宽度
    const ImGuiStyle &style = ImGui::GetStyle();
    const float button_width = 70.0f;
    // 文件名框与第一个按钮、以及两个按钮之间的间隔
    const float reserve = button_width * 2.0f + style.ItemSpacing.x * 2.0f;

    ImGui::SetNextItemWidth(-reserve);
    if( ImGui::InputText("##FileName", &file_name_input_, ImGuiInputTextFlags_EnterReturnsTrue) )
        select_item(file_name_input_);

    if( !is_open_ )
        return;
    ImGui::SameLine();

    const string btn_text = (selection_mode_==SelectionMode::SaveFile) ? "保存" : "打开";
    if( ImGui::Button(btn_text.c_str(), ImVec2(button_width, 0)) )
        select_item(file_name_input_);

    ImGui::SameLine();
    if( ImGui::Button("取消", ImVec2(button_width, 0)) ) {
        selected_path_ = "";
        is_open_ = false;
        file_items_.clear();
    }
}

void FileChooser::render_sidebar() {
    ImGui::BeginChild("Sidebar", ImVec2(200, 0), ImGuiChildFlags_ResizeX);

    fs::path home;
    if( const char* home_env = std::getenv("HOME") )
        home = home_env;
    else if( const char* userprofile = std::getenv("USERPROFILE") )
        home = userprofile;

    if( !home.empty() && fs::exists(home) ) {
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
        for( auto& link : links ) {
            if( ImGui::Selectable(link.label) ) {
                fs::path target = home / link.subdir;
                std::error_code ec;
                if( fs::exists(target, ec) )
                    navigate_to(target);
            }
        }
        ImGui::Separator();
    }

#ifdef _WIN32
    ImGui::Text("驱动器");
    DWORD drives = GetLogicalDrives();
    for( char drive = 'A'; drive <= 'Z'; ++drive ) {
        if( drives & (1 << (drive - 'A')) ) {
            std::string drivePath = std::string(1, drive) + ":\\";
            std::string label = std::string(1, drive) + ":";
            if( ImGui::Selectable(label.c_str()) ) {
                navigate_to(drivePath);
            }
        }
    }
#endif
    ImGui::EndChild();
}

void FileChooser::load_directory() {
    top_dir_input_ = current_directory_.u8string();
    file_items_.clear();
    file_name_input_.clear();

    try {
        for( const fs::directory_entry &entry : fs::directory_iterator(current_directory_) ) {
            if( !show_hidden_files_ ) {
                std::string filename = entry.path().filename().u8string();
                if (!filename.empty() && filename[0] == '.')
                    continue;
            }

            if (selection_mode_ == SelectionMode::DirectoriesOnly && !entry.is_directory())
                continue;

            FileItem item;
            item.path = entry.path().filename();
            item.name = item.path.u8string();
            item.is_directory = entry.is_directory();
            item.is_selected = false;

            if (item.is_directory)
                item.type = "文件夹";
            else if (entry.path().has_extension()) {
                item.type = entry.path().extension().u8string();
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

            if (!item.is_directory && is_filter_match(entry.path()))
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

    std::string ext = path.extension().u8string();
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
            result.push_back(current_directory_ / item.path);
    if( result.empty() && !selected_path_.empty() )
        result.push_back(selected_path_);
    return result;
}

void FileChooser::set_filter(const std::string &filter) {
    filters_ = { filter };
    current_filter_ = filter;
    load_directory();
}

void FileChooser::set_filters(const std::vector<std::string> &filters) {
    filters_ = filters.empty() ? vector<string>({"所有文件 (*.*)"}) : filters;
    if (!filters_.empty()) {
        current_filter_ = filters_[0];
        load_directory();
    }
}


} // namespace ave
