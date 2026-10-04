#ifndef AVE_GUI_CHOOSER_FILE__CHOOSER_H
#define AVE_GUI_CHOOSER_FILE__CHOOSER_H

#include <cstdint>
#include <filesystem>
#include <vector>
#include <string>


namespace ave {


class FileChooser final {
public:
    enum class SelectionMode {
        FilesOnly,               // choose one file
        DirectoriesOnly,         // choose one directory
        FilesOrDirectories,      // choose any
        SaveFile                 // save mode, single file
    };

    FileChooser() {
        reset(true);
    }

    ~FileChooser() = default;

    /**
     * @brief       When user click confirm, this function return true, then you can call get_selected_files().
     *              If multiple selection is allowed, then it will contain possibly multiple files.
     *              Otherwise it will only return 1 file.
     *              If a file is not selected, this function return false.
     * @return      true if file(s) get selected
     */
    bool show();
    void reset(bool all = false);

    bool is_open() const {
        return is_open_;
    }

    void close();

    void navigate_to(const std::filesystem::path& path);
    void navigate_up();

    void set_filter(const std::string& filter);
    void set_filters(const std::vector<std::string>& filters);

    void set_selection_mode(SelectionMode mode) {
        selection_mode_ = mode;
        if( selection_mode_ == SelectionMode::SaveFile )
            multiple_selection_ = false;
    }

    SelectionMode get_selection_mode() const { return selection_mode_; }

    void set_multiple_selection(bool multiple) {
        multiple_selection_ = multiple;
        if( selection_mode_ == SelectionMode::SaveFile && multiple_selection_ )
            selection_mode_ = SelectionMode::FilesOnly;
    }

    bool is_multiple_selection() const { return multiple_selection_; }

    std::vector<std::filesystem::path> get_selected_files() const;

private:
    void render_toolbar();
    void render_file_list();
    void render_footer();
    void render_sidebar();

    bool is_filter_match(const std::filesystem::path& path) const;
    // 返回用于显示的尺寸字符串；out_size 非空时同时回传原始字节数（排序要用，展示串没法比大小）
    static std::string get_file_size(const std::filesystem::path& path, std::uintmax_t* out_size = nullptr);
    static std::string format_time(const std::filesystem::file_time_type& time);

    bool is_open_ = false;
    bool show_hidden_files_ = false;
    bool multiple_selection_ = false;

    SelectionMode selection_mode_ = SelectionMode::FilesOnly;

    std::filesystem::path current_directory_;  // canonical path
    // Used when multiple selection not allowed.
    // When multiple is allowed, this will sometimes save one of the selected answer,
    // but not always, this will reduce the search time for has_selected.
    std::filesystem::path selected_path_;

    bool has_selected() const {
        if( !selected_path_.empty() )
            return true;
        if( multiple_selection_ )
            for( const FileItem& ft : file_items_ )
                if( ft.is_selected )
                    return true;
        return false;
    }

    struct FileItem {
        std::filesystem::path path;
        std::string name;
        std::string type;
        std::string size;                              // 显示用，如 "1.2 KB"
        std::string modified_time;                     // 显示用，如 "2024-01-01 08:30"
        std::uintmax_t size_bytes = 0;                 // 排序用：原始字节数
        std::filesystem::file_time_type modified{};    // 排序用：原始修改时间
        bool is_directory;
        bool is_selected;
    };

    bool is_selectable( const std::filesystem::path& path ) const {
        bool dir_ok = selection_mode_==SelectionMode::DirectoriesOnly || selection_mode_==SelectionMode::FilesOrDirectories;
        bool file_ok = selection_mode_==SelectionMode::FilesOnly || selection_mode_==SelectionMode::SaveFile;
        bool is_dir = std::filesystem::is_directory(current_directory_ / path);
        return is_dir ? dir_ok : file_ok;
    }

    std::vector<FileItem> file_items_;

    std::vector<std::string> filters_;
    std::string current_filter_;

    enum class SortOrder { Name = 1, NameDes, Type, TypeDes, Size, SizeDes, Time, TimeDes };
    SortOrder sort_order_ = SortOrder::Name;

    std::string top_dir_input_;
    std::string file_name_input_;

    float column_widths_[4];

    void single_click( FileItem& item, bool ctrl_down );
    void select_item(const std::filesystem::path&);
    void load_directory();
    void sort_items();
};


}  // namespace ave

#endif  // #ifndef AVE_GUI_CHOOSER_FILE__CHOOSER_H
