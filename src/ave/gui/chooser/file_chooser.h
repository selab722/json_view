#ifndef AVE_GUI_CHOOSER_FILE__CHOOSER_H
#define AVE_GUI_CHOOSER_FILE__CHOOSER_H

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
     * @brief       When return true, you can call get_selected_file or get_selected_files
     *              to get selected. If multiple selection is true, then the former returns
     *              the first file gets selected. If ms is false, the latter return empty vector.
     *              If a file is not selected, these will return empty.
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

    std::filesystem::path get_selected_file() const { return selected_path_; }

    std::vector<std::filesystem::path> get_selected_files() const;

private:
    void render_toolbar();
    void render_file_list();
    void render_footer();
    void render_sidebar();

    bool is_filter_match(const std::filesystem::path& path) const;
    static std::string get_file_size(const std::filesystem::path& path);
    static std::string format_time(const std::filesystem::file_time_type& time);

    bool is_open_ = false;
    bool show_hidden_files_ = false;
    bool multiple_selection_ = false;

    SelectionMode selection_mode_ = SelectionMode::FilesOnly;

    std::filesystem::path current_directory_;  // canonical path
    std::filesystem::path selected_path_;

    struct FileItem {
        std::filesystem::path path;
        std::string name;
        std::string type;
        std::string size;
        std::string modified_time;
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
