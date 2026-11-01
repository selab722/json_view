#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <optional>
#include <utility>
#include <imgui.h>
#include <imgui_stdlib.h>
#include "ave/gui/chooser/file_chooser.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::pair;
using std::optional;

namespace fs = std::filesystem;

#ifdef _WIN32
// windows.h 默认定义 min/max 宏，会破坏本文件中 std::max 等调用
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
        filters_ = {"*"};
        current_filter_ = filters_[0];
        show_hidden_files_ = false;
        multiple_selection_ = false;
        selection_mode_ = SelectionMode::FilesOnly;
        sort_order_ = SortOrder::Name;
    }

    quick_links_.clear();
    file_items_.clear();
}

bool FileChooser::show() {
    if( !is_open_ ) {
        reset(false);
        load_directory();  // 打开时只加载一次
        ImGui::OpenPopup("选择文件");
        is_open_ = true;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.8f, 0.9f, 0.8f, 1));
    if( ImGui::BeginPopupModal("选择文件", &is_open_, ImGuiWindowFlags_NoCollapse) ) {

        render_toolbar();

        if( render_sidebar() )
            ImGui::SameLine();

        ImGui::BeginChild("MainContent");

        render_file_list();
        if( is_open_ )
            render_footer();

        ImGui::EndChild();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();

    return !is_open_ && has_selected();
}

void FileChooser::close() {
    is_open_ = false;
    ImGui::CloseCurrentPopup();
}

namespace {
    char ascii_lower( char c ) {
        return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    }

    string filter_label( const string& filter ) {
        return (filter == "*") ? string("所有文件") : "*." + filter;
    }

    // 文件名是否以 "." + suffix 结尾（ASCII 大小写无关，Windows 上 "A.TXT" 也能被 "txt" 命中）。
    // 点号前面必须还有内容，所以 ".png" 这种只能算隐藏文件名，不算有扩展名。
    bool has_dot_suffix( const string& name, const string& suffix ) {
        if( name.size() < suffix.size() + 2 )
            return false;
        const size_t dot = name.size() - suffix.size() - 1;   // 上面保证了 dot >= 1
        if( name[dot] != '.' )
            return false;
        for( size_t i = 0; i < suffix.size(); ++i )
            if( ascii_lower(name[dot + 1 + i]) != ascii_lower(suffix[i]) )
                return false;
        return true;
    }
}

void FileChooser::render_toolbar() {
    if( ImGui::ArrowButton("##Up", ImGuiDir_Up) )
        navigate_up();
    ImGui::SameLine();

    if( ImGui::Button("刷新") )
        load_directory();
    ImGui::SameLine();

    const ImGuiStyle &style = ImGui::GetStyle();
    const float filter_width = 150.0f;
    const float hidden_text_width = ImGui::CalcTextSize("显示隐藏").x;
    const float checkbox_width = ImGui::GetFrameHeight() + style.ItemInnerSpacing.x + hidden_text_width;
    // 路径框与其后每个控件之间的间隔，以及组合框与复选框之间的间隔
    const float reserve = filter_width + checkbox_width + style.ItemSpacing.x * 2.0f;

    // 负值宽度表示"从右边界向左预留 reserve 像素"，随窗口宽度自适应
    ImGui::SetNextItemWidth(-reserve);
    if( ImGui::InputText("##Path", &top_dir_input_, ImGuiInputTextFlags_EnterReturnsTrue) ) {
        try {
            navigate_to(fs::u8path(top_dir_input_));
        } catch( const std::exception &e ) {}
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth(filter_width);
    if( ImGui::BeginCombo("##Filter", filter_label(current_filter_).c_str()) ) {
        for( const auto &filter : filters_ )
            if( ImGui::Selectable(filter_label(filter).c_str(), filter == current_filter_) ) {
                current_filter_ = filter;
                load_directory();
            }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if( ImGui::Checkbox("显示隐藏", &show_hidden_files_) )
        load_directory();
    ImGui::Separator();
}

namespace {
    optional<pair<unsigned int, bool>> sort_spec_first( ImGuiTableSortSpecs* sort_specs ) {
        if( !sort_specs || !(sort_specs->SpecsDirty) || sort_specs->SpecsCount <= 0 )
            return std::nullopt;
        const ImGuiTableColumnSortSpecs* sort_spec = &sort_specs->Specs[0];
        unsigned int id = sort_spec->ColumnUserID;
        bool asc = (sort_spec->SortDirection) == ImGuiSortDirection_Ascending;
        sort_specs->SpecsDirty = false; // 关键：清除脏标记
        return pair{id, asc};
    }
}

void FileChooser::render_file_list() {
    float availHeight = std::max(ImGui::GetContentRegionAvail().y - 60.0f, 100.0f);

    ImGui::BeginChild("FileListTable", ImVec2(0, availHeight), ImGuiChildFlags_Borders);

    // 点击会触发导航或选中，而这两件事都会 clear()/重建 file_items_：
    // 在遍历过程中做的话，迭代器和 item 引用当场失效（后面几列还要读 item）。
    // 所以循环里只记录动作，等表格和子窗口都结束之后再执行。
    fs::path pending_navigate;
    fs::path pending_select;

    if( ImGui::BeginTable("FileTable", 4,
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
                          ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Sortable) ) {

        ImGui::TableSetupColumn("名称",     ImGuiTableColumnFlags_WidthStretch, column_widths_[0], (int)SortOrder::Name);
        ImGui::TableSetupColumn("类型",     ImGuiTableColumnFlags_WidthStretch, column_widths_[1], (int)SortOrder::Type);
        ImGui::TableSetupColumn("大小",     ImGuiTableColumnFlags_WidthStretch, column_widths_[2], (int)SortOrder::Size);
        ImGui::TableSetupColumn("修改时间", ImGuiTableColumnFlags_WidthStretch, column_widths_[3], (int)SortOrder::Time);
        ImGui::TableHeadersRow();

        {
            optional<pair<unsigned int, bool>> sort_spec = sort_spec_first(ImGui::TableGetSortSpecs());
            if( sort_spec ) {
                SortOrder order = sort_order_;
                switch( sort_spec->first ) {
                    case (int)SortOrder::Name:
                        order = (sort_spec->second) ? SortOrder::Name : SortOrder::NameDes;
                        break;
                    case (int)SortOrder::Type:
                        order = (sort_spec->second) ? SortOrder::Type : SortOrder::TypeDes;
                        break;
                    case (int)SortOrder::Size:
                        order = (sort_spec->second) ? SortOrder::Size : SortOrder::SizeDes;
                        break;
                    case (int)SortOrder::Time:
                        order = (sort_spec->second) ? SortOrder::Time : SortOrder::TimeDes;
                        break;
                    default:
                        break;
                }

                if( order != sort_order_ ) {
                    sort_order_ = order;
                    sort_items();
                }
            }
        }

        // 只提交可见行：几千个文件的目录就不必每帧几千次 TableNextRow/Text
        ImGuiListClipper clipper;
        clipper.Begin( (int)file_items_.size() );
        while( clipper.Step() ) {
            for( int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row ) {
                FileItem &item = file_items_[row];

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
                            pending_navigate = current_directory_/item.path;
                        else
                            pending_select = item.path;
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
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();

    // 表格和子窗口都结束了，现在改 file_items_ 才是安全的
    if( !pending_navigate.empty() )
        navigate_to(pending_navigate);
    else if( !pending_select.empty() )
        select_item(pending_select);
}

void FileChooser::single_click( FileItem& item, bool ctrl_down ) {
    if( !is_selectable(item.path) ) {
        if( item.is_directory )
            file_name_input_ = item.name;
        return;
    }
    if( multiple_selection_ && ctrl_down ) {
        item.is_selected = !item.is_selected;
        if( item.is_selected )
            selected_path_ = current_directory_ / item.path;
        else{
            file_name_input_.clear();
            if( selected_path_ == (current_directory_ / item.path) )
                selected_path_.clear();
        }
    } else {
        if( multiple_selection_ ) {
            for( FileItem &other : file_items_ )
                other.is_selected = false;
            item.is_selected = true;
            selected_path_ = (current_directory_ / item.path);
        }
        file_name_input_ = item.name;
    }
}

void FileChooser::select_item(const fs::path& path) {
    if( path.empty() ) {
        bool has_sel = has_selected();
        if( multiple_selection_ )
            is_open_ = !has_sel;
        if( !has_sel && fs::is_directory(current_directory_) && is_selectable(path) ) {
            is_open_ = false;
            selected_path_ = current_directory_;
            file_items_.clear();
        }
        return;
    }
    if( !is_selectable(path) ) {
        if( fs::is_directory(current_directory_ / path) )
            navigate_to(current_directory_ / path);
        return;
    }
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

bool FileChooser::render_sidebar() {
    // 驱动器列表只是一次 GetLogicalDrives()，没有文件系统访问，不需要缓存。
    // 注意这里不逐个探测盘符是否存在：断开的网络盘/空光驱会让探测卡住。
    vector<string> drive_labels;
#ifdef _WIN32
    const DWORD drive_mask = GetLogicalDrives();
    for( char drive = 'A'; drive <= 'Z'; ++drive )
        if( drive_mask & (1 << (drive - 'A')) )
            drive_labels.push_back(string(1, drive) + ":");
#endif

    // 一个条目都没有就不创建子窗口：否则会白占一块 200 像素宽的空面板
    if( quick_links_.empty() && drive_labels.empty() )
        return false;

    ImGui::BeginChild("Sidebar", ImVec2(200, 0), ImGuiChildFlags_ResizeX);

    fs::path pending_navigate;

    if( !quick_links_.empty() ) {
        ImGui::Text("常用位置");
        ImGui::Separator();

        for( const auto& link : quick_links_ )
            if( ImGui::Selectable(link.first.c_str()) )
                pending_navigate = link.second;

        ImGui::Separator();
    }

    if( !drive_labels.empty() ) {
        ImGui::Text("驱动器");
        for( const string& label : drive_labels )
            if( ImGui::Selectable(label.c_str()) )
                pending_navigate = label + "\\";
    }

    ImGui::EndChild();

    if( !pending_navigate.empty() )
        navigate_to(pending_navigate);

    return true;
}

namespace {
    // 收集侧边栏"常用位置"里真正存在的目录，返回 (标签, 目录)。
    // 需要访问文件系统，所以由调用方负责缓存，不要每帧调用。
    std::vector<std::pair<string, fs::path>> collect_quick_links() {
        std::vector<std::pair<string, fs::path>> links;

        fs::path home;
#ifdef _WIN32
        // Windows 上窄环境变量是 ANSI 代码页，不是 UTF-8：用 u8path() 遇到中文用户名
        // （C:\Users\张三）会抛 filesystem_error，用窄构造则要靠 ACP 往返。
        // 直接取宽字符版本，编码问题彻底消失。
        if( const wchar_t* home_w = _wgetenv(L"HOME") )
            home = home_w;
        else if( const wchar_t* profile_w = _wgetenv(L"USERPROFILE") )
            home = profile_w;
#else
        // POSIX 上路径就是字节串，窄构造是恒等转换
        if( const char* home_env = std::getenv("HOME") )
            home = home_env;
#endif

        if( home.empty() )
            return links;

        static const struct { const char* label; const char* subdir; } candidates[] = {
            {"桌面", "Desktop"},
            {"文档", "Documents"},
            {"下载", "Downloads"},
            {"图片", "Pictures"},
            {"音乐", "Music"},
            {"视频", "Videos"}
        };
        for( const auto& candidate : candidates ) {
            std::error_code ec;
            fs::path target = home / candidate.subdir;
            // 不存在的目录不列出来，否则点了也没反应，还可能让侧边栏白白占地方
            if( fs::is_directory(target, ec) )
                links.emplace_back(candidate.label, target);
        }
        return links;
    }

    pair<string, uintmax_t> get_file_size(const fs::path &path) {
        std::error_code ec;
        const uintmax_t size = fs::file_size(path, ec);
        if( ec )
            return {"未知", 0};

        static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit_index = 0;
        double file_size = static_cast<double>(size);

        while( file_size >= 1024.0 && unit_index < 4 ) {
            file_size /= 1024.0;
            unit_index ++;
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << file_size << " " << units[unit_index];
        return {ss.str(), size};
    }

    std::string format_time(const fs::file_time_type &time) {
        try {
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                time - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            auto tt = std::chrono::system_clock::to_time_t(sctp);

            std::tm tm;
#ifdef _WIN32
            if( localtime_s(&tm, &tt) != 0 ) return "未知";
#else
            if( localtime_r(&tt, &tm) == nullptr ) return "未知";
#endif
            std::stringstream ss;
            ss << std::put_time(&tm, "%Y-%m-%d %H:%M");
            return ss.str();
        } catch (...) {
            return "未知";
        }
    }
}

void FileChooser::load_directory() {
    top_dir_input_ = current_directory_.u8string();
    file_items_.clear();
    file_name_input_.clear();
    quick_links_ = collect_quick_links();

    std::error_code dir_ec;
    fs::directory_iterator it( current_directory_, dir_ec ), end;
    if( dir_ec ) {
        sort_items();
        return;
    }

    for( ; it != end && !dir_ec; it.increment(dir_ec) ) {
        const fs::directory_entry &entry = *it;

        std::error_code ec;
        const bool is_directory = entry.is_directory(ec);   // 不抛异常的版本
        if( ec )
            continue;

        const fs::path file_name = entry.path().filename();
        const string name = file_name.u8string();

        if( !show_hidden_files_ && !name.empty() && name[0] == '.' )
            continue;

        if( selection_mode_ == SelectionMode::DirectoriesOnly && !is_directory )
            continue;

        FileItem item;
        item.path = file_name;
        item.name = name;
        item.is_directory = is_directory;
        item.is_selected = false;

        if( item.is_directory )
            item.type = "文件夹";
        else if( file_name.has_extension() ) {
            item.type = file_name.extension().u8string();
            if( !item.type.empty() )
                item.type = item.type.substr(1); // 移除点
            item.type = item.type + " 文件";
        } else
            item.type = "未知类型";

        if( !item.is_directory ) {
            pair<string, uintmax_t> file_size = get_file_size(entry.path());
            item.size = file_size.first;
            item.size_bytes = file_size.second;
        } else
            item.size = "--";

        const fs::file_time_type ftime = fs::last_write_time(entry.path(), ec);
        if( !ec ) {
            item.modified = ftime;          // 排序用（原始时间）
            item.modified_time = format_time(ftime);
        } else
            item.modified_time = "未知";    // item.modified 保持默认值，排在最前/最后

        if( item.is_directory || is_filter_match(entry.path()) )
            file_items_.push_back(item);
    }

    sort_items();
}

void FileChooser::sort_items() {
    // SortOrder 里升序/降序是成对定义的：Name=1,NameDes=2, Type=3,TypeDes=4, ...
    const SortOrder order = sort_order_;
    const bool descending = (static_cast<int>(order) % 2) == 0;

    // 严格弱序：不能用 descending ^ (a < b) —— 降序且键相等时 less(a,b) 和 less(b,a)
    // 会同时为真（less(a,a) 也为真），std::sort 的前置条件被破坏（MSVC Debug 直接抛
    // "invalid comparator"，Release 是未定义行为）。降序必须显式交换两侧。
    auto key_less = [order, descending]( const FileItem &a, const FileItem &b ) -> bool {
        if( a.is_directory != b.is_directory )
            return a.is_directory > b.is_directory;
        switch( order ) {
            case SortOrder::Name:
            case SortOrder::NameDes:
                return descending ? (b.name < a.name) : (a.name < b.name);
            case SortOrder::Type:
            case SortOrder::TypeDes:
                // 键值相同时用名称兜底，保证排序结果稳定
                if( a.type != b.type )
                    return descending ? (b.type < a.type) : (a.type < b.type);
                return descending ? (b.name < a.name) : (a.name < b.name);
            case SortOrder::Size:
            case SortOrder::SizeDes:
                return descending ? (b.size_bytes < a.size_bytes) : (a.size_bytes < b.size_bytes);
            case SortOrder::Time:
            case SortOrder::TimeDes:
                return descending ? (b.modified < a.modified) : (a.modified < b.modified);
            default:
                return a.name < b.name;  // 新增枚举值时的兜底
        }
    };

    std::sort(file_items_.begin(), file_items_.end(), key_less);
}

void FileChooser::navigate_to(const fs::path &path) {
    std::error_code ec;
    if( !fs::is_directory(path, ec) || ec )
        return;

    const fs::path canonical = fs::canonical(path, ec);
    if( ec )
        return;

    current_directory_ = canonical;
    selected_path_.clear();   // 换了目录，上一个目录里的选择就作废了
    load_directory();
}

void FileChooser::navigate_up() {
    if( current_directory_.has_parent_path() )
        navigate_to(current_directory_.parent_path());
}

bool FileChooser::is_filter_match(const fs::path &path) const {
    if( current_filter_ == "*" )
        return true;

    return has_dot_suffix(path.filename().u8string(), current_filter_);
}

std::vector<fs::path> FileChooser::get_selected_files() const {
    if( !multiple_selection_ )
        return selected_path_.empty() ? vector<fs::path>{} : vector<fs::path>{ selected_path_ };

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
}

void FileChooser::set_filters(const std::vector<std::string> &filters) {
    filters_ = filters.empty() ? vector<string>({"*"}) : filters;
    current_filter_ = filters_.front();
}


} // namespace ave
